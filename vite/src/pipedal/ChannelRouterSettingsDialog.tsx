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
import HelpOutlineIcon from '@mui/icons-material/HelpOutline';
import Divider from '@mui/material/Divider';
import MoreVertIcon from '@mui/icons-material/MoreVert';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButtonEx from './IconButtonEx';
import Button from '@mui/material/Button';
import useWindowSize from "./UseWindowSize";
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import ChannelRouterSettingsHelpDialog from './ChannelRouterSettingsHelpDialog';
import SpeakerIcon from '@mui/icons-material/Speaker';
import MicIcon from '@mui/icons-material/Mic';

import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import Typography from '@mui/material/Typography';
import ChannelRouterSettings from './ChannelRouterSettings';
import { Pedalboard, ControlValue } from './Pedalboard';

import DialogEx from './DialogEx';

import { PiPedalModelFactory } from './PiPedalModel';

export interface ChannelRouterSettingsDialogProps {
    open: boolean;
    onClose: () => void;
}

enum RouteType {
    Main,
    Aux,
    Send
}

interface ChannelRouterSettingsPreset {
    id: number;
    name: string;
    settings: ChannelRouterSettings;
}

let LEFT_LEFT_PRESET_ID = -2;
let RIGHT_STEREO_PRESET_ID = -3;

let channelRouterPresets: ChannelRouterSettingsPreset[] = [
    {
        id: -2, name: "Left -> Left", settings: new ChannelRouterSettings().deserialize({
            configured: true,
            channelRouterPresetId: -2,
            mainInputChannels: [0, 0],
            mainOutputChannels: [0, 0],
            mainInserts: new Pedalboard(),
            auxInputChannels: [-1, -1],
            auxOutputChannels: [-1, -1],
            auxInserts: new Pedalboard(),
            sendInputChannels: [-1, -1],
            sendOutputChannels: [-1, -1],
            controlValues: [],
        })
    },
    {
        id: -3, name: "Left -> Stereo", settings: new ChannelRouterSettings().deserialize({
            configured: true,
            channelRouterPresetId: -3,
            mainInputChannels: [0, 0],
            mainOutputChannels: [0, 1],
            mainInserts: new Pedalboard(),
            auxInputChannels: [-1, -1],
            auxOutputChannels: [-1, -1],
            auxInserts: new Pedalboard(),
            sendInputChannels: [-1, -1],
            sendOutputChannels: [-1, -1],
            controlValues: [],
        })
    },
    {
        id: -4, name: "Right -> Stereo", settings: new ChannelRouterSettings().deserialize({
            configured: true,
            channelRouterPresetId: -4,
            mainInputChannels: [1, 1],
            mainOutputChannels: [0, 1],
            mainInserts: new Pedalboard(),
            auxInputChannels: [-1, -1],
            auxOutputChannels: [-1, -1],
            auxInserts: new Pedalboard(),
            sendInputChannels: [-1, -1],
            sendOutputChannels: [-1, -1],
            controlValues: [],
        })
    },
    {
        id: -5, name: "Right -> Left + re-amp", settings: new ChannelRouterSettings().deserialize({
            configured: true,
            channelRouterPresetId: -5,
            mainInputChannels: [1, 1],
            mainOutputChannels: [0, 0],
            mainInserts: new Pedalboard(),
            auxInputChannels: [1, 1],
            auxOutputChannels: [1, 1],
            auxInserts: new Pedalboard(),
            sendInputChannels: [-1, - 1],
            sendOutputChannels: [-1, -1],
            controlValues: [],
        })
    },
    {
        id: -6, name: "Left -> Left + send", settings: new ChannelRouterSettings().deserialize({
            configured: true,
            channelRouterPresetId: -6,
            mainInputChannels: [0, 0],
            mainOutputChannels: [0, 0],
            mainInserts: new Pedalboard(),
            auxInputChannels: [-1, -1],
            auxOutputChannels: [-1, - 1],
            auxInserts: new Pedalboard(),
            sendInputChannels: [1, 1],
            sendOutputChannels: [1, 1],
            controlValues: [],
        })
    },
    {
        id: -7, name: "Right -> Right + send", settings: new ChannelRouterSettings().deserialize({
            configured: true,
            channelRouterPresetId: -7,
            mainInputChannels: [1, 1],
            mainOutputChannels: [1, 1],
            mainInserts: new Pedalboard(),
            auxInputChannels: [-1, -1],
            auxOutputChannels: [-1, - 1],
            auxInserts: new Pedalboard(),
            sendInputChannels: [0, 0],
            sendOutputChannels: [0, 0],
            controlValues: [],
        })
    },
    {
        id: -8, name: "Left -> Stereo + re-amp", settings: new ChannelRouterSettings().deserialize({
            configured: true,
            channelRouterPresetId: -8,
            mainInputChannels: [0, 0],
            mainOutputChannels: [0, 1],
            mainInserts: new Pedalboard(),
            auxInputChannels: [0, 0],
            auxOutputChannels: [2, 2],
            auxInserts: new Pedalboard(),
            sendInputChannels: [-1, -1],
            sendOutputChannels: [-1, -1],
            controlValues: [],

        })
    },
    {
        id: -9, name: "Left -> Stereo + stereo send", settings: new ChannelRouterSettings().deserialize({
            configured: true,
            channelRouterPresetId: -9,
            mainInputChannels: [0, 0],
            mainOutputChannels: [0, 1],
            mainInserts: new Pedalboard(),
            auxInputChannels: [-1, -1],
            auxOutputChannels: [-1, - 1],
            auxInserts: new Pedalboard(),
            sendInputChannels: [2, 3],
            sendOutputChannels: [2, 3],
            controlValues: [],

        })
    },
];

let cellPortraitInOutDiv: React.CSSProperties = {
    width: 80, display: "flex", columnGap: 4, flexDirection: "row", alignItems: "center", justifyContent: "right",
};
let cellPortraitSectionHead: React.CSSProperties = {
    border: "0px",
    paddingTop: 4,
    paddingBottom: 4,
    paddingLeft: 0,
    paddingRight: 12,
    width: 50,
    margin: "0px",
    textAlign: "left",
    verticalAlign: "middle",
};

let cellPortraitControlStrip: React.CSSProperties = {
    border: "0px",
    paddingTop: 12,
    paddingBottom: 12,
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
    border: "0px",
    paddingTop: 4,
    paddingBottom: 4,
    paddingLeft: 12,
    paddingRight: 12,
    margin: "0px",
    width: 80,
    textAlign: "right",
    verticalAlign: "middle"
};

function MakeChannelMenu(channelCount: number): React.ReactElement[] {
    let items: React.ReactElement[] = [(<MenuItem key={-1} value={-1}>None</MenuItem>)]
    if (channelCount === 0) {
        return items;
    }
    if (channelCount === 2) {
        items.push((<MenuItem key={0} value={0}>Left</MenuItem>));
        items.push((<MenuItem key={1} value={1}>Right</MenuItem>));
        return items;
    }
    for (let i = 0; i < channelCount; ++i) {
        items.push((<MenuItem key={i} value={i}>Ch {i + 1}</MenuItem>));
    }
    return items;
}



function ChannelRouterSettingsDialog(props: ChannelRouterSettingsDialogProps) {
    //const classes = useStyles();
    const { open, onClose } = props;
    let model = PiPedalModelFactory.getInstance();
    let config = model.jackConfiguration.get();


    const [settings, setSettings] = useState<ChannelRouterSettings>(model.channelRouterSettings.get());
    const [controlValues, setControlValues] = useState<ControlValue[]>(model.channelRouterControlValues.get());
    const [showHelp, setShowHelp] = useState<boolean>(false);

    if (settings) {
    }
    if (controlValues) {

    }


    React.useEffect(() => {
        if (open) {
            let handleSettingsChanged = () => {
                setSettings(model.channelRouterSettings.get());
            };
            model.channelRouterSettings.addOnChangedHandler(handleSettingsChanged);
            let handleControlsChanged = () => {
                setControlValues(model.channelRouterControlValues.get());
            };
            model.channelRouterControlValues.addOnChangedHandler(handleControlsChanged);
            return () => {
                model.channelRouterSettings.removeOnChangedHandler(handleSettingsChanged);
                model.channelRouterControlValues.removeOnChangedHandler(handleControlsChanged);
            }
        } else {
            return () => { };
        }
    }, [open]);

    const handleClose = (): void => {
        onClose();
    };
    let [windowSize] = useWindowSize();
    let landscape = windowSize.width > windowSize.height;
    let fullScreen = windowSize.width < 450 || windowSize.height < 600;

    let ChannelSelect = (routeType: RouteType, channelIndex: number, input: boolean, icon: boolean = true) => {
        let channelCount = input ? config.inputAudioPorts.length : config.outputAudioPorts.length;

        let value: number;

        switch (routeType) {
            case RouteType.Main:
                value = input ? settings.mainInputChannels[channelIndex] : settings.mainOutputChannels[channelIndex];
                break;
            case RouteType.Aux:
                value = input ? settings.auxInputChannels[channelIndex] : settings.auxOutputChannels[channelIndex];
                break;
            case RouteType.Send:
                value = input ? settings.sendInputChannels[channelIndex] : settings.sendOutputChannels[channelIndex];
                break;
        }
        if (value >= channelCount) {
            value = -1;
        }
        return (
            <Select variant="standard" style={{ width: 90 }} value={value}
                startAdornment={
                    icon ? (
                        input ? (<MicIcon style={{ opacity: 0.6, width: 16, height: 16, marginRight: 4 }} />) :
                            (<SpeakerIcon style={{ opacity: 0.6, width: 16, height: 16, marginRight: 4 }} />)
                    ) : undefined
                }
                onChange={(event) => {
                    let newValue = event.target.value as number;
                    let newSettings = Object.assign(new ChannelRouterSettings(), settings);
                    switch (routeType) {
                        case RouteType.Main:
                            if (input) {
                                newSettings.mainInputChannels[channelIndex] = newValue;
                            } else {
                                newSettings.mainOutputChannels[channelIndex] = newValue;
                            }
                            break;
                        case RouteType.Aux:
                            if (input) {
                                newSettings.auxInputChannels[channelIndex] = newValue;
                            } else {
                                newSettings.auxOutputChannels[channelIndex] = newValue;
                            }
                            break;
                        case RouteType.Send:
                            if (input) {
                                newSettings.sendInputChannels[channelIndex] = newValue;

                            } else {
                                newSettings.sendOutputChannels[channelIndex] = newValue;
                            }
                            break;
                    }
                    newSettings.channelRouterPresetId = -1; // custom.
                    newSettings.configured = true;
                    model.setChannelRouterSettings(newSettings);
                }}
            >
                {MakeChannelMenu(channelCount)}
            </Select>
        )
    }
    let Vu = () => {
        return (
            <div style={{}} >
                <div style={{ width: 8, height: 48, background: "black" }}></div>
                {/* <VuMeter instanceId={Pedalboard.CHANNEL_ROUTER_MAIN_INSERT_ID} display={"input"} /> */}
            </div>
        );
    }

    let ApplyPresetId = (presetId: number | string | null | undefined) => {
        if (typeof presetId !== 'number') {
            return;
        }
        for (let preset of channelRouterPresets) {
            if (preset.id === presetId) {
                model.setChannelRouterSettings(preset.settings);
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
            (preset.settings.auxOutputChannels.every((ch) => ch < outputChannels || ch === -1)) &&
            (preset.settings.sendInputChannels.every((ch) => ch < inputChannels || ch === -1)) &&
            (preset.settings.sendOutputChannels.every((ch) => ch < outputChannels || ch === -1))
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
            <div style={{ display: "flex", flexDirection: "row", marginTop: 2, alignItems: "center" }}>
                <Select variant="standard" style={{ width: width }} value={settings.channelRouterPresetId}
                    onChange={(event) => {
                        ApplyPresetId(event.target.value);
                    }}
                >
                    {GeneratePresetMenuItems()}
                </Select>
                <IconButtonEx tooltip="Presets" aria-label="more-presets">
                    <MoreVertIcon style={{ opacity: 0.6 }} onClick={() => { }} />
                </IconButtonEx>
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
                            <td rowSpan={2} style={cellLeft}>{Vu()}</td>
                            <td rowSpan={2} style={cellLeft}>
                                <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                    Inserts
                                </Button>
                            </td>

                            <td rowSpan={2} style={cellLeft}>{Vu()}</td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 0, false)}
                            </td>
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
                            <td rowSpan={2} style={cellLeft}>{Vu()}</td>
                            <td rowSpan={2} style={cellLeft}>
                                <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                    Inserts
                                </Button>
                            </td>

                            <td rowSpan={2} style={cellLeft}>{Vu()}</td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 0, false))}
                            </td>
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

                        {/* Spacer ---------------- */}
                        <tr><td colSpan={9} style={{ height: 16 }}></td></tr>

                        {/* Send ---------------- */}
                        <tr>
                            <td rowSpan={1} style={cellLandscapeTitle}>
                                <Typography variant="body2" color="textSecondary">Send</Typography>
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Send, 0, false))}
                            </td>
                            <td rowSpan={2} style={cellLeft}>{Vu()}</td>
                            <td rowSpan={2} style={cellLeft}>
                                {/* No button */}
                            </td>
                            <td rowSpan={2} style={cellLeft}>{Vu()}</td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Send, 0, true))}
                            </td>
                        </tr>
                        <tr>
                            <td></td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Send, 1, false))}
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Send, 1, true))}
                            </td>
                        </tr>

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
                <table style={{ borderCollapse: "collapse" }}>
                    <colgroup>
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                    </colgroup>
                    <tbody>
                        {/* Main */}
                        <tr>
                            <td colSpan={1} style={cellPortraitSectionHead}>
                                <Typography variant="body2" color="textSecondary"  >Main</Typography>
                            </td>
                            <td colSpan={4}>

                            </td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >In</Typography>
                                    <MicIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 0, true, false)}
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 1, true, false)}
                            </td>
                            <td></td>
                        </tr>
                        <tr>
                            <td></td>
                            <td colSpan={4}>
                                <table>
                                    <tbody>
                                        <tr>
                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                            <td style={cellPortraitControlStrip}>
                                                <div style={{ marginLeft: 8, marginRight: 8 }}>
                                                    <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                                        Inserts
                                                    </Button>
                                                </div>
                                            </td>

                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                        </tr>

                                    </tbody>
                                </table>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >Out</Typography>
                                    <SpeakerIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>

                            <td style={cellLeft}>

                                {ChannelSelect(RouteType.Main, 0, false, false)}
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 1, false, false)}
                            </td>
                            <td></td>
                        </tr>

                        {/* Spacer ---------------- */}
                        <tr><td colSpan={5} style={{ height: 16 }}></td></tr>
                        {/* Aux */}
                        <tr>
                            <td colSpan={1} style={cellPortraitSectionHead}>
                                <Typography variant="body2" color="textSecondary"  >Aux</Typography>
                            </td>
                            <td colSpan={4}></td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >In</Typography>
                                    <MicIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 0, true, false))}
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 1, true, false))}
                            </td>
                            <td></td>
                        </tr>
                        <tr>
                            <td></td>
                            <td colSpan={4}>
                                <table>
                                    <tbody>
                                        <tr>
                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                            <td style={cellPortraitControlStrip}>
                                                <div style={{ marginLeft: 8, marginRight: 8 }}>
                                                    <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                                        Inserts
                                                    </Button>
                                                </div>
                                            </td>

                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                        </tr>

                                    </tbody>
                                </table>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >Out</Typography>
                                    <SpeakerIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Aux, 0, false, false)}
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Aux, 1, false, false)}
                            </td>
                            <td></td>
                        </tr>

                        {/* Spacer ---------------- */}
                        <tr><td colSpan={5} style={{ height: 16 }}></td></tr>

                        {/* Send */}
                        <tr>
                            <td colSpan={1} style={cellPortraitSectionHead}>
                                <Typography variant="body2" color="textSecondary"  >Send</Typography>
                            </td>
                            <td colSpan={4}></td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >Send</Typography>
                                    <SpeakerIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Send, 0, false, false))}
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Send, 1, false, false))}
                            </td>
                            <td></td>
                        </tr>
                        <tr>
                            <td></td>
                            <td colSpan={4}>
                                <table>
                                    <tbody>
                                        <tr>
                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                            <td style={cellPortraitControlStrip}>
                                                <div style={{ marginLeft: 8, marginRight: 8, visibility: "hidden" }}>
                                                    <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                                        Inserts
                                                    </Button>
                                                </div>
                                            </td>

                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                        </tr>

                                    </tbody>
                                </table>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellPortraitInOut}>
                                <div style={cellPortraitInOutDiv}>
                                    <Typography variant="body2" color="textSecondary" >Return</Typography>
                                    <MicIcon style={{ verticalAlign: "middle", width: 16, height: 16, opacity: 0.6 }} />
                                </div>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Send, 0, true, false)}
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Send, 1, false, false)}
                            </td>
                            <td></td>
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
                                <div style={{ flexGrow: 1 }}>&nbsp;</div>
                                <div style={{ flexShrink: 1, maxWidth: 440, display: "relative", marginLeft: 16, marginRight: 16 }}>
                                    {PresetSelect(200)}
                                </div>
                                <div style={{ flexGrow: 1 }}>&nbsp;</div>
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
