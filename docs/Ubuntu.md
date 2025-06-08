---
page_icon: img/ubuntu.jpg
icon_width: 320px
icon_float: left
---
## PiPedal support for Ubuntu

{% include pageIconL.html %}

PiPedal will run on Ubunut 24.x (both aarch64 and amd64/x64) as well as Raspberry Pi OS. We recommend using Ubuntu Desktop, but Raspberry Pi OS will also run on an Ubuntu Server install. Ubuntu Studio would also be a fine choice. 

## Configuring a preempt=full Kernel on Ubuntu 24.x 

By default, Ubuntu installs a PREEMT_DYNAMIC kernel, configured to run with volutary preemption. PREEMPT_DYNAMIC/preempt=voluntary kernels are not capable of supporting real-time audio. To run low-latency realtime audio on Ubuntu, you must reconfigure Ubuntu to use a PREEMPT_DYNAMIC/preempt=full kernel. The PiPedal installer includes a convenient command-line utility that allows you to make the change without having to do painful editing of intimidating system files. Although this is a relatively harmless change, it seemed inappropriate to reconfigure the kernel in the Pipedal package installer. Run the following command-line command after you have installed PiPedal:

```
   pipedal_kconfig
```

and then select Preempt=full. Switching between PREEMPT_DYNAMIC is a configuration-only change, and does not install new components. The switch actually takes place at runtime. Astonishingly, you do not even need to reboot!

Full realtime kernels (RT_PREEMPT) may provide slightly better realtime audio than a PREEMPT_DYNAMIC/preempt=full kernel, but the PREEMPT_DYNAMIC kernel should be perfectly adequate. As a point of reference, current versions of Ubuntu Studio uses a PREEMPT_DYNMAIC/preempt=full kernel, and the maintainers of Ubuntu Studio consciously chose to not install an RT_PREEMPT kernel. If you are having problems with intermittent audio dropouts that occur very infrequently, you may want to try an RT_PREEMPT kernel instead. The pre-built Ubuntu RT_PREEMPT kernel can be installed using `apt` but is only available if you have an Ubuntu Pro subscription. Ubuntu Pro is free for non-commercial use. 

## Support for WIFI Auto-Hotspots on Ubuntu Server

The following information applies when installing PiPedal on Ubuntu Server 24.0x only. This information does not apply 
to Unbuntu Desktop installs or to Raspberry Pi OS or Raspberry Pi OS Lite installs, which use the Network Manager TCP/IP stack by default. 

PiPedal's Auto-Hotspot feature only works on Linux systems that are using the Network Manager network services stack.
Ubuntu Server 24.x (and probably other server-specific installs on other Debian distros) uses the Netplan network services stack instead. It does so because the Netplan network stack is the preferred network stack when managing  farms of cloud servers. 

It's easy enough to reconfigure Ubuntu Server to run the Network Manager network stack instead. However you should be 
aware that Netplan TCP/IP configuration settings will not be migrated to Network Manager. If you perform this step on a server on which changes have beeen made to Netplan configuration files, you will lose those configuration changes, and will need to reimplement them in Network Manager. If you are 
a Netplan configuration (or are working with a clean install), then the default Network Manager configuration will work perfectly well too. On a clean install, or for ordinary
home use, switching network stacks is pretty low risk.

To reconfigure Ubuntu Server to use the Network Manager network stack (and therefore enable PiPedal's Auto-Hotspot feature), run the following command: 

    sudo apt install network-manager

PiPedal runs perfectly well with the netplan network stack; but the auto-hotspot will be disabled (and actually won't even show up in the UI).

## Running Headless

To get the best possible audio latency, you should run your Ubuntu computer headless. GPU activity interferes with low-latency audio. Drawing to the screen (or even moving the mouse) can cause audio underruns. This does not mean that you cannot use a desktop install; but it does mean that you should not be using the desktop when using PiPedal. Use a browser on a remote machine, or use a phone or tablet to control PiPedal. 

It is not entirely clear why GPUs don't play well with realtime low-latency audio. It's probably not caused by interrupts, but may be caused by contention for system memory and various system buses. If you are running a very powerful PC with a GPU that doesn't share system memory, you may be able to run a desktop while using PiPedal; but if you are not getting the latency you think you should, try going back to headless operation. 

Headless, in this context means: nothing is using the GPU. On a Raspberry Pi, it is sufficient to just disconnect the HDMI cables, and not be using a remote desktop connection. It is uncertain whether Ubuntu will stop using the GPU if there is an active desktop that is (for example) updating status indicators on the status bar. If in doubt, try disabling automatic login. The system will then stop at the login screen, which does not do any drawing until you start typing credentials. Configuring your Ubuntu desktop to use a text-mode interface instead of a graphical interface would, of course, be perfect; but it seems unnecessarily inconvenient. On an Ubuntu server install, there is no graphical desktop, so this is not a problem. But see the section above about configuring Ubuntu Server before you choose a Server install of Ubuntu Desktop.

You might want to consider purchasing a tiny N95, N100 or N150 micro-pc on which to run PiPedal. They are cheap, provide plenty of CPU power, and are exactly the sort of hardware you need to run PiPedal on at a gig, or away from home where you don't have access to a wi-fi network, or a monitor, or a keyboard. N100-series micro-pcs are in many ways preferrable to a Raspberry Pi 5. They provide significantly more compute power, and cost only slightly more.

