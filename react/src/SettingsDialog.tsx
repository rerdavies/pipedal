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
import Switch from "@mui/material/Switch";
import OkCancelDialog from './OkCancelDialog';
import ListSelectDialog from './ListSelectDialog';
import IconButton from '@mui/material/IconButton';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, State } from './PiPedalModel';
import { ColorTheme } from './DarkMode';
import ButtonBase from "@mui/material/ButtonBase";
import AppBar from '@mui/material/AppBar';
import Button from '@mui/material/Button';
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
import WifiDirectConfigSettings from './WifiDirectConfigSettings';
import WifiConfigDialog from './WifiConfigDialog';
import WifiDirectConfigDialog from './WifiDirectConfigDialog';
import DialogEx from './DialogEx'
import GovernorSettings from './GovernorSettings';
import { AlsaMidiDeviceInfo } from './AlsaMidiDeviceInfo';
import SystemMidiBindingsDialog from './SystemMidiBindingsDialog';
import SelectThemeDialog from './SelectThemeDialog';

import Slide, { SlideProps } from '@mui/material/Slide';
import { createStyles, Theme } from '@mui/material/styles';
import { WithStyles, withStyles } from '@mui/styles';



interface SettingsDialogProps extends WithStyles<typeof styles> {
    open: boolean;
    onboarding: boolean;
    onClose: () => void;


};

interface SettingsDialogState {
    showStatusMonitor: boolean;
    showStatusMonitorDialog: boolean;
    jackConfiguration: JackConfiguration;
    jackSettings: JackChannelSelection;
    jackServerSettings: JackServerSettings;
    jackStatus?: JackHostStatus;
    governorSettings: GovernorSettings;
    continueDisabled: boolean;

    wifiConfigSettings: WifiConfigSettings;
    wifiDirectConfigSettings: WifiDirectConfigSettings;

    showWifiConfigDialog: boolean;
    showWifiDirectConfigDialog: boolean;
    showGovernorSettingsDialog: boolean;
    showInputSelectDialog: boolean;
    showOutputSelectDialog: boolean;
    showMidiSelectDialog: boolean;
    showThemeSelectDialog: boolean;
    showJackServerSettingsDialog: boolean;
    shuttingDown: boolean;
    restarting: boolean;
    isAndroidHosted: boolean;
    showRestartOkDialog: boolean;
    showShutdownOkDialog: boolean;
    showSystemMidiBindingsDialog: boolean;
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
    cpuStatusColor: {
        color: theme.palette.text.secondary,
        opacity: 0.7

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
                showStatusMonitor: this.model.showStatusMonitor.get(),
                showStatusMonitorDialog: false,

                jackServerSettings: this.model.jackServerSettings.get(),
                jackConfiguration: this.model.jackConfiguration.get(),
                jackStatus: undefined,
                jackSettings: this.model.jackSettings.get(),
                wifiConfigSettings: this.model.wifiConfigSettings.get(),
                wifiDirectConfigSettings: this.model.wifiDirectConfigSettings.get(),
                governorSettings: this.model.governorSettings.get(),
                continueDisabled: true,
                showWifiConfigDialog: false,
                showWifiDirectConfigDialog: false,
                showGovernorSettingsDialog: false,
                showInputSelectDialog: false,
                showOutputSelectDialog: false,
                showMidiSelectDialog: false,
                showThemeSelectDialog: false,
                showJackServerSettingsDialog: false,
                shuttingDown: false,
                restarting: false,
                showShutdownOkDialog: false,
                showRestartOkDialog: false,
                showSystemMidiBindingsDialog: false,
                isAndroidHosted: this.model.isAndroidHosted()



            };
            this.handleJackConfigurationChanged = this.handleJackConfigurationChanged.bind(this);
            this.handleJackSettingsChanged = this.handleJackSettingsChanged.bind(this);
            this.handleJackServerSettingsChanged = this.handleJackServerSettingsChanged.bind(this);
            this.handleWifiConfigSettingsChanged = this.handleWifiConfigSettingsChanged.bind(this);
            this.handleWifiDirectConfigSettingsChanged = this.handleWifiDirectConfigSettingsChanged.bind(this);
            this.handleGovernorSettingsChanged = this.handleGovernorSettingsChanged.bind(this);
            this.handleConnectionStateChanged = this.handleConnectionStateChanged.bind(this);
            this.handleShowStatusMonitorChanged = this.handleShowStatusMonitorChanged.bind(this);


        }


        handleShowStatusMonitorChanged(): void {
            this.setState({ showStatusMonitor: this.model.showStatusMonitor.get() });
        }
        handleConnectionStateChanged(): void {
            if (this.model.state.get() === State.Ready) {
                this.setState({ isAndroidHosted: this.model.isAndroidHosted() });
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

        handleApplyWifiDirectConfig(wifiDirectConfigSettings: WifiDirectConfigSettings): void {
            this.setState({ showWifiDirectConfigDialog: false });
            this.model.setWifiDirectConfigSettings(wifiDirectConfigSettings)
                .then(() => {

                })
                .catch((err) => {
                    this.model.showAlert(err);
                });
        }

        handleApplyGovernorSettings(governor: string): void {
            this.model.setGovernorSettings(governor)
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
        handleWifiDirectConfigSettingsChanged(): void {
            this.setState(
                {
                    wifiDirectConfigSettings: this.model.wifiDirectConfigSettings.get()
                }
            )
        }

        handleGovernorSettingsChanged(): void {
            this.setState(
                {
                    governorSettings: this.model.governorSettings.get()
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
                jackServerSettings: this.model.jackServerSettings.get(),
                continueDisabled: !this.model.jackServerSettings.get().valid
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
                    this.model.showStatusMonitor.addOnChangedHandler(this.handleShowStatusMonitorChanged);
                    this.model.jackSettings.addOnChangedHandler(this.handleJackSettingsChanged);
                    this.model.jackConfiguration.addOnChangedHandler(this.handleJackConfigurationChanged);
                    this.model.jackServerSettings.addOnChangedHandler(this.handleJackServerSettingsChanged);
                    this.model.wifiConfigSettings.addOnChangedHandler(this.handleWifiConfigSettingsChanged);
                    this.model.wifiDirectConfigSettings.addOnChangedHandler(this.handleWifiDirectConfigSettingsChanged);
                    this.model.governorSettings.addOnChangedHandler(this.handleGovernorSettingsChanged);
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
                    this.handleShowStatusMonitorChanged();
                    this.handleJackServerSettingsChanged();
                    this.handleWifiConfigSettingsChanged();
                    this.handleWifiDirectConfigSettingsChanged();
                    this.timerHandle = setInterval(() => this.tick(), 1000);
                } else {
                    if (this.timerHandle) {
                        clearInterval(this.timerHandle);
                    }
                    this.model.state.removeOnChangedHandler(this.handleConnectionStateChanged);
                    this.model.showStatusMonitor.removeOnChangedHandler(this.handleShowStatusMonitorChanged);

                    this.model.jackConfiguration.removeOnChangedHandler(this.handleJackConfigurationChanged);
                    this.model.jackSettings.removeOnChangedHandler(this.handleJackSettingsChanged);
                    this.model.jackServerSettings.removeOnChangedHandler(this.handleJackServerSettingsChanged);
                    this.model.wifiConfigSettings.removeOnChangedHandler(this.handleWifiConfigSettingsChanged);
                    this.model.wifiDirectConfigSettings.removeOnChangedHandler(this.handleWifiDirectConfigSettingsChanged);
                    this.model.governorSettings.removeOnChangedHandler(this.handleGovernorSettingsChanged);

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
        handleThemeSelection() {
            this.setState({
                showThemeSelectDialog: true
            });
        }

        handleCheckForUpdates() {
            this.model.showUpdateDialog();
        }

        handleMidiMessageSettings() {
            this.setState({
                showSystemMidiBindingsDialog: true
            });

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
        handleSelectMidiDialogResult(channels: AlsaMidiDeviceInfo[] | null): void {
            if (channels) {
                let newSelection: JackChannelSelection = this.state.jackSettings.clone();
                newSelection.inputMidiDevices = channels;
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
            let ports = this.state.jackSettings.inputMidiDevices;
            if (ports.length === 0) return "Disabled";
            if (ports.length === 1) return ports[0].description;
            return ports.length + " channels";
        }
        handleShowWifiConfigDialog() {
            this.setState({
                showWifiConfigDialog: true
            });
        }
        handleShowWifiDirectConfigDialog() {
            this.setState({
                showWifiDirectConfigDialog: true
            });
        }
        handleShowGovernorSettingsDialogDialog() {
            this.setState({
                showGovernorSettingsDialog: true
            });
        }
        handleRestart() {
            this.setState({ showRestartOkDialog: true });
        }
        handleRestartOk() {
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
            this.setState({ showShutdownOkDialog: true });
        }
        handleShutdownOk() {
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

        onOnboardingContinue() {
            this.model.setOnboarding(false);
            this.props.onClose();
        }
        render() {
            let classes = this.props.classes;
            let isConfigValid = this.state.jackConfiguration.isValid;
            let selectedChannels: string[] = this.state.showInputSelectDialog ? this.state.jackSettings.inputAudioPorts : this.state.jackSettings.outputAudioPorts;
            let disableShutdown = this.state.shuttingDown || this.state.restarting;

            return (
                <DialogEx tag="settings" fullScreen open={this.props.open}
                    onClose={() => { this.props.onClose() }} TransitionComponent={Transition}
                    style={{ userSelect: "none" }}
                >

                    <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                        <div style={{ flex: "0 0 auto" }}>
                            <AppBar className={classes.dialogAppBar} >
                                {
                                    this.props.onboarding ?
                                        (
                                            <Toolbar>
                                                <Typography variant="h6" className={classes.dialogTitle}>
                                                    Initial Configuration
                                                </Typography>
                                            </Toolbar>

                                        )
                                        :
                                        (
                                            <Toolbar>
                                                <IconButton edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                                >
                                                    <ArrowBackIcon />
                                                </IconButton>
                                                <Typography variant="h6" className={classes.dialogTitle}>
                                                    Settings
                                                </Typography>
                                            </Toolbar>

                                        )
                                }
                            </AppBar>
                        </div>
                        <div style={{
                            flex: "1 1 auto", position: "relative", overflow: "hidden",
                            overflowX: "hidden", overflowY: "auto", marginTop: 22
                        }}
                        >
                            <div >
                                {this.props.onboarding &&
                                    (
                                        <div>
                                            <Typography display="block" variant="body1" color="textSecondary" style={{ paddingLeft: 24, paddingBottom: 8 }}>
                                                Select and configure an audio device. You may optionally configure MIDI inputs, and set up a Wi-Fi Direct Hotspot now as well.
                                            </Typography>
                                            <Typography display="block" variant="body1" color="textSecondary" style={{ paddingLeft: 24, paddingBottom: 8 }}>
                                                Access and modify these settings later by selecting the <i>Settings</i> menu item on the main menu.
                                            </Typography>
                                            <Divider />
                                        </div>
                                    )}

                                <div>
                                    <Typography className={classes.sectionHead} display="block" variant="caption" color="secondary">
                                        STATUS
                                    </Typography>
                                    {(!isConfigValid) ?
                                        (
                                            <div className={classes.cpuStatusColor} style={{ paddingLeft: 48, position: "relative", top: -12 }}>
                                                <Typography display="block" variant="caption" color="textSecondary">Status: <span style={{ color: "#F00" }}>Not configured.</span></Typography>
                                                {(!this.props.onboarding) && (
                                                    <Typography display="block" variant="caption" color="inherit">Governor: </Typography>
                                                )}
                                            </div>
                                        ) :
                                        (
                                            <div className={classes.cpuStatusColor} style={{ paddingLeft: 48, position: "relative", top: -12 }}>
                                                {JackHostStatus.getDisplayView("", this.state.jackStatus)}
                                                {(!this.props.onboarding) && JackHostStatus.getCpuInfo("Governor:\u00A0", this.state.jackStatus)}
                                            </div>
                                        )
                                    }
                                </div>

                                {this.state.jackConfiguration.errorState !== "" &&
                                    (
                                        <div style={{ paddingLeft: 48, position: "relative", top: -12 }}>
                                            <Typography variant="caption" color="textSecondary"><span style={{ color: "#F00" }}>{this.state.jackConfiguration.errorState}</span></Typography>
                                        </div>

                                    )
                                }
                                <Divider />
                                <Typography className={classes.sectionHead} display="block" variant="caption" color="secondary">
                                    AUDIO
                                </Typography>
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
                                            <Typography className={classes.primaryItem} display="block" variant="body2" noWrap>Select MIDI input</Typography>

                                            <Typography className={classes.secondaryItem} display="block" variant="caption" color="textSecondary" noWrap>{this.midiSummary()}</Typography>
                                        </div>
                                    </ButtonBase>
                                    <ButtonBase className={classes.setting} disabled={!isConfigValid} onClick={() => this.handleMidiMessageSettings()}  >
                                        <SelectHoverBackground selected={false} showHover={true} />
                                        <div style={{ width: "100%" }}>
                                            <Typography className={classes.primaryItem} display="block" variant="body2" noWrap>System MIDI bindings</Typography>

                                        </div>
                                    </ButtonBase>

                                </div>
                            </div>
                            <Divider />
                            <div >
                                <Typography className={classes.sectionHead} display="block" variant="caption" color="secondary">CONNECTION</Typography>

                                {
                                    this.state.isAndroidHosted &&
                                    (
                                        <ButtonBase className={classes.setting} disabled={!this.state.wifiConfigSettings.valid}
                                            onClick={() => this.model.chooseNewDevice()}  >
                                            <SelectHoverBackground selected={false} showHover={true} />
                                            <div style={{ width: "100%" }}>
                                                <Typography className={classes.primaryItem} display="block" variant="body2" color="textPrimary" noWrap>
                                                    Connect to a different device</Typography>
                                                <Typography display="block" variant="caption" noWrap color="textSecondary">

                                                </Typography>

                                            </div>
                                        </ButtonBase>
                                    )
                                }

                                <ButtonBase
                                    className={classes.setting} disabled={!this.state.wifiConfigSettings.valid}
                                    onClick={() => this.handleShowWifiConfigDialog()}  >
                                    <SelectHoverBackground selected={false} showHover={true} />
                                    <div style={{ width: "100%" }}>
                                        <Typography className={classes.primaryItem} display="block" variant="body2" color="textPrimary" noWrap>
                                            Wi-Fi auto hotspot</Typography>
                                        <Typography display="block" variant="caption" noWrap color="textSecondary">
                                            {this.state.wifiConfigSettings.getSummaryText()}
                                        </Typography>

                                    </div>
                                </ButtonBase>

                                <ButtonBase className={classes.setting} disabled={!this.state.wifiConfigSettings.valid}
                                    style={{ display: "none" }}
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
                            </div>
                            {(!this.props.onboarding) ? (
                                <div >
                                    <Divider />
                                    <Typography className={classes.sectionHead} display="block" variant="caption" color="secondary">SYSTEM</Typography>

                                    <ButtonBase
                                        style={{ display: "none" }}
                                        className={classes.setting} disabled={!this.state.wifiConfigSettings.valid}
                                        onClick={() => this.handleShowGovernorSettingsDialogDialog()}  >
                                        <SelectHoverBackground selected={false} showHover={true} />
                                        <div style={{ width: "100%" }}>
                                            <Typography className={classes.primaryItem} display="block" variant="body2" color="textPrimary" noWrap>
                                                CPU Governor</Typography>
                                            <Typography display="block" variant="caption" noWrap color="textSecondary">
                                                {this.state.governorSettings.governor}
                                            </Typography>

                                        </div>
                                    </ButtonBase>

                                    <ButtonBase
                                        className={classes.setting}
                                        onClick={() => { this.handleThemeSelection(); }}  >
                                        <SelectHoverBackground selected={false} showHover={true} />
                                        <div style={{ width: "100%" }}>
                                            <Typography className={classes.primaryItem} display="block" variant="body2" color="textPrimary" noWrap>
                                                Color theme</Typography>
                                            <Typography className={classes.secondaryItem} display="block" variant="caption" color="textSecondary" noWrap>
                                                {this.model.getTheme() === ColorTheme.Dark ? "Dark" :
                                                    (this.model.getTheme() === ColorTheme.Light ? "Light" : "System")}
                                            </Typography>
                                        </div>
                                    </ButtonBase>


                                    <ButtonBase
                                        className={classes.setting}
                                        onClick={() => {
                                            this.model.setShowStatusMonitor(!this.state.showStatusMonitor)
                                        }}  >
                                        <SelectHoverBackground selected={false} showHover={true} />
                                        <div style={{ width: "100%" }}>
                                            <div style={{
                                                width: "100%", display: "flex", flexDirection: "row", flexWrap: "nowrap",
                                                alignItems: "center", maxWidth: 400
                                            }}>
                                                <div style={{ flex: "1 1 auto" }}>
                                                    <Typography className={classes.primaryItem} display="block" variant="body2" color="textPrimary" noWrap>
                                                        Show status monitor on main screen.</Typography>
                                                </div>

                                                <div style={{ flex: "0 0 auto" }}>
                                                    <Switch
                                                        checked={this.state.showStatusMonitor}
                                                        onChange={
                                                            (e) => { this.model.setShowStatusMonitor(e.target.checked); }
                                                        }
                                                    />
                                                </div>

                                            </div>
                                        </div>
                                    </ButtonBase>

                                    <ButtonBase
                                        className={classes.setting}
                                        onClick={() => { this.handleCheckForUpdates(); }}  >
                                        <SelectHoverBackground selected={false} showHover={true} />
                                        <div style={{ width: "100%" }}>
                                            <Typography className={classes.primaryItem} display="block" variant="body2" color="textPrimary" noWrap>
                                                Check for updates...</Typography>
                                        </div>
                                    </ButtonBase>



                                    <ButtonBase className={classes.setting} disabled={disableShutdown}
                                        onClick={() => this.handleRestart()}  >
                                        <SelectHoverBackground selected={false} showHover={true} />
                                        <div style={{ width: "100%" }}>
                                            {
                                                this.state.restarting ? (
                                                    <Typography className={classes.primaryItem} display="block" variant="body2" color="textSecondary" noWrap>Rebooting...</Typography>
                                                ) : (
                                                    <Typography className={classes.primaryItem} display="block" variant="body2" noWrap>Reboot PiPedal</Typography>
                                                )
                                            }
                                        </div>
                                    </ButtonBase>

                                    <ButtonBase className={classes.setting} disabled={disableShutdown}
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
                            ) : (
                                <div>
                                    <Divider />
                                    <div style={{
                                        display: "grid", placeContent: "end", maxWidth: 550, marginLeft: 16, marginRight: 16,
                                        marginTop: 16, marginBottom: 16
                                    }}>
                                        <Button variant="outlined" onClick={() => { this.onOnboardingContinue() }} disabled={this.state.continueDisabled} style={{ width: 120 }}>Continue</Button>
                                    </div>
                                </div>
                            )
                            }



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
                        (this.state.showGovernorSettingsDialog) &&
                        (
                            <ListSelectDialog
                                width={220}
                                open={this.state.showGovernorSettingsDialog}
                                items={this.state.governorSettings.governors}
                                selectedItem={this.state.governorSettings.governor}
                                title="Governor"
                                onClose={() => this.setState({ showGovernorSettingsDialog: false })}
                                onOk={(selectedValue) => {
                                    this.model.setGovernorSettings(selectedValue)
                                        .then(() => {

                                        })
                                        .catch(error => {
                                            this.model.showAlert(error);
                                        });

                                    this.setState({ showGovernorSettingsDialog: false })
                                }}
                            >
                            </ListSelectDialog>
                        )
                    }
                    {
                        (this.state.showMidiSelectDialog) &&
                        (
                            <SelectMidiChannelsDialog open={this.state.showMidiSelectDialog}
                                onClose={(selectedChannels: AlsaMidiDeviceInfo[] | null) => this.handleSelectMidiDialogResult(selectedChannels)}
                                selectedChannels={this.state.jackSettings.inputMidiDevices}
                                availableChannels={this.state.jackConfiguration.inputMidiDevices}
                            />

                        )
                    }
                    {
                        (this.state.showThemeSelectDialog) &&
                        (
                            <SelectThemeDialog open={this.state.showThemeSelectDialog}
                                onClose={() => { this.setState({ showThemeSelectDialog: false }); }}
                                onOk={(selectedTheme: ColorTheme) => {
                                    this.model.setTheme(selectedTheme);
                                    this.setState({ showThemeSelectDialog: false });
                                }}
                                defaultTheme={this.model.getTheme()}
                            />

                        )
                    }
                    {
                        (this.state.showWifiConfigDialog) &&
                        (
                            <WifiConfigDialog wifiConfigSettings={this.state.wifiConfigSettings} open={this.state.showWifiConfigDialog}
                                onClose={() => this.setState({ showWifiConfigDialog: false })}
                                onOk={(wifiConfigSettings) => this.handleApplyWifiConfig(wifiConfigSettings)}
                            />

                        )
                    }
                    <WifiDirectConfigDialog wifiDirectConfigSettings={this.state.wifiDirectConfigSettings} open={this.state.showWifiDirectConfigDialog}
                        onClose={() => this.setState({ showWifiDirectConfigDialog: false })}
                        onOk={(wifiDirectConfigSettings: WifiDirectConfigSettings) => this.handleApplyWifiDirectConfig(wifiDirectConfigSettings)}
                    />
                    <OkCancelDialog text="Are you sure you want to reboot?" okButtonText='Reboot'
                        open={this.state.showRestartOkDialog}
                        onOk={() => { this.setState({ showRestartOkDialog: false }); this.handleRestartOk(); }}
                        onClose={() => { this.setState({ showRestartOkDialog: false }); }}
                    />
                    <OkCancelDialog text="Are you sure you want to shut down?" okButtonText='Shut down'
                        open={this.state.showShutdownOkDialog}
                        onOk={() => { this.setState({ showShutdownOkDialog: false }); this.handleShutdownOk(); }}
                        onClose={() => { this.setState({ showShutdownOkDialog: false }); }}
                    />
                    <SystemMidiBindingsDialog
                        open={this.state.showSystemMidiBindingsDialog}
                        onClose={() => { this.setState({ showSystemMidiBindingsDialog: false }); }}
                    />
                </DialogEx >

            );

        }

    }

);

export default SettingsDialog;