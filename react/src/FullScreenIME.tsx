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

import React, { ReactNode } from 'react';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { UiControl } from './Lv2Plugin';
import DialogEx from './DialogEx';
import Input from '@mui/material/Input';
import { nullCast } from './Utility';
import Typography from '@mui/material/Typography';


const styles = (theme: Theme) => createStyles({
    frame: {
        display: "block",
        position: "relative",
        flexDirection: "row",
        flexWrap: "nowrap",
        paddingTop: "8px",
        paddingBottom: "0px",
        height: "100%",
        overflowX: "auto",
        overflowY: "auto"
    },

});

interface FullScreenIMEProps extends WithStyles<typeof styles> {
    theme: Theme;
    uiControl?: UiControl;
    caption: string;
    value: number;
    onChange: (key: string, value: number) => void;
    onClose: () => void;
    initialHeight: number;
}
type FullScreenIMEState = {
    error: boolean;
};

const FullScreenIME =
    withStyles(styles, { withTheme: true })(
        class extends ResizeResponsiveComponent<FullScreenIMEProps, FullScreenIMEState>
        {
            model: PiPedalModel;
            inputRef: React.RefObject<HTMLInputElement>;

            constructor(props: FullScreenIMEProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.inputRef = React.createRef();

                this.state = {
                    error: false

                }
                this.onValueChanged = this.onValueChanged.bind(this);
                this.handleClose = this.handleClose.bind(this);
                this.onInputChange = this.onInputChange.bind(this);
                this.onInputKeyPress = this.onInputKeyPress.bind(this);
                this.onInputLostFocus = this.onInputLostFocus.bind(this);
            }

            onValueChanged(key: string, value: number): void {
                this.props.onChange(key, value);
            }


            onWindowSizeChanged(width: number, height: number): void {

                if (this.props.uiControl) {
                    // detecting a keyboard IME is painfully difficult. 
                    // we do it by comparing the current window height against the window height 
                    // when the IME was requested.

                    // a width change indicates a screen flip. Cancel the fullscreen ime in that case
                    // because we can't determine the initial window heigth anymore.
                    if (this.currentWidth === undefined)
                    {
                        this.currentWidth = window.innerWidth;
                    }
                    if (this.currentWidth !== window.innerWidth)
                    {
                        this.validateInput(true);
                    }
                    // eslint-disable-next-line no-restricted-globals
                    if (window.innerHeight >= this.props.initialHeight) {
                        this.validateInput(true);
                    }
                }
            }


            handleClose(e: {}, reason: string): void {
                this.props.onClose();
            }
            onInputLostFocus() {
                this.validateInput(true);

            }

            inputChanged: boolean = false;
            onInputChange() {

                this.inputChanged = true;
                this.validateInput(false);
            }

            currentValue: number = 0;

            validateInput(commitValue: boolean) {
                if (!this.inputRef.current) return;

                let text = this.inputRef.current.value;
                let valid = false;
                let result: number = this.currentValue;
                try {
                    if (text.length === 0) {
                        valid = false;
                    } else {
                        let v = Number(text);
                        if (isNaN(v)) {
                            valid = false;
                        } else {
                            valid = true;
                            result = v;
                        }
                    }
                } catch (error) {
                    valid = false;
                }

                if (commitValue) {
                    this.setState({ error: false });
                    if (!valid) {
                        result = this.currentValue; // reset the value!
                    }
                    // clamp and quantize.
                    let uiControl = nullCast(this.uiControl);

                    // quantize value.
                    result = uiControl.clampValue(result);

                    this.props.onChange(this.props.uiControl!.symbol, result);
                }
            }

            onInputKeyPress(e: any): void {
                if (e.charCode === 13) {
                    if (this.inputChanged) {
                        this.inputChanged = false;
                        this.validateInput(true);
                    }
                    else {
                        this.props.onClose();
                    }
                }
            }

            uiControl?: UiControl;

            currentWidth?: number;

            render(): ReactNode {
                //let classes = this.props.classes;
                let control = this.props.uiControl;
                if (!control) {
                    this.currentWidth = undefined;
                    return (<div/>);
                }
                this.currentWidth = window.innerWidth;

                this.uiControl = control;

                let value = this.props.value;

                return (
                    <DialogEx tag="ime" fullScreen open={!!(this.props.uiControl)} onClose={(e, r) => this.handleClose(e, r)} >
                        <div style={{
                            width: "100%", height: "100%", position: "relative",
                            display: "flex", flexDirection: "column", flexWrap: "nowrap",
                            justifyContent: "center", alignItems: "center"
                        }}
                        >
                            <Typography noWrap variant="body2">{this.props.caption}</Typography>
                            <Input key={value}
                                type="number"
                                defaultValue={control.formatShortValue(value)}
                                error={this.state.error}
                                autoFocus
                                inputProps={{
                                    'aria-label':
                                        control.symbol + " value",
                                    style: { textAlign: "center" },
                                    width: 80
                                }}
                                inputRef={this.inputRef} onChange={this.onInputChange}
                                onBlur={this.onInputLostFocus}

                                onKeyPress={this.onInputKeyPress} />

                        </div>

                    </DialogEx>
                );

            }

        }
    );

export default FullScreenIME;

