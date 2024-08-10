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



import Units from './Units';

interface Deserializable<T> {
    deserialize(input: any): T;
}


export class  Port implements Deserializable<Port> {
    deserialize(input: any): Port {
        this.port_index = input.port_index;
        this.symbol = input.symbol;
        this.name = input.name;
        this.min_value = input.min_value;
        this.max_value = input.max_value;
        this.default_value = input.default_value;
        this.scale_points = ScalePoint.deserialize_array(input.scale_points);
        this.is_input = input.is_input;
        this.is_output = input.is_output;
        this.is_control_port = input.is_control_port;
        this.is_audio_port = input.is_audio_port;
        this.is_atom_port = input.is_atom_port;
        this.is_valid = input.is_valid;
        this.supports_midi = input.supports_midi;
        this.supports_time_position = input.supports_time_position;
        this.port_group = input.port_group;
        this.comment = input.comment;
        this.is_bypass = input.is_bypass;
        this.is_program_controller = input.is_program_controller;
        return this;
    }

    static EmptyPorts: Port[] = [];

    static deserialize_array(input: any): Port[] {
        let result: Port[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result[i] = new Port().deserialize(input[i]);
        }
        return result;
    }
    port_index:      number = -1;
    symbol:          string = "";
    name:            string = "";
    min_value:       number = 0;
    max_value:       number = 1;
    default_value:   number = 0.5;
    scale_points:    ScalePoint[] = [];
    is_input:        boolean = false;
    is_output:       boolean = false
    is_control_port: boolean = false;
    is_audio_port:   boolean = false;
    is_atom_port:    boolean = false;
    is_valid:        boolean = false;
    supports_midi:   boolean = false;
    supports_time_position: boolean = false;
    port_group:      string = "";
    comment: string = "";
    is_bypass: boolean = false;
    is_program_controller: boolean = false;
}

export class PortGroup {
    deserialize(input: any): PortGroup {
        this.uri = input.uri;
        this.symbol = input.symbol;
        this.name = input.name;
        this.parent_group = input.parent_group;
        this.program_list_id = input.program_list_id ?? -1;

        return this;
    }
    static deserialize_array(input: any) : PortGroup[] {
        let result: PortGroup[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result.push(new PortGroup().deserialize(input[i]));
        }
        return result;
    }

    uri: string = "";
    symbol: string = "";
    name: string = "";
    parent_group: string = "";
    program_list_id: number = -1;
};

export class UiFileType {
    deserialize(input: any): UiFileType {
        this.name = input.name;
        this.fileExtension = input.fileExtension;
        this.mimeType = input.mimeType;
        return this;
    }
    static deserialize_array(input: any): UiFileType[]
    {
        let result: UiFileType[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result[i] = new UiFileType().deserialize(input[i]);
        }
        return result;
    }

    private static IsAndroid() : boolean
    {
        return /Android/i.test(navigator.userAgent);
    }
    static MergeMimeTypes(fileTypes: UiFileType[]): string
    {
        if (fileTypes.length === 0) { 
            return "";
        }
        let result = fileTypes[0].mimeType;
        for (let i = 1; i < fileTypes.length; ++i)
        {
            let fileType = fileTypes[i];
            if (fileType.mimeType !== result)
            {
                if (result.startsWith("audio/") && fileType.mimeType.startsWith("audio/"))
                {
                    result = "audio/*";
                } else if (result.startsWith("video/") && fileType.mimeType.startsWith("video/"))
                {
                    result = "video/*";
                } else if (result.startsWith("text/") && fileType.mimeType.startsWith("text/"))
                {
                    result = "text/*";
                } else {
                    result = "application/octet-stream";
                }
            }
        }
        if (this.IsAndroid())
        {
            if (result.startsWith("audio/")) result = "audio/*";
            if (result.startsWith("video/")) result = "video/*";
            if (result.startsWith("text/")) result = "text/*";
        } else {
            // chrome desktop thinks "application/octet-stream" is .exe, .com, or .bat.
            // Feed it file extensions isntead.
            if (result === "application/octet-stream")
            {
                result = "";
                for (let i = 0; i < fileTypes.length; ++i)
                {
                    if (i !== 0)
                    {
                        result += ',';
                    }
                    result += fileTypes[i].fileExtension;
                }
            }
        }
        return result;
    }
    name: string = "";
    fileExtension: string = "";
    mimeType: string = "";
}

export class UiPropertyNotification {
    deserialize(input: any): UiPropertyNotification
    {
        this.portIndex = input.portIndex;
        this.symbol = input.symbol;
        this.plugin = input.plugin;
        this.protocol = input.protocol;
        return this;
    }
    static deserialize_array(input: any): UiPropertyNotification[]
    {
        let result: UiPropertyNotification[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result[i] = new UiPropertyNotification().deserialize(input[i]);
        }
        return result;
    }
    portIndex: number = -1;
    symbol: string = "";
    plugin: string = "";
    protocol: string = "";

};

export class UiFrequencyPlot {
    deserialize(input: any): UiFrequencyPlot
    {
        this.patchProperty = input.patchProperty;
        this.index = input.index;
        this.portGroup = input.portGroup;
        this.xLeft = input.xLeft;
        this.xRight = input.xRight;
        this.xLog = input.xLog;
        this.yTop = input.yTop;
        this.yBottom = input.yBottom;
        this.width = input.width;
        return this;
    }
    static deserialize_array(input: any): UiFrequencyPlot[]
    {
        let result: UiFrequencyPlot[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result[i] = new UiFrequencyPlot().deserialize(input[i]);
        }
        return result;
    }

    patchProperty: string = "";
    index: number = -1;
    portGroup: string = "";
    xLeft: number = -1;
    xRight: number = -1;
    xLog: boolean = true;
    yTop: number = -1;
    yBottom: number = -1;
    width: number = -1;

};
export class UiFileProperty {
        deserialize(input: any): UiFileProperty
        {
            this.label = input.label;
            this.fileTypes = UiFileType.deserialize_array(input.fileTypes);
            this.patchProperty = input.patchProperty;
            this.directory = input.directory;
            this.index = input.index;
            this.portGroup = input.portGroup;
            return this;
        }
        static deserialize_array(input: any): UiFileProperty[]
        {
            let result: UiFileProperty[] = [];
            for (let i = 0; i < input.length; ++i)
            {
                result[i] = new UiFileProperty().deserialize(input[i]);
            }
            return result;
        }

        private static getFileExtension(name: string): string {
            let pos = name.lastIndexOf('.');
            let filenamePos = name.lastIndexOf('/') +1;
            filenamePos = Math.max(name.lastIndexOf('\\')+1);
            filenamePos = Math.max(name.lastIndexOf(':')+1);
            if (pos < filenamePos)
            {
                return "";
            }

            if (pos !== -1) {
                return name.substring(pos);
            }
            return "";
        }
    
        wantsFile(filename: string) : boolean {
            if (this.fileTypes.length === 0) {
                return true;
            }
            let extension = UiFileProperty.getFileExtension(filename);
            for (let fileType of this.fileTypes)
            {
                if (fileType.fileExtension === extension )
                {
                    return true;
                }
                if (fileType.fileExtension === "*" || fileType.fileExtension === "")
                {
                    return true;
                }
            }
            return false;
        }

        label:   string = "";
        fileTypes: UiFileType[] = [];
        patchProperty: string = "";
        directory: string = "";
        index: number = -1;
        portGroup: string = "";

};
export class  Lv2Plugin implements Deserializable<Lv2Plugin> {
    deserialize(input: any): Lv2Plugin
    {
        this.uri = input.uri;
        this.name = input.name;
        this.brand = input.name? input.name: "";
        this.label = input.label? input.label: this.name;
        this.plugin_class = input.plugin_class;
        this.supported_features = input.supported_features;
        this.required_features = input.required_features;
        this.optional_features = input.optional_features;
        this.author_name = input.author_name;
        this.author_homepage = input.author_homepage;
        this.comment = input.comment;
        this.ports=  Port.deserialize_array(input.ports);
        this.port_groups = PortGroup.deserialize_array(input.port_groups);
        if (input.fileProperties)
        {
            this.fileProperties = UiFileProperty.deserialize_array(input.fileProperties)
        } else {
            this.fileProperties = [];
        }
        if (input.frequencyPlots)
        {
            this.frequencyPlots = UiFrequencyPlot.deserialize_array(input.frequencyPlots)
        } else {
            this.frequencyPlots = [];
        }

        if (input.uiPortNotifications)
        {
            this.uiPortNotifications = UiPropertyNotification.deserialize_array(input.uiPortNotifications);
        } else {
            this.uiPortNotifications = [];
        }
        return this;
    }
    static EmptyFeatures: string[] = [];

    uri:                string = "";
    name:               string = "";
    brand:              string = "";
    label:              string = "";
    plugin_class:       string = "";
    supported_features: string[] = Lv2Plugin.EmptyFeatures;
    required_features:  string[] = Lv2Plugin.EmptyFeatures;
    optional_features:  string[] = Lv2Plugin.EmptyFeatures;
    author_name:        string = "";
    author_homepage:    string = "";
    comment:            string = "";
    ports:              Port[] = Port.EmptyPorts;
    port_groups:        PortGroup[] = [];
    fileProperties:     UiFileProperty[] = [];
    frequencyPlots:     UiFrequencyPlot[] = [];
    uiPortNotifications: UiPropertyNotification[] = [];
}


export class  ScalePoint implements Deserializable<ScalePoint> {
    deserialize(input: any): ScalePoint {
        this.value = input.value;
        this.label = input.label;
        return this;
    }
    static deserialize_array(input: any): ScalePoint[]
    {
        let result: ScalePoint[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result[i] = new ScalePoint().deserialize(input[i]);
        }
        return result;
    }
    value: number = 0;
    label: string = "";
}

export enum PluginType {
    // Reserved types used in pedalboards.
    None="",
    InvalidPlugin= "InvalidPlugin",
    
    Plugin = "Plugin",
    AllpassPlugin = "AllpassPlugin",
    AmplifierPlugin = "AmplifierPlugin",
    AnalyserPlugin = "AnalyserPlugin",
    BandpassPlugin = "BandpassPlugin",
    ChorusPlugin = "ChorusPlugin",
    CombPlugin = "CombPlugin",
    CompressorPlugin = "CompressorPlugin",
    ConstantPlugin = "ConstantPlugin",
    ConverterPlugin = "ConverterPlugin",
    DelayPlugin = "DelayPlugin",
    DistortionPlugin = "DistortionPlugin",
    DynamicsPlugin = "DynamicsPlugin",
    EQPlugin = "EQPlugin",
    EnvelopePlugin = "EnvelopePlugin",
    ExpanderPlugin = "ExpanderPlugin",
    FilterPlugin = "FilterPlugin",
    FlangerPlugin = "FlangerPlugin",
    FunctionPlugin = "FunctionPlugin",
    GatePlugin = "GatePlugin",
    GeneratorPlugin = "GeneratorPlugin",
    HighpassPlugin = "HighpassPlugin",
    InstrumentPlugin = "InstrumentPlugin",
    LimiterPlugin = "LimiterPlugin",
    LowpassPlugin = "LowpassPlugin",
    MixerPlugin = "MixerPlugin",
    ModulatorPlugin = "ModulatorPlugin",
    MultiEQPlugin = "MultiEQPlugin",
    OscillatorPlugin = "OscillatorPlugin",
    ParaEQPlugin = "ParaEQPlugin",
    PhaserPlugin = "PhaserPlugin",
    PitchPlugin = "PitchPlugin",
    ReverbPlugin = "ReverbPlugin",
    SimulatorPlugin = "SimulatorPlugin",
    SpatialPlugin = "SpatialPlugin",
    SpectralPlugin = "SpectralPlugin",
    UtilityPlugin = "UtilityPlugin",
    WaveshaperPlugin = "WaveshaperPlugin",

        // psuedo plugin type for the Amps node of the filter dialog.
    PiPedalAmpsNode = "PiPedalAmpsNode",

    // pseudo plugin types for splitter.
    SplitA = "SplitA",
    SplitB = "SplitB",   //"img/fx_split_b.svg";
    SplitMix = "SplitMix", //SplitMix; //"img/fx_dial.svg";
    SplitLR = "SplitLR", //"img/fx_dial.svg";



    // psuedo plugin type for misc icons.
    ErrorPlugin = "ErrorPlugin",
    Terminal = "Terminal", //"img/fx_terminal.svg"; 

}

export enum ControlType {
    Dial,
    OnOffSwitch,
    ABSwitch,
    Select,

    Tuner,
    Vu,
    DbVu,
    OutputSelect
}

export class  UiControl implements Deserializable<UiControl> {
    deserialize(input: any): UiControl
    {
        this.symbol = input.symbol;
        this.name = input.name;
        this.index = input.index;
        this.is_input = input.is_input;
        this.min_value = input.min_value;
        this.max_value = input.max_value;
        this.default_value = input.default_value;
        this.is_logarithmic = input.is_logarithmic;
        this.display_priority = input.display_priority;
        this.range_steps = input.range_steps;
        this.integer_property = input.integer_property;
        this.enumeration_property = input.enumeration_property;
        this.toggled_property = input.toggled_property;
        this.trigger = input.trigger;
        this.not_on_gui = input.not_on_gui;
        this.scale_points = ScalePoint.deserialize_array(input.scale_points);
        this.port_group = input.port_group;
        this.units = input.units as Units;

        this.comment = input.comment ?? "";
        this.is_bypass = input.is_bypass ? true: false;
        this.is_program_controller = input.is_program_controller? true: false;
        this.custom_units = input.custom_units ?? "";
        this.connection_optional = input.connection_optional ? true: false;


        if (this.is_bypass)
        {
            this.not_on_gui = true;
        }

        this.controlType = ControlType.Dial;

        if (!this.is_input)
        {
            if (this.units === Units.midiNote)
            {
                this.controlType = ControlType.Tuner;

            } else if (this.units === Units.db)
            {
                this.controlType = ControlType.DbVu;
            } else if (this.enumeration_property) {
                this.controlType = ControlType.OutputSelect;
            } else {
                this.controlType = ControlType.Vu;
            }
        }
        if (this.isValidEnumeration())
        {
            this.controlType = ControlType.Select;
            if (this.scale_points.length === 2)
            {
                this.controlType = ControlType.ABSwitch;
            }
        } else {
            if (this.toggled_property || (this.integer_property && this.min_value === 0 && this.max_value === 1))
            {
                this.controlType = ControlType.OnOffSwitch;
            }
        }
        return this;
    }
    applyProperties(properties: Partial<UiControl>): UiControl 
    {
        return {...this,...properties};
    }
    private hasScalePoint(value: number): boolean {
        for (let scale_point of this.scale_points)
        {
            if (scale_point.value === value) return true;
        }
        return false;
    }
    private isValidEnumeration() : boolean {
        if (this.enumeration_property) return true;
        if (this.toggled_property && this.min_value === 0 && this.max_value === 1)
        {
            if (this.hasScalePoint(this.min_value) && this.hasScalePoint(this.max_value))
            {
                return true;
            }
        }
        if (this.integer_property && this.min_value === 0 && this.max_value === 1)
        {
            if (this.hasScalePoint(this.min_value) && this.hasScalePoint(this.max_value))
            {
                return true;
            }
        }
        return false;
    }

    controlType: ControlType = ControlType.Dial; // non-serializable.

    static deserialize_array(input: any): UiControl[] {
        let result: UiControl[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result[i] = new UiControl().deserialize(input[i]);
        }
        return result;
    }
    symbol: string = "";
    name:   string = "";
    index: number = -1;
    is_input: boolean = true;
    min_value: number = 0;
    max_value: number = 1;
    default_value:number = 0.5;
    is_logarithmic: boolean = false;
    display_priority: number = -1;
    range_steps: number = 0;
    integer_property:boolean = false;
    enumeration_property: boolean = false;
    trigger: boolean = false;
    not_on_gui: boolean = false;
    toggled_property: boolean = false;
    scale_points: ScalePoint[] = [];
    port_group: string = "";
    units: Units = Units.none;
    comment: string = "";
    is_bypass: boolean = true;
    is_program_controller: boolean = true;
    custom_units: string = "";
    connection_optional: boolean = false;


    // Return the value of the closest scale_point.
    clampSelectValue(value: number): number{
        if (this.scale_points.length !== 0)
        {
            let minError = 1.0E100;
            let bestValue = value;
            for (let i = 0; i < this.scale_points.length; ++i)
            {
                let error = Math.abs(this.scale_points[i].value-value);
                if (error < minError)
                {
                    minError = error;
                    bestValue = this.scale_points[i].value;
                }
            }
            return bestValue;
        } else {
            return value;
        }
    }

    isHidden() : boolean {
        return this.not_on_gui || (this.connection_optional && !this.is_input);
    }
    isOnOffSwitch() : boolean {
        return this.controlType === ControlType.OnOffSwitch;
    }

    isAbToggle(): boolean {
        return this.controlType === ControlType.ABSwitch;
    }
    isSelect() : boolean {
        return this.controlType === ControlType.Select;
    }
    isOutputSelect() : boolean {
        return !this.is_input && this.controlType === ControlType.OutputSelect;
    }


    isLamp(): boolean {
        return this.toggled_property && this.scale_points.length === 0 && !this.is_input;
    }
    isDial() : boolean {
        return this.controlType === ControlType.Dial;
    }

    isTuner() : boolean {
        return this.controlType === ControlType.Tuner;
    }
    isVu() : boolean {
        return this.controlType === ControlType.Vu;
    }

    isDbVu() : boolean {
        return this.controlType === ControlType.DbVu;
    }

    valueToRange(value: number): number {
        if (this.toggled_property) return value === 0 ? 0: 1;

        if (this.integer_property || this.enumeration_property) {
            value = Math.round(value);
        }
        let range = (value - this.min_value) / (this.max_value - this.min_value);
        if (range > 1) range = 1;
        if (range < 0) range = 0;


        return range;
    }

    rangeToValue(range: number) : number {
        if (range < 0) range = 0;
        if (range > 1) range = 1;

        if (this.toggled_property) return range === 0? 0: 1;

        let value = range * (this.max_value - this.min_value) + this.min_value;
        if (this.integer_property || this.enumeration_property) {
            value = Math.round(value);
        }
        return value;
    }
    clampValue(value: number): number {
        return this.rangeToValue(this.valueToRange(value));
    }

    formatDisplayValue(value: number): string {
        if (this.integer_property) {
            value = Math.round(value);
        }

        for (let i = 0; i < this.scale_points.length; ++i)
        {
            let scalePoint = this.scale_points[i];
            if (scalePoint.value === value)
            {
                return scalePoint.label;
            }
        }
        let text = this.formatShortValue(value);

        switch (this.units) {
            case Units.bpm:
                text += "bpm";
                break;
            case Units.cent:
                text += "cents";
                break;
            case Units.cm:
                text += "cm";
                break;
            case Units.db:
                text += "dB";
                break;
            case Units.hz:
                text += "Hz";
                break;
            case Units.khz:
                text += "kHz";
                break;
            case Units.km:
                text += "km";
                break;
            case Units.m:
                text += "m";
                break;
            case Units.mhz:
                text += "MHz";
                break;
            case Units.min:
                text += "min";
                break;
            case Units.ms:
                text += "ms";
                break;
            case Units.pc:
                text += "%";
                break;
            case Units.s:
                text += "s";
                break;
            // Midinote: not handled.
            // semitone12TET not handled. 



        }
        return text;
    }
    formatShortValue(value: number): string
    {
        if (this.enumeration_property) {
            for (let i = 0; i < this.scale_points.length; ++i) {
                let scale_point = this.scale_points[i];
                if (scale_point.value === value) {
                    return scale_point.label;
                }
            }
            return "#invalid";
        } else if (this.integer_property) {
            return value.toFixed(0);
        } else {
            if (value >= 100 || value <= -100) {
                return value.toFixed(0);
            }
            if (value >= 10 || value <= -10) {
                return value.toFixed(1);
            }
            return value.toFixed(2);
        }
    }

    
}


export class UiPlugin implements Deserializable<UiPlugin> {
    deserialize(input: any): UiPlugin 
    {
        this.uri = input.uri;
        this.name = input.name;
        this.brand = input.brand ? input.brand: "";
        this.label = input.label? input.label: this.name;
        this.plugin_type = input.plugin_type as PluginType;
        this.plugin_display_type = input.plugin_display_type;
        this.author_name = input.author_name;
        this.author_homepage = input.author_homepage;
        this.audio_inputs = input.audio_inputs;
        this.audio_outputs = input.audio_outputs;
        this.has_midi_input = input.has_midi_input;
        this.has_midi_output = input.has_midi_output;
        this.description = input.description;
        this.controls = UiControl.deserialize_array(input.controls);
        this.port_groups = PortGroup.deserialize_array(input.port_groups);
        if (input.fileProperties)
        {
            this.fileProperties = UiFileProperty.deserialize_array(input.fileProperties)
        } else {
            this.fileProperties = [];
        }
        if (input.frequencyPlots)
        {
            this.frequencyPlots = UiFrequencyPlot.deserialize_array(input.frequencyPlots)
        } else {
            this.frequencyPlots = [];
        }

        this.is_vst3 = input.is_vst3;
        return this;

    }
    static deserialize_array(input: any): UiPlugin[] {
        let result: UiPlugin[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result[i] = new UiPlugin().deserialize(input[i]);
        }
        return result;
    }

    isSplit(): boolean {
        return this.uri === "uri://two-play/pipedal/pedalboard#Split";

    }
    getControl(key: string): UiControl | undefined {
        for (let i = 0; i < this.controls.length; ++i)
        {
            let control = this.controls[i];
            if (control.symbol === key)
            {
                return control;
            }
        } 
        return undefined;
    }
    
    getPortGroupByUri(uri: string) :PortGroup | null
    {
        for (let i = 0; i < this.port_groups.length; ++i)
        {
            let port_group = this.port_groups[i];
            if (port_group.uri === uri)
            {
                return port_group;
            }
        }
        return null;

    }
    getPortGroupBySymbol(symbol: string): PortGroup | null
    {
        for (let i = 0; i < this.port_groups.length; ++i)
        {
            let port_group = this.port_groups[i];
            if (port_group.symbol === symbol)
            {
                return port_group;
            }
        }
        return null;
    }

    uri:                 string = "";
    name:                string = "";
    brand:               string = "";
    label:               string = "";
    plugin_type:         PluginType = PluginType.InvalidPlugin;
    plugin_display_type: string = "";
    author_name:         string = "";
    author_homepage:     string = "";
    audio_inputs:        number = 0;
    audio_outputs:       number = 0;
    has_midi_input:      number = 0;
    has_midi_output:     number = 0;
    description: string  = "";
    controls:            UiControl[] = [];
    port_groups:         PortGroup[] = [];
    fileProperties:      UiFileProperty[] = [];
    frequencyPlots:      UiFrequencyPlot[] = [];
    is_vst3 :            boolean = false;
}



export function makeSplitUiPlugin(): UiPlugin
{

    return new UiPlugin().deserialize({
        uri:                  "uri://two-play/pipedal/pedalboard#Split",
        name:                 "Split",
        brand:                "",
        label:                "",
        plugin_type:         PluginType.SplitA,
        plugin_display_type: "Split",
        author_name:         "",
        author_homepage:     "",
        audio_inputs:        1,
        audio_outputs:       1,
        has_midi_input:      0,
        has_midi_output:     0,
        description: "",
        controls:       [
            new UiControl().applyProperties({
                symbol: "splitType",
                name:   "Type",
                index: 0,
                is_input: true,
                min_value: 0.0,
                max_value:  2.0,
                enumeration_property: true,
                scale_points: [
                    new ScalePoint().deserialize({value: 0, label: "A/B"}),
                    new ScalePoint().deserialize({value: 1, label: "mix"}),
                    new ScalePoint().deserialize({value: 1, label: "L/R"}),
                ],
                is_bypass: false,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
            
            }) ,
            new UiControl().applyProperties({
                symbol: "select",
                name:   "Select",
                index: 1,
                is_input: true,
                min_value: 0.0,
                max_value:  1.0,
                enumeration_property: true,
                scale_points: [
                    new ScalePoint().deserialize({value: 0, label: "A"}),
                    new ScalePoint().deserialize({value: 1, label: "B"}),
                ],
                is_bypass: false,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
            
            }) ,

            new UiControl().applyProperties({
                symbol: "mix",
                name:   "Mix",
                index: 2,
                is_input: true,
                min_value: -1.0,
                max_value:  1.0,
                is_bypass: false,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
            
            }) ,
            new UiControl().applyProperties({
                symbol: "panL",
                name:   "Pan Top",
                index: 3,
                is_input: true,
                min_value: -1.0,
                max_value:  1.0,
                is_bypass: false,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
            
            }) ,
            new UiControl().applyProperties({
                symbol: "volL",
                name:   "Vol Top",
                index: 4,
                is_input: true,
                min_value: -60.0,
                max_value:  12.0,
                is_bypass: false,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
            
            }) ,
            new UiControl().applyProperties({
                symbol: "panR",
                name:   "Pan Bottom",
                index: 5,
                is_input: true,
                min_value: -1.0,
                max_value:  1.0,
                is_bypass: false,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
            
            }) ,
            new UiControl().applyProperties({
                symbol: "volR",
                name:   "Vol Bottom",
                index: 6,
                is_input: true,
                min_value: -60.0,
                max_value:  12.0,
                is_bypass: false,
                is_program_controller: false,
                custom_units: "",
                connection_optional: false,
            }) 
        ],
        port_groups:    [],
        fileProperties: [],
        frequencyPlots: [],
        is_vst3 :       false,
    
    }
    );
}


