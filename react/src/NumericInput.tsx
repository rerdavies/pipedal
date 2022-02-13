// Copyright (c) 2021 Robin Davies
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
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import Input from '@mui/material/Input';



const styles = ({ palette }: Theme) => createStyles({

});



interface NumericInputProps extends WithStyles<typeof styles> {
    ariaLabel: string;
    defaultValue: number;
    min: number;
    max: number;
    onChange: (value: number) => void;
}

interface NumericInputState {
    error: boolean;
}

export const NumericInput =
    withStyles(styles, { withTheme: true })(
        class extends Component<NumericInputProps, NumericInputState>
        {


            constructor(props: NumericInputProps) {
                super(props);
                this.state = {
                    error: false
                };
            }

            changed: boolean = false;

            handleChange(event: any): void {
                this.changed = true;
                let strValue = event.target.value;
                try {
                    let value = Number(strValue);
                    if (isNaN(value))
                    {
                        this.setState({ error: true });
                    } else if (value >= this.props.min && value <= this.props.max) {
                        this.setState({ error: false });
                    } else {
                        this.setState({ error: true });
                    }
                } catch (error) {
                    this.setState({ error: true });
                }

            }

            toDisplayValue(value: number): string {
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

            apply(input: HTMLInputElement) {
                if (this.changed) {
                    let strValue = input.value;
                    let value = Number(strValue);
                    if (!isNaN(value))
                    {
                        if (value < this.props.min) value = this.props.min;
                        if (value > this.props.max) value = this.props.max;
                        input.value = this.toDisplayValue(value);
                        this.props.onChange(value);
                    } else {
                        input.value = this.toDisplayValue(this.props.defaultValue);
                    }
                    this.changed = false;
                    this.setState({ error: false });
                }
            }

            handleLostFocus(e: any) {
                this.apply(e.currentTarget as HTMLInputElement);
            }
            handleKeyDown(e: React.KeyboardEvent<HTMLInputElement>) {
                if (e.key === "Enter")
                {
                    this.apply(e.currentTarget as HTMLInputElement);
                } else if (e.key === "Escape")
                {
                    e.currentTarget.value = this.toDisplayValue(this.props.defaultValue);
                    this.changed = false;
                }
            }


            render() {
                //let classes = this.props.classes;
                return (<Input style={{ width: 50 }}
                    inputProps={{
                        'aria-label': this.props.ariaLabel,
                        style: { textAlign: 'right' },
                        "onBlur": (e: any) => this.handleLostFocus(e),
                        "onKeyDown": (e: any) => this.handleKeyDown(e)
                    }}
                    onChange={(event) => this.handleChange(event)}
                    defaultValue={this.toDisplayValue(this.props.defaultValue)}
                    error={this.state.error}
                />

                );
            }
        }
    );

export default NumericInput;



