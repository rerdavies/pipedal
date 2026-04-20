
import { Pedalboard, ControlValue } from "./Pedalboard.tsx";
import JackServerSettings from "./JackServerSettings.tsx";
import JackConfiguration from "./Jack.tsx";


function isActiveChannel(channels: number[]): boolean {
    if (channels.length < 2) return false;
    return channels[1] >= 0 || channels[0] >= 0;
}

function chName(ch: number): string {
    if (ch === -1) {
        return "None";
    }
    return "Ch " + (ch + 1);
}
function channelPairName(channels: number[], maxChannels: number): string {
    if (channels.length !== 2) {
        return "Invalid";
    }
    if (channels[0] === -1 && channels[1] === -1) {
        return "None";
    }

    if (maxChannels === 2) {
        if (channels[0] === 0 && channels[1] === 1) {
            return "Stereo";
        }
        if (channels[0] === 0 && (channels[0] === channels[1] || channels[1] === -1)) {
            return "Left";
        }
        if (channels[1] === 1 && (channels[0] === channels[1])) {
            return "Right";
        }
        if (channels[0] === 1 && channels[1] === 0) {
            return "Right,Left";
        }
    }
    return chName(channels[0]) + "," + chName(channels[1]);
}



export default class ChannelRouterSettings {
    configured: boolean = false;
    channelRouterPresetId: number = -1;

    mainInputChannels: number[] = [1, 1];
    mainOutputChannels: number[] = [0, 1];
    mainInserts: Pedalboard = new Pedalboard();

    auxInputChannels: number[] = [0, 0];
    auxOutputChannels: number[] = [-1, -1];
    auxInserts: Pedalboard = new Pedalboard();

    sendInputChannels: number[] = [-1, -1];
    sendOutputChannels: number[] = [-1, -1];

    controlValues: ControlValue[] = [];
    // Inserts...

    deserialize(obj: any): ChannelRouterSettings {
        this.configured = obj.configured;
        this.channelRouterPresetId = obj.channelRouterPresetId ? obj.channelRouterPresetId : -1;
        this.mainInputChannels = obj.mainInputChannels.slice();
        this.mainOutputChannels = obj.mainOutputChannels.slice();
        this.mainInserts = new Pedalboard().deserialize(obj.mainInserts);
        this.auxInputChannels = obj.auxInputChannels.slice();
        this.auxOutputChannels = obj.auxOutputChannels.slice();
        this.auxInserts = new Pedalboard().deserialize(obj.auxInserts);
        this.sendInputChannels = obj.sendInputChannels.slice();
        this.sendOutputChannels = obj.sendOutputChannels.slice();
        this.controlValues = ControlValue.deserializeArray(obj.controlValues);
        return this;
    }
    getControlValue(symbol: string): number | null {
        for (let control of this.controlValues) {
            if (control.key === symbol) {
                return control.value;
            }
        }
        return null;
    }
    setControlValue(symbol: string, value: number): boolean {
        for (let control of this.controlValues) {
            if (control.key === symbol) {
                if (control.value === value) {
                    return false;
                }
                control.value = value;
                this.controlValues = this.controlValues.slice(); // trigger observers.
                return true;
            }
        }
        let newValue = new ControlValue(symbol, value);
        this.controlValues.push(newValue);
        this.controlValues = this.controlValues.slice(); // trigger observers.
        return true;
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
        let description = channelPairName(this.mainInputChannels, nInputs) + " -> " +
            channelPairName(this.mainOutputChannels, nOutputs);
        if (isActiveChannel(this.auxInputChannels) && isActiveChannel(this.auxOutputChannels)) 
        {
            description += " " + "+ re-amp: " + channelPairName(this.auxInputChannels, nInputs) + " -> " +
                channelPairName(this.auxOutputChannels, nOutputs);
        }
        if (isActiveChannel(this.sendInputChannels) && isActiveChannel(this.sendOutputChannels)) {
            description += " " + "+ send: " +channelPairName(this.sendOutputChannels, nOutputs)
                + " -> " + channelPairName(this.sendInputChannels, nInputs);
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
        for (let ch of this.sendInputChannels) {
            if (ch >= maxInputChannels) {
                return false;
            }
        }
        return true;
    }

}