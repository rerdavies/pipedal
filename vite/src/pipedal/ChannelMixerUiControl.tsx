// Copyright (c) 2026 Robin Davies
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
import Units from './Units';
import {UiPlugin, UiControl, PluginType, ControlType, ScalePoint} from './Lv2Plugin';

export const ChannelMixerUiUri = "uri://two-play/pipedal/ppChannelMixer";
export const CHANNEL_MIXER_INSTANCE_ID = -4;

export function makeChanelMixerUiPlugin(): UiPlugin {

    return new UiPlugin().deserialize({
        uri: ChannelMixerUiUri,
        name: "Channel Mixer",
        brand: "",
        label: "",
        plugin_type: PluginType.MixerPlugin,
        plugin_display_type: "Mixer",
        author_name: "",
        author_homepage: "",
        audio_inputs: 1,
        audio_side_chain_inputs: 0,
        audio_outputs: 1,
        has_midi_input: false,
        has_midi_output: false,
        description: "",
        controls: [
            new UiControl().applyProperties({
                symbol: "mainIn",
                name: "Main In",
                index: 0,
                is_input: true,
                min_value: -60.0,
                max_value: 20.0,
                default_value: 0.0,
                is_bypass: false,
                units: Units.db,
                controlType: ControlType.Dial,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
                scale_points: [
                    new ScalePoint().deserialize({ value: -60, label: "-INF" }),
                ],
            }),
            new UiControl().applyProperties({
                symbol: "mainOut",
                name: "Main Out",
                index: 1,
                is_input: true,
                min_value: -60.0,
                max_value: 20.0,
                default_value: 0.0,
                is_bypass: false,
                units: Units.db,
                controlType: ControlType.Dial,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
                scale_points: [
                    new ScalePoint().deserialize({ value: -60, label: "-INF" }),
                ],

            }),
            new UiControl().applyProperties({
                symbol: "auxIn",
                name: "Aux In",
                index: 2,
                is_input: true,
                min_value: -60.0,
                max_value: 20.0,
                default_value: 0.0,
                is_bypass: false,
                units: Units.db,
                controlType: ControlType.Dial,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
                scale_points: [
                    new ScalePoint().deserialize({ value: -60, label: "-INF" }),
                ],
            }),
            new UiControl().applyProperties({
                symbol: "auxOut",
                name: "Aux Out",
                index: 3,
                is_input: true,
                min_value: -60.0,
                max_value: 20.0,
                default_value: 0.0,
                is_bypass: false,
                units: Units.db,
                controlType: ControlType.Dial,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
                scale_points: [
                    new ScalePoint().deserialize({ value: -60, label: "-INF" }),
                ],

            }),
            new UiControl().applyProperties({
                symbol: "mainInVu",
                name: "In",
                index: 4,
                is_input: false,
                min_value: -60.0,
                max_value: 20.0,
                default_value: 0.0,
                controlType: ControlType.Vu,
                is_bypass: false,
                is_program_controller: false,
                units: Units.db,
                custom_units: "",
                connection_optional: false,
            }),
            new UiControl().applyProperties({
                symbol: "mainInVu",
                name: "In",
                index: 5,
                is_input: false,
                min_value: -60.0,
                max_value: 20.0,
                default_value: 0.0,
                controlType: ControlType.Vu,
                is_bypass: false,
                is_program_controller: false,
                units: Units.db,
                custom_units: "",
                connection_optional: false,
            }),
            new UiControl().applyProperties({
                symbol: "mainOutVu",
                name: "Out",
                index: 6,
                is_input: false,
                min_value: -60.0,
                max_value: 20.0,
                default_value: 0.0,
                controlType: ControlType.Vu,
                is_bypass: false,
                is_program_controller: false,
                units: Units.db,
                custom_units: "",
                connection_optional: false,
            }),
            new UiControl().applyProperties({
                symbol: "auxOutVu",
                name: "Out",
                index: 7,
                is_input: false,
                min_value: -60.0,
                max_value: 20.0,
                default_value: 0.0,
                controlType: ControlType.Vu,
                is_bypass: false,
                is_program_controller: false,
                units: Units.db,
                custom_units: "",
                connection_optional: false,
            }),
        ],
        port_groups: [],
        fileProperties: [],
        frequencyPlots: [],
        is_vst3: false,

    }
    );
}



