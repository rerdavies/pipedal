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

import React, { SyntheticEvent, Component } from 'react';
import IconButton from '@mui/material/IconButton';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, State } from './PiPedalModel';
import ButtonBase from "@mui/material/ButtonBase";
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import ArrowBackIcon from '@mui/icons-material//ArrowBack';
import JackConfiguration, { JackChannelSelection } from './Jack';
import Divider from '@mui/material/Divider';
import SelectChannelsDialog from './SelectChannelsDialog';
import SelectMidiChannelsDialog from './SelectMidiChannelsDialog';
import SelectHoverBackground from './SelectHoverBackground';
import JackServerSettings from './JackServerSettings';
import JackServerSettingsDialog from './JackServerSettingsDialog';
import JackHostStatus from './JackHostStatus';
import WifiConfigSettings from './WifiConfigSettings';
import WifiConfigDialog from './WifiConfigDialog';
import DialogEx from './DialogEx'

import Slide, {SlideProps} from '@mui/material/Slide';
import { createStyles, Theme } from '@mui/material/styles';
import { WithStyles, withStyles} from '@mui/styles';



interface SettingsDialogProps extends WithStyles<typeof styles> {
    open: boolean;
    onClose: () => void;


};

interface SettingsDialogState {
    jackConfiguration: JackConfiguration;
    jackSettings: JackChannelSelection;
    jackServerSettings: JackServerSettings;
    jackStatus?: JackHostStatus;

    wifiConfigSettings: WifiConfigSettings;

    showWifiConfigDialog: boolean;
    showInputSelectDialog: boolean;
    showOutputSelectDialog: boolean;
    showMidiSelectDialog: boolean;
    showJackServerSettingsDialog: boolean;
    shuttingDown: boolean;
    restarting: boolean;
};


const styles = (theme: Theme) => createStyles({
    dialogAppBar: {
        position: 'relative',
        top: 0, left: 0
    },
    dialogTitle: {
        marginLeft: theme.spacing(2),
        flex: 1,
    },
    sectionHead: {
        marginLeft: 24,
        marginRight: 24,
        marginTop: 16,
        paddingBottom: 12

    },
    textBlock: {
        marginLeft: 24,
        marginRight: 24,
        marginTop: 16,
        paddingBottom: 0,
        opacity: 0.95

    },
    textBlockIndented: {
        marginLeft: 40,
        marginRight: 24,
        marginTop: 16,
        paddingBottom: 0,
        opacity: 0.95

    },

    setting: {
        minHeight: 64,
        width: "100%",
        textAlign: "left",
        paddingLeft: 22,
        paddingRight: 22
    },
    primaryItem: {

    },
    secondaryItem: {

    }


});


const Transition = React.forwardRef(function Transition(
    props: SlideProps, ref: React.Ref<unknown>
) {
    return (<Slide direction="up" ref={ref} {...props} />);
});



const SettingsDialog = withStyles(styles, { withTheme: true })(

    class extends Component<SettingsDialogProps, SettingsDialogState> {

        model: PiPedalModel;



        constructor(props: SettingsDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();

            this.handleDialogClose = this.handleDialogClose.bind(this);
            this.state = {
                jackServerSettings: this.model.jackServerSettings.get(),
                jackConfiguration: this.model.jackConfiguration.get(),
                jackStatus: undefined,
                jackSettings: this.model.jackSettings.get(),
                wifiConfigSettings: this.model.wifiConfigSettings.get(),
                showWifiConfigDialog: false,
                showInputSelectDialog: false,
                showOutputSelectDialog: false,
                showMidiSelectDialog: false,
                showJackServerSettingsDialog: false,
                shuttingDown: false,
                restarting: false


            };
            this.handleJackConfigurationChanged = this.handleJackConfigurationChanged.bind(this);
            this.handleJackSettingsChanged = this.handleJackSettingsChanged.bind(this);
            this.handleJackServerSettingsChanged = this.handleJackServerSettingsChanged.bind(this);
            this.handleWifiConfigSettingsChanged = this.handleWifiConfigSettingsChanged.bind(this);
            this.handleConnectionStateChanged = this.handleConnectionStateChanged.bind(this);

        }

        handleConnectionStateChanged(): void {
            if (this.model.state.get() === State.Ready) {
                if (this.state.shuttingDown || this.state.restarting) {
                    this.setState({ shuttingDown: false, restarting: false });
                }
            }
        }

        handleApplyWifiConfig(wifiConfigSettings: WifiConfigSettings): void {
            this.setState({ showWifiConfigDialog: false });
            this.model.setWifiConfigSettings(wifiConfigSettings)
                .then(() => {

                })
                .catch((err) => {
                    this.model.showAlert(err);
                });
        }

        handleWifiConfigSettingsChanged(): void {
            this.setState(
                {
                    wifiConfigSettings: this.model.wifiConfigSettings.get()
                }
            )
        }
        handleJackSettingsChanged(): void {
            this.setState({
                jackSettings: this.model.jackSettings.get()
            });
        }
        handleJackServerSettingsChanged(): void {
            this.setState({
                jackServerSettings: this.model.jackServerSettings.get()
            });
        }

        handleJackConfigurationChanged(): void {
            this.setState({
                jackConfiguration: this.model.jackConfiguration.get()
            });
        }

        mounted: boolean = false;
        active: boolean = false;

        timerHandle?: NodeJS.Timeout;

        tick() {
            this.model.getJackStatus()
                .then((jackStatus) =>
                    this.setState(
                        {
                            jackStatus: jackStatus
                        })
                )
                .catch((error) => { });
        }

        updateActive() {
            let active = this.mounted && this.props.open;
            if (active !== this.active) {
                this.active = active;
                if (active) {
                    this.model.state.addOnChangedHandler(this.handleConnectionStateChanged);
                    this.model.jackSettings.addOnChangedHandler(this.handleJackSettingsChanged);
                    this.model.jackConfiguration.addOnChangedHandler(this.handleJackConfigurationChanged);
                    this.model.jackServerSettings.addOnChangedHandler(this.handleJackServerSettingsChanged);
                    this.model.wifiConfigSettings.addOnChangedHandler(this.handleWifiConfigSettingsChanged);
                    this.model.getJackStatus()
                        .then((jackStatus) =>
                            this.setState(
                                {
                                    jackStatus: jackStatus
                                })
                        )
                        .catch((error) => {
                            this.setState(
                                {
                                    jackStatus: undefined
                                })
                        });
                    this.handleConnectionStateChanged();
                    this.handleJackConfigurationChanged();
                    this.handleJackSettingsChanged();
                    this.handleJackServerSettingsChanged();
                    this.handleWifiConfigSettingsChanged();
                    this.timerHandle = setInterval(() => this.tick(), 1000);
                } else {
                    if (this.timerHandle) {
                        clearInterval(this.timerHandle);
                    }
                    this.model.state.removeOnChangedHandler(this.handleConnectionStateChanged);
                    this.model.jackConfiguration.removeOnChangedHandler(this.handleJackConfigurationChanged);
                    this.model.jackSettings.removeOnChangedHandler(this.handleJackSettingsChanged);
                    this.model.jackServerSettings.removeOnChangedHandler(this.handleJackServerSettingsChanged);
                    this.model.wifiConfigSettings.removeOnChangedHandler(this.handleWifiConfigSettingsChanged);

                }
            }

        }

        componentDidMount() {
            this.mounted = true;
            this.updateActive();
            // scroll selected item into view.
        }
        componentDidUpdate() {
            this.updateActive();
        }
        componentWillUnmount() {
            this.mounted = false;
            this.updateActive();
        }

        handleDialogClose(e: SyntheticEvent) {
            this.props.onClose();
        }

        handleMidiSelection() {
            this.setState({
                showMidiSelectDialog: true
            })
        }
        handleJackServerSettings() {
            this.setState({
                showJackServerSettingsDialog: true,
            });
        }

        handleInputSelection() {
            this.setState({
                showInputSelectDialog: true,
                showOutputSelectDialog: false
            });
        }

        handleOutputSelection() {
            this.setState({
                showInputSelectDialog: false,
                showOutputSelectDialog: true
            });

        }
        handleSelectMidiDialogResult(channels: string[] | null): void {
            if (channels) {
                let newSelection = this.state.jackSettings.clone();
                newSelection.inputMidiPorts = channels;
                this.model.setJackSettings(newSelection);
            }
            this.setState({
                showMidiSelectDialog: false,

            });

        }

        handleSelectChannelsDialogResult(channels: string[] | null): void {
            if (channels) {
                let newSelection = this.state.jackSettings.clone();
                if (this.state.showInputSelectDialog) {
                    newSelection.inputAudioPorts = channels;
                } else {
                    newSelection.outputAudioPorts = channels;
                }
                this.setState({
                    jackSettings: newSelection,
                    showInputSelectDialog: false,
                    showOutputSelectDialog: false

                });
                this.model.setJackSettings(newSelection);
            } else {
                this.setState({
                    showInputSelectDialog: false,
                    showOutputSelectDialog: false

                });

            }
        }

        midiSummary(): string {
            let ports = this.state.jackSettings.inputMidiPorts;
            if (ports.length === 0) return "Disabled";
            if (ports.length === 1) return ports[0];
            return ports.length + " channels";
        }
        handleShowWifiConfigDialog() {
            this.setState({
                showWifiConfigDialog: true
            });
        }
        handleRestart() {
            this.setState({ restarting: true });
            this.model.restart()
                .then(() => {
                    // this.setState({ restarting: true });
                })
                .catch((error) => {
                    this.model.showAlert(error);
                    this.setState({ restarting: false });
                });

        }

        handleShutdown() {
            this.setState({ shuttingDown: true });
            this.model.shutdown()
                .then(() => {
                    this.setState({ shuttingDown: true });
                })
                .catch((error) => {
                    this.model.showAlert(error);
                    this.setState({ shuttingDown: false });
                });

        }

        render() {
            let classes = this.props.classes;
            let isConfigValid = this.state.jackConfiguration.isValid;
            let selectedChannels: string[] = this.state.showInputSelectDialog ? this.state.jackSettings.inputAudioPorts : this.state.jackSettings.outputAudioPorts;
            let disableShutdown = this.state.shuttingDown || this.state.restarting;

            return (
                <DialogEx tag="SettingsDialog" fullScreen open={this.props.open}
                    onClose={() => { this.props.onClose() }} TransitionComponent={Transition}
                    style={{ userSelect: "none" }}
                >

                    <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                        <div style={{ flex: "0 0 auto" }}>
                            <AppBar className={classes.dialogAppBar} >
                                <Toolbar>
                                    <IconButton edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                    >
                                        <ArrowBackIcon />
                                    </IconButton>
                                    <Typography variant="h6" className={classes.dialogTitle}>
                                        Settings
                                    </Typography>
                                </Toolbar>
                            </AppBar>
                        </div>
                        <div style={{
                            flex: "1 1 auto", position: "relative", overflow: "hidden",
                            overflowX: "hidden", overflowY: "auto", marginTop: 22
                        }}
                        >
                            <div >
                                <Typography className={classes.sectionHead} display="block" variant="caption" color="secondary">
                                    AUDIO
                                </Typography>
                                {(!isConfigValid) ?
                                    (
                                        <div style={{ paddingLeft: 48, position: "relative", top: -12 }}>
                                            <Typography display="block" variant="caption" color="textSecondary">Status: <span style={{ color: "#F00" }}>Not configured.</span></Typography>
                                            <Typography display="block" variant="caption" color="textSecondary">CPU: </Typography>
                                        </div>
                                    ) :
                                    (
                                        <div style={{ paddingLeft: 48, position: "relative", top: -12 }}>
                                            {JackHostStatus.getDisplayView("Status:\u00A0", this.state.jackStatus)}
                                            {JackHostStatus.getCpuInfo("CPU:\u00A0", this.state.jackStatus)}
                                        </div>
                                    )
                                }

                                {this.state.jackConfiguration.errorState !== "" &&
                                    (
                                        <div style={{ paddingLeft: 48, position: "relative", top: -12 }}>
                                            <Typography variant="caption" color="textSecondary"><span style={{ color: "#F00" }}>{this.state.jackConfiguration.errorState}</span></Typography>
                                        </div>

                                    )
                                }
                                <ButtonBase className={classes.setting} onClick={() => this.handleJackServerSettings()}
                                >
                                    <SelectHoverBackground selected={false} showHover={true} />
                                    <div style={{ width: "100%" }}>
                                        <Typography display="block" variant="body2" noWrap>Audio device</Typography>
                                        <Typography display="block" variant="caption" noWrap color="textSecondary">{this.state.jackServerSettings.getSummaryText()}</Typography>
                                    </div>
                                </ButtonBase>
                                <JackServerSettingsDialog
                                    open={this.state.showJackServerSettingsDialog}
                                    jackServerSettings={this.state.jackServerSettings}
                                    onClose={() => this.setState({ showJackServerSettingsDialog: false })}
                                    onApply={(jackServerSettings) => {
                                        this.setState({
                                            showJackServerSettingsDialog: false,
                                            jackServerSettings: jackServerSettings
                                        });
                                        this.model.setJackServerSettings(jackServerSettings);
                                    }}
                                />



                                <ButtonBase className={classes.setting} onClick={() => this.handleInputSelection()} disabled={!isConfigValid}
                                    style={{ opacity: !isConfigValid ? 0.6 : 1.0 }}

                                >
                                    <SelectHoverBackground selected={false} showHover={true} />
                                    <div style={{ width: "100%" }}>
                                        <Typography display="block" variant="body2" noWrap>Input channels</Typography>
                                        <Typography display="block" variant="caption" color="textSecondary" noWrap>{this.state.jackSettings.getAudioInputDisplayValue(this.state.jackConfiguration)}</Typography>
                                    </div>
                                </ButtonBase>
                                <ButtonBase className={classes.setting} onClick={() => this.handleOutputSelection()} disabled={!isConfigValid}
                                    style={{ opacity: !isConfigValid ? 0.6 : 1.0 }}
                                >
                                    <SelectHoverBackground selected={false} showHover={true} />
                                    <div style={{ width: "100%" }}>
                                        <Typography display="block" variant="body2" noWrap>Output channels</Typography>
                                        <Typography display="block" variant="caption" color="textSecondary" noWrap>{this.state.jackSettings.getAudioOutputDisplayValue(this.state.jackConfiguration)}</Typography>
                                    </div>
                                </ButtonBase>
                                <Divider />
                                <div >
                                    <Typography className={classes.sectionHead} display="block" variant="caption" color="secondary">MIDI</Typography>
                                    <ButtonBase className={classes.setting} disabled={!isConfigValid} onClick={() => this.handleMidiSelection()}  >
                                        <SelectHoverBackground selected={false} showHover={true} />
                                        <div style={{ width: "100%" }}>
                                            <Typography className={classes.primaryItem} display="block" variant="body2" noWrap>Select MIDI input channels</Typography>

                                            <Typography className={classes.secondaryItem} display="block" variant="caption" color="textSecondary" noWrap>{this.midiSummary()}</Typography>
                                        </div>
                                    </ButtonBase>
                                </div>
                            </div>
                            <Divider />
                            <div >
                                <Typography className={classes.sectionHead} display="block" variant="caption" color="secondary">SYSTEM</Typography>
                                <ButtonBase className={classes.setting} disabled={!this.state.wifiConfigSettings.valid}
                                    onClick={() => this.handleShowWifiConfigDialog()}  >
                                    <SelectHoverBackground selected={false} showHover={true} />
                                    <div style={{ width: "100%" }}>
                                        <Typography className={classes.primaryItem} display="block" variant="body2" color="textPrimary" noWrap>
                                            Configure Wi-Fi hotspot</Typography>
                                        <Typography display="block" variant="caption" noWrap color="textSecondary">
                                            {this.state.wifiConfigSettings.getSummaryText()}
                                        </Typography>

                                    </div>
                                </ButtonBase>
                                <ButtonBase className={classes.setting} disabled={!isConfigValid || disableShutdown}
                                    onClick={() => this.handleRestart()}  >
                                    <SelectHoverBackground selected={false} showHover={true} />
                                    <div style={{ width: "100%" }}>
                                        {
                                            this.state.restarting ? (
                                                <Typography className={classes.primaryItem} display="block" variant="body2" color="textSecondary" noWrap>Rebooting...</Typography>
                                            ) : (
                                                <Typography className={classes.primaryItem} display="block" variant="body2" noWrap>Reboot</Typography>
                                            )
                                        }
                                    </div>
                                </ButtonBase>

                                <ButtonBase className={classes.setting} disabled={!isConfigValid || disableShutdown}
                                    onClick={() => this.handleShutdown()}  >
                                    <SelectHoverBackground selected={false} showHover={true} />
                                    <div style={{ width: "100%" }}>
                                        {
                                            this.state.shuttingDown ? (
                                                <Typography className={classes.primaryItem} display="block" variant="body2" color="textSecondary" noWrap>Shutting down...</Typography>
                                            ) : (
                                                <Typography className={classes.primaryItem} display="block" variant="body2" noWrap>Shut down</Typography>
                                            )
                                        }
                                    </div >
                                </ButtonBase>
                            </div>



                        </div>
                    </div>
                    {
                        (this.state.showInputSelectDialog || this.state.showOutputSelectDialog) &&
                        (
                            <SelectChannelsDialog open={this.state.showInputSelectDialog || this.state.showOutputSelectDialog}
                                onClose={(selectedChannels: string[] | null) => this.handleSelectChannelsDialogResult(selectedChannels)}
                                selectedChannels={selectedChannels}
                                availableChannels={this.state.showInputSelectDialog ? this.state.jackConfiguration.inputAudioPorts : this.state.jackConfiguration.outputAudioPorts}
                            />

                        )
                    }
                    {
                        (this.state.showMidiSelectDialog) &&
                        (
                            <SelectMidiChannelsDialog open={this.state.showMidiSelectDialog}
                                onClose={(selectedChannels: string[] | null) => this.handleSelectMidiDialogResult(selectedChannels)}
                                selectedChannels={this.state.jackSettings.inputMidiPorts}
                                availableChannels={this.state.jackConfiguration.inputMidiPorts}
                            />

                        )
                    }
                    <WifiConfigDialog wifiConfigSettings={this.state.wifiConfigSettings} open={this.state.showWifiConfigDialog}
                        onClose={() => this.setState({ showWifiConfigDialog: false })}
                        onOk={(wifiConfigSettings) => this.handleApplyWifiConfig(wifiConfigSettings)}
                    />

                </DialogEx >

            );

        }

    }

);

export default SettingsDialog;