---
page_icon: img/UsbAdapter.jpg
---

## Choosing a USB Audio Adapter

{% include pageIcon.html %}


The quality of the audio adapter you use dramatically affects the quality of the guitar effects. Amp models are particularly sensitive to poor signal-to-noise ratios, because overdrive effects boost the level of noise more than the level of the actual signal. For best results, you should choose a 24-bit audio adapter that provides at least 105db (A-weighted) signal-to-noise ratio. 

Cheap USB audio adapters, claim to to support 24-bit audio, and happily provide 24 bits of data. The problem is that they are usually providing less than 16-bits of signal (the remaining bits being pure noise). 

You will get good results; but not astounding results. Stepping up to a more expensive USB adapter dramatically improves the quality of signal you're going to get. With a decent USB audio adapters, NAM A2 amp emulations are significantly better than Helix or Kemperer amp emulations. 

I personally use and recommend the both the MOTU M2, and Focusrite Scarlett 2i2 4th gen (not the Solo, or 3rd gen models which have significantly less dynamic range). There are plenty of other pro-quality audio adapters that will probably work as well. Check the specs carefully, signal-to-noise-ratio is what matters, not bits of data. (This may show up on a datasheet as Dynamic Range, as well). Cheaper USB audio adapters that sell for less than US$70 will almost certainly not provide adequate signal-to-noise ratio for best results, and invariably won't provide S/N ratio specs for very very good reasons.

Ideally, you want a USB adapter that provides an input volume knob, and an instrument-level input jack, and it's enormously helpful to have a VU meter to display input signal level to the device. Microphone and RCA jacks will have the wrong input impedance, which may affect your guitar tone.

For best results, you want the input signal to the DAC to be as high as possible without clipping. Digitally clipped input signals sound horrible. And every db below "as high as possible" brings up the noise floor. Which is why a VU meter on the 
USB adapter is helpful. The input signal should be peaking solidly in the yellow range of the VU meter, and must NEVER go into the red range. 

We recommend "gain-staging" your input signal to -6dbFS. Input and output signals of each successive plugin in your signal chain should be adjusted so that the signal is approximately -6dBFS when you are playing loudly. This is a good general rule of thumb for getting the best possible sound out of your plugins, and is particularly important for getting the best possible sound out of NAM A2 amp models. And this rule applies to the input signal coming into PiPedal from your USB adapter as well. Gain-staging the input signal to -6dBFs provides a little bit of safe headroom to avoid hard-clippping of the input signal.  If you select the Start node in your PiPedal preset, you can see 
the level of the input signal level coming into PiPedal from your USB adapter on the left-side VU meter. If you prefer to use a lower input trim on your USB adapter, adjust the input trim control in the Start node so that the right-side VU meter is peaking at around -6dBFS when you are playing loudly.


--------
[<< Neural Amp Modeler Calibration](NamCalibration.md) | [Up](Documentation.md) | [Optimizing Audio Latency >>](AudioLatency.md)


