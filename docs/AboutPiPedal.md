---
page_icon: img/playbot.jpg
---
## What PiPedal Is

To get the most out of PiPedal, you need to understand a little about what's going on the world of Guitar effects and amp simulations. 

### PiPedal and the Machine Learning Revolution

{% include pageIcon.html %}


Machine Learning (Artifical Intelligence) has changed everything. 

In the world of guitar effect pedals, the revolution started in 2019 when Jatin Chowdhury published [a paper](https://arxiv.org/pdf/2106.03037) describing the results of using machine learning to simulate guitar amplifier effects in real-time. To put things in perspective, LLM AIs like ChatGPT have billions of parameters. Jatin was more interested in how well AI techniques worked if you used small Neural Net models with a few thousand parameters—models small enough to run in real-time. The answer was surprisingly positive: you can use small models and get impressive results. He then proceeded to publish his source code, both for the real-time simulations and the tools used to train his models, under an open-source MIT license. This has created an avalanche of innovation.

Jatin Chowdhury's ML library continues to exist and can be freely downloaded and incorporated into guitar effect plugins available in most formats. The ML library and model training tools remain substantially the same as Chowdhury's initial release. There are significant gains in quality if you double the size of the Neural Networks he used in the original versions. Most models for the ML library now use a larger model, and many have gone through significantly more training than Chowdhury's originals. The result? The models sound amazing! Amp simulations based on Chowdhury's ML library can run in real-time on an ordinary computer and produce emulations that sound significantly better than previous-generation amp emulations used by commercial stomp boxes costing over $1,000.

Community developers have incorporated the ML library into free, open-source guitar plugins that run on Windows, Mac, and Linux, available in most plugin formats (VST2, VST3, AU, RTAS, etc.). Recently, they've also been made available as LV2 plugins that run well in real-time with low latency on Raspberry Pi.

Steven D. Atkinson has since released the Neural Amp Modeler library, which traces its heritage to Chowdhury's ML library while providing support for a wider variety of Machine Learning algorithms. Amazingly, the Neural Amp Modeler library has also been released under an open-source MIT license and incorporated into plugins for most formats and major computing platforms.

Subsequently, a large open-source-minded community has devoted itself to training new Neural Net models for these libraries. The compute time required is substantial, typically requiring rented time on NVIDIA AI hardware in the cloud. Training models also requires access to the equipment being modeled. While the compute time isn't particularly expensive, it takes time and effort to record good source material and train the models, which is why a community effort is necessary. There are now hundreds of high-quality, free models for both libraries, covering everything from heavy metal amps to sublime Tweed emulations, distortion/overdrive/fuzz pedals, and even famous tube-based mixing board strips.

The quality is readily apparent and not a subtle improvement. These are amp simulations that not only sound exactly like what they're simulating but also play and feel like the amps they're emulating. We're talking about 5150 emulations that actually chug, Twin emulations with that sparkly chime that makes your ears itch, and 1962 Fender Bassmaster emulations with the warmth and forgiveness jazz players seek. (These qualities are not often found in previous-generation amp emulations).

So let's just to put all of that in perspective, because the results of all of that have huge implications for the music industry going forward. Jatin Chowdhury's machine learning experiment escaped from the lab in 2019, and has since taken over the world. You can use his code (and derivatives thereof) for free, in guitar plugins that are availble on all major audio platforms and on all major hardware platforms, for free, and get access to a huge library of community-developed models for those plugins which are also free. All of which sound better than $1000+ stopboxes. 

### So Where Does PiPedal Fit In?

And now you can plug in a USB audio adapter (not free, I'm afraid) into your Raspberry Pi (also not free, but very cheap), and run those incredible amp models in realtime with low latency using PiPedal (which is also free). That isn't entirely what PiPedal started off as. But at this particular moment in time, that's what PiPedal is. 

And yes, all of the easy effects (reverb, delay, chorus, flangers, modulators, phasers, etc. etc. etc) are either included with PiPedal, or are available for free as LV2 plugins that can also be downloaded from the internet. And Machine Learning plugins also provide good emulations of overdrive, fuzz pedals and other distortion effects, so that's covered. And convolution reverb and cab IR effects aren't particularly easy, but once you've coverd that, you pretty much have it all. But the living heart and core of a guitar stomp box is the amp emulations, and how good they are. On Pipedal, thanks to Jatin Chowdhury's escaped monster, they are very good indeed.

### What PiPedal Is

PiPedal is a guitar stomp box implmenentation that runs on a Raspberry Pi. It provides a basic set of plugin to get you started, among are which are, notably,  TooB ML (using Jatin Chowdhury's ML library) and TooB Neural Amp Modeler (using Steven Atkinson's Neural Amp Model library). PiPedal provides a basic set of LV2 plugins to get you started. Among those plugins are TooB ML (which uses the ML library), and TooB Neural Amp Modeler (which uses the Neural Amp Modeler library).  PiPedal uses Linux-standard LV2 plugins, allowing you to download and install additional LV2 plugins as needed.

You access all of those plugins and configure them using PiPedal's web interface, which is important. GPUs and real-time audio effects do not get along well together. So if the user interface you use to control PiPedal is remote, it means that PiPedal can be configured to run with extraordinarily low latency, and use 80% or more of availabe CPU to run what really matters: guitar effects plugins. GPUs, by the way, are why you can't really ever get low latency on a laptop or PC. PiPedal lets you  use your phone, or your tablet  or maybe even your laptop to run the user interface, and let your Raspberry Pi concentrate on processing low-latency realtime audio.

Unlike most other audio host applications, PiPedal runs as a daemon, whether you're logged on or not. So all you have to do is plug in your Rasberry Pi, and play - no login required.

When you're playing away from home, PiPedal provides an auto-hotspot feature, which automatically brings up a Wi-Fi hotspot on your Raspberry Pi whenever Pipedal can't see your home router (or an ethernet connection, if that's how you connect to your Pi at home). So all you have to do when you're playing away from home, is power on your Raspberry Pi, pull out your phone or tablet or laptop, and you're all ready to go.

But most importantly PiPedal sounds great because it leverages the work of Jatin Chodhury, and Steven D. Atkinson. And in the end, whether it sounds great is all that really matters. So please do spend some serious time with the TooB ML and TooB Neural Amp Modeler plugins.

That's what PiPedal is.

PiPedal doesn't come with a lot of models, or a lot of effects. It is a platform. The TooB ML plugin (included with Pipedal) includes Jatin Chowdhury's original amp models, which sound pretty good, but are nowhere near as good as current-generation models. So download some models for TooB ML. And TooB Neural Amp Modeler doesn't come with any models at all. So download some models for TooB NAM as well. And PiPedal comes with a very bare minimum set of LV2 effects, just to get you started.  The plugins it does have are (I think) good and useful plugins. TooB Freeverb is there because Freeverb is my favorite goto reverb even in a world filled with convolution reverbs; a convolution reverb, because not everyone agrees; a good flanger (which sounds unreasonable fabulous in stereo); a sensible no-nonsense delay; a decent chorus; a couple of cab simulator effects and a few others. So if you'e looking for anything but bare basics, download some LV2 plugins as well. 

--------
[Up](Documentation.md) | [Installing PiPedal >>](SystemRequirements.md)
