import {UiControl,ScalePoint} from './Lv2Plugin';
import {PedalBoardSplitItem} from './PedalBoard';
import Units from './Units'



export const SplitTypeControl: UiControl = new UiControl().deserialize({
    symbol: PedalBoardSplitItem.TYPE_KEY,
    name: "Type",
    index: 0,
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
    scale_points: ScalePoint.deserialize_array(
        [
            { value: 0, label: "A/B" },
            { value: 1, label: "Mix" },
            { value: 2, label: "L/R" }
        ]
    )
});

export const SplitAbControl: UiControl = new UiControl().deserialize({
    symbol: PedalBoardSplitItem.SELECT_KEY,
    name: "Select",
    index: 1,
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
    scale_points: ScalePoint.deserialize_array(
        [
            { value: 0, label: "A" },
            { value: 1, label: "B" },
        ]
    )
});


export const SplitMixControl: UiControl = new UiControl().deserialize({
    symbol: PedalBoardSplitItem.MIX_KEY,
    name: "Mix",
    index: 2,
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
    scale_points: []
});

export const SplitPanLeftControl: UiControl = new UiControl().deserialize({
    symbol: PedalBoardSplitItem.PANL_KEY,
    name: "Pan Top",
    index: 3,
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
    scale_points: []
});

export const SplitVolLeftControl: UiControl = new UiControl().deserialize({
    symbol: PedalBoardSplitItem.VOLL_KEY,
    name: "Vol Top",
    index: 4,
    min_value: -60,
    max_value: 12,
    default_value: -3,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: false,
    not_on_gui: false,
    toggled_property: false,
    units: Units.db,
    scale_points: [
        new ScalePoint().deserialize({
            value: -60,
            label: "-INF"
        })
    ]
});


export const SplitPanRightControl: UiControl = new UiControl().deserialize({
    symbol: PedalBoardSplitItem.PANR_KEY,
    name: "Pan Bottom",
    index: 5,
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
    scale_points: []
});

export const SplitVolRightControl: UiControl = new UiControl().deserialize({
    symbol: PedalBoardSplitItem.VOLR_KEY,
    name: "Vol Bottom",
    index: 6,
    min_value: -60,
    max_value: 12,
    default_value: -3,
    range_steps: 0,
    is_logarithmic: false,
    display_priority: -1,
    integer_property: false,
    enumeration_property: false,
    not_on_gui: false,
    toggled_property: false,
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


