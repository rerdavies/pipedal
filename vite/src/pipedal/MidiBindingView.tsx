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
import { ListenHandle, MidiMessage, PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";
import { createStyles } from './WithStyles';
import Snackbar from '@mui/material/Snackbar';

import { UiPlugin } from './Lv2Plugin';

import MenuItem from '@mui/material/MenuItem';
import Select from '@mui/material/Select';
import MidiBinding from './MidiBinding';
import Utility from './Utility';
import Typography from '@mui/material/Typography';
import MicNoneOutlinedIcon from '@mui/icons-material/MicNoneOutlined';
import MicOutlinedIcon from '@mui/icons-material/MicOutlined';
import IconButtonEx from './IconButtonEx';

import NumericInput from './NumericInput';



const styles = (theme: Theme) => createStyles({
    controlDiv: { flex: "0 0 auto", marginRight: 12, verticalAlign: "center", height: 48, paddingTop: 8, paddingBottom: 8 },
    controlDiv2: {
        flex: "0 0 auto", marginRight: 12, verticalAlign: "center",
        height: 48, paddingTop: 0, paddingBottom: 0, whiteSpace: "nowrap"
    }
});

export enum MidiControlType {
    None,
    Select,
    Dial,
    Toggle,
    Trigger,
    MomentarySwitch
}

export function getMidiControlType(uiPlugin: UiPlugin | undefined, symbol: string): MidiControlType {
    if (symbol === "__bypass") {
        return MidiControlType.Toggle;
    }
    if (!uiPlugin) return MidiControlType.None;
    let port = uiPlugin.getControl(symbol);
    if (!port) return MidiControlType.None;



    if (!port) return MidiControlType.None;

    if (port.mod_momentaryOffByDefault || port.mod_momentaryOnByDefault) {
        return MidiControlType.MomentarySwitch;
    }
    if (port.trigger_property) {
        return MidiControlType.Trigger;
    }
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

export function canBindToNote(controlType: MidiControlType): boolean {
    return controlType === MidiControlType.Toggle
        || controlType === MidiControlType.Trigger
        || controlType === MidiControlType.MomentarySwitch
}

interface MidiBindingViewProps extends WithStyles<typeof styles> {
    instanceId: number;
    midiBinding: MidiBinding;
    midiControlType: MidiControlType;
    onChange: (instanceId: number, newBinding: MidiBinding) => void;
}


interface MidiBindingViewState {
    listenSymbol: string;
    listenForRangeSymbol: string;
    listenForRangeMax: number;
    listenForRangeMin: number;
    listenSnackbarOpen: boolean;

}




const MidiBindingView =
    withStyles(
        class extends Component<MidiBindingViewProps, MidiBindingViewState> {

            model: PiPedalModel;

            constructor(props: MidiBindingViewProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    listenSymbol: "",
                    listenForRangeSymbol: "",
                    listenForRangeMax: 127,
                    listenForRangeMin: 0,
                    listenSnackbarOpen: false
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
            handleCtlMinChange(value: number): void {
                let newBinding = this.props.midiBinding.clone();
                newBinding.minControlValue = Math.round(value);
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleCtlMaxChange(value: number): void {
                let newBinding = this.props.midiBinding.clone();
                newBinding.maxControlValue = Math.round(value);
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
                        <MenuItem key={"k" + i} value={i}>{i} ({Utility.midiNoteName(i)})</MenuItem>
                    )
                }

                return result;
            }

            mounted: boolean = false;

            componentDidMount() {
                super.componentDidMount?.();
                this.mounted = true;
            }


            componentWillUnmount() {
                this.mounted = false;
                this.cancelListenForControl();
                super.componentWillUnmount?.();
            }
            validateSwitchControlType(midiBinding: MidiBinding) {
                // :-(

                let controlType = this.props.midiControlType;
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
                return this.props.midiControlType;
            }
            ///////////////////////////////////////
            listenTimeoutHandle?: number;

            listenHandle?: ListenHandle;

            cancelListenForControl() {
                this.stopListenTimeout();
                if (this.listenHandle) {
                    this.model.cancelListenForMidiEvent(this.listenHandle)
                    this.listenHandle = undefined;
                }

                this.setState({ listenSymbol: "", listenForRangeSymbol: "" });

            }

            handleListenForRangeSucceeded(instanceId: number, symbol: string, midiMessage: MidiMessage) {
                if (!midiMessage.isControl()) return;

                let binding = this.props.midiBinding;
                if (binding.control != midiMessage.cc1) {
                    return;
                }


                let value = midiMessage.cc2;
                let min = this.state.listenForRangeMin;
                let max = this.state.listenForRangeMax;

                let update = false;
                if (value < min) {
                    min = value;
                    update = true;
                }
                if (value > max) {
                    max = value;
                    update = true;
                }
                if (!update) {
                    return; // No change, so ignore.
                }

                let newBinding = binding.clone();
                newBinding.minControlValue = min;
                newBinding.maxControlValue = max;

                this.props.onChange(this.props.instanceId, newBinding);
                this.setState({ listenForRangeMin: min, listenForRangeMax: max });
                this.startListenTimeout(20000);

            }

            handleListenSucceeded(instanceId: number, symbol: string, midiMessage: MidiMessage) {
                if (instanceId !== this.props.instanceId) {
                    return;
                }
                if (symbol !== this.props.midiBinding.symbol) {
                    return;
                }


                let binding = this.props.midiBinding;
                let newBinding = binding.clone();
                if (midiMessage.isNote()) {
                    if (!canBindToNote(this.props.midiControlType)) {
                        return;
                    }
                    newBinding.bindingType = MidiBinding.BINDING_TYPE_NOTE
                    newBinding.note = midiMessage.cc1;
                } else if (midiMessage.isControl()) {
                    newBinding.bindingType = MidiBinding.BINDING_TYPE_CONTROL
                    newBinding.control = midiMessage.cc1;
                } else {
                    return;
                }
                this.props.onChange(this.props.instanceId, newBinding);
                this.cancelListenForControl();
            }
            handleListenForControlRange(instanceId: number, symbol: string): void {
                if (this.state.listenForRangeSymbol !== "") {
                    this.cancelListenForControl();
                    return;
                }
                this.cancelListenForControl();

                this.setState({
                    listenSymbol: "",
                    listenForRangeSymbol: symbol,
                    listenForRangeMin: 127,
                    listenForRangeMax: 0,
                    listenSnackbarOpen: true
                });
                this.startListenTimeout(20000);

                this.listenHandle = this.model.listenForMidiEvent(
                    (midiMessage) => {
                        this.handleListenForRangeSucceeded(instanceId, symbol, midiMessage);
                    });
            }

            stopListenTimeout(): void {
                if (this.listenTimeoutHandle) {
                    clearTimeout(this.listenTimeoutHandle);
                }
            }

            startListenTimeout(timeout: number): void {
                this.stopListenTimeout();
                this.listenTimeoutHandle = setTimeout(() => {
                    this.cancelListenForControl();
                    this.listenTimeoutHandle = undefined;
                }, timeout);
            }

            handleListen(): void {
                if (this.state.listenSymbol !== "") {
                    this.cancelListenForControl();
                    return;
                }
                this.cancelListenForControl();

                this.setState({
                    listenSymbol: this.props.midiBinding.symbol,
                    listenForRangeSymbol: "",
                    listenSnackbarOpen: true
                });
                let instanceId = this.props.instanceId;
                let symbol = this.props.midiBinding.symbol;

                this.startListenTimeout(8000);

                this.listenHandle = this.model.listenForMidiEvent(
                    (midiMessage) => {
                        this.handleListenSucceeded(instanceId, symbol, midiMessage);
                    });
            }

            ///////////////////////////////////////

            render() {
                const classes = withStyles.getClasses(this.props);
                let midiBinding = this.props.midiBinding;
                if (this.props.midiControlType === MidiControlType.None) {
                    return (<div />);
                }
                let controlType = this.props.midiControlType;

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
                                {(canBindToNote(this.props.midiControlType)) && (
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
                                    <IconButtonEx
                                        tooltip="Listen for MIDI input"
                                        onBlur={() => {
                                            this.cancelListenForControl();
                                        }}

                                        onClick={() => {
                                            this.handleListen()
                                        }}
                                        size="large">
                                        {this.state.listenSymbol !== "" ? (
                                            <MicOutlinedIcon />
                                        ) : (
                                            <MicNoneOutlinedIcon />
                                        )}
                                    </IconButtonEx>
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
                                    <IconButtonEx
                                        tooltip="Listen for MIDI input"
                                        onBlur={() => {
                                            this.cancelListenForControl();
                                        }}

                                        onClick={() => {
                                            this.handleListen()
                                        }}
                                        size="large">
                                        {this.state.listenSymbol !== "" ? (
                                            <MicOutlinedIcon />
                                        ) : (
                                            <MicNoneOutlinedIcon />
                                        )}
                                    </IconButtonEx>

                                </div>
                            )
                        }
                        {midiBinding.bindingType === MidiBinding.BINDING_TYPE_NONE &&
                            (
                                <IconButtonEx
                                    tooltip="Listen for MIDI input"
                                    onBlur={() => {
                                        this.cancelListenForControl();
                                    }}

                                    onClick={() => {
                                        this.handleListen()
                                    }}
                                    size="large">
                                    {this.state.listenSymbol !== "" ? (
                                        <MicOutlinedIcon />
                                    ) : (
                                        <MicNoneOutlinedIcon />
                                    )}
                                </IconButtonEx>

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
                            )}
                        {showLinearRange && (
                            <div style={{ display: "flex", flexFlow: "row wrap", alignItems: "center" }}>
                                <div className={classes.controlDiv}>
                                    <Typography display="inline" noWrap>Min Ctl:&nbsp;</Typography>
                                    <NumericInput integer value={midiBinding.minControlValue} ariaLabel='min ctl val'
                                        min={0} max={127} step={1} onChange={(value) => { this.handleCtlMinChange(value); }}
                                    />
                                </div>
                                <div style={{ display: "flex", flexFlow: "row nowrap", alignItems: "center" }}>
                                    <div className={classes.controlDiv}>
                                        <Typography display="inline" noWrap>Max Ctl:&nbsp;</Typography>
                                        <NumericInput value={midiBinding.maxControlValue} ariaLabel='max ctl val'
                                            integer={true}
                                            min={0} max={127} step={1} onChange={(value) => { this.handleCtlMaxChange(value); }}
                                        />
                                    </div>

                                    <IconButtonEx
                                        tooltip="Listen for control range"
                                        onBlur={() => {
                                            this.cancelListenForControl();
                                        }}
                                        onClick={() => {
                                            this.handleListenForControlRange(this.props.instanceId, this.props.midiBinding.symbol);
                                        }}
                                        size="large">
                                        {this.state.listenForRangeSymbol !== "" ? (
                                            <MicOutlinedIcon />
                                        ) : (
                                            <MicNoneOutlinedIcon />
                                        )}
                                    </IconButtonEx>
                                </div>

                            </div>
                        )
                        }
                        {
                            showLinearRange && (
                                <div style={{ display: "flex", flexFlow: "row wrap", alignItems: "center" }}>
                                    <div className={classes.controlDiv}>
                                        <Typography noWrap display="inline">Min Val:&nbsp;</Typography>
                                        <NumericInput value={midiBinding.minValue} ariaLabel='min'
                                            min={0} max={1} onChange={(value) => { this.handleMinChange(value); }}
                                        />
                                    </div>
                                    <div className={classes.controlDiv}>
                                        <Typography noWrap display="inline">Max Val:&nbsp;</Typography>
                                        <NumericInput value={midiBinding.maxValue} ariaLabel='max'
                                            min={0} max={1} onChange={(value) => { this.handleMaxChange(value); }} />
                                    </div>
                                </div>
                            )
                        }
                        {
                            canRotaryScale && (
                                <div className={classes.controlDiv}>
                                    <Typography display="inline">Scale:&nbsp;</Typography>
                                    <NumericInput value={midiBinding.maxValue} ariaLabel='scale'
                                        min={-100} max={100} onChange={(value) => { this.handleScaleChange(value); }} />
                                </div>
                            )
                        }

                        <Snackbar
                            anchorOrigin={{
                                vertical: 'bottom',
                                horizontal: 'left',
                            }}
                            open={this.state.listenSnackbarOpen}
                            autoHideDuration={1500}
                            onClose={() => this.setState({ listenSnackbarOpen: false })}
                            message="Listening for MIDI input"
                        />


                    </div>
                );

            }
        },
        styles);

export default MidiBindingView;
