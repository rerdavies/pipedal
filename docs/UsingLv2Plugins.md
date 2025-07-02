---
page_icon: img/Plugins.jpg
---
## Using LV2 PLugins

{% include pageIcon.html %}
PiPedal uses LV2 audio plugins. There are literally thousands of freely available high-quality LV2 plugins that are suitable for use as guitar effects.

To get you started, PiPedal comes with a basic set of plugins from the ToobAmp plugin collection. You will definitely want to install more.

LV2 Plugins are available from a wide variety of sources. Some are available through apt installs; others can be downloaded from a variety places on the web. Some are available through github, and some even require you to build the plugins yourself. Pipedal will automatically detect freshly installed LV2 plugins. New plugins should show up in the list of available plugins a few seconds after the install completes. You don't need to restart PiPedal after installing a new plugin.

A good place to get started is the [PatchStorage](https://patchstorage.com/platform/lv2-plugins/) website. PatchStorage provides hundreds of LV2 plugins that have been precompiled and are ready to install on Raspberry Pi. To see LV2 plugins that are specifically for Raspberry Pi on PatchStorage, click on the `Target` dropdown button and select `rpi-aarch64`. 

Here is a brief list of other particularly recommended plugin collections.

| Collection                      | To Install                            | Description      |
|---------------------------------|---------------------------------------|------------------|
| [Guitarix](https://guitarix.org) ★★★☆☆ | `sudo apt install guitarix-lv2`         | A large collection of guitar amplifiers and effects. Prior to the advent of neural network amplifier emulations, Guitarix was the go-to source for open-source amp modeling.|
| [GxPlugins](https://github.com/brummer10/GxPlugins.lv2) ★★★★★   | `sudo apt install gxplugins` | Additional amps and effects from the the principle developer of Guitarix, many of which are significantly superior to the original Guitarix plugins. Most of these plugins 
use circuit simulation to directly model actual original circuits. The results are uniformly excellent.|
| [DPF Plugins](https://github.com/DISTRHO/DPF-Plugins) ★★★★★   | `sudo apt install dpf-plugins-lv2` | Useful and interesting guitar effect plugins. Bit crusher, pitch shift, reverbs, 3-band EQ and more. |
| [MDA Plugins](http://drobilla.net/software/mda-lv2.html) ★★★★☆ | `sudo apt install mda-lv2` | 36 high-quality plugs |
| Invada Studio Plugins ★★★☆☆         | `sudo apt install invada-studio-plugins-lv2` | Delays, distortion, filters, phaser, reverb |
| [Zam Plugins](https://zamaudio.com) ★★★☆☆ | `sudo apt install zam-plugins`   | Filtering, EQ, and mastering effects. |
| [Calf Studio Gear](https:://calf-studio-gear.org) ★★★☆☆ | `sudo apt install calf-plugins` | Flanger, filters, reverb, rotary speaker &c |
| [Dragonfly Reverb](https://michaelwillis.github.io/dragonfly-reverb/) ★★★☆☆ | `sudo apt install dragonfly-reverb-lv2` | Reverbs |
| [x42](https://x42-plugins.com/x42/) ★★☆☆☆ | `sudo apt install x42-plugins` |Compressors, auto-tune, tuners |

For a more complete (but still incomplete) list of LV2 audio plugins, see [here](https://lv2plug.in/pages/projects.html).


--------
[<< Changing the Web Server Port](ChangingTheWebServerPort.md)  | [Up](Documentation.md) | [Which LV2 Plugins Does PiPedal Support? >>](WhichLv2PluginsAreSupported.md)
