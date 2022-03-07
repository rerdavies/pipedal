## Configuring PiPedal After Installation

After PiPedal is installed, you can connect to the web interface as follows: via the mDNS address "http://pipedal.local" (on Windows, Mac, or iPhone web 
browsers), at http://127.0.0.1 if you are interactively logged into your Raspberry Pi device, or at port 80 of the current network address of your 
Raspberry Pi, if you are connected from an Android device (which does not currently support mDNS). 

Android devices do not support mDNS. If you are connecting to PiPedal with an Android device, via the Host Access Point, PiPedal, the PiPedal user interface can be reached at http://172.22.1.1 If you are connecting via the Raspberry Pi's Ethernet port, connect to http://*address of your Pi*:80

To complete the initial configuration, you must either connect an Ethernet cable to your Raspberry pi so you can connect to the Web App (after which you should be able to connect to http://pipedal.local); or you must launch a web browser on your Raspberry pi device while logged in interactively. 
     
If you already have another web server on port 80, see the section "Changing the Web Server Port", below.

Once connected, select the Settings menu item on the Hamburger menu at the top left corner of the display. Click on Audio Device Settings to select and configure the audio device you want to use. 

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

There are a number of other useful settings in the hamburger menu/Settings dialog. For example, most USB audio devices route instrument
input onto the right channel of the USB audio inputs. So you probably want to configure PiPedal to use only the right USB audio input channel. 
You can choose how to bind USB audio input and output channels (stereo, left only, right only) in the settings dialog. If you are using a Audio 
device that has more than two channels, you will be offered a list of channels to choose from instead.
