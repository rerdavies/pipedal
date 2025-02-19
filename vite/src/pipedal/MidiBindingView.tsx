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

import { Component } from 'react';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";
import {createStyles} from './WithStyles';

import { UiPlugin } from './Lv2Plugin';

import MenuItem from '@mui/material/MenuItem';
import Select from '@mui/material/Select';
import MidiBinding from './MidiBinding';
import Utility from './Utility';
import Typography from '@mui/material/Typography';
import MicNoneOutlinedIcon from '@mui/icons-material/MicNoneOutlined';
import MicOutlinedIcon from '@mui/icons-material/MicOutlined';
import IconButton from '@mui/material/IconButton';
import NumericInput from './NumericInput';



const styles = (theme: Theme) => createStyles({
    controlDiv: { flex: "0 0 auto", marginRight: 12, verticalAlign: "center", height: 48, paddingTop: 8, paddingBottom: 8 },
    controlDiv2: {
        flex: "0 0 auto", marginRight: 12, verticalAlign: "center",
        height: 48, paddingTop: 0, paddingBottom: 0, whiteSpace: "nowrap"
    }
});

enum MidiControlType {
    None,
    Toggle,
    Trigger,
    Dial,
    Select,
}

interface MidiBindingViewProps extends WithStyles<typeof styles> {
    instanceId: number;
    listen: boolean;
    midiBinding: MidiBinding;
    uiPlugin: UiPlugin;
    onChange: (instanceId: number, newBinding: MidiBinding) => void;
    onListen: (instanceId: number, key: string, listenForControl: boolean) => void;
}


interface MidiBindingViewState {
    midiControlType: MidiControlType;
}




const MidiBindingView =
    withStyles(
        class extends Component<MidiBindingViewProps, MidiBindingViewState> {

            model: PiPedalModel;

            constructor(props: MidiBindingViewProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    midiControlType: this.getControlType()
                };
            }

            handleBindingTypeChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.setBindingType(newValue);
                this.validateSwitchControlType(newBinding);
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleNoteChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.note = newValue;
                this.validateSwitchControlType(newBinding);
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleControlChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.control = newValue;
                this.validateSwitchControlType(newBinding);
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleLatchControlTypeChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.switchControlType = newValue;
                this.validateSwitchControlType(newBinding);
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleTriggerControlTypeChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.switchControlType = newValue;
                this.validateSwitchControlType(newBinding);
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleLinearControlTypeChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.linearControlType = newValue;
                this.validateSwitchControlType(newBinding);
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleMinChange(value: number): void {
                let newBinding = this.props.midiBinding.clone();
                newBinding.minValue = value;
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleMaxChange(value: number): void {
                let newBinding = this.props.midiBinding.clone();
                newBinding.maxValue = value;
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleScaleChange(value: number): void {
                let newBinding = this.props.midiBinding.clone();
                newBinding.rotaryScale = value;
                this.props.onChange(this.props.instanceId, newBinding);
            }


            generateMidiNoteSelects(): React.ReactNode[] {
                let result: React.ReactNode[] = [];

                for (let i = 0; i < 127; ++i) {
                    result.push(
                        <MenuItem value={i}>{Utility.midiNoteName(i)}</MenuItem>
                    )
                }

                return result;
            }

            validateSwitchControlType(midiBinding: MidiBinding) {
                // :-(

                let controlType = this.state.midiControlType;
                if (controlType === MidiControlType.Toggle
                    && midiBinding.bindingType === MidiBinding.BINDING_TYPE_NOTE) {
                    if (midiBinding.switchControlType !== MidiBinding.TRIGGER_ON_RISING_EDGE // Toggle on Note On.
                        && midiBinding.switchControlType !== MidiBinding.MOMENTARY_CONTROL_TYPE // Note on/Note off.
                    ) {
                        midiBinding.switchControlType = MidiBinding.TRIGGER_ON_RISING_EDGE;
                    }
                }

            }
            generateControlSelects(): React.ReactNode[] {

                return Utility.validMidiControllers.map((control) => (
                    <MenuItem key={control.value} value={control.value}>{control.displayName}</MenuItem>
                )
                );
            }

            getControlType(): MidiControlType {

                if (this.props.midiBinding.symbol === "__bypass") {
                    return MidiControlType.Toggle;
                }
                if (!this.props.uiPlugin) return MidiControlType.None;

                let port = this.props.uiPlugin.getControl(this.props.midiBinding.symbol);

                if (!port) return MidiControlType.None;
                if (port.trigger_property) {
                    return MidiControlType.Trigger;
                }
                if (port.isAbToggle() || port.isOnOffSwitch()) {
                    return MidiControlType.Toggle;
                }
                if (port.isSelect()) {
                    return MidiControlType.Select;
                }
                return MidiControlType.Dial;
            }

            render() {
                const classes = withStyles.getClasses(this.props);
                let midiBinding = this.props.midiBinding;
                let uiPlugin = this.props.uiPlugin;
                if (!uiPlugin) {
                    return (<div />);
                }
                let controlType = this.state.midiControlType;

                let showLinearRange =
                    controlType === MidiControlType.Dial &&
                    midiBinding.bindingType === MidiBinding.BINDING_TYPE_CONTROL
                    && midiBinding.linearControlType === MidiBinding.LINEAR_CONTROL_TYPE;


                let canRotaryScale: boolean =
                    controlType === MidiControlType.Dial &&
                    midiBinding.bindingType === MidiBinding.BINDING_TYPE_CONTROL
                    && midiBinding.linearControlType === MidiBinding.CIRCULAR_CONTROL_TYPE;


                return (
                    <div style={{ display: "flex", flexDirection: "row", flexWrap: "wrap", justifyContent: "left", alignItems: "center", minHeight: 48 }}>
                        <div className={classes.controlDiv} >
                            <Select variant="standard"
                                style={{ width: 80, }}
                                onChange={(e, extra) => this.handleBindingTypeChange(e, extra)}
                                value={midiBinding.bindingType}
                            >
                                <MenuItem value={0}>None</MenuItem>
                                {(controlType === MidiControlType.Toggle || controlType === MidiControlType.Trigger) && (
                                    <MenuItem value={1}>Note</MenuItem>
                                )}
                                <MenuItem value={2}>Control</MenuItem>
                            </Select>
                        </div>
                        {
                            (midiBinding.bindingType === MidiBinding.BINDING_TYPE_NOTE) &&
                            (
                                <div className={classes.controlDiv2} >
                                    <Select variant="standard"
                                        style={{ width: 80 }}
                                        onChange={(e, extra) => this.handleNoteChange(e, extra)}
                                        value={midiBinding.note}

                                    >
                                        {
                                            this.generateMidiNoteSelects()
                                        }
                                    </Select>
                                    <IconButton
                                        onClick={() => {
                                            if (this.props.listen) {
                                                this.props.onListen(-2, "", false)
                                            } else {
                                                this.props.onListen(this.props.instanceId, this.props.midiBinding.symbol, false)
                                            }
                                        }}
                                        size="large">
                                        {this.props.listen ? (
                                            <MicOutlinedIcon />
                                        ) : (
                                            <MicNoneOutlinedIcon />
                                        )}
                                    </IconButton>
                                </div>
                            )
                        }
                        {
                            (midiBinding.bindingType === MidiBinding.BINDING_TYPE_CONTROL) &&
                            (
                                <div className={classes.controlDiv2} >

                                    <Select variant="standard"
                                        style={{ width: 80 }}
                                        onChange={(e, extra) => this.handleControlChange(e, extra)}
                                        value={midiBinding.control}
                                        renderValue={(value) => { return "CC-" + value }}
                                    >
                                        {
                                            this.generateControlSelects()
                                        }
                                    </Select>
                                    <IconButton
                                        onClick={() => {
                                            if (this.props.listen) {
                                                this.props.onListen(-2, "", false)
                                            } else {
                                                this.props.onListen(this.props.instanceId, this.props.midiBinding.symbol, true)
                                            }
                                        }}
                                        size="large">
                                        {this.props.listen ? (
                                            <MicOutlinedIcon />
                                        ) : (
                                            <MicNoneOutlinedIcon />
                                        )}
                                    </IconButton>

                                </div>
                            )
                        }
                        {
                            ((controlType === MidiControlType.Toggle && midiBinding.bindingType === MidiBinding.BINDING_TYPE_NOTE) &&
                                (

                                    <div className={classes.controlDiv} >
                                        <Select variant="standard"
                                            style={{}}
                                            onChange={(e, extra) => this.handleLatchControlTypeChange(e, extra)}
                                            value={midiBinding.switchControlType}
                                        >
                                            <MenuItem value={MidiBinding.TRIGGER_ON_RISING_EDGE}>Toggle on Note On</MenuItem>
                                            <MenuItem value={MidiBinding.MOMENTARY_CONTROL_TYPE}>Note On/Note off</MenuItem>
                                        </Select>
                                    </div>
                                ))
                        }
                        {
                            ((controlType === MidiControlType.Toggle && midiBinding.bindingType === MidiBinding.BINDING_TYPE_CONTROL) &&
                                (

                                    <div className={classes.controlDiv} >
                                        <Select variant="standard"
                                            style={{}}
                                            onChange={(e, extra) => this.handleLatchControlTypeChange(e, extra)}
                                            value={midiBinding.switchControlType}
                                        >
                                            <MenuItem value={MidiBinding.TOGGLE_ON_RISING_EDGE}>Toggle on rising edge</MenuItem>
                                            <MenuItem value={MidiBinding.TOGGLE_ON_ANY}>Toggle on any value</MenuItem>
                                            <MenuItem value={MidiBinding.TOGGLE_ON_VALUE}>Use control value</MenuItem>
                                        </Select>
                                    </div>
                                ))
                        }
                        {
                            (controlType === MidiControlType.Trigger && midiBinding.bindingType === MidiBinding.BINDING_TYPE_CONTROL) &&
                            (
                                <div className={classes.controlDiv} >
                                    <Select variant="standard"
                                        style={{}}
                                        onChange={(e, extra) => this.handleTriggerControlTypeChange(e, extra)}
                                        value={midiBinding.switchControlType}
                                    >
                                        <MenuItem value={MidiBinding.TRIGGER_ON_RISING_EDGE}>Trigger on rising edge</MenuItem>
                                        <MenuItem value={MidiBinding.TRIGGER_ON_ANY}>
                                            Trigger on any value
                                        </MenuItem>
                                    </Select>
                                </div>
                            )

                        }
                        {
                            (controlType === MidiControlType.Dial && midiBinding.bindingType === MidiBinding.BINDING_TYPE_CONTROL) &&
                            (
                                <div className={classes.controlDiv} >
                                    <Select variant="standard"
                                        style={{ width: 120 }}
                                        onChange={(e, extra) => this.handleLinearControlTypeChange(e, extra)}
                                        value={midiBinding.linearControlType}
                                    >
                                        <MenuItem value={MidiBinding.LINEAR_CONTROL_TYPE}>Linear</MenuItem>
                                        <MenuItem value={MidiBinding.CIRCULAR_CONTROL_TYPE}>Rotary</MenuItem>
                                    </Select>
                                </div>

                            )
                        }
                        {
                            showLinearRange && (
                                <div className={classes.controlDiv}>
                                    <Typography display="inline">Min:&nbsp;</Typography>
                                    <NumericInput defaultValue={midiBinding.minValue} ariaLabel='min'
                                        min={0} max={1} onChange={(value) => { this.handleMinChange(value); }}
                                    />
                                </div>
                            )
                        }
                        {
                            showLinearRange && (
                                <div className={classes.controlDiv}>
                                    <Typography display="inline">Max:&nbsp;</Typography>
                                    <NumericInput defaultValue={midiBinding.maxValue} ariaLabel='max'
                                        min={0} max={1} onChange={(value) => { this.handleMaxChange(value); }} />
                                </div>
                            )
                        }
                        {
                            canRotaryScale && (
                                <div className={classes.controlDiv}>
                                    <Typography display="inline">Scale:&nbsp;</Typography>
                                    <NumericInput defaultValue={midiBinding.maxValue} ariaLabel='scale'
                                        min={-100} max={100} onChange={(value) => { this.handleScaleChange(value); }} />
                                </div>
                            )
                        }
                    </div>
                );

            }
        },
        styles);

export default MidiBindingView;