---
page_icon: img/Plugins.jpg
---
## Using LV2 PLugins

{% include pageIcon.html %}
PiPedal uses LV2 audio plugins. There are literally thousands of freely available high-quality LV2 plugins that are suitable for use as guitar effects.

By default, PiPedal comes with a basic set of plugins from the ToobAmp plugin collection. You will probably want to install more.

Visit [PatchStorage](https://patchstorage.com/platform/lv2-plugins/) to download LV2 pluginst that have been precompiled for use on Raspberry Pi. To see LV2 plugins
that are specifically for Raspberry Pi, click on the `Target` dropdown button and select `rpi-aarch64`.

Here is a brief list of particularly recommended plugin collections.dhcp

| Collection                      | To Install                            | Description      |
|---------------------------------|---------------------------------------|------------------|
|[Guitarix](https://guitarix.org) ★★★★☆ | `sudo apt install guitarix-lv2`         | A large collection of guitar amplifiers and effects. |
| [GxPlugins](https://github.com/brummer10/GxPlugins.lv2) ★★★★★   | [Install GxPlugsin.lv2](GxPlugins.md) | Additional effects from the Guitarix collection |
| [MDA Plugins](http://drobilla.net/software/mda-lv2.html) ★★★★☆ | `sudo apt install mda-lv2` | 36 high-quality plugs |
| Invada Studio Plugins ★★★☆☆         | `sudo apt install invada-studio-plugins-lv2` | Delays, distortion, filters, phaser, reverb |
|[Zam Plugins](https://zamaudio.com) ★★★☆☆ | `sudo apt install zam-plugins`   | Filtering, EQ, and mastering effects. |
| [Calf Studio Gear](https:://calf-studio-gear.org) ★★★☆☆ | `sudo apt install calf-plugins` | Flanger, filters, reverb, rotary speaker &c |

The GxPlugins pack requires a manual build; but it's worth the effort. A pre-built version for aarch64 can be downloaded [here](GxPlugins.md). It contains some extraordinarily beautiful amp and effect emulations, many of which are superior to plugins in the main Guitarix package.

For a more complete (but still incomplete) list of LV2 audio plugins, see [here](https://lv2plug.in/pages/projects.html).


--------
[<< Changing the Web Server Port](ChangingTheWebServerPort.md)  | [Up](Documentation.md) | [Which LV2 Plugins Does PiPedal Support? >>](WhichLv2PluginsAreSupported.md)
