## Optimizing Audio Latency

Note that PiPedal is not intended for use when logged in to Raspbian. Screen updates and heavy filesystem activity will cause audio dropouts. For best results, access PiPedal using the web interface remotely, through the Wi-Fi hotspot. Accessing the web interface via Wi-Fi has little or no effect on audio latency or dropouts.

With a good USB audio device, PiPedal should be able to provide stable audio with 4ms (good), or 2ms (excellent) latency on a Raspberry Pi 4 when running on a stock PREEMPT kernel. Your actual results may vary.

The current Linux kernel provides best latency on USB audio devices when they are configured with a sampling rate of 48kHz, and 3 buffers. Cheap USB audio devices (e.g. M-Audio  M-Track Solo, available for less than $60) should be able to run without dropouts at 48kHz with 3x64 sample buffers. Most devices in this class use the same Burr-Brown chipset. Premium USB Audio devices should run stably at 48kHZ 3x32 sample buffers (about 2ms latency). I personally use the MOTU M2 USB audio adapter, which I highly recommend -- stable, quiet, low-latency, great controls, and built like a tank).

Make sure your system is fully updated, and that you are running with a kernel version of 5.10 or later, since version 5.10 of the Linux kernel provides improved support for class-compliant USB audio. The MOTU M2 (and many other USB audio adapters) will not work on versions of the kernel prior to 5.10.

Prefer 64-bit operating systems to 32-bit operating systems. ARM processors execute 64-bit code about 40% faster than 32-bit code providing the same functionality.

RT_PREEMPT realtime kernels (when available) are preferred but not required. (As if February 2022, there aren't any good sources for latest versions of Ubuntu or Raspberry Pi OS). PiPedal provides better (but not dramatically better) latency when running on a Raspbian Realtime kernel. Stock Raspbian provides PREEMPT real-time scheduling, but does not currently have all of the realtime patches, so interrupt latency is slightly more variable on stock Raspberry Pi OS than on custom builds of Raspbian with RT_PREEMPT patches applied.

The Ubuntu Studio installer will install a realtime kernel if there is one available. But -- at least for Ubuntu 21.04 -- there is no stock RT_PREEMPT kernel for ARM aarch64.

On a Raspberry Pi 4 device, Wi-Fi, USB 2.0, USB 3.0 and SD Card access are performed over separate buses (which is not true for previous versions of Raspberry Pi). It's therefore a good idea to ensure that your USB audio device is either the only device connected to the USB 2.0 ports, or the only device connected to the UBS 3.0 ports. There's no significant advantage to using USB 3.0 over USB 2.0 for USB audio. Network traffic does not seem to adversely affect USB audio operations on Raspberry Pi 4 (which isn't true on previous versions of Raspberry Pi). Filesystem activity does affect USB audio operation on Raspberry Pi OS, even with an RT_PREEMPT kernel; but interestingly, filesystem activity has much less effect on UBS audio on Ubuntu 21.04, even on a plain PREEMPT kernel. 

There is some reason to believe that there are outstanding issues with the Broadcom 2711 PCI Express bus drivers on Raspberry Pi OS realtime kernels, but as of September 2021, this is still a research issue. If you are brave, there is strong anecdotal evidence that these issues arise when the Pi 4 PCI-express bus goes into and out of power-saving mode, which can be prevented by building a realtime kernel with all power-saving options disabled. But this is currently unconfirmed speculation. And building realtime kernels is well outside the scope of this document. (source: a youtube video on horrendously difficult bugs encountered while supporting RT_PREEMPT, by one of the RT_PREEMPT team members).

For the meantime, for best results, log off from your Raspberry Pi, and use the web interface only.

You may also want to watch out for temperature throttling of the CPUs. PiPedal displays the current CPU temperature in the bottom-right corner of the display. The system will reduce CPU speed in order prevent damage to the system if the CPU temperature goes above 70C (perhaps above 60C). The Pi 400 already has good heat sinks, so you shouldn't run into problems when running on a Pi 400. If you run into throttling problems on a Raspberry Pi 4, you may want to buy and install a heat sink (ridiculously cheap), or install a cooling fan. As a temporary work-around, you can orient the Raspberry Pi 4 board vertically, which can provide a real and meaningful reduction in CPU temperature.

--------
[<< System Requirements](ChoosingAUsbAudioAdapter.md) | [Command-Line Configuration of PiPedal >>](CommandLine.md)