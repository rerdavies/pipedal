---
page_icon: img/faq.jpg
icon_width: 320px
icon_float: right
---

## Frequently Asked Questions

{% include pageIcon.html %}

### Q. Why is it so important to use a good USB audio adapter?

If you use a cheap consumer-level USB audio adapter, you will not get the best possible results from PiPedal. You will, in fact, get good results, possibly better results than anything you have used before. But not the best possible results, which, I am somewhat embarrassed to say, are astounding. 

Consumer-level USB audio adapters provide somewhere around 96dB of actual dynamic range between the loudest possible signal and the noise floor. And disturbingly, many consumer-level USB audio adapters have a noise floor that is much higher than -96dB. I, personally, have a USB adapter from a brand that most people would consider reputable, that only has a 60dB measured dynamic range. That would be 10 bits of actual audio data, and 14 bits of pure noise, all packed into 24-bit words, for those of you who are counting. 

Signal to noise ratio matters enormously. By the time you have run that -96dB noise signal through a heavily-overdrive Neural Amp Modeler amp model, you wil end up with a signal to noise ratio that could be somewhere around -12dB, or perhaps even less if you are into very heavy metal. It still sounds ok; but it doesn't sound nearly as good as it could.

Professional-level USB audio adapters, on the other hand, provide much higher signal to noise ratios. A good professional-level USB audio adapter will provide somewhere between 110dB and 120dB of of usable dynamic range. That difference sounds modest, but it makes a huge difference when using NAM amp models. It's like you were playing under a blanket before, and suddenly the blanket is gone, your eyes suddenly clear, and a light shines down from heaven. The emulations have clarity, and definition, and genuinely sound and feel like the real thing. As a point of reference, the current generation of Line6 Helix guitar effects boxes manage to provide 126dB of audio input dynamic range (120dB on output), which is admittedly impressive, and plays a very significant role in how good they do sound.

It's perfectly OK to use consumer-level USB audio adapters with PiPedal when you are trying out the software. I think you will like the result. But I would like to encourage you in the strongest possible terms to invest an a decent professional-level USB audio adapter. They are not that expensive -- somewhere in the $100 to $200 range. You will not get the full experience of using PiPedal at its best without a good USB audio adapter. I promise you, you will not regret the investment. With a consumer-level USB audio adapter, you end up with a pretty good stomp box, frankly. It does sound good. But with a pro level adapter, you are going to get a guitar stomp box that provides significantly genuinely better sound quality with a quality that exceeds that of retail guitar effects boxes like Helix that cost thousands of dollars. 

I can only claim partial credit for this. The TooB Neural Amp Modeler, is based on Steven Atkins' open-source Neural Amp Modeler Core library which is the real hero here. The quality of TooB Neural Amp Modeler emulations is simply astounding. To be honest, I very much see PiPedal as a way to put the Neural Amp Modeler models into the hands of as many guitarists as possible, and to make it easy for them to do so. Those of us who work with Neural Amp Modeler often comment on how game-changing this technology is. How NAM changes everything. And how it is the future of guitar effects because it is so much better than anything that has come before it.

But you can't actually experience the full impact of NAM unless you give PiPedal a good USB audio adapter to work with.

Investing in a professional-level USB audio adapter is not going to be an investment that you will regret. "Professional-level" in this context means something in the $100 to $200 price range, not in a $30-$60 price range. The particular spec you need to look for on datasheets is "110dB dynamic range". "-130.5 dbU EIN A-weighted is actually not quite the right spec, but is a solid indicator that you are dealing with a device in the right class. "dB A-weighted" SN/R slightly inflates SN/R numbers, if you see it on a datasheet. But consumer-level USB audio adapters with only 96dB of actual dynamic range will not be able to quote an an A-weighted SN/R or dynamic range in the 110+ range no matter how hard they try. Generally, if the datasheet doesn't say what the device's SN/R is, you can assume that it not good, and not a professional-level device. The device that I personally use is a MOTU M2, which provides 115db of dynamic range (-129dbU EIN), and is a very solidly built device, with great controls, and extremely usable meters.

Devices that would make excellent choices:

- MOTU M2 
- Focusrite Scarlett 2i2 (4th gen)
 -PreSonus Quantum ES 2 USB-C Audio Interface
- Solid State Logic SSL 2 MK II
- Universal Audio Volt 176

Or other devices in that sort of class and price range.



#### Q. My Neural Amp Modeler amp models don't sound that great. Am I doing something wrong? the output signal not loud enough. 

You probably are. 

To get best results, you must make sure that analog  input and output signal levels are trimmed correctly. If you have knobs and dials and buttons, make sure those knobs and dials and buttons are set correctly. If you don't have knobs and dials and buttons, see the previous question for instructions on how to trim input and output levels using `alsamixer`. 

The knobs and dials on your USB audio adapter (whether physical, or `alsamixer` controls) are used to set the analog signal levels being presented the Analog to Digital Converter (ADC). You want to set the input trim levels so that the digital audio signal that PiPedal gets is as close to 0dB as possible, without clipping (going into the red) at any point. If you set it too low, you will lose accuracy in the digital audio data, and your amp models will sound muddy and unpleasant. If you set it too high, you will cause clipping in the digital audio data, and which sounds extremely harsh and unpleasant. Avoid clipping at all costs. 

If you click on the start node and end nodes of a Pipedal preset, you will see VU meters that measure the loudness of the digital audio coming into and going out of Pipedal. These meters are a good way to check that the analog trim levels of your input and output signals are correct. You should, in fact, NEVER use PiPedal's input and output trim controls to control the volume of your audio, unless you have a compelling reason to do so.

The quality of your USB audio adapter is also important. See "Q. Why is it so important to use a good USB audio adapter?", above.


#### Q. Dial control movements are too coarse. How do I make them finer?

A. If you are using a desktop browser, the Ctrl and Shift keys modify the behavior of the dial control.
If you press the Shift key while you are dragging the control, the values will change in 10x smaller increments.
If you press the Ctrl key while you are dragging the control, the values will change in 50x smaller increments. 

#### Q. What do the buffer size controls do? How do I choose good values?

A. When configuring your audio device, you must choose a buffer size, and how many buffers to use. 

The simple answer is: use 32x4 if your hardware can handle it, and use larger sizes if your hardware can't. 

The longer answer is complicated (but not too complicated).

The buffer size determines how much audio data is processed at once by PiPedal. Pipedal processes audio data in chunks, the buffer size sets how large those chunks are. For example, if the buffer size is set to 64 samples, PiPedal will process audio 64 samples at a time. There is a slight cost to processing each chunk of audio, in addition to the cost of processing each sample of audio. So smaller buffer sizes mean that more CPU overhead is incurred for processing the buffer. Larger buffer sizes mean more efficient use of CPU, at the const of increased latency . 

The number of buffers determines how large a queue of audio buffers Pipedal uses. The longer the queue, the less likely the hardware is going to run out of audio data to send or run out of space to store newly received audio data. For example, if the number of buffers is set to 4, Pipedal will use one of those buffers to process audio, and will do its best to keep the audio queue filled with 3 additional buffers that have already been processed, and are waiting to be sent to the hardware, one of which will be being actively processed by the drivers and/or hardware at any given time. 

If the audio output queue runs out of buffers, this is known as an "underrun"; if the input queue runs out of buffers, this is known as an "overrun". And Pipedal refers to either or both of the conditions as "XRuns". You can see the number of XRuns in the status message that appears at the bottom of the screen, or at the top of the Settings page (if you have turned of status messages on the main screen).

XRuns are bad. They cause snaps and pops in the audio, and can sometimes cause the audio device to stop working altogether until it is restarted. So you want to avoid them at all costs.

The buffer size, and the number of buffers determine the latency of the audio system&mdash;the time it takes for audio to travel from the input jack to the output jack. The larger the buffer size, and the more buffers used, the longer the latency. Generally, you want to ensure that the overall latency, jack to jack, is less than 10 milliseconds. Latency longer than 10ms will make your instrument feel unpleasant to play. And very long latencies (in the order of 50ms) can actually make it impossible to play!

So smaller values are better when it comes to latency. And larger values are better when it comes to CPU efficiency and stability.

Unfortunately, choosing the right values depends on the the hardware you are using, and the operating system you are running, which drivers the device is using, and how heavily you are using the CPU when processing audio plugins, perhaps on which USB port you are using, and even the type of USB cable you are using. So there is no one-size-fits-all answer to this question of how to choose good values. 

Fortunately, there is a soft and loose rule that works well with almost all modern hardware, whether consumer-level or professional-level. and that works well with almost all professional-level audio adapters ever built. And that rule is: use a 32x4 buffer configuration. This provides excellent latency, and is stable on most audio adapters. If you are using old, or cheap hardware, you may need to increase the buffer size, or the number of buffers, or both in order to prevent the audio device from x-running. Increase buffer sizes to the smallest value that prevents device from x-running. 16x4 is an excellent configuration too, with even less latency, but it does use more CPU. The CPU overhead of a 32x configuration is small, and the overhead of a 64x configuration is vanishingly small.

If you have to use larger buffers, there is absolutely nothing wrong with a 64x3 or 32x6 configuration. But by the time you get up to 128x buffer configurations, you are probably getting into the range where latency is going to be a problem.

For what it's worth, hardware doesn't really care whether buffer settings are 64x3 or 32x6. The hardware actually just sees one big buffer, and chases reads and writes memory as fast as it can. The interesting difference between 64x3 and 32x6 is that the 32x6 configuration gives the hardware more queued data. In the 64x3 configuration, PiPedal uses one buffer, the hardware uses another buffer, and there is (hopefully) one queued 64-sample buffer waiting to be sent. In the 32x6 configuration, there is one buffer being used by PiPedal, one buffer being used by the hardware, and there are (hopefully) 4 queued 32-sample buffers waiting to be sent. So twice as much queued data when using a 32x6 configuration. 

#### Q. The signal levels on my USB adapter are too low. The input signal is too quiet, or the output signal not loud enough. 

A. This is a common problem with USB audio adapters, especially inexpensive ones. USB adapters are designed to work with a wide range of signal levels. Most USB audio devices have knobs and controls and buttons on the front panel that allow you to adjust the analog input and output levels. But some USB adapters don't have those controls. For these devices, you must use Linux commandline tools to adjust the signal levels. Unfortunately, the default levels that are set by the operating system are much too low; so if you have a device like this, you wil run into a problem. 

To fix this, you can use the `alsamixer` command to adjust the levels. Open a terminal window and type `alsamixer`. This will open a graphical mixer interface in the terminal. Use the arrow keys to navigate to the input and output channels, and use the up and down arrow keys to adjust the levels. Make sure to set the levels appropriately for both input and output channels. See the following question for details on how to trim our `alsamixer` settings, and why trimming them properly is so important. 

Fortunately, settings made in `alsamixer` are persistent, and will remain in effect even if you reboot your system, so you will usually only have to do this procedure once.

--------
[<< LV2 Plugins with MOD User Interfaces](ModUiSupport.md)  | [Up](Documentation.md) | [BuildingPiPedal from Source >>](BuildingPiPedalFromSource.md)
