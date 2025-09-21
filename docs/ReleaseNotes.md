# Release Notes

## PiPedal 1.5.93 Release

### Major Features in This Build

- New TooB Tremolo plugin. Normal and harmonic tremolo in mono and stereo versions. Harmonic tremolo is tremolo variant that modultes the amplitude of high-frequency and low-frequency in opposite phase. It follows the led of many classic and modern Fender amps, which also provide harmonic tremolo. Stereo harmonic tremolo is particularly smooth and pretty.

- Colored plugin icons. Tap on the title of the plugin in the middle toolbar to edit the display name of the plugin, and choose a color for the icon. 

- Tools for copying presets between banks. You can now import presets from another bank using the Presets menu found at the top of the main screen; or you can import presets from another bank, or export presets in the current bank to another bank, using menu commands found in the "Manage Presets" dialog. 

- Multi-select in the Manage Presets dialog makes it easier to perform bulk exports to another bank, or delete multiple plugins. To select multiple presets when using a mouse press the Ctrl key, and click on presets to toggle selection; or press the Shift key and click on a preset to select all presets from the last selected to the newly selected preset. On touch devices, long-press on a preset (without dragging) in order to  to start multi-select action mode (an Android touch-UI convention of sorts); then tap on individual presets to toggle selection.

### Bug fixes

- several bugs relating to the new Factory Presets bank: preset amp model files were not getting properly installed; one preset used an amp model that is not included in factory installed presets. And a new preset showing off the new TooB Tremolo plugin has been added.

- pipedal_config will no longer accept the --port option without also specifying the --install option. (It used to silently fail).

- pipedal_latency_test now provides instructions on how to enable missing real-time scheduling permissions for non-root users. 


## PiPedal 1.5.92 Release

### Major Features in This Build

- New default presets that make better use of plugins that have been added since the original set of default presets were created.

- The installer creates a new "Factory Presets" bank. If you are upgrading, you can access the new presets in the Factory Presets bank. New factory presets added in future PiPedal updates may add or modify presets in the Factory Presets bank. If you want to use or modify the new Factory Presets, you should copy them to a new bank, or copy individual presets to a bank you currently use for your own presets.

- Stream between audio adapters. You can now select different audio adapters for input and output. (Thanks to Gabriel Hernandez)

### Major Features (Since PiPedal 1.4.76 Release)

- Support for Ubuntu 25.04. Compatibility fixes for Ubuntu 25.04 may also make it easier to build PiPedal on
    other Debian-based Linux distros.

- Code optimizations for Raspberry Pi 5 (Cortex-A76), providing substantial performance improvements for TooB Neural Amp Modeler. ToobAmp 
automatically loads A72- or A76-optimized code depending on which processor you are using.

- New "Threaded" button optionally executes TooB NAM computation on a separate thread. Allows up to 5 NAM instances on Pi 4, and practically unlimited NAM instances on Pi 5.

- TooB Neural Amp Modeler: Input and output calibration. See [Using TooB Neural Amp Modeler](https://rerdavies.github.io/pipedal/UsingNAM.html) for guidance on how to 
  use this feature.

- TooB Convolution Reverb: Predelay, Stretch and Decay controls allow adjustments of impulse response characteristics.

- TooB Convolution Reverb, Freeverb: New "Tails" control, which allows reverb tails to continue after bypassing the effect.

- TooB Parametric EQ: New plugin providing a 4-band parametric EQ. A musically useful Parametric EQ that's suitable both for 
    shaping guitar tone before amp models, or for shaping overall sound at the end of plugin chains. This should become your 
    go-to EQ plugin when using PiPedal.

- New TooB Player plugin that allows you to play (and loop) audio files (even large ones).

- TooB 3 Band EQ: New plugin providing a simple 3-band EQ. (Replicates the EQ stage on Steve Atkinson's Windows and Mac-based 
    Neural Amp Modeler plugin).

- TooB CE-2 Chorus, TooB BF-2 Flanger, TooB Phaser: New "Wet/Dry" controls. 

- Noise Gates threshold ranges have been increased in TooB NAM, TooB Input and TooB Noise Gate plugins.

- MIDI bindings allow users to specify minimum and maximum control values, in order to better support expression pedals.

- Editable plugin labels. You can now edit the labels of plugins in the PiPedal UI. Tap the plugin label to override
    the default label with a custom label. Useful when you have multiple instances of the same plugin in a preset.

- Multi-select support in file browser dialogs. Allows you to easily delete, copy or move multiple files at once.

- long-press and hover Tooltips are now displayed for all controls in the PiPedal UI.

- TooB Neural Amp Modeler: Switch to the NeuralAudio NAM Backend (Thanks to Mike Oliphant for providing 
   MIT-licensed sources that provide significant performance improvements).

- Support for custom MOD User Interfaces for plugins that provide a MOD UI.

- Switch from ALSA rawmidi devices to ALSA sequencer devices for MIDI input. This provides
      a number of useful improvements. PiPedal now supports Bluetooth MIDI controllers.
      PiPedal provides better support for subdevices (multiple MIDI connections on the same device). 
      Connections to the PiPedal MIDI input port can now be made at runtime by other applications, or 
      by using`aconnect`. Changing MIDI devices does not require a restart of the audio stream. Disconnected
      MIDI devices that have been selected will now be automatically reconnected when they are 
      plugged in again.

- Dial control values are displayed in a fly-out when editing, which makes it easier to determine the
     current value of a control when using touch user interfaces.

-  Direct link to TONE3000 website from the TooB Neural Amp Modeler file browser.

- Double-tap gestures now supported in touch user interfaces.  Double-tap a dial to reset it to its default value. Double-tap
    a file in the File Browser to select it. Double-tap a plugin label to edit it.

- Improvements to long-press handling in touch and mouse user interfaces. Long-press a control to display
     a tooltip for the control, and triggers multi-select in the file browser dialog.

- Audio files in the Tracks directory (only) are displayed using audio-file metadata and artwork if available.

### Minor features in This Build

- Improved enumeration of available audio devices. ALSA devices that are currently in use by other applications are now shown in the list of available audio devices, but are disabled. 

- Select README.md or LICENSE.md files in File Selection dialogs to view their contents inside the PiPedal UI.

- PiPedal respects LV2 plugins' minimum-buffer-size requirements. If PiPedal is using a buffer size smaller than the plugin's minimum size, PiPedal will assemble buffers of the currect size before passing them to the plugin. This allows use of plugins written in Faust, including a number of useful plugins from Guitarix/Lv2 and Gx/Lv2 projects.

Please refer to Beta release notes below for a more complete list of features and bug fixes since the last Release build.

## PiPedal 1.5.91 Beta (Release Candidate)

#### Themes for this release

- Major improvements to TooB plugins; new plugins, new features.

- Support for Ubuntu 25.04 (am64 and aarch64).

#### Major Features

- TooB Neural Amp Modeler: Input and output calibration.

- TooB Convolution Reverb: Predelay, Stretch and Decay controls allow adjustments of impulse response characteristics.

- TooB Convolution Reverb, Freeverb: New "Tails" control, which allows reverb tails to continue after bypassing the effect.

- TooB Parametric EQ: New plugin providing a 4-band parametric EQ. A musically useful Parametric EQ that's suitable both for 
    shaping guitar tone before amp models, or for shaping overall sound at the end of plugin chains. This should become your 
    go-to EQ plugin when using PiPedal.

- TooB 3 Band EQ: New plugin providing a simple 3-band EQ. (Replicates the EQ stage on Steve Atkinson's Windows and Mac-based 
    Neural Amp Modeler plugin).

- TooB CE-2 Chorus, TooB BF-2 Flanger, TooB Phaser: New "Wet/Dry" controls. 

- Noise Gates threshold ranges have been increased in TooB NAM, TooB Input and TooB Noise Gate plugins.

- Support for Ubuntu 25.04. Compatibility fixes for Ubuntu 25.04 may also make it easier to build PiPedal on
    other Debian-based Linux distros.

#### Minor Features/Bug Fixes

- Correct missing '-' key on Samsung keyboards when editing control values.
- Default to dark theme on first use. 
- dB values are now displayed on plugin VU output controls.
- Correctly display name of plugin in MIDI bindings dialog. (Thanks to Jiri Popek).
- Auto-focus input control on Rename dialogs. (Thanks to Jiri Popek).
- Display LICENSE files found in upload directories.

#### Notes

A number of plugins have been modified to provide better functionality going forward. Every effort has been made to prevent 
breaking changes in existing plugins. If you encounter a breaking change in an existing preset, please report it as a bug. If 
you are using presets in live performance, it might be a good idea to check them before you next use them in live 
performance, in order to verify that they have not been inadvertently broken by the upgrade. In particular, CPU use for presets
that are using TooB Convolution Reverb may go up slightly; so if you are using presets that run at or close to maximum CPU use, you should check that they still work after the upgrade.

The EQ section in TooB NAM has been removed. It has become apparent that the existing EQ controls were not entirely 
fit for purpose. To prevent breaking changes, EQ controls will still show up if you
load an instance of TooB NAM that was saved with a previous version of PiPedal; but when you load new instances, the EQ section will not be present. Instead, please consider adding
one of the rich set of old or new TooB EQ plugins before or after TooB NAM instead:

- TooB Parametric EQ (recommended).
- TooB GE-7 Graphic EQ.
- TooB 3 Band EQ (equivalent to Steve Atkinson's NAM EQ section).
- TooB Tone Stack (replicates the old TooB NAM EQ functionality).

Normalization of reverb IRs has been significantly improved in order to ensure that newly loaded IR files 
sound reasonable when loaded using default TooB Convolution Reverb control settings. It should now take 
significantly less effort to adjust levels on Convolution Reverb IR files. In order to prevent breaking 
changes, instances of TooB Convolution Reverb that were saved in previous versions of PiPedal will use 
the old normalization procedure; newly loaded instances of TooB Convolution Reverb will use the new normalization. 

In addition, the MaxT control has been removed. IR files will now play to their full length without 
feeding back. This change may cause overruns on existing presets if you are using unusually long Reverb IR files.
TooB Convolution Reverb can comfortably support reverb IRs well in excess of ten seconds long, even on a Pi 4. For reference, 
typical IR files are about 1 to 3 seconds long; and extremely reverberant IRs may sometimes stretch out to five or six 
seconds long. In practice, the increase in CPU use is relatively modest; but you may want to double-check that 
presets using extremely reverberant IRs run without problems before using them in live performance.

## PiPedal 1.4.87 Beta
The Big Feature: 
- Code optimizations for Raspberry Pi 5 (Cortex-A76), providing substantial performance improvements for TooB Neural Amp Modeler.
- New "Threaded" button optionally executes TooB NAM computation on a separate thread. Allows up to 5 NAM instances on Pi 4, and practically unlimited NAM instances on Pi 5.
- TooB Neural Amp Modeler: Switch to the NeuralAudio NAM Backend
Bug fixes:
- Compatibility with plugins expecting certain MOD host features.
- Improvements to recovery from audio underruns and overruns.
- Fixes for loss of currently selected plugin when modifying pedalboard structure.
- Fixes for TooB NAM lockups when using 16- and 32-sample buffer sizes.
- Fixes for performance issues with TooB NAM when using 16- and 32-sample buffer sizes.
- Fixes for volume adjustments in TooB NAM.

(A re-issue of 1.4.86)

## Pipedal 1.4.79 Beta

The Big Feature:

- Support for custom MOD User Interfaces for plugins that provide a MOD UI. See the [MOD UI documentation page](https://rerdavies.github.io/pipedal/ModUiSupport.html) for details on how to use MOD User Interfaces with PiPedal. This feature is experimental. If you experience problems with plugins that have MOD user interfaces, please report them on the [PiPedal GitHub Issues page](https://github.com/rerdavies/pipedal/issues).

New in this release:

- PiPedal remembers which plugin you were viewing when you reload a preset.
- Keep Screen On setting in the PiPedal Remote Client (requires updated Android client).
- Lock Screen Orientation setting in the PiPedal Remote Client (requires updated Android client).
- Disable the default startup preset if it has previously caused a crash of the PiPedal process.

If you are using the Android PiPedal remote app, you should make sure it has upgraded to the latest version. The new Remote app has been fully rolled out and is now available in the Google Play Store. It should update automatically, but you can check by opening the App Info for PiPedal Remote. If an "Update" button is displayed, click it to upgrade to the current version right away. If you have received the update already, you should notice differences in the App startup procedure (notably an animated splash screen). 

The PiPedal Remote Android app has been updated to provide the following new features:

- new Keep Screen On settings.
- new Lock Screen Orientation setting.
- Edit controls are automatically repositioned above the Keyboard IME when it opens.
- The Zoomed Control feature (large controls in landscape mode on phones) has been removed. Improved scrolling provides a better solution to the problem of controls being obscured by the keyboard IME.
- Support for Android 16 Edge-to-Edge display.
- Reduced display flickering while connecting. 
- Complete support for Night Mode (the Android UI now follows the PiPedal Theme setting).
- Shiny new graphics on the opening screen in support of Android 16's Edge-to-Edge display feature (but you can see it in other versions of Android as well).


Bug Fixes:

- The "Listen for Message" buttons in the MIDI Bindings dialog now ignore CC0 (Bank Select), and CC32-63 (CC LSB) MIDI message, since CC0 is used to control preset selection.
- Fixed errors loading plugins with MIDI Event ports.
- Changing volume control on low-resolution displays breaks web page.
- MIDI binding for bypass controls have been restored after having been accidentally disabled in v1.4.77.


## PiPedal 1.4.77 Beta

New TooB Player plugin; improvements to MIDI input handling; and a slew of fit-and finish 
improvements to controls in the Pipedal user interface. All in all, a surprisingly large
release.

The big features for this release

- a new TooB Player plugin that allows you to play (and loop) audio files.

- Switch from ALSA rawmidi devices to ALSA sequencer devices for MIDI input. This provides
      a number of useful improvements. PiPedal now supports Bluetooth MIDI controllers.
      PiPedal provides better support for subdevices (multiple MIDI connections on the same device). 
      Connections to the PiPedal MIDI input port can now be made at runtime by other applications, or 
      by using`aconnect`. Changing MIDI devices does not require a restart of the audio stream. Disconnected
      MIDI devices that have been selected will now be automatically reconnected when they are 
      plugged in again.

- MIDI bindings allow users to specify minimum and maximum control values, in order to better support expression pedals.

- long-press and hover Tooltips are now displayed for all controls in the PiPedal UI.

- Editable plugin labels. You can now edit the labels of plugins in the PiPedal UI. Tap the plugin label to override
    the default label with a custom label. Useful when you have multiple instances of the same plugin in a preset.

- Multi-select support in file browser dialogs. Allows you to easily delete, copy or move multiple files at once.

- Dial control values are displayed in a fly-out when editing, which makes it easier to determine the
     current value of a control when using touch user interfaces.

-  Direct link to TONE3000 website from the TooB Neural Amp Modeler file browser.

- Double-tap gestures now supported in touch user interfaces.  Double-tap a dial to reset it to its default value. Double-tap
    a file in the File Browser to select it. Double-tap a plugin label to edit it.

- Improvements to long-press handling in touch and mouse user interfaces. Long-press a control to display
     a tooltip for the control, and triggers multi-select in the file browser dialog.

- Audio files in the Tracks directory (only) are displayed using audio-file metadata and artwork if available.

Minor bug fixes:

- WiFi scanning is only performed when required by the current Auto-Hotspot
  configuration. WiFi is not turned on when PiPedal starts, unless the Auto-Hotspot configuration requires it.

- MIDI controls continue to operate if the current audio stream has stopped.

- Audio files with invalid UTF-8 sequences in their names are ignored (instead of causing crashes).

- Media files that are referenced in snapshots are now included in preset and bank file bundles.

- Changed all references to tonehunt.org to tone3000.com (tonehunt's new name).

## PiPedal 1.4.76 Release
- **TooB Phaser**. A loose emulation of the MXR Phase 90 phaser.
- **TooB GE-7 Graphic Equalizer**. A 7-band graphic equalizer optimized for use as a guitar effect.


## PiPedal 1.4.75 Release

New plugins:
- **TooB Noise Gate**. A standalone Noise Gate plugin. 
- **TooB Volume**. A standalong plugin to adjust signal volume. 
- **TooB Mix**. A standalone plugin to remix stereo signals.

Bug fixes:
- TooB Tuner improved stability, and accuracy. Allow tuning of notes above Bb4.
- Fix TooB Recorder file playback.



## PiPedal 1.4.74 Release

New plugins:

- **TooB 4Looper** - a 4-track looper. See the plugin information dialog for details on how to use 4Looper with MIDI footswitches.

- **TooB One-Button Looper** - A 1-button looper with overdubbing and undo/redo, designed for use with a single MIDI footswitch. See the plugin information dialog for details on how use TooB One-Button Looper with MIDI.

- **TooB Record Input (mono and stereo versions)** - Plugins that record audio input to a file in .wav, .flac, or .mp3 format. Recorded files can be downloaded from the PiPedal server using the Download button in the file dialog.


New features:

- Official support for Ubuntu 24.x (arm64 and amd64)
- Support for WiFi hotspots on Ubuntu.
- Recorded audio files can be downloaded from the PiPedal server.
- Added Trigger Recording mode to looper plugins.
- Increased maximum number of audio buffers from 4 to 6.
- Support for momentary LV2 MIDI footswitches using MOD-compatible LV2 declarations (commonly used in 3rd-party looper plugins)

## PiPedal 1.4.73 Beta

New plugins:

- **TooB 4Looper** - a 4-track looper. See the plugin information dialog for details on how to use 4Looper with MIDI footswitches.

- **TooB One-Button Looper** - A 1-button looper with overdubbing and undo/redo, designed for use with a single MIDI footswitch. See the plugin information dialog for details on how use TooB One-Button Looper with MIDI.

- **TooB Record Input (mono and stereo versions)** - A plugin that records audio input to a file.

New features: 

- Recorded audio files can be downloaded from the PiPedal server.

Bug Fixes/Minor Features:

- Support for momentary MIDI controls using MOD-compatible LV2 declarations.

- Apply grouping to controls in the Info dialog and MIDI binding dialogs.

- TooB Amp LV2 plugin compatibility fixes for native LV2 hosts other than PiPedal. (Reaper particularly)


## PiPedal 1.4.72 Beta

- Resolves installation issues on Raspberry Pi OS (no alsa-base package)

## PiPedal 1.4.71 Beta

- Resolves installation issues on Ubuntu Server./

## PiPedal 1.4.70 Beta

- Ubuntu 24.x compatibility (arm64 and amd64)
- Ubuntu CPU Freq/governor/thermal status display.
- Support for WiFi on on Ubuntu 
- TooB Plugin compatibility issues on x64/amd64 (handling of NaNs)
- Restart audio automatically after ALSA stalls.
- Improvements to File Upload UI for plugins that have complex file types (e.g. Ratatouille)
- Support for LV2 Trigger controls, and MIDI bindings to LV2 trigger controls (common in looper and recording/playback plugsin).
- MIDI bindings for LV2 trigger controls.
- Port Web app from CRA to Vite framework to avoid deprecated dependencies.
- ToobAmp 1.1.57: Correctly set file path of the default file property for Toob Convolution Reverb.

## PiPedal 1.3.69

Bug fixes:
- Fix file uploads for TooB NAM and TooB Convolution Reverb.
- Load actual list of Wi-Fi regulatory domains from regulatory database.
- Reload channel list after selecting a new Wi-Fi regulatory domain.
- Hotspot configuration dialog: validate country code and channel selection.
- Add `--fix-permissions` and `-list-wifi-country-codes options` to `pipedalconfig`
- Correct error in `wifi_latency_test` help text.
- Cleaned up Debian package dependencies.
- Correct bug with mDNS service announcements after a name collision.
- Cleaned up build prerequisite documentation for PiPedal and ToobAmp projects.
- Clean service shutdown: avoid double-close of web server sockets.
- Numerous fixes for Ubuntu 24.04 LTS builds for arm64 and amd64. (experimental)

## PiPedal 1.3.66 Release

Bug fixes:
- Choose 24-bit or 32-bit audio formats instead of 16-bit audio formats on devices that support both.
- Force selection of stereo channel configuration for legacy I2C audio drivers without support for channel maps.
- Avoid selection of hardware-downmixed Surround Sound channel configurations.

Minor features:
- Improved diagnostic tools for ALSA devices via `pipedalconfig --alsa-devices`

## PiPedal 1.3.64 Beta

Bug fixes:
- Always choose the largest available ALSA sample format.
- With ALSA devices for which maximum and minimum channels are not equal, prefer stereo channel configuration.


## PiPedal 1.3.63 Experimental

Bug fixes:
- With ALSA devices for which maximum and minimum channels are not equal, prefer stereo channel configuration.


## PiPedal 1.3.62 Beta

Features:
- Plugins can now share uploaded model and IR files by type. For plugins with appropriate mod:fileTypes attributes, uploaded
  files will be shared by category (Nam model, IR Files, Audio Tracks, Midi Songs, etc). Sharing of ML models are also supported, although 
  that requires use of an extended mod:fileType (mlmodel) that MOD does not support. 
- TooB Tuner stability issues fixed. The pitch detection algorithm has been improved, and the tuner now does filtering to avoid 
  displaying a value when the current input isn't a steady note.
- LV2 trigger controls are now displayed as buttons instead of toggle controls (maybe useful for 3rd-party looper plugins).

Known issues:

- There are a number of outstanding minor compatibility issues with Ratatouille. I intend to work with the author to address them.

### Backwards Compatibility With the Previous Upload Directory Scheme

Sharing file types poses a problem with previous versions of PiPedal which did not share uploads by file type, but instead placed uploads in a private directory for each plugin.
In order to accomodate this, Pipedal provides legacy support for this situation. If a private upload directory for a plugin exists, the file property selection dialog will show
both the new shared directories, and the old private directory. The private directory has a name that reflects the name of the plugin (e.g. "Ratatoille.lv2"). This directory only
shows up if you have used the plugin on previous versions of Pipedal. If you have nothingof particular value in the old private upload directory, you can delete the plugin's private directory (which can be found in `/var/pipedal/audio_uploads`), and the private directory will no longer be displayed in the PiPedal UI.

I anticipate providing a migration utility in the near future which will clean up and migrate legacy upload directories to the new directory structure (automatically uploading
presets which reference uploaded files that have moved). In the meantime, I think you will find the current occomodation for legacy upload directories perfectly functional. 



## PiPedal 1.3.61 Release

Bug fixes:
- Media files are not extracted properly from preset and bank files.
- Tuner control displayed for 3rd party tuner plugins that have outputs of type units:hz

## PiPedal 1.3.60 Release

Bug fixes:
- Uploading of preset and bank files now  work properly.
- Uploaded plugin presets are merged with existing presets, instead of overwriting existing presets.

## PiPedal 1.3.59 Release

20% performance improvement when using Toob Neural Amp Modeller with large Wavenet models.

Minor bug fixes:
- Display file extensions in File Property dialogs.

(re-issue of v1.3.58)

## PiPedal 1.3.57 Release

Features:
- dB readings displayed as text under the main VUs.
- Snapshot button title text now gets larger on large screens. 
- Selection indicators on Snapshot buttons are more visible.


Bug fixes:
- Setting Dark Mode to follow system settings now tracks properly on the Android PiPedal Remote app.
- No longer crashes when using an audio adapter with mono input.
- Reload after a being suspended in the Android Client in order to ensure that controls and VUs continue to update. Not ideal, but when Android suspends a web app, web socket messages get silently discarded, so there's not much choice about it.
- GxTuner display now updates.
- Correct Escape and Enter key handling in search control of the Load Plugin dialog.
- Android remote: can connect when both Data connection and hotspot connection are active.
- Stack dumps get written to systemd log on segment violations (should make it easier to diagnose crashes in the field).

Known issues:
- To get the Android client to correctly follow system Dark Mode settings, you must update the Android app. You can work around the problem by explicitly selecting Light or Dark mode instead.
- To get the Android client to connect when both the hot-spot and data connections are active, you must update the Android app. In the meantime, you can work around the problem by going to Android Network Settings, and clicking to disconnect and then reconnect the hot-spot connection (which will disable the data connection). 

The updated Android app has been submitted, and should show up on Google Play sometime tomorrow morning.


## PiPedal 1.3.56 Release

More snapshot problems.

Bug fixes:
- MIDI bindings cause crash.
- Crashes related to snapshots and split controls.
- Loading/reloading/error screens go on top of Snapshot dialogs.
  

## PiPedal 1.3.54 Release

Improvements to and fixes for hotspot functionality. Fixes for problems with snapshots.

Minor features;
- Enter key supported in most dialogs. 

- Uploaded .zip files create a top-level directory with the name of the zip file if the zip file doesn't contain a single root-level directory. (released in 1.3.53)

Bug fixes:
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
