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
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER`
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import React from 'react';
import Typography from '@mui/material/Typography';
import IconButtonEx from './IconButtonEx';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconColorDropdownButton, { DropdownAlignment } from './IconColorDropdownButton';
import InputLabel from '@mui/material/InputLabel';
import DialogEx from './DialogEx';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogContent';
import { nullCast } from './Utility';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { PiPedalModel } from './PiPedalModel';
//import TextFieldEx from './TextFieldEx';
import TextField from '@mui/material/TextField';
import { PedalboardItem } from './Pedalboard';


export interface PluginNameDialogProps {
    open: boolean,
    pedalboardItem: PedalboardItem | null,
    allowEmpty?: boolean,
    label?: string,
    onApply: (text: string, color: string) => void,
    onClose: () => void
};

export interface PluginNameDialogState {
    iconColor: string;
};

function isTouchUi() {
    return 'ontouchstart' in window || navigator.maxTouchPoints > 0;
}
export default class PluginNameDialog extends ResizeResponsiveComponent<PluginNameDialogProps, PluginNameDialogState> {

    refText: React.RefObject<HTMLInputElement | null>;

    constructor(props: PluginNameDialogProps) {
        super(props);
        this.state = {
            iconColor: this.props.pedalboardItem?.iconColor ?? ""
        };
        this.refText = React.createRef<HTMLInputElement>();
    }
    mounted: boolean = false;



    onWindowSizeChanged(width: number, height: number): void {
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
    checkForIllegalCharacters(filename: string) {
        if (filename.indexOf('/') !== -1) {
            throw new Error("Illegal character: '/'");
        }
    }

    render() {
        let props = this.props;
        let { open, onClose, onApply } = props;
        let model = PiPedalModel.getInstance();


        if (!open) return null;
        let pedalboardItem = props.pedalboardItem;
        if (!pedalboardItem) return null;

        let uiPlugin = model.getUiPlugin(pedalboardItem.uri);
        if (uiPlugin === null) {
            return null;
        }
        let defaultName = props.pedalboardItem?.title ?? "";
        const handleClose = () => {
            onClose();
        };

        const handleOk = () => {
            let text = nullCast(this.refText.current).value;
            text = text.trim();
            try {
                this.checkForIllegalCharacters(text);
            } catch (e: any) {
                let model: PiPedalModel = PiPedalModel.getInstance();
                model.showAlert(e.toString());
                return;
            }
            if (text.length === 0 && props.allowEmpty !== true) return;
            onApply(text, this.state.iconColor);
            onClose();
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
            <DialogEx tag="nameDialog" open={open} maxWidth="md" onClose={handleClose} aria-labelledby="Rename-dialog-title"
                style={{ userSelect: "none" }}
                onEnterKey={() => { }}
            >
                <DialogTitle style={{paddingBottom: 0, paddingTop: 8}}>
                    <div style={{display: "flex",flexFlow: "row nowrap", alignItems: "center"}} >
                        <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={() => onClose()} aria-label="back"
                        >
                            <ArrowBackIcon />
                        </IconButtonEx>
                        <Typography noWrap variant="h6" >
                            Plugin Properties
                        </Typography>
                    </div>
                </DialogTitle>
                <DialogContent >
                    <div style={{ display: "flex", flexFlow: "column nowrap", alignItems: "start", gap: 8, marginBottom: 8 }}>
                        <TextField
                            style={{ flex: "1 1 auto", width: 240 }}
                            autoFocus={!isTouchUi()}
                            onKeyDown={handleKeyDown}
                            variant="standard"
                            autoComplete="off"
                            autoCorrect="off"
                            onChange={(e) => { this.props.onApply(e.target.value.toString(), this.state.iconColor) }}
                            autoCapitalize="off"
                            slotProps={{
                                input: {
                                    style: { scrollMargin: 24 }
                                },
                                inputLabel: {
                                    shrink: true
                                }
                            }}
                            id="name"
                            type="text"
                            label="Plugin Display name"
                            placeholder={uiPlugin?.label ?? ""}
                            defaultValue={defaultName}
                            inputRef={this.refText}
                        />
                        <div>
                            <InputLabel style={{ fontSize: "0.75rem", fontWeight: 400, marginTop: 16 }}>Icon color</InputLabel>
                            <IconColorDropdownButton colorKey={this.state.iconColor}
                                onColorChange={(selection) => {
                                    if (this.props.pedalboardItem) {
                                        this.setState({ iconColor: selection.key });
                                        onApply(this.props.pedalboardItem.title, selection.key);
                                    }
                                }}
                                pluginType={uiPlugin.plugin_type}
                                dropdownAlignment={DropdownAlignment.SE}
                            />
                        </div>
                    </div>
                </DialogContent>
            </DialogEx>
        );
    }
}
