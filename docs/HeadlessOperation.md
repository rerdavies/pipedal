---
page_icon: img/headless.jpg
icon_width: 120px
icon_float: right
---
## Headless Operation

{% include pageIcon.html %}

## Running Headless

To get the best possible audio latency, your PiPedal server should run headless. GPU activity interferes with low-latency audio. Drawing to the screen (or even moving the mouse) can cause audio underruns. This does not mean that you cannot use a desktop install; but it does mean that you should not be using the server's desktop when using PiPedal. Use a browser on a remote machine, or use a phone or tablet to control PiPedal. PiPedal is very much designed on the expectation that you will be using a remote device to control it. 

It is not entirely clear why GPUs don't play well with realtime low-latency audio. It's probably not caused by interrupts, but may be caused by contention for system memory and various system buses. Or perhaps by graphics drives that cheat a bit in order to get better benchmark scores. If you are running a very powerful PC with a GPU that doesn't share system memory for its framebuffer, you may be able to run a desktop while using PiPedal; but if you are not getting the latency you think you should, try going back to headless operation. The difference is dramatic. A system that may struggle to achieve 15ms latency with a desktop running (256x3 buffer configuration), should easily be able to achieve sub-5ms latency (32x3 buffer configuration) when running headless. As a point of reference, 10ms latency is generally considered the maximum threshold for usable realtime audio that doesn't feel spongy and unpleasant.

If you are running on a Raspberry Pi, it doesn't seem that strange run the Pi without a monitor connected. If you are using a laptop or desktop computer, running Ubuntu 24.x, this may seem a bit odd. But it matters! Running headless makes the difference between ordinary and extra-ordinary. You may want to consider purchasing a dedicated host for PiPedal&mdash;perhaps, a tiny N95, N100 or N150 micro-pc, or, of course, a Raspberry Pi 5. It's perfectly fine to run PiPedal on a desktop, or laptop. But just make sure you are not using the GPU when you run PiPedal.

Headless, in this context means that nothing is using the GPU. On a Raspberry Pi, it is sufficient to just disconnect the HDMI cables, and not be using a remote desktop connection. It is uncertain whether Ubuntu will stop using the GPU if there is an active desktop that is (for example) updating status indicators on the status bar. If in doubt, try disabling automatic login. The system will then stop at the login screen, which does not do any drawing until you start typing credentials. Configuring your PiPedal server to use a text-mode interface instead of a graphical interface would, of course, be perfect; but it seems unnecessarily inconvenient. Personally, I prefer to have a graphical desktop available if I need it to tinker with the system, or run software updates. On an Ubuntu server install, there is no graphical desktop, so this is not a problem. But see the section in [PiPedal on Ubuntu](Ubuntu.md) about configuring Ubuntu Server network services before you choose an Ubuntu Server install.


--------
[<< PiPedal on Ubuntu](Ubuntu.md) | [Up](Documentation.md) | [Configuring PiPedal after Installation >>](Configuring.md)

