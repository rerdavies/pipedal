

export default class AlsaDeviceInfo {
    deserialize(input: any): AlsaDeviceInfo {
        this.cardId = input.cardId;
        this.id = input.id;
        this.name = input.name;
        this.longName = input.longName;
        this.sampleRates = input.sampleRates as number[];
        this.minBufferSize = input.minBufferSize;
        this.maxBufferSize = input.maxBufferSize;
        this.supportsCapture = input.supportsCapture ? true : false;
        this.supportsPlayback = input.supportsPlayback ? true : false;
        return this;
    }
    static deserialize_array(input: any): AlsaDeviceInfo[]
    {
        let result: AlsaDeviceInfo[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result.push(new AlsaDeviceInfo().deserialize(input[i]));
        }
        return result;
    }
    closestSampleRate(sr: number): number
    {
        let result = 48000;
        let bestError = 1E36;
        for (let rate of this.sampleRates)
        {
            let error = (sr-rate)*(sr-rate);
            if (error < bestError)
            {
                bestError = error;
                result = rate;
            }

        }
        return result;
    }
    closestBufferSize(buffSize: number): number
    {
        let result = 64;
        let bestError = 1E36;

        for (let sz = 2; sz <= this.maxBufferSize; sz *= 2)
        {
            if (sz >= this.minBufferSize)
            {
                let error = (sz-buffSize)*(sz-buffSize);
                if (error < bestError)
                {
                    bestError = error;
                    result = sz;
                }
            }
        }
        return result;
    }

    cardId: number = -1;
    id: string = "";
    name: string = "";
    longName: string = "";
    sampleRates: number[] = [];
    minBufferSize: number = 0;
    maxBufferSize: number = 0;
    supportsCapture: boolean = false;
    supportsPlayback: boolean = false;
};