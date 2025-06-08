---
page_icon: img/Setup.jpg
icon_width: 320px
icon_float: left
---

## Configuring PiPedal After Installation

Before using PiPedal, you will need to configure settings for the audio device that PiPedal will use.

{% include pageIconL.html %}

You will also want to configure PiPedal  to provide a Wi-Fi auto-hotspot so that you can connect to using your using your Android phone or tablet. It's fine to use your home Wi-Fi network to connect to PiPedal when you're at home; but don't forget that when you take PiPedal out to a gig, you will need to ensure that PiPedal's Wi-Fi auto-hotspot is enabled before you do. 

PiPedal uses LV2 audio plugins. There are literally thousands of freely available high-quality LV2 plugins that are suitable for use as guitar effects.

By default, PiPedal comes with a few plugins from the ToobAmp plugin collection. You will probably want to install more.

See [_Using LV2 Plugins_](UsingLv2Plugins.md) for more details, and for some concrete recommendations of LV2 Plugin collections that you might want to use
with PiPedal.

### Installing the PiPedal Remote Android app

If you are using an Android phone or table to connect to PiPedal, you should download and install the [PiPedal Android Client](https://play.google.com/store/apps/details?id=com.twoplay.pipedal)

The Android PiPedal client provides three  feature: 

1. It automatically locates running instances of PiPedal on the local Wi-Fi network using dns/Bonjour discovery. So you don't need to keep track of which IP address to connect with.

2. It provides a web view of PiPedal user interface with none of the clutter associated with a web browser. No browser address bars; no additional window clutter from the browser. Just a view of the PiPedal UI. So you get to use every precious pixel of your display to control PiPedal.

3. It happens to interoperate nicely with PiPedal's auto-hotspot feature, allowing one-click connection to PiPedal even when you're away from home and don't have access to a Wi-Fi router. 

In theory, you could do all of this using Chrome, running on your phone. And, in fact, you can. But it's a fiddly procedure. The Android PiPedal Client takes care of all of those details, and allows you to connect to PiPedal from your phone or tablet with just one click to launch the app.

The PiPedal client will connect to the PiPedal server running on your Raspberry Pi automatically if they are both connected to the same Wi-Fi router. When you are away from home, you should configure PiPedal to automatically bring up a Wi-Fi hotspot when the Raspberry Pi is not able to connect to your home network. 

When you do, you can easily connect to PiPedal via the Android app.

- You show up at a local venue to perform.

- You power up your Raspberry pi. The PiPedal server boots up, and decides that it is not able to connect to your home network, so it starts up a Wi-Fi hotspot.

- Your phone or tablet doesn't have a Wi-Fi connection, because you're not at home. But when it sees the PiPedal hotspot it will automatically connect.

- You launch the PiPedal client on your Android device. It searches the network associated with the current active Wi-Fi connection (the PiPedal hotspot, in this instance), and finds a PiPedal server announcing itself via mDNS/Bonjour. And having located the pipedal server, it automatically opens up a Web View and connects to the PiPedal web user interface.

None of this is particularly clever. But it does make things simple.

## First Connection

You can complete the initial configuration procedure using any of the following methods:

1. _Log on to the Raspberry Pi device_. Launch a web browser, and connect to http:://127.0.0.1/

2.  _Connect your Raspberry Pi to a Wi-Fi or Ethernet network, and connect from your laptop or desktop._ Connect to http://raspberrypi/ (substitute the host name of your Raspberry Pi if you have changed it from the default).

3. _From your Android device._ Install PiPedal Remote on your Android device. Connect your Raspberry Pi to your home router using a Wi-Fi or Ethernet connection. Launch PiPedal Remote on your Android device.

4. _From a LINUX command prompt._ As a last resort, you can configure the Wi-Fi hotspotusing the `pipedalconfig` program. Type `pipedalconfig --help` at a command prompt on your Raspberry Pi.

If you already have another web server on port 80, see [*How to Change the Web Server Port*](ChangingTheWebServerPort.md).

When you connect to PiPedal for the first time, you will be presented with an Onboarding dialog, which allows you to complete the setup of things that need to be configured before you use PiPedal. From the Onboarding dialog, you can select and configure the audio device, and (optionally) configure Pipedal's Wi-Fi auto hotspot.

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

Selecting 2 buffers provides lower latency, but leaves very little CPU time for audio processing, so you may experience more underruns if 
you select 2. Selecting 4 buffers provides much more time for proessing.  Using a buffer size of 16 increases CPU load slightly, since there a 
certain amoung of CPU overhead associated with handling of each buffer. That being said, 16x4 audio buffer configuration is a highly recommended
choice if your audio adapter can support it. It provides excellent latency with very little chance of overruns.

All things being equal, you should perfer 48000hz sample rates to 44100Hz sample rates. The higher sample rate makes it easier for plugins to implement 
audio effects without high-frequency artifacts; and Nueral Amp Models are usually designed to work best at 48000Hz sample rates.

### Configuring Input Trim Levels on Older USB Audio Devices

For best results, you should set the input gain of your audio device so that the signal level is as high as possible without clipping. If you click on the 
input node of a preset, the left VU meter will show the audio input levels as received directly from your audio adapter. 

Some older USB audio devices do not provide volume knobs to control input gain of the audio signal, and the default trim settings are often less than 
ideal. To configure the input gain on these devices, use the following procedure. 

- ssh to the host, or launch a terminal window on the host if you have configured your Raspberry Pi to boot to a graphical desktop.
- Run `alsamixer` to configure the input gain.
- Press F6 to select the sound card. 
- Press TAB to scroll across pages/channels until you reach the CAPTURE slider(s). 
- Connect and play your instrument, while watch the input VU meter in the PiPedal UI. Use the UP and DOWN arrow keys to adjust the input gain.
- Press the ESC key to close alsa mixer. 
- Run `sudo alsactl store` to save the settings permanantly.



## Activating the Wi-Fi Auto-Hotspot

The PiPedal <b><i>Auto-Hotspot</i></b> feature allows you to connect to your Raspberry Pi even if you don't have
access to a Wi-Fi router. For example, if you are performing at a live venue, you probably will not
have access to a Wi-Fi router; but you can configure PiPedal so that your Raspberry Pi  automatically
starts a Wi-Fi hotspot when you are not at home. The feature is primarily intended for use with the
PiPedal Android client, but you may find it useful for other purposes as well.

Raspberry Pi devices are unable to run hotspots, and have another active Wi-Fi connection at the same time; so the auto-hotspot feature
automatically turns the hotspot on, when your Raspberry Pi cannot otherwise be connected to, and can be configured to
automatically turn the PiPedal hotspot off when you do want your Raspberry Pi to connect to another Wi-Fi access point.
How you configure PiPedal's auto-hostpot depends on how you normally connect to your Raspberry Pi when you are at home.

To configure Pipedal's Auto-Hotspot, open the Auto-Hotspot configuration dialog. You can access this dialog directly from the Onboarding page; or if 
you have completed the onboarding procedure, you can open the dialog from the <b><i>Auto Hotspot</i></b> item in the <b><i>PiPedal Settings</i></b> dialog.

The Auto-Hotspot offers you several choices as to how and PiPedal should open up a Wi-Fi hotspot. Which option you choose depends on how you connect to 
PiPedal when you are at home.

If you normally connect to your Raspberry Pi using an ethernet connection, starting the Wi-Fi hotspot when there is <b><i>No ethernet connection</i></b> is a
good choice. The PiPedal Wi-Fi hotspot will be available whenever the ethernet cable is unplugged. <b><i>Always on</i></b> is
also a good choice, but may confuse your phone or tablet, since your Android device will now have to decide whether to auto-connect to your home Wi-Fi
router, or to the Raspberry Pi hotspot. If you use the <b><i>No ethernet connection</i></b> option, your phone or tablet will
never see the PiPedal hotspot and your Wi-Fi router at the same time.

If you normally connect to your Raspberry Pi through a Wi-Fi router, <b><i>Not at home</i></b> is a good choice. The
PiPedal hotspot will be automatically turned off whenever your home Wi-Fi router is in range, and automatically turned on
when you are out of range of your home Wi-Fi router.

If there are multiple locations, and multiple Wi-Fi routers you use with PiPedal on a regular basis, you can choose to start the 
PiPedal Wi-Fi hotspot when <b><i>No remembered Wi-Fi connections</i></b> are visible, but this is a riskier option. The PiPedal hotspot will be automatically turned on if there are no
Wi-Fi access points in range that you have previously connected to from your Raspberry Pi, and will be automatically turned on otherwise.
The risk is that you could find yourself unable to connect to your Raspberry Pi when performing
at a local bar, after you have used your Rasberry Pi to connect to the Wi-Fi access point at the coffee shop nextdoor. (Public Wi-Fi access
points usually won't work because devices that are connected to a public access point can't connect to each other).
Will you ever do that? Probably not. But there is some risk that you might find yourself unable to connect at a live venue. Whether that's an
acceptable risk is up to you.

Typically, when you're away from home, there's no easy way to connect to your Raspberry Pi from a laptop in order to
correct the problem. So you should carefully test that your auto-hotspot configuration works as expected before you adventure
away from home with PiPedal.

### Connecting to PiPedal Using a Laptop when you are Away From Home

If your laptop has Wi-Fi support, you can use a laptop to connect to Raspberry Pi when you are away from home. 

When you are at home, connect both the Raspberry Pi and your Laptop to the same Wi-Fi router. Out of the box, Raspberry Pi OS is 
configured to announce itself via mDNS/Bonjour, and Window and Mac operating systems can both use mDNS/Bonjour name resolution.

Simply launch

    http://raspberrypi

(or supply the actual hostname of your Raspberry Pi if you have changed the default hostname).

When you are away from home, you can use Wi-Fi hotspots to connect your laptop and your Raspberry Pi device. You can either configure 
your laptop to provide a Wi-Fi hotspot, and then get your Raspberry Pi to connect to the hotspot on your laptop. Raspberry Pi OS will 
automatically connect to your laptop hotspot whenever it sees it, after you have connected for the first time, and entered login 
credentials on your Raspberry Pi. After the first time, all you need to do is turn on your laptop hotspot, and Raspberry Pi OS will 
connect to it. 

Usually, you can conect from your laptop using the same web address:  `http://raspberrypi` (`http://raspberrypi.local` on Ubuntu), or the hostname of your raspberry pi, if you have changed it. Unlike Android phones (where mDNS name resolution doesn't work on Android-hosted hotspots), mDNS/Bonjour name resolution usually 
works on laptop-hosted hotspots when using Windows or Mac OS. If are running Linux on your laptop, you may need to install and configure the
Avahi package to get mDNS/Bonjour name resolution to work.

And if that doesn't work, you can configure PiPedal to launch an auto-hotspot, and then connect from your laptop to the PiPedal hotspot on your Raspberry Pi.
When the Raspberry Pi hosts the hotspot, mDNS discovery is definitely enabled; so you should be able to connect using http://raspberrypi. But if that doesn't work, PiPedal's IP address will always be 10.40.0.1, when the PiPedal Wi-Fi hotspot is running, so you can always connect using  http://10.40.0.1.


--------
[<< PiPedal on Ubuntu](Ubuntu.md)  | [Up](Documentation.md) | | [An Intro to Snapshots >>](Snapshots.md)
