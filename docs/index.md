<div style="position: relative">
  <img src="img/pipedal.png" style="width: 100%; height: 160px;object-fit: cover"/>
</div>

<h2 style="font-weight: 300; font-size: 1.6em;line-height: 1em; padding-top: 32px; padding-bottom: 0px;margin-bottom: 4px">PiPedal Guitar Effects Processor</h2>
<p style="padding-top: 0px; opacity: 0.6; padding-bottom: 16px">A Raspberry Pi-based stomp box designed to be controlled from a phone or tablet.</p>

_To download PiPedal, click [*here*](download.md). 
To view PiPedal documentation, click [*here*](Documentation.md)._

_ NEW version 1.4.76 Release. See the [release notes](https://rerdavies.github.io/pipedal/ReleaseNotes) for details. New Phaser and Graphic Equalizer plugins._

Use your Raspberry Pi as a guitar effects pedal. Configure and control PiPedal with your phone or tablet.
PiPedal running on a Raspberry Pi 4 or Pi 5 provides stable super-low-latency audio via external USB audio devices, or internal Raspberry Pi audio hats.

PiPedal will also run on Ubuntu 22.x (amd64/x64 and aarch64). Make sure you follow the [Ubuntu post-install 
instructions](https://rerdavies.github.io/pipedal/Configuring.html) to make sure your Ubuntu OS is using a  realtime-capable kernel.


{% include demo.html %}

{% include gallery.html %}

PiPedal can be remotely controlled via a web interface over Ethernet, or Wi-Fi. If you don't have access to a Wi-Fi router, PiPedal can be configured to 
start a Wi-Fi hotspot automatically, whenever your Raspberry Pi can't connect to your home network.

Install the [PiPedal Remote Android app](https://play.google.com/store/apps/details?id=com.twoplay.pipedal) to get one-click access to PiPedal via Wi-Fi networks, or Wi-Fi hotspots. If you are using PiPedal away from home, you can configure PiPedal to automatically start a Wi-Fi hotspot on your Raspberry Pi 
whenever Pipedal is unable to detect your home network. The PiPedal Client Android app will allow to connect by simply launching the app, whether you are at home, or using a Wi-Fi auto-hotspot at a gig, when away from home.

PiPedal's user interface has been specifically designed to work well on small form-factor touch devices like phones or tablets. Clip a phone or tablet on your microphone stand on stage, and you're ready to play! Or connect via a desktop browser, for a slightly more luxurious experience. The PiPedal user-interface adapts to the screen size and orientation of your device, providing easy control of your guitar effects across a broad variety devices and screen sizes.

PiPedal includes a pre-installed selection of LV2 plugins from the ToobAmp collection of plugins; but it works with most LV2 Audio plugins. There are literally hundreds of free high-quality LV2 audio plugins that will work with PiPedal. Just install them on your Raspberry Pi, and they will show up in PiPedal.  

If your USB audio adapter has MIDI connectors, you can use MIDI devices (keyboards, controllers, or midi floor boards) to control PiPedal while performing. A simple interface allows you to select how you would like to bind PiPedal controls to midi messages. 





