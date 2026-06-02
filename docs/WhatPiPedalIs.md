---
page_icon: img/PiPedal20Thumb.jpg
---
## What PiPedal Is

{% include pageIcon.html %}

PiPedal is a Linux-based guitar effects processor&mdash;a "stomp box"&mdash;that is designed to run on a Raspberry Pi, or a small AMD64/x86-64 micro-PC running Ubuntu. It provides a web-based user interface that you can access from any modern web browser on the same network as the PiPedal server. 

PiPedal is built around Neural Amp Modeler (NAM) A1 and A2 guitar amp simulation technology, which provide the best amp simulations in the world, bar none. Sounds like the real thing; and feels like the real thing when you play!

Small computers like a Raspberry Pi 4, or a small N150-class micro-PC provide more processing power than commercial guitar stomp boxes costing thousands of dollars more. PiPedal takes advantage of that processing power to provide state-of-the-art NAM A2 guitar amp simulations. Up to three NAM A2 models can be run simultaneously on a Raspberry Pi 4, and up to seven NAM A2 models can be run simultaneously on a Pi 5, or an N150-class micro-PC. (Even more if you use the Threading feature of Toob Neural Amp Modeler). All with plenty of processing power left over for running additional guitar plugins within the same preset.

PiPedal is designed to be easy to use whether you are at home, or performing onstage, or in a studio. Configure the PiPedal server to automatically start a Wi-Fi hotspot whenever it is unable to see your home Wi-Fi network, and you can confidently use PiPedal when you are on the road, even though you don't have a shared Wi-Fi connection with the PiPedal server! The [PiPedal Android Remote](https://play.google.com/store/apps/details?id=com.twoplay.pipedal) provides one-click access to your PiPedal server on your Android phone or tablet whenever you are on the same Wi-Fi network, and will automatically connect to the PiPedal server Hotspot whenever you are away from home.

The PiPedal Remote Android app allows you to quickly connect to your PiPedal server no matter where you are. 

PiPedal allows installation of the Pipedal client as a Progressive Web App (PWA). This allows you to use PiPedal as a native application on your Windows or Apple desktop or laptop, without the unnecessary clutter of browser-hosted web applications. 


Major features of PiPedal include:

- Stable ultra-low-latency audio processing on the Pipedal server. 
- Direct support for Neural Amp Modeler (NAM) A1 and A2 guitar simulations, providing the best amp simulations in the world, all running on a Raspberry Pi or Ubuntu AMD64/x86-64computer. 
- Support for direct downloads of NAM A2 models from Tone3000.com's web services.
- A web-based user interface that can be accessed from any modern web browser on the same network as the PiPedal server. 
- A UI designed specifically for use on small form-factor touch devices like phones and tablets (although it works equally well in desktop browsers).
- A suite of 26 bundled effects from the TooB effect plugin bundle, specifically designed for use as guitar effects that provide out-of-the-box functionality required to build most guitar effect signal chains.
- The TooB Convolution Reverb provides dramatic and accurate reverb effects based on recordings of real spaces, along with a powerful set 
of controls that allow you to shape reverb IRs to your exact preference.
- Support for most LV2 guitar effect plugins, including support for LV2 plugins that have MOD web interfaces.
- Support for MIDI control of PiPedal parameters from external MIDI controllers, or MIDI floorboards, via USB or Bluetooth connections.
- A brand new set of NAM A2-based Factory Presets, and a small selection of NAM A2 models pre-installed and ready to use.
- A Progressive Web App (PWA) implementation that allows you to install PiPedal as a native application (without associated browser controls) on a Windows or Apple computer.
- An auto Hotspot on the PiPedal server that starts up whenever the Pipedal server is unable to connect to your home network. This allows you to confidently use PiPedal when you are gigging or in the studio, even though you don't have a shared Wi-Fi connection with the PiPedal server. 
- The [PiPedal Remote Android app](https://play.google.com/store/apps/details?id=com.twoplay.pipedal) provides one-click access to the PiPedal server on your Android phone or tablet whenever you are on the same Wi-Fi network, and will automatically search for and connect to the PiPedal server Hotspot when you are not at home.
- Up to six snapshots per preset, which allow you to build flexible presets that can be reconfigured on the fly during a live performance. 
- A Performance View, which provides a minimal interface for easy and efficient use of PiPedal on stage.

PiPedal started as a Pandemic Project labor of love, but has evolved into the solution of choice for running live guitar effects in performance on Linux, in no small part due to contributions made by other open-source developers like Jatin Chowdhury, Steven Atkinson and Mike Oliphant. 

PiPedal is open-source software released under an MIT license. You can download the source code, and contribute to the project, on GitHub: [https://github.com/rerdavies/pipedal](https://github.com/rerdavies/pipedal). If you would like to sponsor further development of PiPedal, you can do so via [PiPedal's GitHub Sponsorship page](https://github.com/sponsors/rerdavies).

--------
[<< Configuring PiPedal After Installation](Configuring.md)  | [Up](Documentation.md) | [Machine Learning in PiPedal (A History)](PiPedalHistory.md)
