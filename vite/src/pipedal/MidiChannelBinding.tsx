// Copyright (c) 2024 Robin E. R. Davies
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


export enum MidiDeviceSelection {
    DeviceAny = 0,
    DeviceNone = 1,
    DeviceList = 2
}

// This feature is not yet completely implemented.
export const midiChannelBindingControlFeatureEnabled: boolean  = false; // set to true to enable the midi channel binding control in the plugin control view.



export default class MidiChannelBinding {
    deserialize(input: any) : MidiChannelBinding {
        this.deviceSelection = input.deviceSelection;
        this.midiDevices = input.midiDevices;

        this.channel = input.channel;
        this.acceptProgramChanges = input.acceptProgramChanges;
        this.acceptCommonMessages = input.acceptCommonMessages;
        return this;
    }

    clone() { return new MidiChannelBinding().deserialize(this);}

    deviceSelection: number = MidiDeviceSelection.DeviceAny as number;
    midiDevices: string[] = [];
    channel: number = -1;
    acceptProgramChanges: boolean = true;
    acceptCommonMessages: boolean = true;


    static CreateMissingValue() : MidiChannelBinding {
        let result = new MidiChannelBinding();
        return result;
    }
}
