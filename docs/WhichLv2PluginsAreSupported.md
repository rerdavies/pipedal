## Which LV2 Plugins does PiPedal support?

PiPedal will automatically detect installed LV2 plugins and make them selectable from the web app interface, as long as they meet the 
following conditions:

- Must have mono or stereo audio inputs and outputs.

- Must not be a MIDI instrument or have CV (Control Voltage) inputs or outputs.

- Must be remotely controllable (no hard dependency on GUI-only controls), which is true of all but a tiny minority of LV2 plugins.

If you install a new LV2 plugin, PiPedal will detect the change and make it available immediately. Wait a few seconds, and the newly-installed plugin should show up in the list of avaialable plugins.

PiPedal does all plugins to use custom user interfaces.  However, we would be pleased to collaborate with developers who need more than the basic set of controls. Please contact rerdaves at gmail.com for further details.

--------
[<< Using LV2 Audio Plugins](UsingLv2Plugins.md)  | [Up](Documentation.md) | [BuildingPiPedal from Source >>](BuildingPiPedalFromSource.md)
