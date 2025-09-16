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
import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";
import { PluginType } from './Lv2Plugin';


import FxSimulatorIcon from './svg/fx_simulator.svg?react';
import FxPhaserIcon from './svg/fx_phaser.svg?react';
import FxFilterIcon from './svg/fx_filter.svg?react';
import FxDelayIcon from './svg/fx_delay.svg?react';

import FxNamIcon from './svg/fx_nam.svg?react';
import FxAmplifierIcon from './svg/fx_amplifier.svg?react';
import FxChorusIcon from './svg/fx_chorus.svg?react';
import FxModulatorIcon from './svg/fx_modulator.svg?react';
import FxReverbIcon from './svg/fx_reverb.svg?react';
import FxDistortionIcon from './svg/fx_distortion.svg?react';
import FxFlangerIcon from './svg/fx_flanger.svg?react';
import FxFilterHpIcon from './svg/fx_filter_hp.svg?react';
import FxParametricEqIcon from './svg/fx_parametric_eq.svg?react';
import FxEqIcon from './svg/fx_eq.svg?react';
import FxUtilityIcon from './svg/fx_utility.svg?react';
import FxCompressorIcon from './svg/fx_compressor.svg?react';
import FxLimiterIcon from './svg/fx_limiter.svg?react';
import FxConstantIcon from './svg/fx_constant.svg?react';
import FxFunctionIcon from './svg/fx_function.svg?react';
import FxGateIcon from './svg/fx_gate.svg?react';
import FxConverterIcon from './svg/fx_converter.svg?react';
import FxGeneratorIcon from './svg/fx_generator.svg?react';
import FxOscillatorIcon from './svg/fx_oscillator.svg?react';
import FxInstrumentIcon from './svg/fx_instrument.svg?react';
import FxMixerIcon from './svg/fx_mixer.svg?react';
import FxPitchIcon from './svg/fx_pitch.svg?react';
import FxSpatialIcon from './svg/fx_spatial.svg?react';
import FxSpectralIcon from './svg/fx_spectral.svg?react';
import FxAnalyzerIcon from './svg/fx_analyzer.svg?react';
import FxPluginIcon from './svg/fx_plugin.svg?react';
import SplitAIcon from './svg/fx_split_a.svg?react';
import SplitBIcon from './svg/fx_split_b.svg?react';
import SplitMixIcon from './svg/fx_dial.svg?react';
import SplitLrIcon from './svg/fx_lr.svg?react';
import FxEmptyIcon from './svg/fx_empty.svg?react';

import FxTerminalIcon from './svg/fx_terminal.svg?react';

import { isDarkMode } from './DarkMode';
import { PiPedalModel } from './PiPedalModel';

export interface IconColorSelect {
    key: string, 
    value: string | undefined,
};

export const getIconColorSelections = (darkMode?: boolean): IconColorSelect[] => {
    if (darkMode === undefined) darkMode = isDarkMode();
    let result: IconColorSelect[] = [];

    let l: number;
    let c: number;
    if (darkMode) {
        l = 0.8
        c = 0.25;
    } else {
        l = 0.5;
        c = 0.20;
    }

    for (let angle = 0; angle < 360; angle += 22.5) {
        if (angle === 270-22.5) 
        {
            // for some reason, this one is useless. Patch in a more useful light blue. :-/
            let a2 = 270;
            let l2: number;
            let c2: number;
            if (darkMode) {
                l2 = 0.9
                c2 = 0.25;
            } else {
                l2 = 0.7;
                c2 = 0.20;
            }
            result.push(
                {
                    key: "oklch" + Math.round(angle*10).toString(), 
                    value: ("oklch("  + l2.toString() + " " + c2.toString() + " " + (a2).toString() + ")")

                }
            );

        } else  if (angle === 270-45) 
        {
            // for some reason, this one is useless. Patch in a more useful light blue. :-/
            let a2 = 180;
            let l2: number;
            let c2: number;
            if (darkMode) {
                l2 = 0.95
                c2 = 0.25;
            } else {
                l2 = 0.75;
                c2 = 0.20;
            }
            result.push(
                {
                    key: "oklch" + Math.round(angle*10).toString(), 
                    value: ("oklch("  + l2.toString() + " " + c2.toString() + " " + (a2).toString() + ")")

                }
            );

        } else   {
            result.push(
                {
                    key: "oklch" + Math.round(angle*10).toString(), 
                    value: ("oklch("  + l.toString() + " " + c.toString() + " " + (angle).toString() + ")")

                }
            );
        }
    }
    return result;

};


function makeDefaultColorMap(): {[key:string]: string} {
    let result: {[key:string]: string} = {};
    for (let selection  of getIconColorSelections())
    {
        result[selection.key] = selection.value??"";
    }
    return result;
}
    
const iconColors: {[key:string]: string} = makeDefaultColorMap();

export const getIconColor = (key?: string): string| undefined => {
    if (iconColors.hasOwnProperty(key!))
    {
        return iconColors[key!];
    }
    return undefined;
}




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
    color?: string;
    offsetY?: number;
    pluginType: PluginType;
    pluginMissing?: boolean;
}

export function SelectSvgIcon(plugin_type: PluginType, className: string, size: number = 24, opacity: number = 1.0, missingPlugin: boolean = false, color = "") {
    if (missingPlugin) {
        color = isDarkMode() ? "#C00000" : "#B00000";
    }
    let myStyle = { width: size, height: size, opacity: opacity, color: color, fill: color };
    switch (plugin_type) {

        case PluginType.NamPlugin:
            return <FxNamIcon className={className} style={myStyle} />;
        case PluginType.PhaserPlugin:
            return <FxPhaserIcon className={className} style={myStyle} />;
        case PluginType.FilterPlugin:
        case PluginType.BandpassPlugin:
        case PluginType.AllpassPlugin:
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

        case PluginType.Plugin:
        default:
            return <FxPluginIcon className={className} style={myStyle} />;
    }
}
export function SelectBaseIcon(plugin_type: PluginType): string {
    switch (plugin_type) {
        case PluginType.NamPlugin:
            return "fx_nam.svg";
        case PluginType.PhaserPlugin:
            return "fx_phaser.svg";
        case PluginType.FilterPlugin:
        case PluginType.BandpassPlugin:
        case PluginType.AllpassPlugin:
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


export function SelectIconUri(plugin_type: PluginType) {
    let icon = SelectBaseIcon(plugin_type);
    return PiPedalModel.getInstance().svgImgUrl(icon);
}

const PluginIcon = withStyles((props: PluginIconProps) => {
    const { pluginType, opacity, pluginMissing,color } = props;
    const classes = withStyles.getClasses(props);

    let pluginMissing_: boolean = pluginMissing ?? false;

    let size: number = 24;
    if (props.size) size = props.size;
    let topVal: number = (props.offsetY ?? 0);

    let svgIcon = SelectSvgIcon(pluginType, classes.icon, size, opacity, pluginMissing_,color? color: "" );
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
},
    styles);

export default PluginIcon;