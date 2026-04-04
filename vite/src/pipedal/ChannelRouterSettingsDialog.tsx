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

import React from 'react';
import { useState } from 'react';
import VuMeter from './VuMeter';
import HelpOutlineIcon from '@mui/icons-material/HelpOutline';
import Divider from '@mui/material/Divider';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButtonEx from './IconButtonEx';
import useWindowSize from "./UseWindowSize";
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import ChannelRouterSettingsHelpDialog from './ChannelRouterSettingsHelpDialog';
import SpeakerIcon from '@mui/icons-material/Speaker';
import MicIcon from '@mui/icons-material/Mic';
import ChannelRouterSettings from './ChannelRouterSettings';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import Typography from '@mui/material/Typography';
import { Pedalboard  } from './Pedalboard';

import DialogEx from './DialogEx';

import  { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { JackConfiguration } from './Jack';


let debugInputChannels: number | null = null;
let debugOutputChannels: number | null = null;


export interface ChannelRouterSettingsDialogProps {
    open: boolean;
    onClose: () => void;
}

enum RouteType {
    Main,
    Aux,
}

interface ChannelRouterSettingsPreset {
    id: number;
    name: string;
    settings: ChannelRouterSettings;
}


let DEFAULT_MONO_TO_MONO_ROUTING_PRESET = -2; // corresponds to Input->Output.
let DEFAULT_LEFT_TO_STEREO_ROUTING_PRESET = -3; // corresponds to Input->Stereo out.
let DEFAULT_RIGHT_TO_STEREO_ROUTING_PRESET = -4; // corresponds to Right->Stereo out.

let factoryRouterPresets: ChannelRouterSettings[] = [
    // Input->Output
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -2,
        mainInputChannels: [0, -1],
        mainOutputChannels: [0, -1],
        auxInputChannels: [-1, -1],
        auxOutputChannels: [-1, -1],
    })
    ,
    // Input->Stereo out.
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -3,
        mainInputChannels: [0, -1],
        mainOutputChannels: [0, 1],
        auxInputChannels: [-1, -1],
        auxOutputChannels: [-1, -1],
    })
    ,
    // Right->Right
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -4,
        mainInputChannels: [1,-1],
        mainOutputChannels: [1, -1],
        auxInputChannels: [-1, -1],
        auxOutputChannels: [-1, -1],
    })
    ,
    // Right->Stereo out.
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -5,
        mainInputChannels: [1,-1],
        mainOutputChannels: [0, 1],
        auxInputChannels: [-1, -1],
        auxOutputChannels: [-1, -1],
    })
    ,
    // Right->right, aux: Left->Left    .
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -6,
        mainInputChannels: [1,-1],
        mainOutputChannels: [1, -1],
        auxInputChannels: [0,-1],
        auxOutputChannels: [0,-1],
    })
    ,
    // Right-Left, re-amp->Right.
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -7,
        mainInputChannels: [1,-1],
        mainOutputChannels: [0, -1],
        auxInputChannels: [1,-1],
        auxOutputChannels: [1,-1],
    })
    ,
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -8,
        mainInputChannels: [0, -1],
        mainOutputChannels: [0, -1],
        auxInputChannels: [1,-1],
        auxOutputChannels: [1,-1],
    })
    ,
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -9,
        mainInputChannels: [1, -1],
        mainOutputChannels: [0, 1],
        auxInputChannels: [2, 3],
        auxOutputChannels: [0, 1],
    })
    ,
    new ChannelRouterSettings().deserialize({
        configured: true,
        channelRouterPresetId: -10,
        mainInputChannels: [1, -1],
        mainOutputChannels: [0, 1],
        auxInputChannels: [2, 3],
        auxOutputChannels: [2, 3],
    })
    ,
];

let cellPortraitInOutDiv: React.CSSProperties = {
    display: "flex", columnGap: 4, flexDirection: "row", alignItems: "center", justifyContent: "right",
};
let cellPortraitSectionHead: React.CSSProperties = {
    border: "0px",
    paddingLeft: 0,
    paddingRight: 12,
    width: 50,
    margin: "0px",
    textAlign: "left",
    verticalAlign: "middle",
};

let cellPortraitControlStrip: React.CSSProperties = {
    paddingTop: 0,
    paddingBottom: 0,
    paddingLeft: 0,
    paddingRight: 0,
    margin: "0px",
    textAlign: "left",
    verticalAlign: "middle"
};
let cellLeft: React.CSSProperties = {
    border: "0px",
    paddingTop: 4,
    paddingBottom: 4,
    paddingLeft: 4,
    paddingRight: 12,
    margin: "0px",
    textAlign: "left",
    whiteSpace: "nowrap",
    verticalAlign: "middle"
};
let cellPortraitLeft: React.CSSProperties = {
    border: "0px",
    paddingTop: 0,
    paddingBottom: 0,
    paddingLeft: 4,
    paddingRight: 12,
    margin: "0px",
    textAlign: "left",
    whiteSpace: "nowrap",
    verticalAlign: "middle"
};
let cellLandscapeTitle: React.CSSProperties = {
    border: "0px",
    paddingTop: 4,
    paddingBottom: 4,
    paddingLeft: 4,
    paddingRight: 20,
    margin: "0px",
    textAlign: "left",
    verticalAlign: "middle"
};
let cellPortraitInOut: React.CSSProperties = {
    paddingTop: 0,
    paddingBottom: 0,
    paddingLeft: 24,
    paddingRight: 12,
    margin: "0px",
    textAlign: "right",
    verticalAlign: "middle"
};


function normalizeChannelSelection(channelSelection: number[]) {
    if (channelSelection.length === 2) {
        if (channelSelection[0] === -1) {
            channelSelection[0] = channelSelection[1];
            channelSelection[1] = -1;
        } else {
            if (channelSelection[0] === channelSelection[1]) {
                channelSelection[1] = -1;
            }
        }
    }
}
function makeRoutingPresets(audioConfig: JackConfiguration): ChannelRouterSettingsPreset[] {
    let result: ChannelRouterSettingsPreset[] = [];

    for (let preset of factoryRouterPresets) {
        if (preset.isValid(audioConfig)) {
            normalizeChannelSelection(preset.mainInputChannels);
            normalizeChannelSelection(preset.mainOutputChannels);
            normalizeChannelSelection(preset.auxInputChannels);
            normalizeChannelSelection(preset.auxOutputChannels);
            let newPreset = {
                id: preset.channelRouterPresetId,
                name: preset.getDescription(audioConfig),
                settings: preset
            };
            result.push(newPreset);
        }
    }


    return result;
}

function MakeChannelMenu(channelCount: number, routeType: RouteType, input: boolean, disallowedSelections: number[]): React.ReactElement[] {
    let items: React.ReactElement[] = [(<MenuItem key={-1} value={-1}>None</MenuItem>)]
    if (channelCount === 0) {
        return items;
    }
    if (channelCount === 1) {
        if (input) {
            items.push((<MenuItem key={0} value={0} disabled={disallowedSelections.includes(0)}>Input</MenuItem>));
        } else {
            items.push((<MenuItem key={0} value={0} disabled={disallowedSelections.includes(0)}>Output</MenuItem>));
        }
    } else if (channelCount === 2) {
        items.push((<MenuItem key={0} value={0} disabled={disallowedSelections.includes(0)}>Left</MenuItem>));
        items.push((<MenuItem key={1} value={1} disabled={disallowedSelections.includes(1)}>Right</MenuItem>));
        return items;
    } else {
        for (let i = 0; i < channelCount; ++i) {
            items.push((<MenuItem key={i} value={i} disabled={disallowedSelections.includes(i)}>Ch {i + 1}</MenuItem>));
        }
    }
    return items;
}


function makeDebugConfig(baseConfig: JackConfiguration): JackConfiguration {
    let config = new JackConfiguration().deserialize(baseConfig); // clone.
    for (let i = config.inputAudioPorts.length; i < (debugInputChannels ?? 0); ++i) {
        config.inputAudioPorts.push("ch" + (i + 1));
    }
    if (config.inputAudioPorts.length > (debugInputChannels ?? 0)) {
        config.inputAudioPorts = config.inputAudioPorts.slice(0, debugInputChannels ?? 0);
    }
    for (let i = config.outputAudioPorts.length; i < (debugOutputChannels ?? 0); ++i) {
        config.outputAudioPorts.push("ch" + (i + 1));
    }
    if (config.outputAudioPorts.length > (debugOutputChannels ?? 0)) {
        config.outputAudioPorts = config.outputAudioPorts.slice(0, debugOutputChannels ?? 0);
    }
    return config;
}

function getChannelRouterSettingsPresetById(presetId: number): ChannelRouterSettings {
    for (let preset of factoryRouterPresets) {
        if (preset.channelRouterPresetId === presetId) {
         return preset.clone();
        }
    }
    throw new Error("Error setting defaults. Preset not found: " + presetId);   
}

function GetDefaultChannelRouterSettings(model: PiPedalModel) : ChannelRouterSettings {
    let currentSettings = model.channelRouterSettings.get();
    if (currentSettings.configured) {
        return currentSettings;
    }
    // we're onboarding the user, so set an appropriate default channel routing preset based on the number of ports on the audio interface.

    let jackSettings = model.jackConfiguration.get();
    let numInputs = jackSettings.inputAudioPorts.length;
    let numOutputs = jackSettings.outputAudioPorts.length;
    if (numInputs < 2 || numOutputs < 2) {
        currentSettings = getChannelRouterSettingsPresetById(DEFAULT_MONO_TO_MONO_ROUTING_PRESET);
    } else if (numInputs === 2 && numOutputs === 2) {
        currentSettings =  getChannelRouterSettingsPresetById(DEFAULT_RIGHT_TO_STEREO_ROUTING_PRESET);
    } else {
        currentSettings = getChannelRouterSettingsPresetById(DEFAULT_LEFT_TO_STEREO_ROUTING_PRESET);
    }
    model.setChannelRouterSettings(currentSettings);
    return model.channelRouterSettings.get();
}

function ChannelRouterSettingsDialog(props: ChannelRouterSettingsDialogProps) {
    //const classes = useStyles();
    const { open, onClose } = props;
    let model = PiPedalModelFactory.getInstance();
    let config = model.jackConfiguration.get();
    if (debugInputChannels != null || debugOutputChannels != null) {
        config = makeDebugConfig(config);
    }
    let channelRouterPresets = makeRoutingPresets(config);

    const [settings, setSettings] = useState<ChannelRouterSettings>(GetDefaultChannelRouterSettings(model));
    const [showHelp, setShowHelp] = useState<boolean>(false);


    React.useEffect(() => {
        if (open) {
            let handleSettingsChanged = () => {
                setSettings(model.channelRouterSettings.get());
            };
            model.channelRouterSettings.addOnChangedHandler(handleSettingsChanged);
            return () => {
                model.channelRouterSettings.removeOnChangedHandler(handleSettingsChanged);
            }
        } else {
            return () => { };
        }
    }, [open]);

    const handleClose = (): void => {
        onClose();
    };
    let [windowSize] = useWindowSize();
    let landscape = windowSize.height < 600;
    let fullScreen = windowSize.width < 450 || windowSize.height < 600;

    let ChannelSelect = (routeType: RouteType, channelIndex: number, input: boolean, icon: boolean = true) => {
        let channelCount = input ? config.inputAudioPorts.length : config.outputAudioPorts.length;
        if (input && debugInputChannels != null) {
            channelCount = debugInputChannels;
        }
        if (!input && debugOutputChannels != null) {
            channelCount = debugOutputChannels;
        }

        let value: number;

        let disallowedSelections: number[] = [];

        let channelSelection: number[] = [];
        switch (routeType) {
            case RouteType.Main:
                channelSelection = input ? settings.mainInputChannels : settings.mainOutputChannels;
                break;
            case RouteType.Aux:
                channelSelection = input ? settings.auxInputChannels : settings.auxOutputChannels;
                break;
        }
        value = channelSelection[channelIndex];

        if (value >= channelCount) {
            value = -1;
        }
        if (channelIndex === 1) {
            if (channelSelection[0] !== -1) {
                disallowedSelections.push(channelSelection[0]);
            }
        }
        let error = false;
        if (routeType === RouteType.Main && channelSelection[0] === -1 && channelSelection[1] === -1) {
            error = true;
        }
        return (
            <Select variant="standard" style={{ width: icon ? 130 : 110 }} value={value}
                startAdornment={
                    icon ? (
                        input ? (<MicIcon style={{ opacity: 0.6, width: 16, height: 16, marginRight: 4 }} />) :
                            (<SpeakerIcon style={{ opacity: 0.6, width: 16, height: 16, marginRight: 4 }} />)
                    ) : undefined
                }
                error={error}
                onChange={(event) => {
                    let newValue = event.target.value as number;
                    let newSettings = new ChannelRouterSettings().deserialize(settings);
                    switch (routeType) {
                        case RouteType.Main:
                            if (input) {
                                newSettings.mainInputChannels[channelIndex] = newValue;
                                normalizeChannelSelection(newSettings.mainInputChannels);
                            } else {
                                newSettings.mainOutputChannels[channelIndex] = newValue;
                                normalizeChannelSelection(newSettings.mainOutputChannels);
                            }
                            break;
                        case RouteType.Aux:
                            if (input) {
                                newSettings.auxInputChannels[channelIndex] = newValue;
                                normalizeChannelSelection(newSettings.auxInputChannels);
                            } else {
                                newSettings.auxOutputChannels[channelIndex] = newValue;
                                normalizeChannelSelection(newSettings.auxOutputChannels);
                            }
                            break;
                    }
                    if  (newSettings.channelRouterPresetId >= 0) {
                        newSettings.modified = true; // custom.
                    } else {
                        newSettings.modified = true;
                        newSettings.channelRouterPresetId = -1; // custom.
                    }
                    newSettings.configured = true;
                    model.setChannelRouterSettings(newSettings);
                }}
            >
                {MakeChannelMenu(channelCount, routeType, input, disallowedSelections)}
            </Select>
        )
    }
    let Vu = (routeType: RouteType, input: boolean) => {
        let instanceId: number;
        switch (routeType) {
            case RouteType.Main:
                instanceId = input ? Pedalboard.START_CONTROL_ID : Pedalboard.END_CONTROL_ID;
                break;
            case RouteType.Aux:
                instanceId = input ? Pedalboard.AUX_START_CONTROL_ID : Pedalboard.AUX_END_CONTROL_ID;
                break;

        }
        return (
                <VuMeter instanceId={instanceId} 
                    display={input? "input": "output"} 
                    height={64} displayText={true} /> 
        );    
    }

    let ApplyPresetId = (presetId: number | string | null | undefined) => {
        if (typeof presetId !== 'number') {
            return;
        }
        for (let preset of channelRouterPresets) {
            if (preset.id === presetId) {
                let newPreset = Object.assign(new ChannelRouterSettings(), preset.settings);
                model.setChannelRouterSettings(newPreset);
                return;
            }
        }
    }

    let CanUsePreset = (preset: ChannelRouterSettingsPreset): boolean => {
        let inputChannels = config.inputAudioPorts.length;
        let outputChannels = config.outputAudioPorts.length;
        return (
            (preset.settings.mainInputChannels.every((ch) => ch < inputChannels || ch === -1)) &&
            (preset.settings.mainOutputChannels.every((ch) => ch < outputChannels || ch === -1)) &&
            (preset.settings.auxInputChannels.every((ch) => ch < inputChannels || ch === -1)) &&
            (preset.settings.auxOutputChannels.every((ch) => ch < outputChannels || ch === -1))
        );
    }

    let GeneratePresetMenuItems = (): React.ReactElement[] => {
        let items: React.ReactElement[] = [];

        for (let preset of channelRouterPresets) {
            if (CanUsePreset(preset))
                items.push(
                    <MenuItem key={preset.id} value={preset.id}>
                        {preset.name}
                    </MenuItem>
                );
        }
        if (settings.channelRouterPresetId === -1) {
            items.push(
                (<MenuItem key={-1} value={-1} disabled={true}>
                    <span style={{ opacity: 0.6 }}>(Custom)</span></MenuItem>)
            )
        }

        return items;
    }

    let PresetSelect = (width: number | string | undefined) => {
        return (
            <div style={{ display: "flex", flexDirection: "row", alignItems: "center", width: width }}>
                <div style={{ flex: "1 1", display: "relative" }}>
                    <Select variant="standard" style={{ width: "100%" }} value={settings.channelRouterPresetId}
                        onChange={(event) => {
                            ApplyPresetId(event.target.value);
                        }}
                    >
                        {GeneratePresetMenuItems()}
                    </Select>
                </div>
            </div>
        )
    }


    let LandscapeView = () => {
        return (
            <div style={{
                display: "flex", flexFlow: "row", alignItems: "center", justifyContent: "center",
                width: "100%", height: "100%", flexGrow: 1, fontSize: "14px"
            }} >
                <table style={{ borderCollapse: "collapse" }}>
                    <colgroup>
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                    </colgroup>
                    <tbody>
                        {/* Main */}
                        <tr>
                            <td rowSpan={1} style={cellLandscapeTitle}>
                                <Typography variant="body2" color="textSecondary">Main</Typography>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 0, true)}
                            </td>
                            <td rowSpan={3} style={cellLeft}>{Vu(RouteType.Main, true)}</td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 0, false)}
                            </td>
                            <td rowSpan={3} style={cellLeft}>{Vu(RouteType.Main, false)}</td>
                        </tr>
                        <tr>
                            <td></td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Main, 1, true))}
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Main, 1, false))}
                            </td>
                        </tr>

                        {/* Spacer ---------------- */}
                        <tr><td colSpan={9} style={{ height: 16 }}></td></tr>

                        {/* Aux ----------------------*/}
                        <tr>
                            <td rowSpan={1} style={cellLandscapeTitle}>
                                <Typography variant="body2" color="textSecondary">Aux</Typography>
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 0, true))}
                            </td>
                            <td rowSpan={3} style={cellLeft}>{Vu(RouteType.Aux, true)}</td>

                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 0, false))}
                            </td>
                            <td rowSpan={3} style={cellLeft}>{Vu(RouteType.Aux, false)}</td>
                        </tr>
                        <tr>
                            <td></td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 1, true))}
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 1, false))}
                            </td>
                        </tr>
                        <tr><td colSpan={9} style={{ height: 2 }}></td></tr>


                    </tbody>
                </table>
            </div>
        );
    };
    let PortraitView = () => {
        return (
            <div style={{
                display: "flex", flexFlow: "row", alignItems: "center", justifyContent: "center",
                width: "100%", flexGrow: 0, fontSize: "14px"
            }} >
                <table style={{ borderCollapse: "collapse", borderSpacing: 0 }}>
                    <colgroup>
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                    </colgroup>
                    <tbody>
                        {/* Main */}
                        <tr>
                            <td colSpan={3} style={cellPortraitSectionHead}>
                                <Typography variant="body2" color="textSecondary"  >Main</Typography>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut} rowSpan={1}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >In</Typography>
                                    <MicIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellPortraitLeft}>
                                {ChannelSelect(RouteType.Main, 0, true, false)}
                            </td>
                            <td style={cellPortraitControlStrip} rowSpan={3}>
                                {Vu(RouteType.Main,true)}
                            </td>
                            <td></td>
                        </tr>
                        <tr>
                            <td></td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Main, 1, true, false))}
                            </td>
                        </tr>
                        {/* Spacer */}
                        <tr>
                            <td colSpan={3} style={{ height: 16 }}></td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut} rowSpan={1}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >Out</Typography>
                                    <SpeakerIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellLeft}>

                                {ChannelSelect(RouteType.Main, 0, false, false)}
                            </td>
                            <td style={cellPortraitControlStrip} rowSpan={3}>
                                {Vu(RouteType.Main,false)}</td>
                            <td></td>
                        </tr>
                        <tr>
                            <td style={cellLeft}>
                                </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 1, false, false)}
                            </td>
                        </tr>

                        {/* Spacer ---------------- */}
                        <tr><td colSpan={3} style={{ height: 16 }}></td></tr>
                        {/* Aux */}
                        <tr>
                            <td colSpan={3} style={cellPortraitSectionHead}>
                                <Typography variant="body2" color="textSecondary"  >Aux</Typography>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut}>
                                <div style={cellPortraitInOutDiv} >
                                    <Typography variant="body2" color="textSecondary" >In</Typography>
                                    <MicIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 0, true, false))}
                            </td>
                             <td style={cellPortraitControlStrip} rowSpan={3}>{Vu(RouteType.Aux, true)}</td>
                        </tr>
                        <tr>
                            <td></td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 1, true, false))}
                            </td>
                        </tr>
                        {/* Spacer */}
                        <tr>
                            <td colSpan={3} style={{ height: 16 }}></td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut} rowSpan={1}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >Out</Typography>
                                    <SpeakerIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Aux, 0, false, false)}
                            </td>
                            <td style={cellPortraitControlStrip} rowSpan={3}>{Vu(RouteType.Aux, false)}</td>
                        </tr>
                        <tr>
                            <td></td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Aux, 1, false, false)}
                            </td>
                        </tr>

                    </tbody>
                </table>
            </div>
        );
    };

    return (
        <DialogEx tag="channelRouterSettings"
            onClose={handleClose}
            aria-labelledby="select-channel_mixer_settings"
            open={open}
            fullWidth
            maxWidth={landscape ? "sm" : "xs"}
            onEnterKey={handleClose}
            fullScreen={fullScreen}
        >
            <DialogTitle id="select-channel_mixer_settings" style={{ paddingTop: 0, paddingBottom: 0, marginBottom: 0 }}>
                <div style={{
                    display: "flex", flexFlow: "row", flexWrap: "nowrap", alignItems: "center",
                    marginTop: 8

                }}>
                    <IconButtonEx
                        tooltip="Close"
                        edge="start"
                        color="inherit"
                        aria-label="cancel"
                        style={{ opacity: 0.6 }}
                        onClick={() => { onClose(); }}
                    >
                        <ArrowBackIcon style={{ width: 24, height: 24 }} />
                    </IconButtonEx>

                    <Typography noWrap component="div" sx={{}}>
                        Channel Routing
                    </Typography>
                    {
                        landscape ? (
                            <>
                                <div style={{ flexGrow: 1 }}></div>
                                <div style={{ flexShrink: 0, display: "relative", marginLeft: 8, marginRight: 8  }}>
                                    {PresetSelect("325px")}
                                </div>
                                <div style={{ flexGrow: 1 }}></div>
                            </>

                        ) : (
                            <div style={{ flexGrow: 1 }}>&nbsp;</div>
                        )
                    }

                    <IconButtonEx tooltip="Help"
                        edge="end"
                        aria-label="help"
                        onClick={() => { setShowHelp(true); }}
                    >
                        <HelpOutlineIcon style={{ width: 24, height: 24, opacity: 0.6 }} />
                    </IconButtonEx>

                </div>
            </DialogTitle>
            {!landscape && (
                <>
                    <div style={{ marginLeft: 16 + 32 + 8, marginBottom: 8 }}>
                        {PresetSelect("60%")}
                    </div>
                    <Divider />
                </>
            )}

            <DialogContent>
                {landscape ?
                    LandscapeView()
                    :
                    PortraitView()
                }
            </DialogContent>
            {
                showHelp && (
                    <ChannelRouterSettingsHelpDialog open={showHelp} onClose={() => setShowHelp(false)} />
                )
            }
        </DialogEx >
    );
}

export default ChannelRouterSettingsDialog;
