
import { Pedalboard, ControlValue } from "./Pedalboard.tsx";




export default class ChannelRouterSettings {
    configured: boolean = false;
    channelRouterPresetId: number = -1;

    mainInputChannels: number[] = [1,1];
    mainOutputChannels: number[] = [0,1];
    mainInserts: Pedalboard = new Pedalboard();

    auxInputChannels: number[] = [0,0];
    auxOutputChannels: number[] = [-1,-1];
    auxInserts: Pedalboard = new Pedalboard();

    sendInputChannels: number[] = [-1,-1];
    sendOutputChannels: number[] = [-1,-1];

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
        let newValue = new ControlValue(symbol,value);
        this.controlValues.push(newValue);
        this.controlValues = this.controlValues.slice(); // trigger observers.
        return true;
    }

}