

export default class JackServerSettings {
    deserialize(input: any) : JackServerSettings{
        this.valid = input.valid;
        this.rebootRequired = input.rebootRequired;
        this.sampleRate = input.sampleRate;
        this.bufferSize = input.bufferSize;
        this.numberOfBuffers = input.numberOfBuffers;
        return this;
    }
    constructor(sampleRate?: number, bufferSize?: number, numberOfBuffers?: number)
    {
        if (sampleRate) this.sampleRate = sampleRate;
        if (bufferSize) this.bufferSize = bufferSize;
        if (numberOfBuffers) this.numberOfBuffers = numberOfBuffers;
        if (numberOfBuffers) {
            this.valid = true;
        }
    }
    valid: boolean = false;
    rebootRequired = false;
    sampleRate = 48000;
    bufferSize = 64;
    numberOfBuffers = 3;

    getSummaryText() {
        if (this.valid) {
            return "Sample Rate: " + this.sampleRate + " BufferSize: " + this.bufferSize + " Number of Buffers: " + this.numberOfBuffers;
        } else {
            return "";
        }
    }

}