# Release Notes
## Pipedal 1.2.37 Beta4

Feature:
- ToobML allows uploading of models.
- TooBML support for large models (e.g. GuitarML Proteus models)
- Support for sub-directories when uploading files.
- Support for uploading .zip file bundles.

You can now upload GuitarML Proteus models from the GuitarML project. 

- Download protues models from https://guitarml.com/tonelibrary/tonelib-pro.html
- Load TooB ML.
- click on Model control.
- click on the Upload button in the lower left corner of the Model selection dialog.
- Upload the .json model or .zip model collection file that you downloaded from The GuitarML Tone Library.

Fixes for the following issues: 
- changes to dialog button color change.
- automatic provisioning of resource files in .LV2 plugins.
- The web server supports uploading of large files (limited by default to 512MB).

## Pipedal 1.2.36 Beta3

Fixes for the following issues: 

- Failure to open audio devices on reboot for devices that initialize slowly.
- TooB ML and Toob Neural Amp Modeller UI issues.
- Misbehavior with preset names after renaming a preset.
- Change default audio buffers to 64x3.
- Filtering not working in plugin selection dialog.

## PiPedal 1.2.34 Beta2

Fixes the following issues:

- Midi Bindings can now be created for controls in a Split.
- Gain control for ToobML is enabled or disabled depending on whether the modal has a gain control input.
- The control view for split controls now displays properly.
- Stereo connectors in dark mode now render properly.
- Permissions are correctly set on existing files and folders in /var/pipedal at install time.
- pipedalconfig has a new --fix-permissions option, which corrects permissions on files in /var/pipedal.

## PiPedal 1.2.33 Beta1

This release provides compatibility with Raspberry Pi OS bookworm. If you are using Raspberry Pi OS bullseye, or Ubuntu 21.04, please use PiPedal v1.1.31 instead.

If you are using the Android client, you should make sure you are using the most recent version, which includes significant improvements to Wi-Fi Direct pairing.

Bug Fixes:

- Support for NetworkManager network stacks used buy Raspberry Pi OS Bookworm, and Raspberry Pi 5 devices.
- The .deb package now installs on Raspberry Pi OS Bookworm and Raspberry Pi 5 devices.
- Bypass MIDI bindings now work properly. Thanks to @FoolHen.
- GxTuner now correctly displays note pitches. Thanks to @FoolHen.
- Fixed glitch in on-boarding process when changing Wi-Fi direct settings.
- Fixed incorrect display of banks in dark mode Thanks to @FoolHen.
- pidedald service shuts down cleanly. With help from @FoolHen.
- Fixed missing dependency in build procedure documentation. Thanks to @FoolHen
- Minor documentation corrections. With help from @FoolHen.
- uninstall.sh now removes all files installed by ./install.sh (used only during development).
  
## PiPedal 1.1.31

- Use system preferences for dark mode when connecting from Android app.


## PiPedal 1.1.29

Bug fixes:

- TooB Convolution filenames not updating properly.


## PiPedal 1.1.28

New in this release.

- TooB Neural Amp Modeler - an LV2 port of  Steven Atkinson's astounding Neural Amp Modeler. Donwload model files from [ToneHunt.org](https://tonehunt.org). CPU is very high -- it runs on a Raspberry Pi 4 with just enough CPU left over for a couple of extra effects; but the results are worth it. TooB ML provides Neural Net modelled guitar amps with more modest CPU use (but considerably less flexibility).

- Dark mode. Go to the settings dialog to turn on Dark Mode. 

Bug fixes:

- Plugin information is updated in the web app after reconnecting, in order to pick up new plugins that may have been detected while the web server was being restarted.


## PiPedal 1.1.27

- Add missing lv2-dev dependency.

## PiPedal 1.1.26

- Revised file browser protocol for NAM and MODP plugin compatibility.
- Revised resource file publication extension.

## PiPedal 1.1.25

- New TooB BF-2 Flanger plugin
- New TooB Stereo Convolution Reverb

## PiPedal 1.1.24

- Reduce TooB Convolution Reverb and Toob Cab IR CPU use by 90%.

## PiPedal 1.1.23

Features:

-    Two new plugins: TooB Convolution Reverb, and TooB Cab IR.

-    Output ports for LV2 plugins are now displayed (vu meters and status controls)

-    File selection dialog, and uploads for LV2 atom:path Properties for 3rd party LV2 plugins.

Bug fixes:

-    Improvements to onboarding procedure.

-    Problems with restarting audio after changing audio parameters.

-    Occasional loss of audio after reboot caused by ALSA USB initialization race conditions.
