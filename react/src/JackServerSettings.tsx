// Copyright (c) 2021 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.



export default class JackServerSettings {
    deserialize(input: any) : JackServerSettings{
        this.valid = input.valid;
        this.rebootRequired = input.rebootRequired;
        this.alsaDevice = input.alsaDevice?? "";
        this.sampleRate = input.sampleRate;
        this.bufferSize = input.bufferSize;
        this.numberOfBuffers = input.numberOfBuffers;
        return this;
    }
    // constructor(alsaDevice: string, sampleRate?: number, bufferSize?: number, numberOfBuffers?: number)
    // {
    //     if (sampleRate) this.sampleRate = sampleRate;
    //     if (bufferSize) this.bufferSize = bufferSize;
    //     if (numberOfBuffers) this.numberOfBuffers = numberOfBuffers;
    //     if (numberOfBuffers) {
    //         this.valid = true;
    //     }
    // }
    clone(): JackServerSettings
    {
        return new JackServerSettings().deserialize(this);
    }
    valid: boolean = false;
    rebootRequired = false;
    alsaDevice: string = "";
    sampleRate = 48000;
    bufferSize = 64;
    numberOfBuffers = 3;

    getSummaryText() {
        if (this.valid) {
            let device = this.alsaDevice;
            if (device.startsWith("hw:")) device = device.substr(3);
            return device + " - Rate: " + this.sampleRate + " BufferSize: " + this.bufferSize + " Buffers: " + this.numberOfBuffers;
        } else {
            return "Not configured";
        }
    }

}