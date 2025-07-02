---
page_icon: img/AudioLatency.jpg
---
## Optimizing Audio Latency

 {% include pageIcon.html %}

This section provides information on optimizing USB audio latency on operating systems other than Raspberry Pi OS.

Note that PiPedal is not intended for use when logged in interactively. Screen updates and heavy filesystem activity will cause audio dropouts. For best results, access PiPedal using the web interface remotely, through a Wi-Fi hotspot connection, Wi-Fi connection or Ethernet connection. Accessing the web interface has little or no effect on audio latency or dropouts.

With a good USB audio device, PiPedal should be able to provide stable audio with sub-4ms latency on a Raspberry Pi 4 when running on a stock PREEMPT kernel. Your actual results may vary.

PiPedal uses ALSA audio drivers directly. Unlike Jack audio drivers, ALSA can use 44.1k or 48k audio with no significant difference in latency, and
does not require 3 audio buffers to work efficiently.  

PiPedal provides the pipedal_latency_test utility to measure actual round-trip audio latency. You must temporarily disable pipedal (`sudo systemctl stop pipedal`), and connect the left audio output of your audio device to the left audio input of your audio device with a guitar cable to use this test. 

The following table shows measured round-trip audio latencies for a MOTU M2 external USB adapter running on Raspberry Pi OS with a 48 KHz sample rate. You can use these figures as a rough guideline; but actual round-trip audio latency will depend on the audio device you are using.

<table align='center'>
    <tr><td></td><td colspan=3>Buffers</td></tr>
    <tr><td>Size</td><td>2</td><td>3</td><td>4</td></tr>
    <tr><td>16</td><td>Fails</td><td>185/3.9ms</td><td>201/4.2ms</td></tr>
    <tr><td>24</td><td>192/4.0ms</td><td>213/4.4ms</td><td>236/4.9ms</td></tr>
    <tr><td>32</td><td>219/4.6ms</td><td>236/4.9ms</td><td>272/5.7ms</td></tr>
    <tr><td>48</td><td>253/5.3ms</td><td>299/6.2ms</td><td>348/7.2ms</td></tr>
    <tr><td>64</td><td>280/5.8ms</td><td>346/7.2ms</td><td>411/8.6ms</td></tr>
    <tr><td>128</td><td>442/9.2ms</td><td>571/11.9ms</td><td>699/14.6ms</td></tr>
</table>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Table 1: Delay in samples/delay in ms on MOTU M2 @ 48KHz.

Make sure your system is fully updated. A kernel version greater than 5.15 is recommend, as Linux 5.15 is known to contain patches that 
provide significant performance improvements for USB audio devices. Version 5.10 provides patches that provide broad support for 
USB audio adapters. Kernel versions prior to 5.10 provide limited support for a limited number of mostly obsolete USB adapters.
PiPedal will run on kernel versions less than 5.10, but may not provide robust, stable audio.

Prefer 64-bit operating systems to 32-bit operating systems. ARM processors execute 64-bit audio code about 40% faster than 32-bit code providing the same functionality.

RT_PREEMPT realtime kernels (when available) are preferred but not required. However, as of February 2022, there aren't any good sources for RT_PREEMPT kernels for Ubuntu or Raspberry Pi OS, and this may not change in the future. PiPedal provides better (but not dramatically better) latency when running on an RT_PREEMPT kernel. Stock Raspberry PI OS provides PREEMPT real-time scheduling, but does not currently have all of the realtime patches, so interrupt latency is slightly more variable on stock Raspberry Pi OS than on custom builds of Raspbian with RT_PREEMPT patches applied.

The Ubuntu Studio installer will install a realtime kernel if there is one available. But -- at least for Ubuntu 21.04 -- there is no stock RT_PREEMPT kernel for ARM aarch64.

On a Raspberry Pi 4 device, Wi-Fi, USB 2.0, USB 3.0 and SD Card access are performed over separate buses (which is not true for previous versions of Raspberry Pi). It's therefore a good idea to ensure that your USB audio device is either the only device connected to the USB 2.0 ports, or the only device connected to the UBS 3.0 ports. There are theoretical advantages to USB 3.0 over USB 2.0 for USB audio; but I don't believe there are currently any USB audio devices that take advantage of USB 3.0 features. If in doubt, use a UBS 3.0 ports; if there are problems, of if you have a high-speed drive attached to a USB 3.0 port, use a UBS 2.0 port. 

Network traffic does not seem to adversely affect USB audio operations on Raspberry Pi 4 (which isn't true on previous versions of Raspberry Pi, which use a common bus for network and USB devices). Filesystem activity does affect USB audio operation on Raspberry Pi OS, even with an RT_PREEMPT kernel; but interestingly, filesystem activity has much less effect on USB audio on Ubuntu 21.04, even on a plain PREEMPT kernel. 

There is some reason to believe that there are outstanding issues with the Broadcom 2711 PCI Express bus drivers on Raspberry Pi OS kernels, but as of September 2021, this is still a research issue. If you are brave, there is weak anecdotal evidence that these issues arise when the Pi 4 PCI-express bus goes into and out of power-saving mode, which can be prevented by building a realtime kernel with all power-saving options disabled. But this is currently unconfirmed speculation. And building realtime kernels is well outside the scope of this document. (source: a youtube video on horrendously difficult bugs encountered while supporting RT_PREEMPT, by one of the RT_PREEMPT team members hints at this problem).

Temperature Throttling
----------------------

You may want to watch out for temperature throttling of the CPUs. PiPedal can be configured to display the current CPU temperature in the bottom-right corner of the display. The system will reduce CPU speed in order prevent damage to the system if the CPU temperature goes above 70C (perhaps above 60C). The Pi 400 already has good heat sinks, so you shouldn't run into problems when running on a Pi 400. If you run into throttling problems on a Raspberry Pi 4, you may want to buy and install a heat sink (ridiculously cheap), or install a cooling fan (a couple of dollars). As a temporary work-around, you can orient the Raspberry Pi 4 board vertically, which can provide a real and meaningful reduction in CPU temperature.

My development system has both a heat sink, and a fan. The CPU temperature rarely goes above 60C.

--------
[<< System Requirements](ChoosingAUsbAudioAdapter.md)  | [Up](Documentation.md) | [Command-Line Configuration of PiPedal >>](CommandLine.md)