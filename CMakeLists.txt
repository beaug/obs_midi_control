cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/mapping.cpp
    src/mapping.hpp
    src/midi-engine.cpp
    src/midi-engine.hpp
    src/obs-hotkeys.cpp
    src/obs-hotkeys.hpp
    src/settings-window.cpp
    src/settings-window.hpp
    deps/rtmidi/RtMidi.cpp
    deps/rtmidi/RtMidi.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE src deps/rtmidi)

if(WIN32)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE __WINDOWS_MM__)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE winmm)
elseif(APPLE)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE __MACOSX_CORE__)
  find_library(COREMIDI_LIB CoreMIDI)
  find_library(COREAUDIO_LIB CoreAudio)
  find_library(COREFOUNDATION_LIB CoreFoundation)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${COREMIDI_LIB} ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
else()
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE __LINUX_ALSA__)
  find_library(ASOUND_LIB asound)
  if(ASOUND_LIB)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${ASOUND_LIB} pthread)
  else()
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE asound pthread)
  endif()
endif()

set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
