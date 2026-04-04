
import JackConfiguration from "./Jack.tsx";


export const NONE_CHANNEL: number = -1;
// valid for Aux channel input only.


function isActiveChannel(channels: number[]): boolean {
    if (channels.length < 2) return false;
    return channels[1] !== -1 || channels[0] !== -1;
}

function chName(ch: number, maxChannels: number): string {
    if (ch === -1) {
        return "None";
    }
    if (maxChannels <= 2) {
        if (ch === 0) {
            return "Left";
        }
        if (ch === 1) {
            return "Right";
        }
    }
    return "Ch" + (ch + 1);
}
function channelPairName(channels: number[], maxChannels: number, input: boolean): string {
    if (channels.length !== 2) {
        return "Invalid";
    }
    if (channels[0] === -1 && channels[1] === -1) {
        return "None";
    }
    if (maxChannels === 1) {
        return input ? "Input": "Output";
    }

    if (maxChannels === 2) {
        if (channels[0] === 0 && channels[1] === 1) {
            return "Stereo";
        }
        if (channels[0] === 0 && (channels[0] === channels[1] || channels[1] === -1)) {
            return "Left";
        }
        if (channels[1] === 1 && (channels[0] === channels[1] || channels[0] === -1)) {
            return "Right";
        }
    }
    if (channels[0] === channels[1] || channels[1] === -1) {
        return chName(channels[0],maxChannels);
    }
    return '[' + chName(channels[0],maxChannels) + " " + chName(channels[1],maxChannels) + "]";
}


function arrayEquals(a: number[], b: number[]): boolean {
    if (a.length !== b.length) {
        return false;
    }
    for (let i = 0; i < a.length; i++) {
        if (a[i] !== b[i]) {
            return false;
        }
    }
    return true;
}

export default class ChannelRouterSettings {
    configured: boolean = false;
    modified: boolean = false;
    channelRouterPresetId: number = -1;

    mainInputChannels: number[] = [1, 1];
    mainOutputChannels: number[] = [0, 1];

    auxInputChannels: number[] = [0, 0];
    auxOutputChannels: number[] = [-1, -1];

    // Inserts...

    deserialize(obj: any): ChannelRouterSettings {
        this.configured = obj.configured;
        this.modified = obj.modified;
        this.channelRouterPresetId = obj.channelRouterPresetId ? obj.channelRouterPresetId : -1;
        this.mainInputChannels = obj.mainInputChannels.slice();
        this.mainOutputChannels = obj.mainOutputChannels.slice();
        this.auxInputChannels = obj.auxInputChannels.slice();
        this.auxOutputChannels = obj.auxOutputChannels.slice();
        return this;
    }
    clone() : ChannelRouterSettings {
        return  new ChannelRouterSettings().deserialize(this);
    }

    getDescription(jackConfiguration: JackConfiguration): string {
        if (!this.configured) {
            return "Not configured";
        }
        if (!jackConfiguration.isValid)
        {
            return "Not configured"
        }
        if (!this.isValid(jackConfiguration)) {
            return "Invalid configuration";
        }
        let nInputs = jackConfiguration.inputAudioPorts.length;
        let nOutputs = jackConfiguration.outputAudioPorts.length;
        let description = channelPairName(this.mainInputChannels, nInputs,true) + " -> " +
            channelPairName(this.mainOutputChannels, nOutputs,false);
        if (isActiveChannel(this.auxInputChannels) && isActiveChannel(this.auxOutputChannels)) 
        {
            if (arrayEquals(this.mainInputChannels, this.auxInputChannels))
            {
                description += ", re-amp -> " + channelPairName(this.auxOutputChannels,nOutputs, false);
            } else {
                description += " " + ", aux: " + channelPairName(this.auxInputChannels, nInputs,true) + " -> " +
                    channelPairName(this.auxOutputChannels, nOutputs,false);
            }
        }
        return description;
    }
    canEdit(jackConfiguration: JackConfiguration): boolean {
        return  jackConfiguration.isValid;
    }


    isValid(jackConfiguration: JackConfiguration): boolean {
        if (!this.configured) {
            return false;
        }
        let maxInputChannels = jackConfiguration.inputAudioPorts.length;
        let maxOutputChannels = jackConfiguration.outputAudioPorts.length;

        let hasInput = false;
        let hasOutput = false;
        for (let ch of this.mainInputChannels) {
            if (ch >= 0) {
                hasInput = true;
                if (ch >= maxInputChannels) {
                    return false;
                }
            }
        }
        if (!hasInput) {
            return false;
        }
        for (let ch of this.mainOutputChannels) {
            if (ch >= 0) {
                hasOutput = true;
                if (ch >= maxOutputChannels) {
                    return false;
                }
            }
        }
        if (!hasOutput) {
            return false;
        }
        for (let ch of this.auxInputChannels) {
            if (ch >= maxInputChannels) {
                return false;
            }
        }
        for (let ch of this.auxOutputChannels) {
            if (ch >= maxOutputChannels) {
                return false;
            }
        }
        return true;
    }
    isAuxActive() : boolean {
        return isActiveChannel(this.auxInputChannels) && isActiveChannel(this.auxOutputChannels);
    }

}