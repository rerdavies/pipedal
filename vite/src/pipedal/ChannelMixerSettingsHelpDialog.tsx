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

import React from 'react';
import Button from '@mui/material/Button';
import DialogEx from './DialogEx';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import Typography from '@mui/material/Typography';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import Divider from '@mui/material/Divider';
import DialogTitle from '@mui/material/DialogTitle';
import Toolbar from '@mui/material/Toolbar';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButtonEx from './IconButtonEx';


export interface ChannelMixerSettingsHelpDialogProps {
    open: boolean,
    onClose: () => void,
};

export interface ChannelMixerSettingsHelpDialogState {
    fullScreen: boolean;
};

export default class ChannelMixerSettingsHelpDialog extends ResizeResponsiveComponent<ChannelMixerSettingsHelpDialogProps, ChannelMixerSettingsHelpDialogState> {

    refText: React.RefObject<HTMLInputElement | null>;

    constructor(props: ChannelMixerSettingsHelpDialogProps) {
        super(props);
        this.state = {
            fullScreen: false
        };
        this.refText = React.createRef<HTMLInputElement>();
    }
    mounted: boolean = false;



    onWindowSizeChanged(width: number, height: number): void {
        this.setState({ fullScreen: height < 200 })
    }


    componentDidMount() {
        super.componentDidMount();
        this.mounted = true;
    }
    componentWillUnmount() {
        super.componentWillUnmount();
        this.mounted = false;
    }

    componentDidUpdate() {
    }
    render() {
        let props = this.props;
        let { open, onClose } = props;

        const handleClose = () => {
            onClose();
        };

        return (
            <DialogEx tag="bindingHelp" open={open} fullWidth maxWidth="sm"
                onClose={handleClose}
                aria-labelledby="mixer-settings-dialog-title"
                style={{ userSelect: "none" }}
                fullwidth
                onEnterKey={() => { }}
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
                            Channel Routing Dialog Help
                        </Typography>
                    </Toolbar>
                </DialogTitle>

                <DialogContent style={{ minHeight: 96 }}>
                    <div style={{ fontSize: "0.9em", lineHeight: "18pt" }}>
                        <p> The Mixer Routing Dialog determines how audio signals are processed and routed between input and output audio channels.
                        </p>
                        <p>Guitar effects processing occurs on the <i>Main</i> route only. Plugins in the main PiPedal window are
                            applied before any insert plugins that have been added to the <i>Main</i> route. Main insert plugins are
                            applied globally, regardless of the selected preset or effects in the main PiPedal window. You might
                            want to add an EQ or reverb plugin here, to allow your presets to be globally adjusted to suit the room
                            in which you are currently performing.
                        </p>
                        <p>Plugins in the main PiPedal window are not applied to the <i>Aux</i> route. The Aux route is intended to be
                            used for
                        </p>
                        <ul>
                            <li>Passing through a dry (unprocessed) guitar signal that can be recorded by a DAW and
                                re-amped later during mixing.
                            </li>
                            <li>Passing through a vocal mic signal from one of the input channels, perhaps with compression, EQ or
                                other processing performed by plugins in Aux inserts.
                            </li>
                            <li>
                                Passing through a backing track or other external audio signal from one of the input channels, and then either
                                mixing it with the processed guitar signal, or passing it out on a dedicated output channel.
                            </li>
                        </ul>
                        <p>although you may find other creative uses for the Aux channel as well.</p>
                        <p>If the Main and Aux routes share output channels, then the results of the Main and Output signals are
                            summed together on those output channels. For stereo mixing, assign the same left and  right output
                            channels to both the Main and Aux routes. If you have a 2x4 or 4x4 audio interface with two guitar input channels
                            you can also pass  through stereo dry signals by configuring Main and Aux input  and output channels appropriately.
                        </p>
                    </div>

                </DialogContent>
            </DialogEx>
        );
    }
}