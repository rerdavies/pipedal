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
import VuMeter from './VuMeter';
import Toolbar from '@mui/material/Toolbar';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButtonEx from './IconButtonEx';
import Button from '@mui/material/Button';
import useWindowSize from "./UseWindowSize";
import DialIcon from './svg/fx_dial.svg?react';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import ChannelMixerSettingsHelpDialog from './ChannelMixerSettingsHelpDialog';
import PluginControl from './PluginControl';

import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import Typography from '@mui/material/Typography';
import ChannelMixerSettings from './ChannelMixerSettings';
import { makeChanelMixerUiPlugin, CHANNEL_MIXER_INSTANCE_ID } from './ChannelMixerUiControl';
import { ControlValue } from './Pedalboard';

import DialogEx from './DialogEx';

import { PiPedalModelFactory } from './PiPedalModel';

export interface ChannelMixerSettingsDialogProps {
    open: boolean;
    onClose: () => void;
}

let channelMixerUiPlugin = makeChanelMixerUiPlugin();

enum RouteType {
    Main,
    Aux
}
let cellSectionHead: React.CSSProperties = {
    border: "0px",
    paddingTop: 4,
    paddingBottom: 4,
    paddingLeft: 0,
    paddingRight: 12,
    margin: "0px",
    textAlign: "left",
    verticalAlign: "middle"
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
    paddingRight: 4,
    margin: "0px",
    textAlign: "left",
    verticalAlign: "middle"
};
let cellLeftIndent: React.CSSProperties = {
    border: "0px",
    paddingTop: 4,
    paddingBottom: 4,
    paddingLeft: 12,
    paddingRight: 4,
    margin: "0px",
    textAlign: "right",
    verticalAlign: "middle"
};
let cellLeftH: React.CSSProperties = {
    border: "0px",
    paddingTop: 4,
    paddingBottom: 4,
    paddingLeft: 4,
    paddingRight: 4,
    margin: "0px",
    textAlign: "left",
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



function ChannelMixerSettingsDialog(props: ChannelMixerSettingsDialogProps) {
    //const classes = useStyles();
    const { open, onClose } = props;
    let model = PiPedalModelFactory.getInstance();
    let config = model.jackConfiguration.get();


    const [settings, setSettings] = useState<ChannelMixerSettings>(model.channelMixerSettings.get());
    const [controlValues,setControlValues] = useState<ControlValue[]>(model.channelMixerControlValues.get());
    const [showHelp, setShowHelp] = useState<boolean>(false);

    if (settings) {
    }
    if (controlValues) {

    }


    React.useEffect(() => {
        if (open) {
            let handleSettingsChanged = () => {
                setSettings(model.channelMixerSettings.get());
            };
            model.channelMixerSettings.addOnChangedHandler(handleSettingsChanged);
            let handleControlsChanged = () => {
                setControlValues(model.channelMixerControlValues.get());
            };
            model.channelMixerControlValues.addOnChangedHandler(handleControlsChanged);
            return () => {
                model.channelMixerSettings.removeOnChangedHandler(handleSettingsChanged);
                model.channelMixerControlValues.removeOnChangedHandler(handleControlsChanged);
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

    let Dial = (symbol: string) => {
        let instanceId = CHANNEL_MIXER_INSTANCE_ID;
        let control = channelMixerUiPlugin.getControl(symbol);
        let value = settings.getControlValue(symbol);
        if (value === null) {
            value = control?.default_value ?? 0;
        }

        return (<PluginControl
            instanceId={instanceId}
            uiControl={control}
            onPreviewChange={(value: number) => {
                model.previewPedalboardValue(instanceId, symbol, value);
             }}
            onChange={(value: number) => { 
                model.setPedalboardControl(instanceId, symbol, value);
            }}
            requestIMEEdit={() => { }}
            value={value}
        />);

        return <DialIcon width={32} height={32} style={{ verticalAlign: "middle", fill: "white" }} />;
    }
    let ChannelSelect = (routeType: RouteType, channelIndex: number, input: boolean) => {
        let channelCount = input ? config.inputAudioPorts.length : config.outputAudioPorts.length;

        let value: number;

        switch (routeType) {
            case RouteType.Main:
                value = input ? settings.guitarInputChannels[channelIndex] : settings.guitarOutputChannels[channelIndex];
                break;
            case RouteType.Aux:
                value = input ? settings.auxInputChannels[channelIndex] : settings.auxOutputChannels[channelIndex];
                break;
        }
        if (value >= channelCount) {
            value = -1;
        }
        return (
            <Select variant="standard" style={{ width: 80 }} value={value}
                onChange={(event) => {
                    let newValue = event.target.value as number;
                    let newSettings = Object.assign(new ChannelMixerSettings(), settings);
                    switch (routeType) {
                        case RouteType.Main:
                            if (input) {
                                newSettings.guitarInputChannels[channelIndex] = newValue;
                            } else {
                                newSettings.guitarOutputChannels[channelIndex] = newValue;
                            }
                            break;
                        case RouteType.Aux:
                            if (input) {
                                newSettings.auxInputChannels[channelIndex] = newValue;
                            } else {
                                newSettings.auxOutputChannels[channelIndex] = newValue;
                            }
                            break;
                    }
                    model.channelMixerSettings.set(newSettings);
                }}
            >
                {MakeChannelMenu(channelCount)}
            </Select>
        )
    }
    let Vu = () => {
        return (
            <div style={{  }} >
                <VuMeter instanceId={CHANNEL_MIXER_INSTANCE_ID} display={"input"} />
            </div>
        );
    }

    let LandscapeView = () => {
        return (
            <div style={{ width: "100%", height: "100%", flexGrow: 1, fontSize: "14px" }} >
                <table style={{ borderCollapse: "collapse", width: "100%" }}>
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
                    <thead>
                        <tr>
                            <th></th>
                            <th style={cellLeftH}><Typography variant="caption" >In</Typography></th>
                            <th></th>
                            <th></th>
                            <th></th>
                            <th></th>
                            <th></th>
                            <th style={cellLeftH}><Typography variant="caption" >Out</Typography></th>
                        </tr>
                    </thead>
                    <tbody>
                        {/* Main */}
                        <tr>
                            <td rowSpan={1} style={cellLeft}>
                                <Typography variant="body2" color="textSecondary">Main</Typography>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 0, true)}
                            </td>
                            <td rowSpan={2} style={cellLeft}>{Dial("mainIn")}</td>
                            <td rowSpan={2} style={cellLeft}>{Vu()}</td>
                            <td rowSpan={2} style={cellLeft}>
                                <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                    Inserts
                                </Button>
                            </td>

                            <td rowSpan={2} style={cellLeft}>{Dial("mainOut")}</td>
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

                        {/* Spacer */}
                        <tr><td colSpan={9} style={{ height: 16 }}></td></tr>

                        {/* Aux */}
                        <tr>
                            <td rowSpan={1} style={cellLeft}>
                                <Typography variant="body2" color="textSecondary">Aux</Typography>
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 0, true))}
                            </td>
                            <td rowSpan={2} style={cellLeft}>{Dial("auxIn")}</td>
                            <td rowSpan={2} style={cellLeft}>{Vu()}</td>
                            <td rowSpan={2} style={cellLeft}>
                                <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                    Inserts
                                </Button>
                            </td>

                            <td rowSpan={2} style={cellLeft}>{Dial("auxOut")}</td>
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

                    </tbody>
                </table>
            </div>
        );
    };
    let PortraitView = () => {
        return (
            <div style={{ width: "100%", height: "100%", flexGrow: 1, fontSize: "14px" }} >
                <table style={{ borderCollapse: "collapse", width: "100%" }}>
                    <colgroup>
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "auto" }} />
                        <col style={{ width: "100%" }} />
                    </colgroup>
                    <tbody>
                        {/* Main */}
                        <tr>
                            <td colSpan={4} style={cellSectionHead}>
                                <Typography variant="body2" color="textSecondary"  >Main</Typography>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellLeftIndent}>
                                <Typography variant="body2" color="textSecondary" >In</Typography>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 0, true)}
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 1, true)}
                            </td>
                            <td></td>
                        </tr>
                        <tr>
                            <td></td>
                            <td colSpan={3}>
                                <table>
                                    <tbody>
                                        <tr>
                                            <td style={cellPortraitControlStrip}>{Dial("mainIn")}</td>
                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                            <td style={cellPortraitControlStrip}>
                                                <div style={{ marginLeft: 8, marginRight: 8 }}>
                                                    <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                                        Inserts
                                                    </Button>
                                                </div>
                                            </td>

                                            <td style={cellPortraitControlStrip}>{Dial("mainOut")}</td>
                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                        </tr>

                                    </tbody>
                                </table>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellLeftIndent}>
                                <Typography variant="body2" color="textSecondary" >Out</Typography>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 0, false)}
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Main, 1, false)}
                            </td>
                            <td></td>
                        </tr>

                        {/* Spacer */}
                        <tr><td colSpan={4} style={{ height: 16 }}></td></tr>

                        {/* Aux */}
                        <tr>
                            <td colSpan={4} style={cellSectionHead}>
                                <Typography variant="body2" color="textSecondary"  >Aux</Typography>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellLeftIndent}>
                                <Typography variant="body2" color="textSecondary" >In</Typography>
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 0, true))}
                            </td>
                            <td style={cellLeft}>
                                {(ChannelSelect(RouteType.Aux, 1, true))}
                            </td>
                            <td></td>
                        </tr>
                        <tr>
                            <td></td>
                            <td colSpan={3}>
                                <table>
                                    <tbody>
                                        <tr>
                                            <td style={cellPortraitControlStrip}>{Dial("auxIn")}</td>
                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                            <td style={cellPortraitControlStrip}>
                                                <Button variant="outlined" style={{ textTransform: "none", borderRadius: 24 }}>
                                                    Inserts
                                                </Button>
                                            </td>

                                            <td style={cellPortraitControlStrip}>{Dial("auxOut")}</td>
                                            <td style={cellPortraitControlStrip}>{Vu()}</td>
                                        </tr>

                                    </tbody>
                                </table>
                            </td>
                        </tr>
                        <tr>
                            <td style={cellLeftIndent}>
                                <Typography variant="body2" color="textSecondary" >Out</Typography>
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Aux, 0, false)}
                            </td>
                            <td style={cellLeft}>
                                {ChannelSelect(RouteType.Aux, 1, false)}
                            </td>
                            <td></td>
                        </tr>
                    </tbody>
                </table>
            </div>
        );
    };

    return (
        <DialogEx tag="channelMixerSettings" onClose={handleClose} aria-labelledby="select-channel_mixer_settings"
            open={open}
            fullWidth maxWidth="sm"
            onEnterKey={handleClose}
            fullScreen={fullScreen}
            style={{ maxWidth: 1200 }}
        >
            <DialogTitle id="select-channel_mixer_settings" style={{ paddingTop: 0, paddingBottom: 0, marginBottom: 8 }}>
                <Toolbar style={{ padding: 0, margin: 0 }}>
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

                    <Typography noWrap component="div" sx={{ flexGrow: 1 }}>
                        Channel Routing
                    </Typography>

                    <IconButtonEx tooltip="Help"
                        edge="end"
                        aria-label="help"
                        onClick={() => { setShowHelp(true); }}
                    >
                        <HelpOutlineIcon style={{ width: 24, height: 24, opacity: 0.6 }} />
                    </IconButtonEx>


                </Toolbar>
            </DialogTitle>
            <DialogContent>
                {landscape ?
                    LandscapeView()
                    :
                    PortraitView()
                }
            </DialogContent>
            {showHelp && (
                <ChannelMixerSettingsHelpDialog open={showHelp} onClose={() => setShowHelp(false)} />
            )}
        </DialogEx>
    );
}

export default ChannelMixerSettingsDialog;
