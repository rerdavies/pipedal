---
page_icon: img/playbot.jpg
---
## Machine Learning and Guitar Amp Simulation (A History)


{% include pageIcon.html %}


Machine Learning (Artificial Intelligence) has changed everything. 

In the world of guitar effect pedals, the revolution started in 2019 when Jatin Chowdhury published [a paper](https://arxiv.org/pdf/2106.03037) describing the results of using machine learning to simulate guitar amplifier effects in real-time. To put things in perspective, LLM AIs like ChatGPT have billions of parameters. Jatin was more interested in how well AI techniques worked if you used small Neural Net models with a few thousand parameters—models small enough to run in real-time. The answer was surprisingly positive: you can use small models and get impressive results. He then proceeded to publish his source code, both for the real-time simulations and the tools used to train his models, under an open-source MIT license. This has launched an avalanche of innovation.

Jatin Chowdhury's ML library continues to exist and can be freely downloaded and incorporated into guitar effect plugins available in most formats. The ML library and model training tools remain substantially the same as Chowdhury's initial release. There are significant gains in quality if you double the size of the Neural Networks he used in the original versions. Most models for the ML library now use a larger model, and many have gone through significantly more training than Chowdhury's originals. The result? The models sound amazing! Amp simulations based on Chowdhury's ML library can run in real-time on an ordinary computer and produce emulations that sound significantly better than previous-generation amp emulations.

Community developers quickly incorporated the ML library into free, open-source guitar plugins that run on Windows, Mac, and Linux. The TooB ML plugin, included with Pipedal, uses Jatin Chowdhury's ML library to implement amp simulations using ML technology. And some of Jatin Chowdhury's original ML models are also pre-installed by PiPedal. Original ML models, are still pretty good, but are not as good as current-generation NAM A1 and A2 models. 

But the story does not end there.

Steven Atkinson has since released the [Neural Amp Modeler library](https://github.com/sdatkinson/NeuralAmpModelerCore), which traces its heritage to Chowdhury's ML library while providing support for a wider variety of Machine Learning algorithms. The Neural Amp Modeler library has also been released under an open-source MIT license and incorporated into plugins for most plugin formats and major computing platforms. The TooB Neural Amp Modeler plugin (bundled with Pipedal) uses the Neural Amp Modeler Core library to implement NAM A2 models. Credit also needs to be given to Mike Oliphant, whose [NeuralAudio library](http:s://github.com/mikeoliphant/NeuralAudio) provides a highly-optimized implementation of the NAM A1 algorithms that allow NAM A1 models to be used on a Raspberry Pi 4.  

And finally (for now), in June of 2025, Steven Atkinson released NAM A2&mdash;the next generation of Neural Amp Modeler technology, which provides even more accurate amp simulations than NAM A1, while using even less CPU. NAM A2 models are also supported by the TooB Neural Amp Modeler plugin bundled with Pipedal, and very much form the living breathing core of PiPedal.

And the quality of NAM models is breathtaking! Game changing, even (a phrase that comes up suprisingly often when people are talking about NAM). 

Subsequently, a large open-source-minded community has devoted itself to training new Neural Net models for NAM. Of these initiatives, [Tone3000.com](https://www.tone3000.com) is the most important. Tone3000 provides easy access to thousands of free, high-quality, downloadable NAM models. Models were originally available in NAM A1 format, but Tone3000.com has just retrained all of the models on Tone3000.com using NAM A2 technology. If you have a favorite amp, you will probably find good models for your amp on Tone3000. And Tone3000 also provides an easy way to [generate .nam models](https://www.tone3000.com/capture) from captures of your own amps and pedals as well! 

So let's just to put all of that in perspective, because the results of all of that have huge implications for the music industry going forward. Jatin Chowdhury's machine learning experiment escaped from the lab in 2019, and has since taken over the world. You can use his code (and derivatives thereof) for free, in guitar plugins that are available on all major audio platforms and on all major hardware platforms, for free, and get access to a huge library of community-developed models for those plugins which are also free. All of which sound better than $1000+ stompboxes. All of which runs on a hardware platform that costs less than $150, when you use PiPedal! And sounds beter than other commercial digital amp simulations running on hardware costing thousands of dollars more.

## How PiPedal Fits Into All of This

PiPedal as first conceived, was a Pandemic Project that I wrote to explore better ways to do amp simulations. At that time, Guitarix was the go-to solution for open-source amp emulations. But as a player, I was not at all satisfied with any of the available digital amp simulation technologies. Nor was I particularly impressed by commercial digital amp simulations, which, while they often sounded somewhat 
like the amp that were being simulated, but often felt cold and lifeless and unpleasant to play. That was somewhat overtaken by the release of ML libraries, which do provide much better amp simulations than the line of research I was pursuing at the time. But PiPedal was completely overtaken by the NAM revolution. 

And, of course, somewhere along the line of development (after including an ML-based amp simulator, but before including support for NAM), it became more and more  clear to me that PiPedal was becoming something quite extraordinary. It had vastly exceeded my personal expectations, and I felt that I had a duty (not a word used lightly) to share it with the world. So I released it as open source software, and have been sharing it with the world ever since. Yes, it has gone through a lot of polishing and refinement. 
As a professional software developer, I have standards for the software I write; and I have been working hard to make sure that PiPedal is a product that meets those standards. As a professional developer, it has to be product that I can confidently associate my name with. All that to say, that it didn't neccesarily start out that way, but it has evolved into something that I am proud of, something that seems strangely larger on the inside than it is on the outside, and something that I am happy to share with the world.

Currently, the heart and soul of PiPedal is the open-source work of Jatin Chowdhury and Steven Atkinson, and the incredible community of developers who have built on their work to create the plugin code and models that run on PiPedal. Credit where credit is due. Pipedal would not be what it is without the open-source contributions of these people. And the folks at Tone3000.com, who have done an amazing job of making it easy to find and download high-quality NAM models, and to create your own NAM models from captures of your own amps and pedals. And, of course, Mike Oliphant, whose NeuralAudio library provides a highly-optimized implementation of the NAM A1 algorithms that allow NAM A1 models to be used on a Raspberry Pi 4.


PiPedal 2.0 is a major new release that completes the transition to NAM-based amp simulations (and picks up NAM A2 support as bonus). The Factory Presets are now all based on NAM A2 models, and a small selection of NAM A2 models are pre-installed and ready to use. The updated TooB Neural Amp Modeler plugin, which provides support for both NAM A1 and A2 models, is bundled with PiPedal. And the ability to download NAM A2 models directly from Tone3000.com's web services is built into the TooB Neural Amp Modeler plugin as well. 

In the end, what really counts in a guitar stomp box is the amp emulations, and how good they are. On Pipedal, thanks to Jatin Chowdhury's escaped monster, they are very good indeed. And thanks to Steven Atkinson's amazing NAM A2 technology, Pipedal is a platform for running the best amp simulations in the world, on a Raspberry Pi or Ubuntu AMD64/x86-64 computer, using a phone or tablet to control the PiPedal server (for which I would like to take a little bit of credit). 


--------
[<< What PiPedal Is](WhatPiPedalIs.md) | [Up](Documentation.md) | [How to Use PiPedal >>](HowToUsePiPedal.html)
