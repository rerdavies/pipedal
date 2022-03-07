## Using LV2 PLugins

PiPedal uses standard LV2 audio plugins for effects. There's a huge variety of LV2 guitar effect plugins, and plugins collections, many 
of which are specifically intended for use as guitar effect plugins. Ubuntu Studio comes with an enormous collection of LV2 plugins preinstalled. On Rasbian, you will have to manually select and install the plugins you want to use.

The following LV2 Plugin collections (all recommended) are available on both Rasbian and Ubuntu.

  sudo apt install guitarix-lv2     # https://guitarix.org/
  
  sudo apt install zam-plugins    # http://www.zamaudio.com/
  
  sudo apt install mda-lv2  # http://drobilla.net/software/mda-lv2/
  
  sudo apt install calf-plugins  # http://calf-studio-gear.org/
  
  sudo apt install fomp  # http://drobilla.net/software/fomp/

But there are many more.

Visit https://lv2plug.in/pages/projects.html for more suggestions.

There is also a set of supplementary Gx effect plugins which did not make it into the main Guitarix distribution. You will
have to compile these plugins yourself, as they are not currently available via apt. But if you are comfortable building
packages on Raspbian, the effort is well worthwhile. The GxPlugins collection provides a number of excellent boutique amp emulations,
as well as emulations of famous distortion effect pedals that are not part of the main Guitarix distribution.

- https://github.com/brummer10/GxPlugins.lv2
