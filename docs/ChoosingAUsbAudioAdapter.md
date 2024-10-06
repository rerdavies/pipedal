---
page_icon: img/UsbAdapter.jpg
---

## Choosing a USB Audio Adapter

{% include pageIcon.html %}


The quality of the audio adapter you use dramatically affects the quality of the guitar effects. Amp models are particularly sensitive to poor signal-to-noise ratios, because overdrive effects boost the level of noise more than the level of the actual signal. For best results, you should choose a 24-bit audio adapter that provides at least 105db (A-weighted) signal-to-noise ratio. 

Cheap USB audio adapters, claim to to support 24-bit audio, and happily provide 24 bits of data. The problem is that they are usually providing less than 16-bits of signal (the remaining bits being pure noise). 

You will get decent results; but not great results. Stepping up to a more expensive USB adapter dramatically improves the quality of signal you're going to get. With a decent USB audio adapters, some of the state-of-the art Machine Learning plugin produce results that are as good as or better the Helix or Kemperer amp emulations. 

I think it's worthwhile, at this point, to make a brief discursus into the state of the art when it comes to machine learning models of guitar amps. There are two major Machine Learning amp simulators that are in the public domain. Jatin Chowdry produced some of the first really impressive Machine Learning amp simulators (I beleive) as part of his PhD thesis. Jatin Chowdry's ML amp simulation library (which was graciously provided under an open-source MIT license) forms the core of the ToobML plugin included with PiPedal. Steven Atkins' Neural Amp Modeler library (also provided under an MIT license) is the other major ML implementation, that implements a variety of Machine Learning algorithms that have been developed since Jatin Chowdry's initial publication of his results. TooB Neural Amp Modeler uses Steven Atkins' Neural Amp Modeler library. And both have large community-developed libaries of amp models. The quality of amp simulations produced by both of these libraries is breathtaking. As I said previously, as good as or better than Helix or Kemperer amp emulations. 

But to get Helix-quality (or better) results, you need a good USB adapter. For amp simulators, particulary, every extra bit of input signal is precious.

I personally use and recommend the Motu M2 USB audio adapter, although there are plenty of other pro-quality audio adapters that will probably work as well. Although, the MOTU devices are -- in my experience, exceptional. Check the specs carefully, signal-to-noise-ratio is what matters, not bits of data. Cheaper USB audio adapters that sell for less than US$70 will almost certainly not provide adequate signal-to-noise ratio for best results, and invariably won't provide S/N ratio specs for very very good reasons.

Ideally, you want a USB adapter that provides an input volume knob, and an instrument-level input jack, and it's enormously helpful to have a VU meter to display input signal level to the device. Line-level or RCA jacks will have the wrong input impedence, and that has strange effects when 20 feet of guitar cable and tone controls that are designed for instrument-level impedance are involved. For best results, you want the input signal to be as high as possible without clipping. Clipped input signals sound horrible. And every db below "as high as possible" brings up the noise floor. Which is why a VU meter on the 
USB adapter is helpful.

If you don't have an audio adapter with a VU meter, pay close attention to the input VU meter of the first effect in your guitar effect chain. That will indicate the signal level coming into the USB adapter. Ideally, you want the value peaking solidly in the yellow range of the VU meter, and NEVER going red.

Again, the MOTU M2 excels in this regard. It provides large volume knobs for input and output trim, along with very readable VU meters on the front panel which indicate both input and output signal levels. 

--------
[<< An Intro to Snapshots](Snapshots.md)  | [Up](Documentation.md) | [Optimizing Audio Latency >>](AudioLatency.md)


