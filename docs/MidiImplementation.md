---
---
## Appendix A: MIDI Implementation

By default, PiPedal operates in "OMNI" mode, meaning that it will respond to MIDI messages on all channels. You can configure PiPedal to respond only to a specific MIDI channel by tapping the "Select MIDI Input..." list item found within the PiPedal Settings dialog.

Note that MIDI messages are not currently passed to individual LV2 plugins. This may change in a future version, when PiPedal adds support for instrument plugins.

PiPedal responds to MIDI program and MIDI bank change messages. When you select a bank via MIDI, PiPedal will use the selected bank for subsequent program changes within that bank. Note that the selected bank in the user interface does not change until PiPedal receives a MIDI program change. MIDI bank 0 corresponds to the first bank displayed in the PiPedal user interface, MIDI bank 1 corresponds to the second bank, and so on. You can reorder PiPedal's banks using the "Manage Banks..." menu entry found in the main hamburger menu. To reorder banks, long-press on a bank in the Manage Banks dialog and drag it into the position you want.

Previous versions of PiPedal incorrectly responded only to the MIDI Bank MSB message (CC0). This has been corrected in the current version of PiPedal. You should use MIDI CC32 to select banks, not CC0. Use of CC0 to select banks is deprecated. PiPedal will respond to either CC0 or CC32 for bank selection, but this behaviour is provided only for backward compatibility, and is likely to be removed in a future version of PiPedal. This does mean that you are currently only able to select from the first 128 banks, but this seems like a reasonable compromise for maintaining backward compatibility.

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

&nbsp;

### MIDI Implementation Chart

<table class="midi-table" style="border: 2px solid black;"> 
 <thead > 
  <tr >
    <th colspan="2"  >Function</th>
    <th>Transmitted</th>
    <th>Recognized</th>
    <th>Remarks</th>
  </tr>
 </thead>
 <tbody>
  <tr>
    <td class="tdl" align="left">Basic<br/>Channel</td>
    <td class="tdr" align="left">Default<br/>Changed</td>
    <td align="center">X<br/>X</td>
    <td align="center">0 - 127<br/>0 - 127</td>
    <td align="left">Selected in UI<br/>In active bank</td>
  </tr>
  <tr>
    <td align="left" colspan="2">Mode</td>
    <td align="center">X</td>
    <td align="center">X</td>
    <td align="left"></td>
  </tr>
  <tr>
    <td align="left" colspan="2">Note<br/>Number</td>
    <td align="center">X</td>
    <td align="center">0 - 127</td>
    <td align="left">Available for binding *1</td>
  </tr>
  <tr>
    <td align="left" colspan="2">Velocity</td>
    <td align="center">X</td>
    <td align="center">X</td>
    <td align="left"></td>
  </tr>
  <tr>
    <td align="left" colspan="2">After Touch</td>
    <td align="center">X</td>
    <td align="center">X</td>
    <td align="left"></td>
  </tr>
  <tr>
    <td align="left" colspan="2">Pitch Bender</td>
    <td align="center">X</td>
    <td align="center"></td>
    <td align="left"></td>
  </tr>
  <tr>
    <td class="tdl" align="left">Control<br/>Change</td>
    <td class="tdr" align="right">0<br/>1 -31<br/>32<br/>33-127</td>
    <td align="center">X<br/>X<br/>X<br/>X</td>
    <td align="center">O<br/>O<br/>O<br/>O</td>
    <td align="left">Bank Select MSB<br/>Available for binding *1<br/>Bank Select LSB<br/>Available for binding *1</td>
  </tr>  
  <tr>
    <td colspan="2" align="left">RPNs</td>
    <td align="center">X</td>
    <td align="center">X</td>
    <td align="left"></td>
  </tr>  
  <tr>
    <td colspan="2" align="left">NRPNs</td>
    <td align="center">X</td>
    <td align="center">X</td>
    <td align="left"></td>
  </tr>  
  <tr>
    <td align="left" colspan="2">Prog<br/>Change</td>
    <td align="center">X</td>
    <td align="center">0 - 127</td>
    <td align="left">Preset index in active bank</td>
  </tr>
  <tr>
    <td align="left" colspan="2">System Exclusive</td>
    <td align="center">X</td>
    <td align="center">X</td>
    <td align="left"></td>
  </tr>
  <tr>
    <td class="tdl" align="left">System<br/>Common</td>
    <td class="tdr" align="left">: Song Pos<br/>: Song Sel<br/>: Tune</td>
    <td align="center">X<br/>X<br/>X</td>
    <td align="center">X<br/>X<br/>X</td>
    <td align="left"></td>
  </tr>
  <tr>
    <td class="tdl" align="left">System<br/>Realtime</td>
    <td class="tdr" align="left">: Clock<br/>: Command</td>
    <td align="center">X<br/>X</td>
    <td align="center">X<br/>X</td>
    <td align="left"></td>
  </tr>
  <tr>
    <td class="tdl" align="left">Aux<br/>Messages</td>
    <td class="tdr" align="left">: Local ON/OFF<br/>: All Notes Off<br/>Active Sense<br/>Reset</td>
    <td align="center">X<br/>X<br/>X<br/>X</td>
    <td align="center">X<br/>X<br/>X<br/>X</td>
    <td align="left"></td>
  </tr>
  <tr>
    <td class="tdl" align="left" colspan="1">Notes</td>
    <td class="tdr" align="left" colspan="4">
      * 1 - Available for MIDI bindings, but not sent to LV2 plugins.
    </td>
  </tr>
 </tbody>
</table>


--------
[<< PiPedal Architecture](Architecture.md) | [Up](Documentation.md)
