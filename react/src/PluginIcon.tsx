/* eslint-disable @typescript-eslint/no-unused-vars */
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

import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import { PiPedalModelFactory } from "./PiPedalModel";
import { PluginType } from './Lv2Plugin';


import { ReactComponent as FxSimulatorIcon } from './svg/fx_simulator.svg';
import { ReactComponent as FxPhaserIcon } from './svg/fx_phaser.svg';
import { ReactComponent as FxFilterIcon } from './svg/fx_filter.svg';
import { ReactComponent as FxDelayIcon } from './svg/fx_delay.svg';

import { ReactComponent as FxAmplifierIcon } from './svg/fx_amplifier.svg';
import { ReactComponent as FxChorusIcon } from './svg/fx_chorus.svg';
import { ReactComponent as FxModulatorIcon } from './svg/fx_modulator.svg';
import { ReactComponent as FxReverbIcon } from './svg/fx_reverb.svg';
import { ReactComponent as FxDistortionIcon } from './svg/fx_distortion.svg';
import { ReactComponent as FxFlangerIcon } from './svg/fx_flanger.svg';
import { ReactComponent as FxFilterHpIcon } from './svg/fx_filter_hp.svg';
import { ReactComponent as FxParametricEqIcon } from './svg/fx_parametric_eq.svg';
import { ReactComponent as FxEqIcon } from './svg/fx_eq.svg';
import { ReactComponent as FxUtilityIcon } from './svg/fx_utility.svg';
import { ReactComponent as FxCompressorIcon } from './svg/fx_compressor.svg';
import { ReactComponent as FxLimiterIcon } from './svg/fx_limiter.svg';
import { ReactComponent as FxConstantIcon } from './svg/fx_constant.svg';
import { ReactComponent as FxFunctionIcon } from './svg/fx_function.svg';
import { ReactComponent as FxGateIcon } from './svg/fx_gate.svg';
import { ReactComponent as FxConverterIcon } from './svg/fx_converter.svg';
import { ReactComponent as FxGeneratorIcon } from './svg/fx_generator.svg';
import { ReactComponent as FxOscillatorIcon } from './svg/fx_oscillator.svg';
import { ReactComponent as FxInstrumentIcon } from './svg/fx_instrument.svg';
import { ReactComponent as FxMixerIcon } from './svg/fx_mixer.svg';
import { ReactComponent as FxPitchIcon } from './svg/fx_pitch.svg';
import { ReactComponent as FxSpatialIcon } from './svg/fx_spatial.svg';
import { ReactComponent as FxSpectralIcon } from './svg/fx_spectral.svg';
import { ReactComponent as FxAnalyzerIcon } from './svg/fx_analyzer.svg';
import { ReactComponent as FxPluginIcon } from './svg/fx_plugin.svg';
import { ReactComponent as SplitAIcon } from './svg/fx_split_a.svg';
import { ReactComponent as SplitBIcon } from './svg/fx_split_b.svg';
import { ReactComponent as SplitMixIcon } from './svg/fx_dial.svg';
import { ReactComponent as SplitLrIcon } from './svg/fx_lr.svg';
import { ReactComponent as FxEmptyIcon } from './svg/fx_empty.svg';

import { ReactComponent as FxTerminalIcon } from './svg/fx_terminal.svg';

import {isDarkMode} from './DarkMode';
import { Color } from '@mui/material';



const styles = (theme: Theme) =>
    createStyles({
        icon: {
            width: "24px",
            height: "24px",
            margin: "0px",
            opacity: "0.6",
            color: theme.palette.text.primary,
            fill: theme.palette.text.primary
        },

    });

export interface PluginIconProps extends WithStyles<typeof styles> {
    size?: number;
    opacity?: number;
    offsetY?: number;
    pluginType: PluginType;
    pluginMissing?: boolean;
}

export function SelectSvgIcon(plugin_type: PluginType, className: string, size: number = 24, opacity: number = 1.0, missingPlugin: boolean = false) {
    let color = "";
    if (missingPlugin)
    {
        color = isDarkMode()? "#C00000": "#B00000";
    }
    let myStyle = { width: size, height: size, opacity: opacity, color: color, fill: color };
    switch (plugin_type) {
        case PluginType.PhaserPlugin:
            return <FxPhaserIcon className={className} style={myStyle} />;
        case PluginType.FilterPlugin:
            return <FxFilterIcon className={className} style={myStyle} />;
        case PluginType.DelayPlugin:
            return <FxDelayIcon className={className} style={myStyle} />;
        case PluginType.SimulatorPlugin:
            return <FxSimulatorIcon className={className} style={myStyle} />;
        case PluginType.AmplifierPlugin:
            return <FxAmplifierIcon className={className} style={myStyle} />;
        case PluginType.ChorusPlugin:
            return <FxChorusIcon className={className} style={myStyle} />;
        case PluginType.ModulatorPlugin:
        case PluginType.CombPlugin:
            return <FxModulatorIcon className={className} style={myStyle} />;
        case PluginType.ReverbPlugin:
            return <FxReverbIcon className={className} style={myStyle} />;
        case PluginType.DistortionPlugin:
            return <FxDistortionIcon className={className} style={myStyle} />;
        case PluginType.FlangerPlugin:
            return <FxFlangerIcon className={className} style={myStyle} />;
        case PluginType.LowpassPlugin:
            return <FxFilterIcon className={className} style={myStyle} />;
        case PluginType.HighpassPlugin:
            return <FxFilterHpIcon className={className} style={myStyle} />;
        case PluginType.ParaEQPlugin:
            return <FxParametricEqIcon className={className} style={myStyle} />;
        case PluginType.MultiEQPlugin:
        case PluginType.EQPlugin:
            return <FxEqIcon className={className} style={myStyle} />;
        case PluginType.UtilityPlugin:
            return <FxUtilityIcon className={className} style={myStyle} />;
        case PluginType.CompressorPlugin:
        case PluginType.DynamicsPlugin:
        case PluginType.ExpanderPlugin:
            return <FxCompressorIcon className={className} style={myStyle} />;

        case PluginType.LimiterPlugin:
            return <FxLimiterIcon className={className} style={myStyle} />;
        case PluginType.ConstantPlugin:
            return <FxConstantIcon className={className} style={myStyle} />;
        case PluginType.FunctionPlugin:
        case PluginType.WaveshaperPlugin:
            return <FxFunctionIcon className={className} style={myStyle} />;
        case PluginType.GatePlugin:
            return <FxGateIcon className={className} style={myStyle} />;
        case PluginType.ConverterPlugin:
            return <FxConverterIcon className={className} style={myStyle} />;
        case PluginType.GeneratorPlugin:
            return <FxGeneratorIcon className={className} style={myStyle} />;
        case PluginType.OscillatorPlugin:
            return <FxOscillatorIcon className={className} style={myStyle} />;
        case PluginType.InstrumentPlugin:
            return <FxInstrumentIcon className={className} style={myStyle} />;
        case PluginType.MixerPlugin:
            return <FxMixerIcon className={className} style={myStyle} />;
        case PluginType.PitchPlugin:
            return <FxPitchIcon className={className} style={myStyle} />;
        case PluginType.SpatialPlugin:
            return <FxSpatialIcon className={className} style={myStyle} />;
        case PluginType.SpectralPlugin:
            return <FxSpectralIcon className={className} style={myStyle} />;
        case PluginType.AnalyserPlugin:
            return <FxAnalyzerIcon className={className} style={myStyle} />;
        case PluginType.SplitA:
            return <SplitAIcon className={className} style={myStyle} />;
        case PluginType.SplitB:
            return <SplitBIcon className={className} style={myStyle} />;
        case PluginType.SplitMix:
            return <SplitMixIcon className={className} style={myStyle} />;
        case PluginType.SplitLR:
            return <SplitLrIcon className={className} style={myStyle} />;

        case PluginType.None:
            return <FxEmptyIcon className={className} style={myStyle} />;
        case PluginType.Terminal:
            return <FxTerminalIcon className={className} style={myStyle} />;

            default:
                case PluginType.Plugin:
            return <FxPluginIcon className={className} style={myStyle} />;
    }
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
        case PluginType.LowpassPlugin:
            return "fx_filter.svg";
        case PluginType.HighpassPlugin:
            return "fx_filter_hp.svg";
        case PluginType.ParaEQPlugin:
            return "fx_parametric_eq.svg";
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

export function SelectIconUri(plugin_type: PluginType) {
    let icon = SelectBaseIcon(plugin_type);
    return PiPedalModelFactory.getInstance().svgImgUrl(icon);
}

const PluginIcon = withStyles(styles)((props: PluginIconProps) => {
    const { classes, pluginType, opacity,pluginMissing } = props;

    let pluginMissing_ : boolean = pluginMissing??false;

    let size: number = 24;
    if (props.size) size = props.size;
    let topVal: number = (props.offsetY ?? 0);

    let svgIcon = SelectSvgIcon(pluginType, classes.icon, size, opacity,pluginMissing_);
    if (svgIcon) {
        return svgIcon;
    }

    // colored icons that don't have dark/light variants.
    return (
        <img src={SelectIconUri(pluginType)} className={classes.icon} style={{ width: size, height: size, position: "relative", top: topVal, opacity: " " }} alt=''
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