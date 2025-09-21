// Copyright (c) Robin E. R. Davies
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
import DialogTitle from '@mui/material/DialogTitle';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import { nullCast } from './Utility';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import {PiPedalModel,PiPedalModelFactory} from './PiPedalModel';
//import TextFieldEx from './TextFieldEx';
import TextField from '@mui/material/TextField';


export interface RenameDialogProps {
    open: boolean,
    defaultName: string,
    acceptActionName: string,
    title: string,
    allowEmpty?: boolean,
    label?: string,
    onOk: (text: string) => void,
    onClose: () => void
};

export interface RenameDialogState {
    fullScreen: boolean;
};

function isTouchUi() 
{
    return 'ontouchstart' in window || navigator.maxTouchPoints > 0;
}
export default class RenameDialog extends ResizeResponsiveComponent<RenameDialogProps, RenameDialogState> {

    refText: React.RefObject<HTMLInputElement|null>;

    constructor(props: RenameDialogProps) {
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
    checkForIllegalCharacters(filename: string) {
        if (filename.indexOf('/') !== -1)
        {
            throw new Error("Illegal character: '/'");
        }
    }

    render() {
        let props = this.props;
        let { open, defaultName, acceptActionName, onClose, onOk } = props;

        const handleClose = () => {
            onClose();
        };

        const handleOk = () => {
            let text = nullCast(this.refText.current).value;
            text = text.trim();
            try {
                this.checkForIllegalCharacters(text);
            } catch (e:any)
            {
                let model:PiPedalModel = PiPedalModelFactory.getInstance();
                model.showAlert(e.toString());
                return;
            }
            if (text.length === 0 && props.allowEmpty !== true) return;
            onOk(text);
        }
        const handleKeyDown = (event: React.KeyboardEvent<HTMLDivElement>): void => {
            // 'keypress' event misbehaves on mobile so we track 'Enter' key via 'keydown' event
            if (event.key === 'Enter') {
                event.preventDefault();
                event.stopPropagation();
                handleOk();
            }
        };
        return (
            <DialogEx tag="nameDialog" open={open} fullWidth maxWidth="sm" onClose={handleClose} aria-labelledby="Rename-dialog-title" 
                fullScreen={this.state.fullScreen}
                style={{userSelect: "none"}}
                onEnterKey={()=>{}}
                >
                    <DialogTitle>
                        {props.title ?? "Rename"}
                    </DialogTitle>

                <DialogContent >
                    <TextField
                        autoFocus={!isTouchUi()}
                        onKeyDown={handleKeyDown}
                        variant="standard"
                        autoComplete="off"
                        autoCorrect="on"
                        autoCapitalize="on"
                        spellCheck={false}
                        
                        slotProps={{
                            inputLabel: {
                                shrink: true
                            },

                            input: {
                                style: { scrollMargin: 24 }
                            }
                        }}
                        id="name"
                        type="text"
                        label={props.label ?? "Name"}
                        fullWidth
                        defaultValue={defaultName}
                        inputRef={this.refText}
                    />
                </DialogContent>
                <DialogActions style={{flexShrink: 1}}>
                    <Button onClick={handleClose} variant="dialogSecondary" >
                        Cancel
                    </Button>
                    <Button onClick={handleOk} variant="dialogPrimary"  >
                        {acceptActionName}
                    </Button>
                </DialogActions>
            </DialogEx>
        );
    }
}
