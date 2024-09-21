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


import React, { SyntheticEvent } from 'react';
import DialogEx from './DialogEx';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { PiPedalModel, PiPedalModelFactory, ListenHandle } from './PiPedalModel';
import Typography from '@mui/material/Typography';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButton from '@mui/material/IconButton';
import MidiBinding from './MidiBinding';
import SystemMidiBindingView from './SystemMidiBindingView';
import Snackbar from '@mui/material/Snackbar';

const styles = (theme: Theme) => createStyles({
    dialogAppBar: {
        position: 'relative',
        top: 0, left: 0
    },
    dialogTitle: {
        marginLeft: theme.spacing(2),
        flex: "1 1 auto",
    },
    pluginTable: {
        border: "collapse",
        width: "100%",

    },
    pluginHead: {
        borderTop: "1pt #DDD solid", paddingLeft: 22, paddingRight: 22
    },
    bindingTd: {
        verticalAlign: "top",
        paddingLeft: 12, paddingBottom: 8
    },
    nameTd: {
        paddingLeft: 16,
        verticalAlign: "top",
        paddingTop: 12
    },
});


export interface SystemMidiBindingDialogProps extends WithStyles<typeof styles> {
    open: boolean,
    onClose: () => void
}

class BindingEntry {
    constructor(
        displayName: string,
        instanceId: number,
        midiBinding: MidiBinding,
    ) {
        this.displayName = displayName;
        this.instanceId = instanceId;
        this.midiBinding = midiBinding;
    }
    displayName: string;
    instanceId: number;
    midiBinding: MidiBinding;
}

export interface SystemMidiBindingDialogState {
    listenInstanceId: number;
    listenSymbol: string;
    listenSnackbarOpen: boolean;
    systemMidiBindings: BindingEntry[];

}

export const SystemMidiBindingDialog =
    withStyles(styles, { withTheme: true })(
        class extends ResizeResponsiveComponent<SystemMidiBindingDialogProps, SystemMidiBindingDialogState> {

            model: PiPedalModel;

            constructor(props: SystemMidiBindingDialogProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    listenInstanceId: -2,
                    listenSymbol: "",
                    listenSnackbarOpen: false,
                    systemMidiBindings: this.createBindings()
                };
                this.handleClose = this.handleClose.bind(this);
                this.onMidiBindingsChanged = this.onMidiBindingsChanged.bind(this);
            }

            createBindings(): BindingEntry[] {
                let result: BindingEntry[] = [];
                for (var item of this.model.systemMidiBindings.get())
                {
                    let displayName  = "";
                    let instanceId = -1;

                    if (item.symbol === "prevProgram")
                    {
                        displayName = "Previous Preset";
                        instanceId = 1;
                    } else if (item.symbol === "nextProgram")
                    {
                        displayName = "Next Preset";
                        instanceId = 2;
                    } else if (item.symbol === "startHotspot")
                    {
                        displayName = "Enable Hotspot";
                        instanceId = 3;
                    } else if (item.symbol === "stopHotspot")
                    {
                        displayName = "Disable Hotspot";
                        instanceId = 4;
                    } else if (item.symbol === "shutdown")
                    {
                        displayName = "Shutdown";
                        instanceId = 5;
                    } else if (item.symbol === "reboot")
                    {
                        displayName = "Reboot";
                        instanceId = 6;
                    }

                    if (instanceId !== -1)
                    {
                        result.push(new BindingEntry(displayName,instanceId,item));
                    }
                }
                return result;
            }
            mounted: boolean = false;

            hasHooks: boolean = false;

            handleClose() {
                this.cancelListenForControl();
                this.props.onClose();
            }

            listenTimeoutHandle?: NodeJS.Timeout;

            listenHandle?: ListenHandle;

            cancelListenForControl() {
                if (this.listenTimeoutHandle) {
                    clearTimeout(this.listenTimeoutHandle);
                    this.listenTimeoutHandle = undefined;
                }
                if (this.listenHandle)
                {
                    this.model.cancelListenForMidiEvent(this.listenHandle)
                    this.listenHandle = undefined;
                }

                this.setState({ listenInstanceId: -2, listenSymbol: "" });

            }

            handleListenSucceeded(instanceId: number, symbol: string, isNote: boolean, noteOrControl: number)
            {
                this.cancelListenForControl();

                for (var binding of this.state.systemMidiBindings)
                {
                    if (binding.instanceId === instanceId)
                    {
                        let newBinding = binding.midiBinding.clone();

                        if (isNote) {
                            newBinding.bindingType = MidiBinding.BINDING_TYPE_NOTE;
                            newBinding.note = noteOrControl;
                        } else {
                            newBinding.bindingType = MidiBinding.BINDING_TYPE_CONTROL;
                            newBinding.control = noteOrControl;
                        }
        
                        this.model.setSystemMidiBinding(instanceId,newBinding);
                        return;
                    }
                }
            }


            handleListenForControl(instanceId: number, symbol: string, listenForControl: boolean): void {
                this.cancelListenForControl();
                this.setState({ listenInstanceId: instanceId, listenSymbol: symbol, listenSnackbarOpen: true });
                this.listenTimeoutHandle = setTimeout(() => {
                    this.cancelListenForControl();
                }, 8000);

                this.listenHandle = this.model.listenForMidiEvent(listenForControl,
                    (isNote: boolean, noteOrControl: number) => {
                        this.handleListenSucceeded(instanceId,symbol,isNote, noteOrControl);
                    });
                


            }

            onWindowSizeChanged(width: number, height: number): void {
            }

            onMidiBindingsChanged() {
                this.setState({systemMidiBindings: this.createBindings()});
            }

            componentDidMount() {
                super.componentDidMount();
                this.mounted = true;

                this.model.systemMidiBindings.addOnChangedHandler(this.onMidiBindingsChanged);
                this.onMidiBindingsChanged();
            }
            componentWillUnmount() {
                super.componentWillUnmount();

                this.mounted = false;
                this.model.systemMidiBindings.removeOnChangedHandler(this.onMidiBindingsChanged);
            }

            componentDidUpdate() {
            }
            handleItemChanged(instanceId: number, newBinding: MidiBinding) {
                this.model.setSystemMidiBinding(instanceId, newBinding);
            }
            generateTable(): React.ReactNode[] {
                let classes = this.props.classes;
                let result: React.ReactNode[] = [];

                let items = this.state.systemMidiBindings;
                
                for (var item of items) {
                    result.push(
                        <tr key={item.instanceId}>
                            <td className={classes.nameTd}>
                                <Typography noWrap style={{ verticalAlign: "center", height: 48 }}>
                                    {item.displayName}
                                </Typography>
                            </td>
                            <td className={classes.bindingTd}>
                                <SystemMidiBindingView instanceId={item.instanceId} midiBinding={item.midiBinding}
                                    onListen={(instanceId: number, symbol: string, listenForControl: boolean) => {
                                        if (instanceId === -2)
                                        {
                                            this.cancelListenForControl();
                                        } else {
                                            this.handleListenForControl(instanceId, symbol, listenForControl);
                                        }
                                    }}
                                    listen={item.instanceId === this.state.listenInstanceId && this.state.listenSymbol === item.midiBinding.symbol}
                                    onChange={(instanceId: number, newItem: MidiBinding) => this.handleItemChanged(instanceId, newItem)}
                                />
                            </td>
                        </tr>
                    );
                }


                return result;
            }

            supressDefault(e: SyntheticEvent) {
                //e.preventDefault();
                //e.stopPropagation();
            }

            render() {
                let props = this.props;
                let { open, classes } = props;
                if (!open) {
                    return (<div />);
                }

                return (
                    <DialogEx tag="systemMidiBindings" open={open} fullWidth onClose={this.handleClose} aria-labelledby="Rename-dialog-title" 
                        fullScreen={true} 
                        style={{userSelect: "none"}}
                    >
                            <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                                <div style={{ flex: "0 0 auto" }}>
                                    <AppBar className={classes.dialogAppBar} >
                                        <Toolbar>
                                            <IconButton
                                                edge="start"
                                                color="inherit"
                                                onClick={this.handleClose}
                                                aria-label="back"
                                                size="large">
                                                <ArrowBackIcon />
                                            </IconButton>
                                            <Typography variant="h6" className={classes.dialogTitle}>
                                                System MIDI Bindings
                                            </Typography>
                                        </Toolbar>
                                    </AppBar>
                                </div>
                                <div style={{ overflow: "auto", flex: "1 1 auto", width: "100%" }}>
                                    <table className={classes.pluginTable} >
                                        <colgroup>
                                            <col style={{ width: "auto" }} />
                                            <col style={{ width: "100%" }} />
                                        </colgroup>

                                        <tbody>
                                            {this.generateTable()}

                                        </tbody>
                                    </table>
                                </div>
                            </div>
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
                    </DialogEx >
                );
            }
        });


export default SystemMidiBindingDialog;