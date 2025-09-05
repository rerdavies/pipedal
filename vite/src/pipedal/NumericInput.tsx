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

import React, { Component } from 'react';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";
import Input from '@mui/material/Input';



const styles = ({ palette }: Theme) => createStyles({

});


function isSamsungDevice() {
    const userAgent = navigator.userAgent.toLowerCase();
    return userAgent.includes('samsung') || userAgent.includes('sm-');
}


interface NumericInputProps extends WithStyles<typeof styles> {
    ariaLabel: string;
    value: number;
    min: number;
    max: number;
    step?: number;
    integer?: boolean;
    onChange: (value: number) => void;
}

interface NumericInputState {
    stateValue: string;
    error: boolean;
    focused: boolean;
}

export const NumericInput =
    withStyles(
        class extends Component<NumericInputProps, NumericInputState>
        {


            constructor(props: NumericInputProps) {
                super(props);
                this.state = {
                    stateValue: this.toDisplayValue(props.value),
                    error: false,
                    focused: false
                };
            }

            changed: boolean = false;

            handleChange(event: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>): void {
                this.changed = true;
                let strValue = event.target.value;
                this.setState({ stateValue: strValue });
                try {
                    let value = Number(strValue);
                    if (isNaN(value))
                    {
                        this.setState({ error: true });
                    } else if (this.props.integer === true && !Number.isInteger(value)) {
                        this.setState({ error: true });
                    } else if (value >= this.props.min && value <= this.props.max) {
                        this.setState({ error: false });
                        this.props.onChange(value);
                    } else {
                        this.setState({ error: true });
                    }
                } catch (error) {
                    this.setState({ error: true });
                }

            }

            toDisplayValue(value: number): string {
                if (this.props.integer === true)
                {
                    return Math.round(value).toFixed(0);
                }
                if (value <= -1000 || value >= 1000)
                {
                    return value.toFixed(0);
                }
                if (value <= -100 || value >= 100)
                {
                    return value.toFixed(1);
                }
                if (value <= -10 || value >= 10)
                {
                    return value.toFixed(2);
                } else {
                    return value.toFixed(3);
                }
            }

            apply(input: HTMLInputElement| HTMLTextAreaElement) {
                if (this.changed) {
                    let strValue = input.value;
                    let value = Number(strValue);
                    if (!isNaN(value))
                    {
                        if (value < this.props.min) value = this.props.min;
                        if (value > this.props.max) value = this.props.max;
                        if (this.props.integer === true) value = Math.round(value);
                        input.value = this.toDisplayValue(value);
                        this.props.onChange(value);
                    } else {
                        input.value = this.toDisplayValue(this.props.value);
                        this.props.onChange(this.props.value);
                    }
                    this.changed = false;
                    this.setState({ error: false });
                }
            }

            handleFocus(e: React.FocusEvent<HTMLInputElement| HTMLTextAreaElement>) {
                this.changed = false;
                this.setState({ focused: true });
                e.currentTarget.select();
                this.setState({ stateValue: this.toDisplayValue(this.props.value) });
            }
            handleBlur(e: any) {
                this.apply(e.currentTarget as HTMLInputElement);
                this.setState({ focused: false });
            }
            handleKeyDown(e: React.KeyboardEvent<HTMLInputElement | HTMLTextAreaElement>) {
                if (e.key === "Enter")
                {
                     this.setState({ stateValue: this.toDisplayValue(this.props.value) });
                     this.changed = false;
                } 
            }


            render() {
                //const classes = withStyles.getClasses(this.props);
                return (<Input style={{ width: 50 }}
                    inputProps={{
                        'aria-label': this.props.ariaLabel,
                        style: { textAlign: 'right' },
                        type: isSamsungDevice()? "text": "number",
                        inputMode: "decimal",
                        autoComplete: "off",
                        autoCorrect: "off",
                        autoCapitalize: "off",
                        min: this.props.min,
                        max: this.props.max,
                        step: this.props.step
                    }}
                    onFocus={(e) => {this.handleFocus(e);}}
                    onBlur= {(e) => { this.handleBlur(e); }}
                    onKeyDown={(e) => {this.handleKeyDown(e);}}
                    onChange={(e) => this.handleChange(e)}
                    value={this.state.focused ? this.state.stateValue: this.toDisplayValue(this.props.value)}
                    error={this.state.error}
                />

                );
            }
        },
        styles
    );

export default NumericInput;



