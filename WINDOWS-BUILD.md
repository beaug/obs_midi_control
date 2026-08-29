# Build on Windows 11

Native Windows is the right path. WSL can compile a Linux `.so`, which OBS on Windows will not load.

## 1. Install tools

- [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) with workload **Desktop development with C++**
- [CMake 3.30+](https://cmake.org/download/) (tick “Add CMake to PATH”)
- Git is optional

You do **not** need to install OBS source yourself. First configure downloads OBS 31 headers + Qt6 into `.deps`.

## 2. Unzip and configure

In **x64 Native Tools Command Prompt for VS 2022** (Start menu), not a normal cmd:

```bat
cd /d %USERPROFILE%\Downloads\obs-midi-hotkeys
cmake --preset windows-x64
cmake --build build_x64 --config RelWithDebInfo
```

First configure takes a few minutes (downloads deps). Later builds are quick.

## 3. Install into OBS

Copy the built plugin here (create the folder if needed):

```
%APPDATA%\obs-studio\plugins\obs-midi-hotkeys\bin\64bit\
```

From the build tree that file is:

```
build_x64\RelWithDebInfo\obs-midi-hotkeys.dll
```

Also copy locale:

```
%APPDATA%\obs-studio\plugins\obs-midi-hotkeys\data\locale\en-US.ini
```

from `data\locale\en-US.ini` in the zip.

Restart OBS. Open **Tools → MIDI Hotkeys**.

## If configure fails

- Use the **VS 2022 x64** developer prompt, not PowerShell without the VS env
- CMake must be 3.28+
- Delete `build_x64` and `.deps` and rerun `cmake --preset windows-x64`

WSL is only useful if you run OBS *inside* Linux. For OBS installed on Windows 11, build with Visual Studio as above.
