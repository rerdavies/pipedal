# Release Notes
## PiPedal 1.3.54 Release

Improvements to and fixes for hotspot functionality. Fixes for problems with snapshots.

Minor features;
- Enter key supported in most dialogs. 

- Uploaded .zip files create a top-level directory with the name of the zip file if the zip file doesn't contain a single root-level directory. (released in 1.3.53)

Bug fixes.
-    Various crashes related to snapshots.

-    Applying changes to Wi-Fi hotspot configuration would not change the configuration until PiPedal was rebooted (a regression).
     The configuration of the Wi-Fi hotspot now changes as soon as you apply it.

-    The state of bypass switches was not getting captured in snapshots. 

-    Crashes when scrolling through presets rapidly in the Snapshot view.

-    Fix exponential growth in memory use each time TooB Freeverb is loaded. (released in v1.3.53).

-    Web server redirects link-local IPv6 connections to non-link local address. (Improves client connectivity).

-    Force DHCP routing on hotspot connections. (Improves android client connectivity)

-    Missing MIDI device causes audio to stop.

-    The file model for snapshots was confusing and didn't entirely work as expected. Changes to snapshots now 
     only get saved when the preset to which they belong gets saved (explicit-save file model, instead of 
     Android file model).

-    Layout improvement to a number of dialogs when displayed on wide screens.


## PiPedal 1.3.53 Release

Major features:
- Snapshots. For a discussion of what snapshot are, see [An Intro to Snapshots](https://rerdavies.github.io/pipedal/Snaphots.html)
- New <b><i>Performance View<</i></b> providing an user-interface optimized for live performance rather than editing of plugins.
- New System Midi Bindings for snapshot selection, and for previous and next bank.

This should have probably gone out as a Beta release, but the fixes for the "Audio Stopped" error are significant. I would recommend updating to this release as soon as you can. Please consider the snapshot user interface to be somehwat fluid for now. I am open to suggestions and feedback on the new feature.

Bug Fixes:
- A fix for the dreaded "Audio Stopped" error (and associated memory corruption). You may still underrun, but at least audio won't stop.
- Further improvements to clean (and timely) shutdown procedure.
- Minor UI improvements for adaptive display on small devices.
- A bug in TooB Freeverb that causes memory use to increase exponentially each time the plugin is loaded.


## PiPedal 1.2.52 Release

Modest performance improvements for TooB Neural Amp Modeler (about 8% faster).

Bug fixes:
- Unnecessary exported symbols stripped from ToobAmp.so.

## PiPedal 1.2.51 Release

<b>Important notice:</b> Changes to the Auto-update code prevent previous versions from automatically updating to this version. You will have to install this version by downloading the [debian package](https://rerdavies.github.io/pipedal/download.html) and installing it manually.

This release replaces Wi-Fi Direct connections with Wi-Fi hotspots. Support for Wi-Fi Direct on Linux and Android has been fragile for some time. An update to Raspberry Pi OS in early September broke Wi-Fi Direct support completely. As it turns out, Auto-hotspots work much better. 

The new Auto-Hostpot feature in PiPedal allows you to configure your Raspberry Pi so it automatically starts a Wi-Fi hotspot whenever you are away from home. The updated Android PiPedal Client will automatically detect and connect to your Raspberry PI whenever it is visible on the current Wi-Fi network. An updated Android [PiPedal Remote](https://play.google.com/store/apps/details?id=com.twoplay.pipedal&hl=en_US&pli=1) app has been posted on Google Play. Make sure you are using the updated version.

New MIDI system bindings allow you to enable or disable the Wi-Fi hotspot, and to shut down or reboot your Raspberry Pi using MIDI-triggered events.

NOTICE: PiPedal 1.2.47 fixed a significant and dangerous defect that may cause loss of presets, banks or configuration data if you remove power from your Raspberry Pi within up to five minutes of last saving data instead of performing an orderly shutdown or reboot. Users of PiPedal should upgrade to at least version 1.2.47 immediately, if they have not already done so.


Bug fixes:
- PiPedal Remote client theme native pages do not match the selected PiPedal UI theme.
- Unable to configure audio buffers properly for devices with minBufferSize=256.
- Release and Beta updates not visible if the most recent release is a development build.
- Revised auto-update procedure, including a new signing procedure.
- Ongoing improvements to TooB Neural Amp Modeler (bug fixes, performance improvements)
- pipedald service hangs and/or throws exceptions on shutdown.
- Update DNS/SD service announcements when the device name is changed. 
- Unannounce DNS/DS services when the pipedal services shuts down.
- Auto-uprade checks may exceed the throttling rate of Github API calls when checking for updates.
- Memory corruption when ALSA device creation fails.
- MIDI input causing audio glitches and underruns.
- Unable to connect when using IPv6 connections and mDNS name resolution.
- Default web port configuration overwritten by upgrades.
- Remove dependencies on old pre-NetworkManager network services required by Wifi-Direct code.
- Refuse to install on versions of Raspberry Pi OS prior to bookworm.

Known issues:
- TooB Neural Amp Modeler runs 5% slower than Mike Oliphant's Neural Amp Modeler, despite running virtually identical code from Steven Atkins' NeuralAmpModelerCore project. Investigation continues.



## PiPedal 1.2.47 Release

This version fixes a significant and dangerous defect that may cause loss of presets, banks or configuration data if you remove power from your Raspberry Pi within up to five minutes of last saving data instead of performing an orderly shutdown or reboot. Users of PiPedal should upgrade immediately.

This release also fixes a significant performance issue with TooB Neural Amp Modeler.

Bug fixes:
- Explicity sync files to disk immiately after saving, so that they won't be lost if power is removed from the Raspberry Pi.
- Fixes a significant performance issue with TooB Neural Amp Modeler.

## PiPedal 1.2.45 Release

This version focuses on fixes and improvements to TooB Neural Amp Modeler.
TooB NAM now uses about 20% less CPU, which can help enormously on Raspberry Pi 4. 

You may need to change your audio buffers to prevent dropouts when using TooB Neural Amp Modeler. Setting your audio buffers to 32x4 seems to produce good results. You may also want experiment with 16x4 audio buffers. This seems to work, but it has had limited testing so far, so I can't say with any certainty whether it is stable for long-term use. 2-buffer audio settings should be avoided. As always, using your Pi for anything else at the same time you are using PiPedal will cause XRuns.

I would once again like to extend special thanks and acknowledgements to Steven Atkins for his outstanding work on the NeuralAmpModelerCore project on which TooB Neural Amp Modeler depends (as do almost all other Neural Amp Modeler plugins).

Bug fixes:

- The About page now displays the correct version.
- TooB NAM frequency response now displays correctly.
- TooB NAM no longer displays error messages when loading.
- Update Dialog has better layout on portrait phone displays.
- Prevent brief flashing of the Reload... button during the last phase of an update.


## PiPedal 1.2.44 Release 

This version includes the following new features:

- Double-tap (or double-click) dial controls to reset them to default values.
- Update PiPedal to the latest version from within the PiPedal web UI or PiPedal client. PiPedal monitors the PiPedal github repository for new releases,
  and prompts you to update when new releases become available.
- Choose whether to monitor Release, Beta or Developer update streams, or disable update monitoring altogether.
- All PiPedal configuration settings now preserved when upgrading.
- Advanced configuration options are now stored in /var/pipedal/config/config.json so that they are preserved when upgrading.

<img src="img/Updates-sshot.png" width="80%"/>

Bug fixes:

- Fixed a bug which prevents the PiPedal UI from updating properly when using TooB Convolution Reverb and CabIR effects.
- Plugin controls now work when there's no audio device running, or when the audio device stops.
- Minor style and theming issues.

## PiPedal 1.2.41 Release 

This version includes the following new features:

- Supports Raspberry Pi 5.
- Supports Rasberry Pi OS Bookworm.
- TooB ML allows uploading of models. See below for further details.
- TooBML support for large models (e.g. GuitarML Proteus models)
- Upload .zip file bundles to all File plugin controls.
- Uploads can be organized into sub-directories.
- PiPedal now monitors LV2 plugin directories for changes, and automatically updates available plugins in the 
  PiPedal UI, without requiring a restart.
- Various minor improvements to TooB plugin user interfaces.

You can now upload new models into TooB ML. TooB ML models are compatible with models for the GuitarML Proteus plugin. Currently, the GuitarML Proteus model download pages are the best source for new models to use with TooB ML. To install 
new models in TooB ML, follow these instructions:

- Download Proteus models from https://guitarml.com/tonelibrary/tonelib-pro.html to your local computer or Android
  device. TooB ML supports both Proteus .json models, and also allows you to install model collections contained in .zip files.
- Load the TooB ML plugin.
- click on the Model control.
- click on the Upload button in the lower left corner of the Model selection dialog.
- Upload the .json model file or .zip model collection that you downloaded from The GuitarML Tone Library.

Big fixes:
- Wi-Fi is restarted properly without requiring a reboot when the PiPedal Wi-Fi Direct connection is disabled.
- Dialog button colors follow Android UI conventions.
- The web server supports uploading of large files (limited by default to 512MB).
- Failure to open audio devices upon reboot for devices that initialize slowly.
- Misbehavior with preset names after renaming a preset.
- Dialog button colors follow Android UI conventions.
- Toggle and on/off controls now display in purple instead of red/pink.
- automatic provisioning of resource files in .LV2 plugins.
- Gain control for ToobML is enabled or disabled depending on whether the model has a gain control input (Thanks to @Foolhen)
- The control view for split controls now displays properly.
- Stereo connectors in dark mode now render properly.
- Permissions are correctly set on existing files and folders in /var/pipedal at install time. `pipedalconfig1 has a new
  `--fix-permissions` option, which corrects permissions on files in /var/pipedal.
- Bypass MIDI bindings now work properly. Thanks to @FoolHen.
- GxTuner now correctly displays note pitches. Thanks to @FoolHen.
- Fixed glitch in on-boarding process when changing Wi-Fi direct settings.
- Fixed incorrect display of banks in dark mode Thanks to @FoolHen.
- pidedald service shuts down cleanly. With help from @FoolHen.
- Fixed missing dependency in build procedure documentation. Thanks to @FoolHen
- Documentation edits and corrections. With help from @FoolHen.
- automatic provisioning of resource files in .LV2 plugins (devolopment feature)
- uninstall.sh now removes all files installed by ./install.sh (used only during development).





## Pipedal 1.2.40 Beta6
- PiPedal now monitors lv2 directories in order to automatically add newly-installed LV2 plugins.
- Toggle and on/off switch colors changed from red/pink to purple.
- MIDI bindings can be created for split controls. 

## Pipedal 1.2.38 Beta5

Fixes:
- PiPedal won't run on non-en-US locales.

## Pipedal 1.2.37 Beta4

You can now upload GuitarML Proteus models from the GuitarML project into TooB ML.

- Download Proteus models from https://guitarml.com/tonelibrary/tonelib-pro.html
- Load the TooB ML plugin.
- click on the Model control.
- click on the Upload button in the lower left corner of the Model selection dialog.
- Upload the .json model file or .zip model collection that you downloaded from The GuitarML Tone Library.

Features:
- ToobML allows uploading of models.
- TooBML support for large models (e.g. GuitarML Proteus models)
- Support for sub-directories when uploading files.
- Support for uploading .zip file bundles.

Fixes for the following issues: 
- Dialog button colors follow Android UI conventions.
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
