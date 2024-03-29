---
page_icon: img/Setup.jpg
icon_width: 320px
icon_float: left
---
## Configuring PiPedal After Installation


Before using PiPedal, you will need to configure settings for the audio device that PiPedal will use.

{% include pageIconL.html %}

You will also want to configure PiPedal  to provide a Wi-Fi Direct access point (hotspot) that you can connect to using your phone. It's fine to use your home Wi-Fi network to connect to PiPedal when you're at home; but don't forget that when you take PiPedal out to a gig, you will need to ensure that the Wi-Fi Direct access point is enabled before you do. 

PiPedal uses LV2 audio plugins. There are literally thousands of freely available high-quality LV2 plugins that are suitable for use as guitar effects.

By default, PiPedal comes with a few plugins from the ToobAmp plugin collection. You will probably want to install more.

See [_Using LV2 Plugins_](UsingLv2Plugins.md) for more details, and for some concrete recommendations of LV2 Plugin collections that you might want to use
with PiPedal.

### Installing the PiPedal Remote Android app

Install [PiPedal Remote](https://play.google.com/store/apps/details?id=com.twoplay.pipedal) on your Android device. PiPepedal Remote can connect to
PiPedal on your Raspberry Pi device  as long as your Android device and Raspberry Pi are connected to the same network via a normal Wi-Fi access point, or via an Ethernet connection. 

## First Connection

You can complete the initial configuration procedure using any of the following methods:

1. _Log on to the Raspberry Pi device_. Launch a web browser, and connect to http:://127.0.0.1/

2.  _Connect Raspberry Pi to a Wi-Fi or Ethernet network, and connect from your laptop or desktop._ Connect to http://raspberrypi.local/ (substitute the host name of your Raspberry Pi if you have changed it from the default).

3. _From your Android device._ Install PiPedal Remote on your Android device. Connect your Raspberry Pi to your home router using a Wi-Fi or Ethernet connection. Launch PiPedal Remote on your Android device.

4. _From a LINUX command prompt._ As a last resort, you can configure the Wi-Fi Direct access point using the `pipedalconfig` program. Type `pipedalconfig --help` at a command prompt on your Raspberry Pi.

If you already have another web server on port 80, see [*How to Change the Web Server Port*](ChangingTheWebServerPort.md).

### Configuring Audio

Once connected, select the Settings menu item on the Hamburger menu at the top left corner of the display. Click on Audio Device Settings to select and configure the audio device you want to use. 

You may also need to choose which audio input and output channels you will use for guitar signals, once you have selected and configured 
an audio device. Many external USB audio devices that have two inputs provide the guitar signal on the right channel only, so you will set the audio input chanels to  "Right Only". If your USB audio adapter has more than two input or output channels, you will be offered a list of channels to choose from.

#### Selecting Audio Buffer Sizes

You may have to experiment to find the buffer size, and buffer count that works best for your. The actual round-trip latency and frequency of audio overruns and underruns depends the the operating system system version, the audio hardware being used, and upon CPU use of the audio patches you are using. Selecting larger buffer sizes or larger buffer counts increase the amount of computing power available for your audio effects, and will reduce the number of dropouts caused by audio overruns/underruns (xruns); smaller buffer sizes and buffer counts improve the overall round-trip latency but will increase the likelihood of xruns.

Please note that the Raspberry Pi OS is not completely robust with respect to realtime scheduling. Significant display activity, SD-CARD activity or heavy CPU use by programs other than PiPedal may cause audio xruns. For best results, run PiPedal without a display attached, and with no other programs running. Connecting remotely to the PiPedal web interface should not cause problems. 

PiPedal provides the pipedal_latency_test utility to measure actual round-trip audio latency. You must temporarily disable pipedal (`sudo systemctl stop pipedal`), and connect the left audio output of your audio device to the left audio input of your audio device with a guitar cable to use this test. 

The following table shows measured round-trip audio latencies for a MOTU M2 external USB adapter running on Raspbery Pi OS. You can use these figures as a rough guideline; but actual round-trip audio latency will depend on the audio device you are using.

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

LINUX Kernel version 5.16 includes fixes to ALSA audio that are supposed to dramatically improve USB audio latency. As of July 2022, these fixes have not yet made it into Raspberry Pi OS, but are probably available on non-LTS versions of Ubuntu.

PiPedal uses the ALSA audio stack; so (unlike Jack Audio) there is no performance penalty for using 44100Hz sample rates. However, using a 48000Hz sampling rates does provide significant improvements in high-frequency audio quality when performing digital audio signal processing.

### Activating the P2P (Wi-Fi Direct) Hotspot

The preferred way to connect to a PiPedal host device is via Wi-Fi Direct connections. Doing so allows you to use your PiPedal device when you are away from home.
     
Enable PiPedal's Wi-Fi Direct hotspot connection from the Settings dialog (Click on the hamburger icon; click on *Settings*; click on *Wi-Fi Direct Hotspot*). You will need to select a PIN and enable the connection before you can use it. The PiPedal hotspot will be visible to anyone with Wi-Fi distance; so choose your PIN carefully. Do NOT use trivial PINs like 12345678, or 00000000! Note that when connecting via Wi-Fi direct, you only have to enter the PIN once on the device you are connecting from; and it will be remembered automatically for subsequent connections.

Best practice would be to generate a random PIN (using the Generate Random Pin button in the setup dialog), and then tape a label to the bottom of your Raspberry PI 
(not the top) with the PIN written on it, in case you ever need to connect to PiPedal with a device that hasn't been previously set up. 
     
Wi-Fi Direct connections differ in a couple of ways from normal Wi-Fi connections
     
- You only have to enter the pin once; the device you connect from will remember the PIN and connect without a PIN after that.

- You can have a simultaneous connection to your Wi-Fi router when using a Wi-Fi Direct connection to your Raspberry Pi. The device you
  are connecting from will continue use the Internet over your Wi-Fi router, and only use the Wi-Fi Direct connection to communication 
  with your Raspberry Pi.

- You can use a Wi-Fi Direct connection even when you don't have a Wi-Fi router connection. Android phones will continue to communicate 
  with the Internet over their
  data connections, even when a Wi-Fi Direct connection is active.

- More than one device can connect to the Wi-Fi Direct connection on the Raspberry Pi when it is active/

- Your Raspberry Pi can also have simultaneous access to a Wi-Fi router access point when the Wi-Fi Direct connection is enabled.

- Wi-Fi Direct connections are backward compatible with Wi-Fi access points if you are using an older device. Look for the DIRECT-xx-YourDeviceName access point.
  But if you use a legacy connection, the connecting device cannot have a simultanous Wi-Fi router connection.

If you use the PiPedal Android app, the Android app will manage discovery and setting up of Wi-Fi direct connections to your Raspberry Pi device automatically; but you must complete the initial configuration using a direct web connection first.
     
PC support for Wi-Fi Direct connections varies dramatically. Modern PCs support Wi-Fi Direct; but you may find it easier to communicate with PiPedal via your Wi-Fi router. Configure PiPedal to use a Wi-Fi connection as well as a Wi-Fi Direct access point (or connect an ethernet cable to your Raspberry Pi). Just remember to configure your phone to use a Wi-Fi Diret connection _before_ you take PiPedal out on a gig, because you won't have a Wi-Fi router then.

PiPedal advertises the address of the Raspberry Pi host  machine via Multi-cast DNS (M-DNS). You should be able to establish web connections to the Raspberry Pi using the address `http://<hostname-of-your-pi>.local/`, no matter how you are connected.
     
Older devices may not support Wi-Fi Direct connections; but they will be able to use the Wi-Fi Direct connection as an ordinary Wi-Fi Hotspot. But they won't have access to the Internet over your Wi-Fi router while they have a connection with your Raspberry Pi.

You _must_ select the correct country when setting up your Wi-Fi Direct connection. Regulations for use of Wi-Fi vary greatly from coutry to country, and define both the channels you are allowed to use, and the features and signal strength of Wi-Fi connections on those channels. 

For best results, you should select Wi-Fi channel 1, 6 or 11 (referred to as the "Wi-Fi Direct Social Channels"). Doing so reduces the time it takes for other devices to discover the Raspberry Pi. While it is possible to use 5Ghz channels for Wi-Fi Direct, it may take some time for connecting devices to find your PiPedal device.

Support for Apple/IOS devices: a client for Apple/IoS devices is in long-term development plans; but I don't own any Apple device on which to do development and testing. If you'd like to see an Apple/IOS client, your sponsorship would help. (The client performs automatic discovery and set up of Wi-Fi direct connections, and relies on the Web interface after that. Not difficult to implement. Just expensive.)

--------
[<< Installing PiPedal](Installing.md)  | [Up](Documentation.md) | | [Choosing a USB Audio Adapter >>](ChoosingAUsbAudioAdapter.md)
