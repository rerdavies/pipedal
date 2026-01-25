
import { ControlValue } from "../pipedal/Pedalboard.tsx";


export class ChannelMixerStrip {
    inputVu: number = -100.0;
    inputVolume: number = 0;
    outputVu: number = -100.0;
    outputVolume: number = 0;
};

export default class ChannelMixerSettings {
    configured: boolean = false;

    guitarInputChannels: number[] = [1,1];

    guitarOutputChannels: number[] = [0,1];

    guitarMixerSettings: ChannelMixerStrip = new ChannelMixerStrip();


    auxInputChannels: number[] = [0,0];
    auxOutputChannels: number[] = [-1,-1];

    auxMixerSettings: ChannelMixerStrip = new ChannelMixerStrip();

    controls: ControlValue[] = [];
    // Inserts...

    getControlValue(symbol: string): number | null {
        for (let control of this.controls) {
            if (control.key === symbol) {
                return control.value;
            }
        }
        return null;
    }
    setControlValue(symbol: string, value: number): boolean {
        for (let control of this.controls) {
            if (control.key === symbol) {
                if (control.value === value) {
                    return false;
                }
                control.value = value;
                this.controls = this.controls.slice(); // trigger observers.
                return true;
            }
        }
        let newValue = new ControlValue(symbol,value);
        this.controls.push(newValue);
        this.controls = this.controls.slice(); // trigger observers.
        return true;
    }

}