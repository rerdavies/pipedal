import React, { SyntheticEvent } from 'react';
import Dialog from '@material-ui/core/Dialog';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { PiPedalModel, PiPedalModelFactory, ListenHandle } from './PiPedalModel';
import Typography from '@material-ui/core/Typography';
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';
import AppBar from '@material-ui/core/AppBar';
import Toolbar from '@material-ui/core/Toolbar';
import ArrowBackIcon from '@material-ui/icons/ArrowBack';
import IconButton from '@material-ui/core/IconButton';
import MidiBinding from './MidiBinding';
import MidiBindingView from './MidiBindingView';
import Snackbar from '@material-ui/core/Snackbar';

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
        paddingLeft: 48,
        verticalAlign: "top",
        paddingTop: 12
    },
});


export interface MidiBindingDialogProps extends WithStyles<typeof styles> {
    open: boolean,
    onClose: () => void
};

export interface MidiBindingDialogState {
    listenInstanceId: number;
    listenSymbol: string;
    listenSnackbarOpen: boolean;

};

export const MidiBindingDialog =
    withStyles(styles, { withTheme: true })(
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
                this.handlePopState = this.handlePopState.bind(this);
                this.handleClose = this.handleClose.bind(this);
            }
            mounted: boolean = false;

            hasHooks: boolean = false;

            stateWasPopped: boolean = false;
            handlePopState(e: any): any {
                this.stateWasPopped = true;
                let shouldClose = (!e.state);
                if (shouldClose) {
                    this.props.onClose();
                }
            }
            handleClose() {
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
                this.listenHandle = undefined; // (one-shot event)
                this.cancelListenForControl();
                
                let pedalBoard = this.model.pedalBoard.get();
                let item = pedalBoard.getItem(instanceId);
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

                this.model.setMidiBinding(instanceId,newBinding);
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

            updateHooks(): void {
                let wantHooks = this.mounted && this.props.open;
                if (wantHooks !== this.hasHooks) {
                    this.hasHooks = wantHooks;

                    if (this.hasHooks) {
                        this.stateWasPopped = false;
                        window.addEventListener("popstate", this.handlePopState);
                        // eslint-disable-next-line no-restricted-globals
                        let state = history.state;
                        if (!state) {
                            state = {};
                        }
                        state.midiBindingDialog = true;

                        // eslint-disable-next-line no-restricted-globals
                        history.pushState(
                            state,
                            "Preset MIDI Bindings",
                            "#MidiBindingDialog"
                        );
                    } else {
                        window.removeEventListener("popstate", this.handlePopState);
                        if (!this.stateWasPopped) {
                            // eslint-disable-next-line no-restricted-globals
                            history.back();
                        }
                        this.cancelListenForControl();

                    }

                }
            }
            onWindowSizeChanged(width: number, height: number): void {
            }


            componentDidMount() {
                super.componentDidMount();
                this.mounted = true;
                this.updateHooks();
            }
            componentWillUnmount() {
                super.componentDidMount();

                this.mounted = false;
                this.updateHooks();

            }

            componentDidUpdate() {
                this.updateHooks();
            }
            handleItemChanged(instanceId: number, newBinding: MidiBinding) {
                this.model.setMidiBinding(instanceId, newBinding);
            }
            generateTable(): React.ReactNode[] {
                let classes = this.props.classes;
                let result: React.ReactNode[] = [];

                let pedalboard = this.model.pedalBoard.get();
                let iter = pedalboard.itemsGenerator();
                while (true) {
                    let v = iter.next();
                    if (v.done) break;
                    let item = v.value;


                    let plugin = this.model.getUiPlugin(item.uri);
                    if (plugin) {
                        result.push(
                            <tr>
                                <td colSpan={2} className={classes.pluginHead}>
                                    <Typography variant="caption" color="textSecondary" noWrap>
                                        {plugin.name}
                                    </Typography>
                                </td>
                            </tr>
                        );
                        result.push(
                            <tr>
                                <td className={classes.nameTd}>
                                    <Typography noWrap style={{ verticalAlign: "center", height: 48 }}>
                                        Bypass
                                    </Typography>
                                </td>
                                <td className={classes.bindingTd}>
                                    <MidiBindingView instanceId={item.instanceId} midiBinding={item.getMidiBinding("__bypass")}
                                        onListen={(instanceId: number, symbol: string, listenForControl: boolean) => {
                                            if (instanceId === -2)
                                            {
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

                        for (let i = 0; i < plugin.controls.length; ++i) {
                            let control = plugin.controls[i];
                            let symbol = control.symbol;
                            result.push(
                                <tr>
                                    <td className={classes.nameTd}>
                                        <Typography noWrap style={{ verticalAlign: "middle", maxWidth: 120 }} >
                                            {control.name}
                                        </Typography>
                                    </td>
                                    <td className={classes.bindingTd}>
                                        <MidiBindingView instanceId={item.instanceId}
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
                let { open, classes } = props;
                if (!open) {
                    return (<div />);
                }

                return (
                    <Dialog open={open} fullWidth onClose={this.handleClose} aria-labelledby="Rename-dialog-title" 
                        fullScreen={true} 
                        style={{userSelect: "none"}}
                    >
                            <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                                <div style={{ flex: "0 0 auto" }}>
                                    <AppBar className={classes.dialogAppBar} >
                                        <Toolbar>
                                            <IconButton edge="start" color="inherit" onClick={this.handleClose} aria-label="back"
                                            >
                                                <ArrowBackIcon />
                                            </IconButton>
                                            <Typography variant="h6" className={classes.dialogTitle}>
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
                    </Dialog >
                );
            }
        });


export default MidiBindingDialog;