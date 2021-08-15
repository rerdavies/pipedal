import React, { Component } from 'react';
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';
import Input from '@material-ui/core/Input';



const styles = ({ palette }: Theme) => createStyles({

});



interface NumericInputProps extends WithStyles<typeof styles> {
    ariaLabel: string;
    defaultValue: number;
    min: number;
    max: number;
    onChange: (value: number) => void;
};

interface NumericInputState {
    error: boolean;
};

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



