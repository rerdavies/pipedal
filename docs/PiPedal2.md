<div style="position: relative">
  <img src="img/pipedal.png" style="width: 100%; height: 160px;object-fit: cover"/>
</div>

<h2 style="font-weight: 300; font-size: 1.6em;line-height: 1em; padding-top: 32px; padding-bottom: 0px;margin-bottom: 4px">PiPedal 2.0 Guitar Effects Processor</h2>
<p style="padding-top: 0px; opacity: 0.6; padding-bottom: 16px">A Raspberry Pi-based stomp box designed to be controlled from a phone or tablet.</p>

<span style="display: none">
  <a href="https://rerdavies.github.io/pipedal/ReleaseNotes"><img src="https://img.shields.io/github/v/release/rerdavies/pipedal?color=%23808080"/></a>
</span>
<a href="https://rerdavies.github.io/pipedal/download2"><img src="https://img.shields.io/badge/Download-008060" /></a>
<a href="https://rerdavies.github.io/pipedal/Documentation"><img src="https://img.shields.io/badge/Documentation-0060d0"/></a>
 
_To download PiPedal v2.0.102-alpha, click [*here*](download.md). 
To view PiPedal documentation, click [*here*](Documentation.md)._

Use your Raspberry Pi as a guitar effects pedal. Configure and control PiPedal with your phone or tablet.
PiPedal running on a Raspberry Pi 4 or Pi 5 provides stable super-low-latency audio via external USB audio devices, or internal Raspberry Pi audio hats.

PiPedal will also run on Ubuntu 22.x (amd64/x64 and aarch64). Make sure you follow the [Ubuntu post-install 
instructions](https://rerdavies.github.io/pipedal/Configuring.html) to make sure your Ubuntu OS is using a  realtime-capable kernel.

PiPedal 2.0 adds support for Neural Amp Modeler A2 models&mdash;the next generation of the ground-breaking Neural Amp Modeler technology. NAM A2 models provide even more accurate and realistic amp emulation than the original NAM A1 modelling technology, with while requiring less CPU. "Slimmable" A2 models allow users to select the level of CPU use vs. emulation accuracy that best suits their needs Slim down models for lower CPU use; or use full models for the best possible emulation quality. 

NAM A2 models are available as free downloads from Tone3000.com. PiPedal 2.0 now integrates with Tone3000.com services in order to allow you to easily download NAM A2 models and Cabinet Impulse Response Files directly into Pipedal right from within the PiPedal app. All existing NAM models on Tone3000.com have been retrained using the new A2 architecture, and are available as A2 models right now. There is also a rich ecosystem of professionally developed NAM models available for purchase on the Internet. 

Whether you are looking for free models, or premium models, NAM A2 provides the best amp modelling available anywhere.

PiPedal 2.0 provides a new Channel Routing dialog, which replaces the Channel Selection Dialog in Pipedal 1.x. You can now globally (for all existing presets) route auxiliary input channels on your audio device (such as backing tracks or microphone inputs), or route clean unprocessed guitar input signals to otherwise-unused output audio channels for later re-amping later in a DAW. 


{% include gallery.html %}

PiPedal includes state-of-the-art AI-based guitar amp emulation plugins based on the famous Neural Amp Modeler (NAM) and ML libraries which provide amp modelling that will blow your mind.

{% include demo.html %}

PiPedal can be remotely controlled via a web interface over Ethernet, or Wi-Fi. If you don't have access to a Wi-Fi router, PiPedal can be configured to 
start a Wi-Fi hotspot automatically, whenever your Raspberry Pi can't connect to your home network.

Install the [PiPedal Remote Android app](https://play.google.com/store/apps/details?id=com.twoplay.pipedal) to get one-click access to PiPedal via Wi-Fi networks, or Wi-Fi hotspots. If you are using PiPedal away from home, you can configure PiPedal to automatically start a Wi-Fi hotspot on your Raspberry Pi 
whenever Pipedal is unable to detect your home network. The PiPedal Client Android app will allow to connect by simply launching the app, whether you are at home, or using a Wi-Fi auto-hotspot at a gig, when away from home.

PiPedal's user interface has been specifically designed to work well on small form-factor touch devices like phones or tablets. Clip a phone or tablet on your microphone stand on stage, and you're ready to play! Or connect via a desktop browser, for a slightly more luxurious experience. The PiPedal user-interface adapts to the screen size and orientation of your device, providing easy control of your guitar effects across a broad variety devices and screen sizes.

PiPedal includes a pre-installed selection of LV2 plugins from the ToobAmp collection of plugins; but it works with most LV2 Audio plugins. There are literally hundreds of free high-quality LV2 audio plugins that will work with PiPedal. Just install them on your Raspberry Pi, and they will show up in PiPedal.  

If your USB audio adapter has MIDI connectors, you can use MIDI devices (keyboards, controllers, or midi floor boards) to control PiPedal while performing. A simple interface allows you to select how you would like to bind PiPedal controls to midi messages. 





