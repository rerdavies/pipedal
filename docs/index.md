<div style="position: relative">
  <img src="img/pipedal.png" style="width: 100%; height: 160px;object-fit: cover"/>
</div>

<h2 style="font-weight: 300; font-size: 1.6em;line-height: 1em; padding-top: 32px; padding-bottom: 0px;margin-bottom: 4px">PiPedal Guitar Effects Processor</h2>
<p style="padding-top: 0px; opacity: 0.6; padding-bottom: 16px">A Raspberry Pi-based stomp box designed to be controlled from a phone or tablet.</p>

<div style="padding-left:48px; padding-right: 48px; padding-top: 32px; padding-bottom: 16px;
background-color: #DCDCFF; border-radius: 8px; border: 0px solid #555; 
margin-bottom: 48px; margin-top: 16px; 
margin-left: 16px; margin-right: 48px;
box-shadow: 1px 4px 12px rgba(0, 0, 0, 0.3);">
    <div style="opacity: 0.8;">
        <h2 style='font-weight: 300; font-family: "Roboto Light", "Segoe UI", Roboto, Helvetica, Arial, sans-serif, "Apple Color Emoji", "Segoe UI Emoji", "Segoe UI Symbol"; font-weight: 400; font-size: 1.3em;line-height: 1.2em; padding-top: 0px; padding-bottom: 16px;margin-bottom: 8px'
        >Announcing <a href="PiPedal2.html">PiPedal 2.0</a></h2>
        <p>
        The first public preview release of Pipedal version 2.0.102-alpha is now available! 
        </p>

        <p>This release includes </p>

        <ul>
        <li style="margin-bottom: 8px">
        Support  for Neural Amp Modeler A2 models&mdash;the next generation of the ground-breaking 
        Neural Amp Modeler technology.</li>
        <li style="margin-bottom: 8px">
        Integration with Tone3000.com services for easy downloading of Neural Amp Modeler A2 models and IIRs.</li>
        <li style="margin-bottom: 8px">
        A new Channel Routing dialog for global routing of auxiliary input channels, and unprocessed re-amp output channels.</li> 
        <li>Install Pipedal as a Progressive Web Application (PWA), which allows you to launch Pipedal from your desktop as a native desktop application. (A nice feature if you are accessing Pipedal from an Apple device).</li>
        <li style="margin-bottom: 8px">
        Many other minor features, improvements, and bug fixes.</li>
        </ul>

        <p>For more information, and to download PiPedal v2.0.102-alpha <a href="PiPedal2.html">click here</a>.
        </p>
    </div>
</div>

<a href="https://rerdavies.github.io/pipedal/ReleaseNotes"><img src="https://img.shields.io/github/v/release/rerdavies/pipedal?color=%23808080"/></a>
<a href="https://rerdavies.github.io/pipedal/download"><img src="https://img.shields.io/badge/Download-008060" /></a>
<a href="https://rerdavies.github.io/pipedal/Documentation"><img src="https://img.shields.io/badge/Documentation-0060d0"/></a>
 
_To download PiPedal v1.5.99, click [*here*](download.md). 
To view PiPedal documentation, click [*here*](Documentation.md)._

Use your Raspberry Pi as a guitar effects pedal. Configure and control PiPedal with your phone or tablet.
PiPedal running on a Raspberry Pi 4 or Pi 5 provides stable super-low-latency audio via external USB audio devices, or internal Raspberry Pi audio hats.

PiPedal will also run on Ubuntu 24.x or later (amd64/x86-64 and aarch64). Make sure you follow the [Ubuntu post-install 
instructions](https://rerdavies.github.io/pipedal/Configuring.html) to make sure your Ubuntu OS is using a  realtime-capable kernel.

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





