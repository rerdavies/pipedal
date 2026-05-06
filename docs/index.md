<div style="position: relative">
  <img src="img/pipedal.png" style="width: 100%; height: 160px;object-fit: cover"/>
</div>

<h2 style="font-weight: 300; font-size: 1.6em;line-height: 1em; padding-top: 32px; padding-bottom: 0px;margin-bottom: 4px">PiPedal Guitar Effects Processor</h2>
<p style="padding-top: 0px; opacity: 0.6; padding-bottom: 16px">A Raspberry Pi-based stomp box designed to be controlled from a phone or tablet.</p>

<a href="https://rerdavies.github.io/pipedal/ReleaseNotes"><img src="https://img.shields.io/github/v/release/rerdavies/pipedal?color=%23808080"/></a>
<a href="https://rerdavies.github.io/pipedal/download"><img src="https://img.shields.io/badge/Download-008060" /></a>
<a href="https://rerdavies.github.io/pipedal/Documentation"><img src="https://img.shields.io/badge/Docmentation-0060d0"/></a>

_To download PiPedal, click [*here*](download.md). 
To view PiPedal documentation, click [*here*](Documentation.md)._

Use your Raspberry Pi as a guitar effects pedal. Configure and control PiPedal with your phone or tablet.
PiPedal running on a Raspberry Pi 4 or Pi 5 provides stable super-low-latency audio via external USB audio devices, or internal Raspberry Pi audio hats.

PiPedal will also run on Ubuntu 22.x (amd64/x64 and aarch64). Make sure you follow the [Ubuntu post-install 
instructions](https://rerdavies.github.io/pipedal/Configuring.html) to make sure your Ubuntu OS is using a  realtime-capable kernel.

{% include gallery.html %}

PiPedal includes state-of-the-art AI-based guitar amp emulation plugins based on the famous open source Neural Amp Modeler (NAM) and ML libraries. A Raspberry Pi 4 provides almost four times more CPU power than commercial guitar stomp boxes costing a thousand dollars or more. Pi 5s and Intel N50 NUCs provide even more power! PiPedal uses that CPU power to run breakthrough Neural Amp emulation that not only sounds like the amps they are modelling, but also responds to your playing in a way that is indistinguishable from the real thing. PiPedal's NAM-based amp emulation plugins are the best-sounding guitar amp simulators available, and they run on a Raspberry Pi 4 with latency as low as 3ms. Thousands of amp models are available for free online, and you can even create your own amp models with the Neural Amp Modeler software, which is also free and open source. The [Tone3000](https://tone3000.com) website provides thousands of free NAM amp models, and also allows you to create models of your own amps by uploading recordings of your amps to their website.

{% include demo.html %}

PiPedal can be remotely controlled via a web interface over Ethernet, or Wi-Fi. If you don't have access to a Wi-Fi router, PiPedal can be configured to 
start a Wi-Fi hotspot automatically, whenever your Raspberry Pi can't connect to your home network.

Install the [PiPedal Remote Android app](https://play.google.com/store/apps/details?id=com.twoplay.pipedal) to get one-click access to PiPedal via Wi-Fi networks, or Wi-Fi hotspots. If you are using PiPedal away from home, you can configure PiPedal to automatically start a Wi-Fi hotspot on your Raspberry Pi 
whenever Pipedal is unable to detect your home network. The PiPedal Client Android app will allow to connect by simply launching the app, whether you are at home, or using a Wi-Fi auto-hotspot at a gig, when away from home.

PiPedal's user interface has been specifically designed to work well on small form-factor touch devices like phones or tablets. Clip a phone or tablet on your microphone stand on stage, and you're ready to play! Or connect via a desktop browser, for a slightly more luxurious experience. The PiPedal user-interface adapts to the screen size and orientation of your device, providing easy control of your guitar effects across a broad variety devices and screen sizes.

PiPedal includes a pre-installed selection of LV2 plugins from the ToobAmp collection of plugins; but it works with most LV2 Audio plugins. There are literally hundreds of free high-quality LV2 audio plugins that will work with PiPedal. Just install them on your Raspberry Pi, and they will show up in PiPedal.  

If your USB audio adapter has MIDI connectors, you can use MIDI devices (keyboards, controllers, or midi floor boards) to control PiPedal while performing. A simple interface allows you to select how you would like to bind PiPedal controls to midi messages. 





