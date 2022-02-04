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

import { Component } from 'react';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { Theme, withStyles, WithStyles, createStyles } from '@material-ui/core/styles';
import MenuItem from '@material-ui/core/MenuItem';
import Select from '@material-ui/core/Select';
import MidiBinding from './MidiBinding';
import Utility, { nullCast } from './Utility';
import Typography from '@material-ui/core/Typography';
import MicNoneOutlinedIcon from '@material-ui/icons/MicNoneOutlined';
import MicOutlinedIcon from '@material-ui/icons/MicOutlined';
import IconButton from '@material-ui/core/IconButton';
import NumericInput from './NumericInput';



const styles = (theme: Theme) => createStyles({
    controlDiv: { flex: "0 0 auto", marginRight: 12,verticalAlign: "center", height: 48, paddingTop: 8, paddingBottom: 8  },
    controlDiv2: { flex: "0 0 auto", marginRight: 12,verticalAlign: "center", 
            height: 48, paddingTop: 0, paddingBottom: 0, whiteSpace: "nowrap"  }
});

interface MidiBindingViewProps extends WithStyles<typeof styles> {
    instanceId: number;
    listen: boolean;
    midiBinding: MidiBinding;
    onChange: (instanceId: number, newBinding: MidiBinding) => void;
    onListen: (instanceId: number, key: string, listenForControl: boolean) => void;
};


interface MidiBindingViewState {
};




const MidiBindingView =
    withStyles(styles, { withTheme: true })(
        class extends Component<MidiBindingViewProps, MidiBindingViewState> {

            model: PiPedalModel;

            constructor(props: MidiBindingViewProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                };
            }

            handleTypeChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.bindingType = newValue;
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleNoteChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.note = newValue;
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleControlChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.control = newValue;
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleLatchControlTypeChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.switchControlType = newValue;
                this.props.onChange(this.props.instanceId, newBinding);
            }
            handleLinearControlTypeChange(e: any, extra: any) {
                let newValue = parseInt(e.target.value);
                let newBinding = this.props.midiBinding.clone();
                newBinding.linearControlType = newValue;
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


            generateMidiSelects(): React.ReactNode[] {
                let result: React.ReactNode[] = [];

                for (let i = 0; i < 127; ++i) {
                    result.push(
                        <MenuItem value={i}>{Utility.midiNoteName(i)}</MenuItem>
                    )
                }

                return result;
            }
            generateControlSelects(): React.ReactNode[] {

                return Utility.validMidiControllers.map((control) => (
                    <MenuItem key={control.value} value={control.value}>{control.displayName}</MenuItem>
                    )
                );
            }


            render() {
                let classes = this.props.classes;
                let midiBinding = this.props.midiBinding;
                let pedalBoardItem = this.model.pedalBoard.get().getItem(this.props.instanceId);
                let uiPlugin = this.model.getUiPlugin(pedalBoardItem.uri);
                if (!uiPlugin) {
                    return (<div />);
                }

                let canLatch: boolean = false;
                let canTrigger: boolean = false;
                let showLinearControlTypeSelect: boolean = false;
                let isBinaryControl: boolean = false;
                let showLinearRange: boolean = false;
                let canRotaryScale: boolean = false;
                if (this.props.midiBinding.symbol === "__bypass") {
                    isBinaryControl = true;
                    canLatch = midiBinding.bindingType !== MidiBinding.BINDING_TYPE_NONE;
                } else {
                    let port = nullCast(uiPlugin!.getControl(this.props.midiBinding.symbol));
                    isBinaryControl = (port.isAbToggle() || port.isOnOffSwitch());
                    if (midiBinding.bindingType !== MidiBinding.BINDING_TYPE_NONE) {
                        canLatch = isBinaryControl;
                        canTrigger = port.trigger;
                        showLinearControlTypeSelect = !(canLatch || canTrigger);
                        showLinearRange = showLinearControlTypeSelect && midiBinding.linearControlType === MidiBinding.LINEAR_CONTROL_TYPE;
                        canRotaryScale = showLinearControlTypeSelect && midiBinding.linearControlType === MidiBinding.CIRCULAR_CONTROL_TYPE;
                    }
                }
                return (
                    <div style={{ display: "flex", flexDirection: "row", flexWrap: "wrap", justifyContent: "left", alignItems: "center", minHeight: 48 }}>
                        <div className={classes.controlDiv} >
                            <Select
                                style={{ width: 80, }}
                                onChange={(e, extra) => this.handleTypeChange(e, extra)}
                                value={midiBinding.bindingType}
                            >
                                <MenuItem value={0}>None</MenuItem>
                                {(isBinaryControl) && (
                                    <MenuItem value={1}>Note</MenuItem>
                                )}
                                <MenuItem value={2}>Control</MenuItem>
                            </Select>
                        </div>
                        {
                            (midiBinding.bindingType === MidiBinding.BINDING_TYPE_NOTE) &&
                            (
                                <div className={classes.controlDiv2} >
                                    <Select
                                        style={{ width:80}}
                                        onChange={(e, extra) => this.handleNoteChange(e, extra)}
                                        value={midiBinding.note}
                                        
                                    >
                                        {
                                            this.generateMidiSelects()
                                        }
                                    </Select>
                                    <IconButton onClick={()=> {
                                        if (this.props.listen)
                                        {
                                            this.props.onListen(-2, "", false)
                                        } else {
                                            this.props.onListen(this.props.instanceId, this.props.midiBinding.symbol, false)
                                        }
                                    }}>
                                        { this.props.listen ? (
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

                                    <Select
                                        style={{ width: 80 }}
                                        onChange={(e, extra) => this.handleControlChange(e, extra)}
                                        value={midiBinding.control}
                                        renderValue={(value)=> { return "CC-" + value}}
                                    >
                                        {
                                            this.generateControlSelects()
                                        }
                                    </Select>
                                    <IconButton onClick={()=> {
                                        if (this.props.listen)
                                        {
                                            this.props.onListen(-2, "", false)
                                        } else {
                                            this.props.onListen(this.props.instanceId, this.props.midiBinding.symbol, true)
                                        }
                                    }}>
                                        { this.props.listen ? (
                                            <MicOutlinedIcon />
                                        ) : (
                                        <MicNoneOutlinedIcon />
                                        )}
                                    </IconButton>

                                </div>
                            )
                        }
                        {
                            ((canLatch) &&
                            (
                                <div className={classes.controlDiv} >
                                    <Select
                                        style={{ width: 120 }}
                                        onChange={(e, extra) => this.handleLatchControlTypeChange(e, extra)}
                                        value={midiBinding.switchControlType}
                                    >
                                        <MenuItem value={MidiBinding.LATCH_CONTROL_TYPE}>Latch</MenuItem>
                                        <MenuItem value={MidiBinding.MOMENTARY_CONTROL_TYPE}>Momentary</MenuItem>
                                    </Select>
                                </div>
                            ))
                        }
                        {
                            (canTrigger) &&
                            (
                                <div className={classes.controlDiv2} >
                                    <Typography style={{paddingTop: 12, paddingBottom: 12}}>(Trigger)</Typography>
                                </div>
                            )

                        }
                        {
                            (showLinearControlTypeSelect) && 
                            (
                                <div className={classes.controlDiv} >
                                    <Select
                                        style={{ width: 120 }}
                                        onChange={(e, extra) => this.handleLinearControlTypeChange(e, extra)}
                                        value={midiBinding.linearControlType}
                                    >
                                        <MenuItem value={MidiBinding.LATCH_CONTROL_TYPE}>Linear</MenuItem>
                                        <MenuItem value={MidiBinding.MOMENTARY_CONTROL_TYPE}>Rotary</MenuItem>
                                    </Select>
                                </div>

                            )
                        }
                        {
                            showLinearRange && (
                                <div className={classes.controlDiv}>
                                    <Typography display="inline">Min:&nbsp;</Typography>
                                    <NumericInput defaultValue={midiBinding.minValue} ariaLabel='min'
                                        min={0} max={1} onChange={(value) => { this.handleMinChange(value); } }
                                />
                                </div>
                            )
                        }
                        {
                            showLinearRange && (
                                <div className={classes.controlDiv}>
                                    <Typography display="inline">Max:&nbsp;</Typography>
                                    <NumericInput defaultValue={midiBinding.maxValue} ariaLabel='max'
                                        min={0} max={1} onChange={(value) => { this.handleMaxChange(value); } } />
                                </div>
                            )
                        }
                        {
                            canRotaryScale && (
                                <div className={classes.controlDiv}>
                                    <Typography display="inline">Scale:&nbsp;</Typography>
                                    <NumericInput defaultValue={midiBinding.maxValue} ariaLabel='scale'
                                        min={-100} max={100} onChange={(value) => { this.handleScaleChange(value); } } />
                                </div>
                            )
                        }
                    </div>
                )

            }
        });

export default MidiBindingView;