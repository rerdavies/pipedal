// Copyright (c) 2022 Robin Davies
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

import {PiPedalArgumentError} from './PiPedalError';
import {AlsaMidiDeviceInfo} from './AlsaMidiDeviceInfo';

function getChannelNumber(channelId: string): number {
    let pos = channelId.indexOf("_");
    let i = Number(channelId.substring(pos+1));
    return i;
}

export class JackChannelSelection {
    deserialize(input: any): JackChannelSelection
    {
        this.inputAudioPorts = input.inputAudioPorts.slice();
        this.outputAudioPorts = input.outputAudioPorts.slice();
        this.inputMidiDevices = AlsaMidiDeviceInfo.deserializeArray(input.inputMidiDevices);
        this.sampleRate = input.sampleRate;
        this.bufferSize = input.bufferSize;
        this.numberOfBuffers = input.numberOfBuffers;
        return this;
    }
    clone() : JackChannelSelection {
        return new JackChannelSelection().deserialize(this);
    }
    inputAudioPorts: string[] = [];
    outputAudioPorts: string[] = [];
    inputMidiDevices: AlsaMidiDeviceInfo[] = [];

    sampleRate: number = 48000;
    bufferSize: number = 64;
    numberOfBuffers: number = 3;

    static makeDefault(jackConfiguration: JackConfiguration): JackChannelSelection {
        let result = new JackChannelSelection();
        result.inputAudioPorts = jackConfiguration.inputAudioPorts.slice(0,2);
        result.outputAudioPorts = jackConfiguration.inputAudioPorts.slice(0,2);
        result.inputMidiDevices = [];
        return result;
    }
    getChannelDisplayValue(selectedChannels: string[], availableChannels: string[],isConfigValid: boolean): string
    {
        if (!isConfigValid)
        {
            if (selectedChannels.length === 2) {
                return "Stereo";
            } 
            if (selectedChannels.length === 1) return "Mono";

            return "\u00A0"; // nbsp

        }
        if (selectedChannels.length === 0) return "Invalid selection";
        if (availableChannels.length === 1) return "Mono";
        if (availableChannels.length === 2) {
            if (selectedChannels.length === 2) return "Stereo";
            if (selectedChannels[0] === availableChannels[0]) {
                return "Mono (left channel only)";
            } else if (selectedChannels[0] === availableChannels[1]) {
                return "Mono (right channel only)";
            } else {
                return "\u00A0"; // nbsp
            }
        } else {
            if (selectedChannels.length === 2) 
            {
                let result: string = "Stereo (" + (getChannelNumber(selectedChannels[0])+1) + "," + (getChannelNumber(selectedChannels[1])+1) + ")";
                return result;
            }
            if (selectedChannels.length === 1) 
            {
                let result: string = "Mono (" + (getChannelNumber(selectedChannels[0])+1) +")";
                return result;

            }
            return "\u00A0"; // nbsp
        }
    }
    getAudioInputDisplayValue(jackConfiguration: JackConfiguration): string
    {
        return this.getChannelDisplayValue(this.inputAudioPorts,jackConfiguration.inputAudioPorts, jackConfiguration.isValid);
    }
    getAudioOutputDisplayValue(jackConfiguration: JackConfiguration): string
    {
        return this.getChannelDisplayValue(this.outputAudioPorts,jackConfiguration.outputAudioPorts, jackConfiguration.isValid);
    }

};

export class JackConfiguration {
    deserialize(input: any): JackConfiguration {
        this.isValid = input.isValid;
        this.isRestarting = input.isRestarting;
        this.errorState = input.errorState;
        this.sampleRate = input.sampleRate;
        this.blockLength = input.blockLength;
        this.midiBufferSize = input.midiBufferSize;
        this.maxAllowedMidiDelta = input.maxAllowedMidiDelta;
        this.inputAudioPorts = input.inputAudioPorts;
        this.outputAudioPorts = input.outputAudioPorts;
        this.inputMidiDevices = AlsaMidiDeviceInfo.deserializeArray(input.inputMidiDevices);
        return this;
    }
    isValid: boolean = false;
    isRestarting: boolean = false;
    errorState: string = "Not loaded.";
    sampleRate: number = 0;
    blockLength: number = 0;
    midiBufferSize: number = 0;
    maxAllowedMidiDelta: number = 0;

    inputAudioPorts: string[] = [];
    outputAudioPorts: string[] = [];
    inputMidiDevices: AlsaMidiDeviceInfo[] = [];


};

export default JackConfiguration;