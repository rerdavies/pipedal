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

import React, { SyntheticEvent } from 'react';
import DialogEx from './DialogEx';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { PiPedalModel, PiPedalModelFactory, ListenHandle } from './PiPedalModel';
import Typography from '@mui/material/Typography';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import { createStyles } from './WithStyles';

import { withStyles } from "tss-react/mui";
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButtonEx from './IconButtonEx';
import MidiBinding from './MidiBinding';
import MidiBindingView from './MidiBindingView';
import Snackbar from '@mui/material/Snackbar';
import { PortGroup, UiPlugin, makeSplitUiPlugin } from './Lv2Plugin';
import { css } from '@emotion/react';

const styles = (theme: Theme) => createStyles({
    dialogAppBar: css({
        position: 'relative',
        top: 0, left: 0
    }),
    dialogTitle: css({
        marginLeft: theme.spacing(2),
        flex: "1 1 auto",
    }),
    pluginTable: css({
        border: "collapse",
        width: "100%",

    }),
    pluginHead: css({
        borderTop: "1pt #DDD solid", paddingLeft: 22, paddingRight: 22
    }),
    groupHead: css({
        borderBottom: "1px solid " + theme.palette.divider,
        marginLeft: 0,
        marginBottom: 0
    }),
    bindingTd: css({
        verticalAlign: "top",
        paddingLeft: 12, paddingBottom: 8
    }),
    nameTd: css({
        paddingLeft: 48,
        verticalAlign: "top",
        paddingTop: 12
    }),
});

function not_null<T>(value: T | null) {
    if (!value) throw Error("Unexpected null value");
    return value;
}

export interface MidiBindingDialogProps extends WithStyles<typeof styles> {
    open: boolean,
    onClose: () => void
}

export interface MidiBindingDialogState {
    listenInstanceId: number;
    listenSymbol: string;
    listenSnackbarOpen: boolean;

}

export const MidiBindingDialog =
    withStyles(
        class extends ResizeResponsiveComponent<MidiBindingDialogProps, MidiBindingDialogState> {

            model: PiPedalModel;

            constructor(props: MidiBindingDialogProps) {
                super(props);
                this.state = {
                    listenInstanceId: -2,
                    listenSymbol: "",
                    listenSnackbarOpen: false
                };
                this.model = PiPedalModelFactory.getInstance();
                this.handleClose = this.handleClose.bind(this);
            }
            mounted: boolean = false;

            hasHooks: boolean = false;

            handleClose() {
                this.cancelListenForControl();
                this.props.onClose();
            }

            listenTimeoutHandle?: number;

            listenHandle?: ListenHandle;

            cancelListenForControl() {
                if (this.listenTimeoutHandle) {
                    clearTimeout(this.listenTimeoutHandle);
                    this.listenTimeoutHandle = undefined;
                }
                if (this.listenHandle) {
                    this.model.cancelListenForMidiEvent(this.listenHandle)
                    this.listenHandle = undefined;
                }

                this.setState({ listenInstanceId: -2, listenSymbol: "" });

            }

            handleListenSucceeded(instanceId: number, symbol: string, isNote: boolean, noteOrControl: number) {
                this.cancelListenForControl();

                let pedalboard = this.model.pedalboard.get();
                let item = pedalboard.getItem(instanceId);
                if (!item) return;

                let binding = item.getMidiBinding(symbol);
                let newBinding = binding.clone();
                if (isNote) {
                    newBinding.bindingType = MidiBinding.BINDING_TYPE_NOTE;
                    newBinding.note = noteOrControl;
                } else {
                    newBinding.bindingType = MidiBinding.BINDING_TYPE_CONTROL;
                    newBinding.control = noteOrControl;
                }

                this.model.setMidiBinding(instanceId, newBinding);
            }


            handleListenForControl(instanceId: number, symbol: string, listenForControl: boolean): void {
                this.cancelListenForControl();
                this.setState({ listenInstanceId: instanceId, listenSymbol: symbol, listenSnackbarOpen: true });
                this.listenTimeoutHandle = setTimeout(() => {
                    this.cancelListenForControl();
                }, 8000);

                this.listenHandle = this.model.listenForMidiEvent(listenForControl,
                    (isNote: boolean, noteOrControl: number) => {
                        this.handleListenSucceeded(instanceId, symbol, isNote, noteOrControl);
                    });



            }

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
            handleItemChanged(instanceId: number, newBinding: MidiBinding) {
                this.model.setMidiBinding(instanceId, newBinding);
            }
            generateTable(): React.ReactNode[] {
                const classes = withStyles.getClasses(this.props);
                let result: React.ReactNode[] = [];

                let pedalboard = this.model.pedalboard.get();
                let iter = pedalboard.itemsGenerator();
                let keyIx = 1;
                while (true) {
                    let v = iter.next();
                    if (v.done) break;
                    let item = v.value;


                    let isSplit = item.uri === "uri://two-play/pipedal/pedalboard#Split";
                    let plugin: UiPlugin | null = this.model.getUiPlugin(item.uri);
                    if (plugin === null && isSplit) {
                        plugin = makeSplitUiPlugin();
                        let splitType = item.getControlValue("splitType");
                        not_null(plugin.getControl("splitType")).not_on_gui = true;
                        switch (splitType) {
                            case 0: // A/B
                                not_null(plugin.getControl("select")).not_on_gui = false;
                                not_null(plugin.getControl("mix")).not_on_gui = true;
                                not_null(plugin.getControl("volL")).not_on_gui = true;
                                not_null(plugin.getControl("panL")).not_on_gui = true;
                                not_null(plugin.getControl("volR")).not_on_gui = true;
                                not_null(plugin.getControl("panR")).not_on_gui = true;
                                break;
                            case 1: //mixer
                                not_null(plugin.getControl("select")).not_on_gui = true;
                                not_null(plugin.getControl("mix")).not_on_gui = false;
                                not_null(plugin.getControl("volL")).not_on_gui = true;
                                not_null(plugin.getControl("panL")).not_on_gui = true;
                                not_null(plugin.getControl("volR")).not_on_gui = true;
                                not_null(plugin.getControl("panR")).not_on_gui = true;
                                break;
                            case 2: // L/R
                                not_null(plugin.getControl("select")).not_on_gui = true;
                                not_null(plugin.getControl("mix")).not_on_gui = true;
                                not_null(plugin.getControl("volL")).not_on_gui = false;
                                not_null(plugin.getControl("panL")).not_on_gui = false;
                                not_null(plugin.getControl("volR")).not_on_gui = false;
                                not_null(plugin.getControl("panR")).not_on_gui = false;
                                break;
                        }

                        // xxx
                    }
                    if (plugin) {
                        result.push(
                            <tr key={"k" + keyIx++}>
                                <td colSpan={2} className={classes.pluginHead}>
                                    <Typography variant="caption" color="textSecondary" noWrap>
                                        {plugin.name}
                                    </Typography>
                                </td>
                            </tr>
                        );
                        if (!isSplit) {
                            result.push(
                                <tr key={"k" + keyIx++}>
                                    <td className={classes.nameTd}>
                                        <Typography noWrap style={{ verticalAlign: "center", height: 48 }}>
                                            Bypass
                                        </Typography>
                                    </td>
                                    <td className={classes.bindingTd}>
                                        <MidiBindingView instanceId={item.instanceId} midiBinding={item.getMidiBinding("__bypass")}
                                            uiPlugin={plugin}
                                            onListen={(instanceId: number, symbol: string, listenForControl: boolean) => {
                                                if (instanceId === -2) {
                                                    this.cancelListenForControl();
                                                } else {
                                                    this.handleListenForControl(instanceId, symbol, listenForControl);
                                                }
                                            }}
                                            listen={item.instanceId === this.state.listenInstanceId && this.state.listenSymbol === "__bypass"}
                                            onChange={(instanceId: number, newItem: MidiBinding) => this.handleItemChanged(instanceId, newItem)}
                                        />
                                    </td>
                                </tr>
                            );
                        }

                        let lastPortGroup: PortGroup | null = null;
                        for (let i = 0; i < plugin.controls.length; ++i) {
                            let control = plugin.controls[i];
                            if (!control.is_input) {
                                continue;
                            }
                            let portGroup = plugin.getPortGroupBySymbol(control.port_group);
                            if (portGroup != lastPortGroup) {
                                lastPortGroup = portGroup;
                                if (portGroup != null) {
                                    result.push(
                                        <tr key={"k" + keyIx++}>
                                            <td colSpan={2} className={classes.nameTd} >
                                                <div className={classes.groupHead}>
                                                    <Typography variant="caption" color="textSecondary" noWrap>
                                                        {portGroup?.name}
                                                    </Typography>
                                                </div>
                                            </td>
                                        </tr>
                                    );
                                }
                            }

                            let symbol = control.symbol;
                            result.push(
                                <tr key={"k" + keyIx++}>
                                    <td className={classes.nameTd}>
                                        <Typography noWrap style={{ verticalAlign: "middle", maxWidth: 120, marginLeft: portGroup ? 24:0 }} >
                                            {control.name}
                                        </Typography>
                                    </td>
                                    <td className={classes.bindingTd}>
                                        <MidiBindingView instanceId={item.instanceId}
                                            uiPlugin={plugin}
                                            midiBinding={item.getMidiBinding(symbol)}
                                            listen={item.instanceId === this.state.listenInstanceId && this.state.listenSymbol === symbol}
                                            onListen={(instanceId: number, symbol: string, listenForControl: boolean) => {
                                                this.handleListenForControl(instanceId, symbol, listenForControl);
                                            }}

                                            onChange={(instanceId: number, newItem: MidiBinding) => this.handleItemChanged(instanceId, newItem)}
                                        />
                                    </td>
                                </tr>
                            );
                        }
                    }

                }


                return result;
            }

            supressDefault(e: SyntheticEvent) {
                //e.preventDefault();
                //e.stopPropagation();
            }

            render() {
                let props = this.props;
                let { open } = props;
                const classes = withStyles.getClasses(props);
                if (!open) {
                    return (<div />);
                }

                return (
                    <DialogEx tag="midiBindings" open={open} fullWidth onClose={this.handleClose} aria-labelledby="Rename-dialog-title"
                        fullScreen={true}
                        style={{ userSelect: "none" }}
                        onEnterKey={() => { }}

                    >
                        <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                            <div style={{ flex: "0 0 auto" }}>
                                <AppBar className={classes.dialogAppBar} >
                                    <Toolbar>
                                        <IconButtonEx
                                            tooltip="Back"
                                            edge="start"
                                            color="inherit"
                                            onClick={this.handleClose}
                                            aria-label="back"
                                            size="large">
                                            <ArrowBackIcon />
                                        </IconButtonEx>
                                        <Typography noWrap variant="h6" className={classes.dialogTitle}>
                                            Preset MIDI Bindings
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
        },
        styles);


export default MidiBindingDialog;