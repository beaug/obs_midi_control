# obs-midi-hotkeys

This plugin was created to allow the use of an Airstep to control functions inside of OBS but it should also work with other MIDI devices.

This plugin is a remake inspired by
[obs-midi-mg](https://github.com/nhielost/obs-midi-mg) by
[nhielost](https://github.com/nhielost).

obs-midi-mg itself credits
[cpyarger](https://github.com/cpyarger) and
[Alzy](https://github.com/alzy) /
[obs-midi](https://github.com/cpyarger/obs-midi/).

## Features

This is based on my own testing but some of the things I was testing (in Windows 11 x64 OBS Version 32.2.2 64 Bit):

- Ability to use a "learn" button to figure out the midi message being sent
- Ability to manually set midi type, values and CC/PC/Note and the MIDI Channel
- No need to set keyboard hotkeys as triggers MIDI itself is the trigger
- Out of Focus Control i.e. you don't need to have OBS "in focus" for it to interpret and receive a midi message
- You can set an app (e.g. Guitar Rig 7) to be on one midi channel, and OBS on another and control both applications, this is just how midi works i'm pretty sure but I had seen people mention it being a problem
- Should support any device that can send MIDI messages

## Install Instructions

This is the quickest way to get started - For a Windows installer, download the latest EXE from
[Releases](https://github.com/beaug/obs_midi_control/releases/latest).

Open and run the exe, it should copy the dll to the appropriate folders then read [Opening The Plugin](#opening-the-plugin) for getting started

## Build Instructions

To build yourself instead of using the installer

Windows 11, 64-bit:

1. Install [Visual Studio 2022](https://aka.ms/vs/17/release/vs_BuildTools.exe) with **Desktop development with C++**, and [CMake 3.28+](https://cmake.org/download/).
2. Clone this repo.
3. Open **x64 Native Tools Command Prompt for VS 2022** and run:

```bat
cd path\to\obs-midi-hotkeys
cmake --preset windows-x64
cmake --build build_x64 --config RelWithDebInfo
```

4. Quit OBS completely, then copy the plugin in:

```bat
copy /Y build_x64\RelWithDebInfo\obs-midi-hotkeys.dll "%ProgramFiles%\obs-studio\obs-plugins\64bit\"
mkdir "%ProgramFiles%\obs-studio\data\obs-plugins\obs-midi-hotkeys\locale"
copy /Y data\locale\en-US.ini "%ProgramFiles%\obs-studio\data\obs-plugins\obs-midi-hotkeys\locale\"
```

## Opening The Plugin

In OBS go to Tools and then Midi Hot Keys

<img width="228" height="237" alt="image" src="https://github.com/user-attachments/assets/1ec8cea1-fd03-4912-bec8-ad620f9f6e3c" />

In the window that pops up select your MIDI device and set the MIDI (Listen) channel 

<img width="978" height="548" alt="image" src="https://github.com/user-attachments/assets/e2947780-3d3f-4bfa-a0da-3b80e4bd7b7e" />

### Setting a MIDI Mapping

Click on "Add Mapping" which will add a row and allow you to select what "hotkey" function you would like e.g. Switch Scene, Start Stream etc

<img width="977" height="105" alt="image" src="https://github.com/user-attachments/assets/773adfa2-f2f6-4b99-b3ef-d9c215177bbb" />

If you know how MIDI works, and how to setup your channels set your channel, message type and CC number on the row


Alternatively - select the relevant row (shown by the outline) and then click on "Learn Selected"


<img width="966" height="444" alt="image" src="https://github.com/user-attachments/assets/dc7f2a12-684b-4e4b-b976-b6f213f99536" />


The plugin will await midi input then auto learn from the message


<img width="974" height="239" alt="image" src="https://github.com/user-attachments/assets/506ac47a-fd9d-4562-b51b-ecec7c0fd72d" />


Repeat for other mappings you want to add and then click on "Save" to save the mappings. 

At the moment the mappings don't persist e.g. if you close and reopen OBS you need to redo the mappings - something that will hopefully be fixed in a future release

### Example Use Cases

- Scene switching for multiple camera angles
- Trigger Start/Stop Stream or Recording

## Resources

Creating this project required learning more about MIDI, if you are interested in that to the links below will be helpful

- Scott Uhl explaining the Midi funnel https://youtu.be/IoUtshlzCTo?si=bq2qdnwbg2FTeH-9
- Learning How MIDI Works https://www.youtube.com/watch?v=YrBo3KuUsbg
- The history of MIDI https://youtu.be/cd-9mx_p4Xs?si=06h8BuHnQqRWT935

## Licences
License and credits: see [LICENSE](LICENSE) and [NOTICE](NOTICE).
