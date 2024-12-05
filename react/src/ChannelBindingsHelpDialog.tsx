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
//import TextFieldEx from './TextFieldEx';


export interface ChannelBindingHelpDialogProps {
    open: boolean,
    onClose: () => void,
};

export interface ChannelBindingHelpDialogState {
    fullScreen: boolean;
};

export default class ChannelBindingHelpDialog extends ResizeResponsiveComponent<ChannelBindingHelpDialogProps, ChannelBindingHelpDialogState> {

    refText: React.RefObject<HTMLInputElement>;

    constructor(props: ChannelBindingHelpDialogProps) {
        super(props);
        this.state = {
            fullScreen: false
        };
        this.refText = React.createRef<HTMLInputElement>();
    }
    mounted: boolean = false;



    onWindowSizeChanged(width: number, height: number): void 
    {
        this.setState({fullScreen: height < 200})
    }


    componentDidMount()
    {
        super.componentDidMount();
        this.mounted = true;
    }
    componentWillUnmount()
    {
        super.componentWillUnmount();
        this.mounted = false;
    }

    componentDidUpdate()
    {
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
                    aria-labelledby="Rename-dialog-title" 
                    style={{userSelect: "none"}}
                    fullwidth
                onEnterKey={()=>{}}
                >
                <DialogContent style={{minHeight: 96}}>
                <Typography variant="h5" style={{marginBottom: 8}}>
                    MIDI Control Binding Priority
                </Typography>
                <div style={{fontSize: "0.9em", lineHeight: "18pt" }}>
                    <p style={{lineHeight: "1.4em"}}>MIDI Channel Filters determine which MIDI messages get sent to a plugin that accepts MIDI messages. Note that the 
                        MIDI Channel Filters do NOT affect system MIDI bindings or control bindings.
                    </p>
                    <p>
                        MIDI message processing occurs in the following order
                    </p>
                    <ul>
                        <li>If the message is a Program Change message, the message is first offered to any MIDI plugins in the currently loaded preset (Message Filters permitting).
                            The message is sent to each MIDI plugin in the current preset that wants it. If any MIDI plugin accepts the program change, 
                            no futher processing occurs. Specifically, the program change will not change the currently selected PiPedal preset.
                            <br/>
                        </li>
                        <li>
                            If the message is a Program Change message, and no MIDI plugin has accepted the message, PiPedal selects the PiPedal preset that corresponds
                            to the requested program. 
                            <br/>
                        </li>
                        <li>
                            The message is then checked to see if it has been bound to a Pipedal feature using the System Midi Bindings settings. If it has, 
                            Pipedal processes the message, and it is not forwarded to MIDI plugins.
                            <br/>
                        </li>
                        <li>
                            Otherwise, the message is sent to each MIDI plugin in the currently loaded Pipedal preset (channel filters permitting).)
                            <br/>
                        </li>
                    </ul>
                </div>

                </DialogContent>
                <Divider/>
                <DialogActions style={{flexShrink: 1}}>
                    <Button onClick={handleClose} variant="dialogPrimary"  >
                        OK
                    </Button>
                </DialogActions>
            </DialogEx>
        );
    }
}