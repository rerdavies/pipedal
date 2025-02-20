/*
 * MIT License
 * 
 * Copyright (c) 2022-2023 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


import { Component } from 'react';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";
import {createStyles} from './WithStyles';

import MenuItem from '@mui/material/MenuItem';
import Select from '@mui/material/Select';
import MidiBinding from './MidiBinding';
import Utility from './Utility';
import MicNoneOutlinedIcon from '@mui/icons-material/MicNoneOutlined';
import MicOutlinedIcon from '@mui/icons-material/MicOutlined';
import IconButton from '@mui/material/IconButton';



const styles = (theme: Theme) => createStyles({
    controlDiv: { flex: "0 0 auto", marginRight: 12, verticalAlign: "center", height: 48, paddingTop: 8, paddingBottom: 8 },
    controlDiv2: {
        flex: "0 0 auto", marginRight: 12, verticalAlign: "center",
        height: 48, paddingTop: 0, paddingBottom: 0, whiteSpace: "nowrap"
    }
});

interface SystemMidiBindingViewProps extends WithStyles<typeof styles> {
    instanceId: number;
    listen: boolean;
    midiBinding: MidiBinding;
    onChange: (instanceId: number, newBinding: MidiBinding) => void;
    onListen: (instanceId: number, key: string, listenForControl: boolean) => void;
}


interface SystemMidiBindingViewState {
}




const SystemMidiBindingView =
    withStyles(
        class extends Component<SystemMidiBindingViewProps, SystemMidiBindingViewState> {

            model: PiPedalModel;

            constructor(props: SystemMidiBindingViewProps) {
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
                const classes = withStyles.getClasses(this.props);
                let midiBinding = this.props.midiBinding;

                return (
                    <div style={{ display: "flex", flexDirection: "row", flexWrap: "wrap", justifyContent: "left", alignItems: "center", minHeight: 48 }}>
                        <div className={classes.controlDiv} >
                            <Select variant="standard"
                                style={{ width: 80, }}
                                onChange={(e, extra) => this.handleTypeChange(e, extra)}
                                value={midiBinding.bindingType}
                            >
                                <MenuItem value={0}>None</MenuItem>
                                <MenuItem value={1}>Note</MenuItem>
                                <MenuItem value={2}>Control</MenuItem>
                            </Select>
                        </div>
                        {
                            (midiBinding.bindingType === MidiBinding.BINDING_TYPE_NOTE) &&
                            (
                                <div className={classes.controlDiv} >
                                    <Select variant="standard"
                                        style={{ width: 80, verticalAlign: 'center' }}
                                        onChange={(e, extra) => this.handleNoteChange(e, extra)}
                                        value={midiBinding.note}

                                    >
                                        {
                                            this.generateMidiSelects()
                                        }
                                    </Select>
                                </div>
                            )
                        }
                        {
                            (midiBinding.bindingType === MidiBinding.BINDING_TYPE_CONTROL) &&
                            (
                                <div className={classes.controlDiv} >

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
                                </div>
                            )
                        }
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
                );

            }
        },
        styles
        );

export default SystemMidiBindingView;