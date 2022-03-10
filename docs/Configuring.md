## Configuring PiPedal After Installation

Before using PiPedal, you will need to configure settings for the audio device that PiPedal will use.

Optionally, you may want to configure PiPedal 
to provide a Wi-Fi hotspot that you can connect to using your phone. It's fine to use your home Wi-Fi network to connect to PiPedal when your at home;
but don't forget that when you take PiPedal out to a gig, you will need to configure a Wi-Fi hotspot before you do so.

### First Connection

After PiPedal is installed, you can connect to the web interface as follows: via the following url:

-    "http://pipedal.local" (on Windows, Mac, or iPhone web browsers)

-    http://127.0.0.1 if you are interactively logged into your Raspberry Pi device

-    at port 80 of the current network address of your Raspberry Pi, if you are connecteing from an Android device (which does not currently support mDNS). 

To complete the initial configuration, you must either connect an Ethernet cable to your Raspberry pi so you can connect to the Web App (after which you should be able to connect to http://pipedal.local); or you must launch a web browser on your Raspberry pi device while logged in interactively. 
     
If you already have another web server on port 80, see [*How to Change the Web Server Port*](ChangingTheWebServerPort.md).

### Configuring Audio

Once connected, select the Settings menu item on the Hamburger menu at the top left corner of the display. Click on Audio Device Settings to select and configure the audio device you want to use. 

-    IMPORTANT NOTE: If you are using a USB audio device, you *MUST* set the number of buffers to 3, and you *MUST* set the sample rate to 48,000
     in order to acheive reasonable latency. For other devices, you probably want to use 2 buffers.

You may also need to choose which audio input and output channels you will use for guitar signals, once you have selected and configured 
an audio device. Must external USB audio devices provide the guitar signal on the right channel only, so you will set the audio input chanels to  "Right Only". If 
your USB audio adapter has more than two input or output channels, you will be offered a list of channels to choose from..

### Installing LV2 Plugins

By default, PiPedal comes with a few plugins from the ToobAmp plugin collection. You will probably want to install more.

PiPedal uses LV2 audio plugins. There are literally thousands of freely available high-quality LV2 plugins that are suitable for use as guitar effects.

Here is a brief list of particularly recommend plugin collections.

| Collection                      | To Install                            | Description      |
|---------------------------------|:--------------------------------------|------------------|
|[Guitarix](https://guitarix.org) | sudo apt install guitarix-lv2         | A large collection guitar amplifiers and effects. |
| [GxPlugins](https://github.com/brummer10/GxPlugins.lv2)    | [(manual build)](https://github.com/brummer10/GxPlugins.lv2) | Additional effects from the Guitarix collection |
| Invadio Studio Plugins          | sudo apt install invada-studio-plugins-lv2 | Guitar effects        |
|[Zam Plugins](https://zamaudio.com) | sudo apt install zam-plugins   | Filtering, EQ, and mastering effects. |
| [Calf Studio Gear](https:://calf-studio-gear.org) | sudo apt install calf-plugins | Large collection of effects plugins |

The GxPlugins pack requires a manual build; but it's worth the effort. Just follow the instructions.

### Activating the Wi-Fi Hotspot

You can also activate PiPedal's Wi-Fi hotspot connection from the Settings dialog. Click on the Wifi Hotspot menu item in the Settings dialog.

-    IMPORTANT NOTE:

     Activating the WiFi hotspot will DISABLE outbound Wi-Fi connections from the Raspberry Pi.
     You will be able to access the PiPedal web interface through the hotspot connection, and make ssh and VNC 
     connections to the Raspberry Pi through the hotspot connection; but your Raspberry Pi will not have 
     outbound access to the Internet, unless an Ethernet cable is connected to the Raspberry Pi.

If you need access to the internet once the hotspot has been enabled, connect an Ethernet cable to
the Raspberry Pi. Note that the PiPedal hotspot is NOT configured to forward internet traffic from the Wi-Fi hotspot to the LAN connection, and 
generally, the Raspberry Pi will not be able to access the internet via devices connected to the Wi-Fi hotspot. Consult documentation for hostapd 
if you want to do this.

If you are connecting to PiPedal with an Android device, via the Host Access Point, PiPedal, the PiPedal user interface can be reached at http://172.22.1.1 (you'll have to memorize the address or add it to a browser Favorites entry). On other devices, the http://pipedal.local url should work over the Hotspot address as well.

(We're working on providing Wi-Fi P2P services in an imminent release, so this will improve soon). 



--------
[<< Installing PiPedal](Installing.md) | [Choosing a USB Audio Adapter >>](ChoosingAUsbAudioAdapter.md)
