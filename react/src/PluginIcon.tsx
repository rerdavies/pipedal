// Copyright (c) 2021 Robin Davies
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

import { createStyles, Theme, withStyles, WithStyles } from '@material-ui/core/styles';
import { PiPedalModelFactory } from "./PiPedalModel";
import { PluginType } from './Lv2Plugin';


const styles = (theme: Theme) =>
    createStyles({
        icon: {
            width: "24px",
            height: "24px",
            margin: "0px",
            opacity: "0.6"
        },
    });

export interface PluginIconProps extends WithStyles<typeof styles> {
    size?: number;
    offsetY?: number;
    pluginType: PluginType;
    pluginUri: string;
}
export function SelectBaseIcon(plugin_type: PluginType): string {
    switch (plugin_type) {
        case PluginType.PhaserPlugin:
            return "fx_phaser.svg";
        case PluginType.FilterPlugin:
            return "fx_filter.svg";
        case PluginType.DelayPlugin:
            return "fx_delay.svg";
        case PluginType.SimulatorPlugin:
            return "fx_simulator.svg";
        case PluginType.AmplifierPlugin:
            return "fx_amplifier.svg";
        case PluginType.ChorusPlugin:
            return "fx_chorus.svg";
        case PluginType.ModulatorPlugin:
        case PluginType.CombPlugin:
            return "fx_modulator.svg";
        case PluginType.ReverbPlugin:
            return "fx_reverb.svg";
        case PluginType.DistortionPlugin:
            return "fx_distortion.svg";
        case PluginType.FlangerPlugin:
            return "fx_flanger.svg";
        case PluginType.HighpassPlugin:
            return "fx_highpass.svg";
        case PluginType.ParaEQPlugin:
        case PluginType.LowpassPlugin:
        case PluginType.HighpassPlugin:            
        case PluginType.MultiEQPlugin:
        case PluginType.EQPlugin:
            return "fx_eq.svg";
        case PluginType.UtilityPlugin:
            return "fx_utility.svg";
        case PluginType.CompressorPlugin:
        case PluginType.DynamicsPlugin:
        case PluginType.ExpanderPlugin:
            return "fx_compressor.svg";
        case PluginType.LimiterPlugin:
            return "fx_limiter.svg";
        case PluginType.ConstantPlugin:
            return "fx_constant.svg";
        case PluginType.FunctionPlugin:
        case PluginType.WaveshaperPlugin:
            return "fx_function.svg";
        case PluginType.GatePlugin:
            return "fx_gate.svg";
        case PluginType.ConverterPlugin:
            return "fx_converter.svg"
        case PluginType.GeneratorPlugin:
            return "fx_generator.svg";
        case PluginType.OscillatorPlugin:
            return "fx_oscillator.svg";
        case PluginType.InstrumentPlugin:
            return "fx_instrument.svg";
        case PluginType.MixerPlugin:
            return "fx_mixer.svg";
        case PluginType.PitchPlugin:
            return "fx_pitch.svg";
        case PluginType.SpatialPlugin:
            return "fx_spatial.svg";
        case PluginType.SpectralPlugin:
            return "fx_spectral.svg";
        case PluginType.AnalyserPlugin:
            return "fx_analyzer.svg";
        case PluginType.Plugin:
        default:
            return "fx_plugin.svg";
    }
}

var colors: string[] = [
    "#14003d",
    "#630651",
    "#005010",
    "#281431",
    "#5E0012",
    "#401222",
    "#3B1502"
];

// eslint-disable-next-line @typescript-eslint/no-unused-vars
function colorFromHash(uri: string) {
    let hash = hashString(uri);

    return colors[hash % colors.length];

}

function hashString(str: string): number {
    var hash = 0, i, chr;
    for (i = 0; i < str.length; i++) {
        chr = str.charCodeAt(i);
        hash = ((hash << 5) - hash) + chr;
        hash |= 0; // Convert to 32bit integer
    }
    if (hash < 0) return -hash;
    return hash;
}

export function SelectIcon(plugin_type: PluginType, itemUri: string) {
    let icon = SelectBaseIcon(plugin_type);
    return PiPedalModelFactory.getInstance().svgImgUrl(icon);
}

const PluginIcon = withStyles(styles)((props: PluginIconProps) => {
    const { classes, pluginUri, pluginType } = props;

    let size: number = 24;
    if (props.size) size = props.size;
    let topVal: number = (props.offsetY ?? 0);

    return (
        <img src={SelectIcon(pluginType, pluginUri)} className={classes.icon} style={{ width: size, height: size, color: "#F88", position: "relative", top: topVal }} alt={pluginType as string}
        />);

    // let maskText = "url(" + SelectIcon(pluginType, pluginUri) + ") no-repeat center";
    // let color=colorFromHash(pluginUri);
    // return (
    //     <div style={{ display: "block", opacity: 0.8, width: size, height: size, backgroundColor: color, 
    //         mask: maskText, WebkitMask: maskText, WebkitMaskSize: size + "px", maskSize: size + "px" }}
    //     >&nbsp;</div>
    // );
});

export default PluginIcon;