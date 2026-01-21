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

import {UiControl,ScalePoint} from './Lv2Plugin';
import {PedalboardSplitItem} from './Pedalboard';
import Units from './Units'



export const SplitTypeControl: UiControl = new UiControl().deserialize({
    symbol: PedalboardSplitItem.TYPE_KEY,
    name: "Type",
    index: 0,
    is_input: true,
    min_value: 0,
    max_value: 2,
    default_value: 0,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: true,
    not_on_gui: false,
    toggled_property:  false,
    trigger_property: false,
    pipedal_ledColor: "",
    
    scale_points: ScalePoint.deserialize_array(
        [
            { value: 0, label: "A/B" },
            { value: 1, label: "Mix" },
            { value: 2, label: "L/R" }
        ]
    )
});

export const SplitAbControl: UiControl = new UiControl().deserialize({
    symbol: PedalboardSplitItem.SELECT_KEY,
    name: "Select",
    index: 1,
    is_input: true,

    min_value: 0,
    max_value: 1,
    default_value: 0,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: true,
    not_on_gui: false,
    toggled_property: false,
    trigger_property: false,
    pipedal_ledColor: "",
    scale_points: ScalePoint.deserialize_array(
        [
            { value: 0, label: "A" },
            { value: 1, label: "B" },
        ]
    )
});


export const SplitMixControl: UiControl = new UiControl().deserialize({
    symbol: PedalboardSplitItem.MIX_KEY,
    name: "Mix",
    index: 2,
    is_input: true,
    min_value: -1,
    max_value: 1,
    default_value: 0,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: false,
    not_on_gui: false,
    toggled_property: false,
    trigger_property: false,

    pipedal_ledColor: "",

    scale_points: []
});

export const SplitPanLeftControl: UiControl = new UiControl().deserialize({
    symbol: PedalboardSplitItem.PANL_KEY,
    name: "Pan Top",
    index: 3,
    is_input: true,
    min_value: -1,
    max_value: 1,
    default_value: 0,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: false,
    not_on_gui: false,
    toggled_property: false,
    trigger_property: false,
    pipedal_ledColor: "",

    scale_points: []
});

export const SplitVolLeftControl: UiControl = new UiControl().deserialize({
    symbol: PedalboardSplitItem.VOLL_KEY,
    name: "Vol Top",
    index: 4,
    is_input: true,
    min_value: -60,
    max_value: 12,
    default_value: 0,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: false,
    not_on_gui: false,
    toggled_property: false,
    trigger_property: false,
    pipedal_ledColor: "",

    units: Units.db,
    scale_points: [
        new ScalePoint().deserialize({
            value: -60,
            label: "-INF"
        })
    ]
});


export const SplitPanRightControl: UiControl = new UiControl().deserialize({
    symbol: PedalboardSplitItem.PANR_KEY,
    name: "Pan Bottom",
    index: 5,
    is_input: true,
    min_value: -1,
    max_value: 1,
    default_value: 0,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: false,
    not_on_gui: false,
    toggled_property: false,
    trigger_property: false,
    pipedal_ledColor: "",

    scale_points: []
});

export const SplitVolRightControl: UiControl = new UiControl().deserialize({
    symbol: PedalboardSplitItem.VOLR_KEY,
    name: "Vol Bottom",
    index: 6,
    is_input: true,
    min_value: -60,
    max_value: 12,
    default_value: 0,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: false,
    not_on_gui: false,
    toggled_property: false,
    trigger_property: false,
    pipedal_ledColor: "",

    units: Units.db,
    scale_points: [
        new ScalePoint().deserialize({
            value: -60,
            label: "-INF"
        })
    ]
});

export const SplitControls: UiControl[] = 
[
    SplitTypeControl,
    SplitAbControl,
    SplitMixControl,
    SplitPanLeftControl,
    SplitVolLeftControl,
    SplitPanRightControl,
    SplitVolRightControl,
];


