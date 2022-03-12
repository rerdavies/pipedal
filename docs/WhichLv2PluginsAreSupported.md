## Which LV2 Plugins does PiPedal support?

PiPedal will automatically detect installed LV2 plugins and make them selectable from the web app interface, as long as they meet the 
following conditions:

- Must have mono or stereo audio inputs and outputs.

- Must not be MIDI instruments or have CV (Control Voltage) inputs or outputs.

- Must be remotely controllable (no hard dependency on GUI-only controls), which is true of the vast majority of LV2 plugins.

If you install new LV2 plugins, you will have to restart the PiPedal web service (or reboot the machine) to get them to show up in the web interface.

   sudo pipedalconfig --restart

Although most LV2 plugins provide GUI interfaces, when running on a Linux desktop, the LV2 plugin standard is specifically designed to allow remote control 
without using the provided desktop GUI interface. And all but a tiny minority of LV2 plugins (most of them analyzers, unfortunately) support this.

--------
[<< Using LV2 Audio Plugins](UsingLv2Plugins.md)  | [Up](Documentation.md) | [BuildingPiPedal from Source >>](BuildingPiPedalFromSource.md)