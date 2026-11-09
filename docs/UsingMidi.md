---
---
## Using MIDI with PiPedal

If you connect an external MIDI controller to PiPedal, you can use it to control various aspects of the software. A MIDI controller can be connected to PiPedal via a USB port, or a MIDI cable if your audio adapter has a MIDI input.

To configure PiPedal to use MIDI, open the PiPedal Settings dialog and tap the "Select MIDI Input..." list item. From there, you can choose one or more of the MIDI input device you want PiPedal to listen to. Once selected, PiPedal will start receiving MIDI messages from the chosen input devices. PiPedal allows you to select from all available ALSA MIDI sequencer devices that the system recognizes. If you can't see your device, you may need to debug using ALSA command-line utlitities, such as `aseqdump` to verify that your external MIDI device is properly connected and recognized by the system.

By default, PiPedal operates in "OMNI" mode, meaning that it will respond to MIDI messages on all MIDI channels on all selected MIDI devices. You can configure PiPedal to respond only to a specific MIDI channel by tapping the "Select MIDI Input..." list item found within the PiPedal Settings dialog.

Note that MIDI messages are not currently passed to individual LV2 plugins, other than through the MIDI bindings dialogs described below. This may change in a future version of PiPedal, when more comprehensive MIDI support is added for instrument plugins.

PiPedal responds to MIDI program change and MIDI bank change messages automatically. When you select a bank via MIDI, PiPedal will use the selected bank for subsequent program changes within that bank. Note that the selected bank in the user interface does not change until PiPedal receives a MIDI program change. MIDI bank 0 corresponds to the first bank displayed in the PiPedal user interface (see the main Hamburger menu), MIDI bank 1 corresponds to the second bank, and so on. You can reorder PiPedal's banks using the "Manage Banks..." menu entry found in the main hamburger menu. To reorder banks, long-press on a bank in the Manage Banks dialog and drag it into the position you want.

Previous versions of PiPedal incorrectly responded only to the MIDI Bank MSB message (CC0). This has been corrected in the current version of PiPedal. You should use MIDI CC32 to select banks, not MIDI CC0. Use of CC0 to select banks is deprecated. PiPedal will currently respond to either CC0 or CC32 for bank selection (but not both); This behaviour is provided only for backward compatibility, and is likely to be removed in a future version of PiPedal. This does mean that you are currently only able to select from the first 128 banks, but this seems like a reasonable compromise for maintaining backward compatibility.

MIDI program change messages are used to select a specific program within the currently selected bank. Selecting Program 0 will select the first preset in the active bank. Selecting Program 1 will select the second preset, and so on. You can reorder presets in a bank using the "Manage Presets..." menu entry found in the dropdown menu beside the Preset selection dropdown control in the top panel in the UI. 
To reorder presets in the current bank, long-press on a preset in the Manage Presets dialog and drag it into the position you want.

MIDI note and control events can be used to control PiPedal, using either the System MIDI Bindings list item found in the PiPedal settings dialog or the Preset MIDI Bindings button, which is the second button from the left in the group of buttons found immediately below the pedalboard panel.

The System MIDI Bindings dialog allows you to bind MIDI notes to various global functions within PiPedal. 

- Select the next or previous bank.
- Select the next or previous preset in the current bank.
- Select from up to six setting snapshots within the current preset (see [An Intro to Snapshots](Snapshots.md) for more details).
- Enable or disable the PiPedal Wi-Fi hotspot.
- Shut down or reboot the PiPedal server.

The Preset MIDI Bindings dialog allows you to assign MIDI note and control change messages to plugin controls within the currently active preset. The Preset MIDI Bindings dialog displays a list of all plugins within the current preset, plugin controls for each plugin, along with a Bypass control that can be used to bypass individual plugins. Split controls are also displayed, so you can, for instance, use MIDI messages to toggle between A and B routes of a split. As a general rule, you should map continuous controllers (knobs) to MIDI controllers, and switches, toggles, and trigger controls to MIDI notes. If your hardware MIDI controller cannot be configured to send a note-on/note-off message, you may need to use control change messages instead; however, this should be avoided if possible, as the interpretation of what constitutes an "ON" or "OFF" control message can vary between hardware controllers. The behavior of switches bound to a continuous controller may be confusing, so, if possible, it is better to configure your hardware controller to send MIDI note messages.

Note that System MIDI Bindings take precedence over Preset MIDI Bindings. If you bind a MIDI note to a System MIDI Binding, it will not be able to use it for a Preset MIDI Binding. 

PiPedal does not provide a MIDI thru port. However, multiple programs can connect to the same ALSA MIDI Sequencer port simultaneously, allowing MIDI messages to be shared between PiPedal and other software. For example, if you want to sync PiPedal preset changes with an external MIDI device, you can use ALSA command-line tools to forward MIDI messages from the MIDI input port that PiPedal uses to the external MIDI device.


--------
[<< [Building Presets](BuildingPresets.md) | [Up](Documentation.md) | [An Intro to Snapshots >>](Snapshots.md)
