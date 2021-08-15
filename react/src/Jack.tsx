import {PiPedalArgumentError} from './PiPedalError';

export class JackChannelSelection {
    deserialize(input: any): JackChannelSelection
    {
        this.inputAudioPorts = input.inputAudioPorts.slice();
        this.outputAudioPorts = input.outputAudioPorts.slice();
        this.inputMidiPorts = input.inputMidiPorts.slice();
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
    inputMidiPorts: string[] = [];

    sampleRate: number = 48000;
    bufferSize: number = 64;
    numberOfBuffers: number = 3;

    static makeDefault(jackConfiguration: JackConfiguration): JackChannelSelection {
        let result = new JackChannelSelection();
        result.inputAudioPorts = jackConfiguration.inputAudioPorts.slice(0,2);
        result.outputAudioPorts = jackConfiguration.inputAudioPorts.slice(0,2);
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
            return "Invalid selection";

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
                throw new PiPedalArgumentError("Invalid channel selection."); // should be subset of jackConfiguration.
            }
        } else {
            if (selectedChannels.length === 2) return "Stereo (custom selection)";
            if (selectedChannels.length === 1) return "Mono (custom selection)";
            throw new PiPedalArgumentError("Invalid channel selection."); // should be subset of jackConfiguration.        
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
        this.inputMidiPorts = input.inputMidiPorts;
        this.outputMidiPorts = input.outputMidiPorts;
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
    inputMidiPorts: string[] = [];
    outputMidiPorts: string[] = [];

};

export default JackConfiguration;