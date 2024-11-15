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

import React from 'react';
import IconButton from '@mui/material/IconButton';
import VisibilityIcon from '@mui/icons-material/Visibility';
import VisibilityOffIcon from '@mui/icons-material/VisibilityOff';
import FormHelperText from '@mui/material/FormHelperText';
import Button from '@mui/material/Button';
import Autocomplete from '@mui/material/Autocomplete';
import TextField from '@mui/material/TextField';
import DialogEx from './DialogEx';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogTitle';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import WifiConfigSettings from './WifiConfigSettings';
import Typography from '@mui/material/Typography';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import withStyles from '@mui/styles/withStyles';
import createStyles from '@mui/styles/createStyles';
import Select from '@mui/material/Select';
import FormControl from '@mui/material/FormControl';
import InputLabel from '@mui/material/InputLabel';
import MenuItem from '@mui/material/MenuItem';
import WifiChannel from './WifiChannel';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import HelpOutlineIcon from '@mui/icons-material/HelpOutline';

const styles = (theme: Theme) => createStyles({
    pgraph: {
        paddingBottom: 16
    }
});

const NBSP = "\u00A0";

export interface WifiConfigProps extends WithStyles<typeof styles> {
    open: boolean,
    wifiConfigSettings: WifiConfigSettings,
    onOk: (wifiConfigSettings: WifiConfigSettings) => void,
    onClose: () => void
}

export interface WifiConfigState {
    autoStartMode: number;
    showPassword: boolean;
    fullScreen: boolean;
    compactWidth: boolean;
    compactHeight: boolean;
    showWifiWarningDialog: boolean;
    showHelpDialog: boolean;
    wifiWarningGiven: boolean;
    name: string;
    newPassword: string;
    hasPassword: boolean;
    nameError: boolean;
    nameErrorMessage: string;
    passwordError: boolean;
    passwordErrorMessage: string;
    homeNetworkSsid: string;
    homeNetworkError: boolean;
    homeNetworkErrorMessage: string;
    countryCodeError: boolean;
    countryCodeErrorMessage: string;
    channelError: boolean;
    channelErrorMessage: string;
    countryCode: string;
    channel: string;
    knownWifiNetworks: string[];
    wifiChannels: WifiChannel[];
}

let gCountryCodeOptions: { id: string, label: string }[] | undefined = undefined;


const WifiConfigDialog = withStyles(styles, { withTheme: true })(
    class extends ResizeResponsiveComponent<WifiConfigProps, WifiConfigState> {

        refName: React.RefObject<HTMLInputElement>;
        refPassword: React.RefObject<HTMLInputElement>;
        model: PiPedalModel;

        constructor(props: WifiConfigProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();
            this.state = {
                showPassword: false,
                fullScreen: false,
                compactWidth: false,
                compactHeight: false,
                autoStartMode: this.props.wifiConfigSettings.autoStartMode,
                name: this.props.wifiConfigSettings.hotspotName,
                newPassword: "",
                hasPassword: false,
                nameError: false,
                nameErrorMessage: NBSP,
                passwordError: false,
                homeNetworkSsid: this.props.wifiConfigSettings.homeNetwork,
                homeNetworkError: false,
                homeNetworkErrorMessage: NBSP,
                countryCodeError: false,
                countryCodeErrorMessage: NBSP,
                channelError: false,
                channelErrorMessage: NBSP,
                showWifiWarningDialog: false,
                showHelpDialog: false,
                wifiWarningGiven: this.props.wifiConfigSettings.wifiWarningGiven,
                passwordErrorMessage: NBSP,
                countryCode: this.props.wifiConfigSettings.countryCode,
                channel: this.props.wifiConfigSettings.channel,
                knownWifiNetworks: [],
                wifiChannels: []
            };
            this.requestWifiChannels(this.props.wifiConfigSettings.countryCode);
            this.requestKnownWifiNetworks();

            this.refName = React.createRef<HTMLInputElement>();
            this.refPassword = React.createRef<HTMLInputElement>();
        }
        mounted: boolean = false;



        onWindowSizeChanged(width: number, height: number): void {
            this.setState({ fullScreen: height < 200, compactWidth: width < 600, compactHeight: height < 450 })
        }


        componentDidMount() {
            super.componentDidMount();
            this.mounted = true;
            this.applyDeferredWifiChannels();
            this.applyDeferredWifiNetworks();
        }
        componentWillUnmount() {
            super.componentWillUnmount();
            this.mounted = false;
        }

        private deferredWifiNetworks?: string[] = undefined;

        applyDeferredWifiNetworks() {
            if (this.mounted && this.deferredWifiNetworks) {
                this.setState({ knownWifiNetworks: this.deferredWifiNetworks });
                this.deferredWifiNetworks = undefined;
            }
        }
        requestKnownWifiNetworks() {
            this.model.getKnownWifiNetworks()
                .then((networks: string[]) => {
                    if (this.mounted) {
                        this.setState({ knownWifiNetworks: networks });
                    } else {
                        this.deferredWifiNetworks = networks;
                    }
                }).catch(() => {

                });
        }

        private lastChannelRequestCountryCode = "";
        private deferredWifiChannels?: WifiChannel[] = undefined;

        applyDeferredWifiChannels() {
            if (this.mounted && this.deferredWifiChannels) {
                this.setState({ wifiChannels: this.deferredWifiChannels });
                this.deferredWifiChannels = undefined;
            }
        }

        requestWifiChannels(countryCode: string) {
            if (countryCode === this.lastChannelRequestCountryCode) {
                return;
            }
            this.lastChannelRequestCountryCode = countryCode;
            this.model.getWifiChannels(countryCode)
                .then((wifiChannels) => {
                    if (this.mounted) {
                        this.setState({ wifiChannels: wifiChannels });
                    } else {
                    }
                })
                .catch((error) => {
                    this.lastChannelRequestCountryCode = ""; // try again later. :-/

                });

        }

        componentDidUpdate(prevProps: WifiConfigProps) {

            if (this.props.open && !prevProps.open) {
                this.setState({
                    name: this.props.wifiConfigSettings.hotspotName,
                    newPassword: "",
                    autoStartMode: this.props.wifiConfigSettings.autoStartMode,
                    homeNetworkSsid: this.props.wifiConfigSettings.homeNetwork,
                    wifiWarningGiven: this.props.wifiConfigSettings.wifiWarningGiven,
                    countryCode: this.props.wifiConfigSettings.countryCode,
                    channel: this.props.wifiConfigSettings.channel
                });
                this.requestKnownWifiNetworks();
                this.requestWifiChannels(this.props.wifiConfigSettings.countryCode);

            }
            this.applyDeferredWifiChannels();

        }

        getDefaultPasswordValue(): string {
            return (this.state.showPassword && this.state.newPassword === "") ?
                "password" :
                this.state.newPassword;
        }

        onEnterKey() {
            this.handleOk(false);
        }
    
        handleOk(wifiWarningGiven: boolean) {
            let hasError = false;
            if (this.state.autoStartMode !== 0) {
                let name = this.state.name;
                if (name.length === 0) {
                    this.setState({ nameError: true, nameErrorMessage: "* Required" });
                    hasError = true;
                } else if (name.length > 31) {
                    this.setState({ nameError: true, nameErrorMessage: "> 31 characters" });
                    hasError = true;
                }
                let password = this.state.newPassword;
                if (!this.props.wifiConfigSettings.hasSavedPassword && password.length === 0) {
                    this.setState({ passwordError: true, passwordErrorMessage: "* required" });

                } else if (password.length > 0) {
                    if (password.length < 8) {
                        this.setState({ passwordError: true, passwordErrorMessage: "Less than 8 characters" });
                        hasError = true;

                    } else if (password.length > 63) {
                        this.setState({ passwordError: true, passwordErrorMessage: "> 63 characters" });
                        hasError = true;

                    }
                }
                if (this.state.autoStartMode === 2 && this.state.homeNetworkSsid.length === 0) {
                    this.setState({ homeNetworkError: true, homeNetworkErrorMessage: "* Required" });
                    hasError = true;
                }
                if (this.state.countryCode === "")
                {
                    this.setState({ countryCodeError: true, countryCodeErrorMessage: "* Required" });
                    hasError = true;
                }
                if (this.validChannelSelection() === "")
                {
                    this.setState({ channelError: true, channelErrorMessage: "* Required" });
                    hasError = true;

                }
            }

            if (this.state.autoStartMode !== 0 && (!this.props.wifiConfigSettings.hasSavedPassword) && this.state.newPassword.length === 0) {
                this.setState({ passwordError: true, passwordErrorMessage: "* Required" });
                hasError = true;
            }
            if (!hasError) {
                let wifiConfigSettings = new WifiConfigSettings();
                wifiConfigSettings.valid = true;

                wifiConfigSettings.autoStartMode = this.state.autoStartMode;
                wifiConfigSettings.homeNetwork = this.state.homeNetworkSsid;
                wifiConfigSettings.hotspotName = this.state.name;
                wifiConfigSettings.countryCode = this.state.countryCode;
                wifiConfigSettings.channel = this.state.channel;

                if (this.state.newPassword.length === 0 || this.state.autoStartMode === 0) {
                    wifiConfigSettings.hasPassword = false;
                } else {
                    wifiConfigSettings.hasPassword = true;
                    wifiConfigSettings.password = this.state.newPassword;
                }

                if (!this.props.wifiConfigSettings.wifiWarningGiven && !wifiWarningGiven) {
                    this.setState({ showWifiWarningDialog: true });
                } else {
                    wifiConfigSettings.wifiWarningGiven = true;
                    this.props.onOk(wifiConfigSettings);
                }
            }

        }

        handleChannelChange(e: any) {
            let value = e.target.value as string;
            this.setState({ channel: value, channelError: false, channelErrorMessage: NBSP });
        }
        handleCountryChanged(value: string) {

            this.setState({ countryCode: value, 
                countryCodeError: false, countryCodeErrorMessage: NBSP,
                channelError: false, channelErrorMessage: NBSP
             });
            this.requestWifiChannels(value);
        }
        handleTogglePasswordVisibility() {
            this.setState({ hasPassword: true, showPassword: !this.state.showPassword });
        }

        private homeNetworkSsidElement?: HTMLInputElement = undefined;

        getCountryCodeValue(countryCode: string) {
            let value = this.model.countryCodes[countryCode];
            if (!value) return undefined;
            return { label: value, id: countryCode };
        }

        getCountryCodeOptions(): { id: string, label: string }[] {
            if (gCountryCodeOptions) {
                return gCountryCodeOptions;
            }
            let result: { label: string, id: string }[] = [];
            for (let key in this.model.countryCodes) {
                let value = this.model.countryCodes[key];
                result.push({ id: key, label: value });
            }
            result.sort((left, right) => {
                return left.label.localeCompare(right.label);
            });
            if (result.length !== 0) {
                gCountryCodeOptions = result;
            }
            return result;
        }
        handleWarningDialogOk() 
        {
            this.setState({ showWifiWarningDialog: false });
            this.handleOk(true);
        }

        validChannelSelection() {
            if (!this.state.wifiChannels)
            {
                return ''; // null selector.
            }
            for (let wifiChannel of this.state.wifiChannels)
            {
                if (wifiChannel.channelId === this.state.channel)
                {
                    return this.state.channel; // yes, it's a valid selection.
                }
            }
            return ''; // null selector value.
        }

        render() {
            let props = this.props;
            let classes = this.props.classes;
            let { open, onClose } = props;

            const handleClose = () => {
                onClose();
            };
            let enabled = this.state.autoStartMode !== 0;
            return (
                <DialogEx tag="wifiConfig" open={open} fullWidth maxWidth="sm" onClose={handleClose} style={{ userSelect: "none" }}
                    fullScreen={this.state.fullScreen
                    }
                    onEnterKey={()=>{ this.handleOk(false); }}

                >
                    {(this.state.fullScreen || !this.state.compactHeight) && (
                        <DialogTitle>Wi-fi Auto-Hotspot</DialogTitle>
                    )}
                    <DialogContent>
                        <div style={
                            !this.state.compactWidth
                                ? { display: "flex", gap: 16, flexDirection: "column", flexFlow: "nowrap", alignItems: "start" }
                                : { display: "block", gap: 16, flexDirection: "row", flexFlow: "nowrap" }
                        }
                        >
                            <div style={{
                                display: "flex", flexGrow: 1, flexBasis: 100,
                                gap: 16, flexDirection: "column", flexFlow: "nowrap", alignItems: "start"
                            }}
                            >

                                <FormControl variant="standard" style={{ flexGrow: 1, flexBasis: 1, minWidth: 150 }} >
                                    <InputLabel htmlFor="behavior">Auto-start hotspot when...</InputLabel>
                                    <Select id="behavior" value={this.state.autoStartMode}
                                        onChange={(el) => {
                                            let value = el.target.value as number
                                            this.setState({
                                                autoStartMode: value

                                            });
                                            if (value === 2 && this.state.homeNetworkSsid.length === 0 && this.state.knownWifiNetworks.length !== 0) {
                                                this.setState({
                                                    homeNetworkSsid: this.state.knownWifiNetworks[0]
                                                });
                                                if (this.homeNetworkSsidElement) {
                                                    this.homeNetworkSsidElement.value = this.state.knownWifiNetworks[0];
                                                }
                                            }
                                        }}
                                    >
                                        <MenuItem value={0}>Never</MenuItem>
                                        <MenuItem value={1}>No ethernet connection</MenuItem>
                                        <MenuItem value={2}>Not at home</MenuItem>
                                        <MenuItem value={3}>No remembered Wi-Fi connections</MenuItem>
                                        <MenuItem value={4}>Always</MenuItem>
                                    </Select>
                                    <FormHelperText>{NBSP}</FormHelperText>
                                </FormControl>
                                {(this.state.compactWidth || this.state.autoStartMode !== 2) && (
                                    <IconButton style={{ flexGrow: 0, flexShrink: 0, marginTop: 8 }}
                                        onClick={() => { this.setState({ showHelpDialog: true }); }}
                                    >
                                        <HelpOutlineIcon />
                                    </IconButton>
                                )}
                            </div>
                            <div style={{
                                display: this.state.autoStartMode === 2 ? "flex" : "none", gap: 16, flexGrow: 1, flexBasis: 100, flexDirection: "column", flexFlow: "nowrap", alignItems: "center",
                            }}>
                                <Autocomplete options={this.state.knownWifiNetworks}
                                    freeSolo autoSelect={false} forcePopupIcon={true}
                                    style={{
                                        flexGrow: 1, marginLeft: this.state.compactWidth ? 32 : 0
                                    }}
                                    value={this.state.homeNetworkSsid}

                                    onChange={(event, value): void => {
                                        this.setState({ homeNetworkSsid: value?.toString() ?? "", homeNetworkError: false, homeNetworkErrorMessage: NBSP });
                                    }
                                    }

                                    renderInput={
                                        (params) =>
                                            <TextField {...params} variant="standard" label="Home Network SSID"
                                                autoComplete="off"
                                                spellCheck="false"
                                                error={this.state.homeNetworkError}
                                                onChange={
                                                    (event) => {
                                                        this.setState({ homeNetworkSsid: event.target.value, homeNetworkError: false, homeNetworkErrorMessage: NBSP });
                                                    }

                                                }
                                                helperText={this.state.homeNetworkErrorMessage}
                                                InputLabelProps={{
                                                    shrink: true
                                                }}
                                            />}
                                />
                                <IconButton style={{ visibility: this.state.compactWidth ? "hidden" : "visible", flexGrow: 0, flexShrink: 0 }}
                                    onClick={() => { this.setState({ showHelpDialog: true }); }}
                                >
                                    <HelpOutlineIcon />
                                </IconButton>

                            </div>
                        </div>
                        <div style={
                            !this.state.compactWidth
                                ? { display: "flex", flexFlow: "row nowrap", gap: 24, alignItems: "start" }
                                : { display: "block" }
                        }>

                            <TextField variant="standard" style={{ marginBottom: 8, flexGrow: 1, flexBasis: 1 }}

                                autoComplete="off"
                                spellCheck="false"
                                error={this.state.nameError}
                                id="name"
                                label="SSID"
                                type="text"
                                fullWidth
                                helperText={this.state.nameErrorMessage}
                                value={this.state.name}
                                onChange={(e) => this.setState({ name: e.target.value, nameError: false, nameErrorMessage: NBSP })}
                                inputRef={this.refName}
                                InputLabelProps={{
                                    shrink: true
                                }}
                            />
                            <div style={{ marginBottom: 8, flexGrow: 1, flexBasis: 1 }}>
                                <form autoComplete='off' onSubmit={() => false}> {/*Prevents chrome from saving passwords */}
                                    <TextField label="WEP passphrase" variant="standard"
                                        autoComplete="off"
                                        spellCheck="false"
                                        fullWidth
                                        error={this.state.passwordError}

                                        onFocus={() => { this.setState({ hasPassword: true }) }}
                                        type={this.state.showPassword ? "text" : "password"}
                                        onChange={(event): void => {
                                            this.setState({ hasPassword: true, newPassword: event.target.value.toString(), passwordError: false, passwordErrorMessage: NBSP });
                                        }
                                        }
                                        helperText={this.state.passwordErrorMessage}
                                        defaultValue={
                                            this.getDefaultPasswordValue()
                                        }
                                        disabled={!enabled}
                                        InputLabelProps={{
                                            shrink: true
                                        }}
                                        InputProps={{
                                            startAdornment:
                                                    !this.state.hasPassword && !this.state.showPassword ? 
                                                        (this.props.wifiConfigSettings.hasSavedPassword? "(Unchanged)" : "(Required)")
                                                    : ""
                                            ,
                                            endAdornment: (
                                                <IconButton size="small"
                                                    aria-label="toggle password visibility"
                                                    onClick={() => { this.handleTogglePasswordVisibility(); }}
                                                >
                                                    {
                                                        (this.state.showPassword) ?
                                                            (
                                                                <VisibilityIcon fontSize="small" />
                                                            ) :
                                                            (
                                                                <VisibilityOffIcon fontSize="small" />

                                                            )
                                                    }
                                                </IconButton>
                                            )
                                        }}
                                    />
                                </form>
                            </div>

                        </div>
                        <div style={
                            !this.state.compactWidth
                                ? { display: "flex", flexFlow: "row nowrap", gap: 24, alignItems: "start" }
                                : { display: "block" }
                        }>
                            <FormControl variant="standard" style={{ display: "flex", marginBottom: 8, flexGrow: 1, flexBasis: 1 }} >
                                <Autocomplete fullWidth
                                    defaultValue={this.getCountryCodeValue(this.state.countryCode)}
                                    disableClearable={true}
                                    onChange={(event, value) => { if (value) { this.handleCountryChanged(value.id as string ) } }}
                                    options={this.getCountryCodeOptions()}
                                    renderInput={(params) => (<TextField {...params} variant="standard" label="Regulatory Domain" />)}
                                    disabled={!enabled}
                                />
                                <FormHelperText error={this.state.countryCodeError}>{this.state.countryCodeErrorMessage}</FormHelperText>
                            </FormControl>
                            <FormControl variant="standard" style={{ display: "flex", marginBottom: 8, flexGrow: 1, flexBasis: 1 }} >
                                <InputLabel htmlFor="channelSelect">Channel</InputLabel>
                                <Select variant="standard" id="channelSelect" value={
                                        this.validChannelSelection()}
                                    fullWidth
                                    disabled={!enabled || this.state.wifiChannels.length === 0}
                                    onChange={(e) => {
                                        this.handleChannelChange(e);
                                    }}>
                                    {this.state.wifiChannels.map((channel) => {
                                        return (
                                            <MenuItem value={channel.channelId}>{channel.channelName}</MenuItem>
                                        )
                                    })}
                                </Select>
                                <FormHelperText error={this.state.channelError}>{this.state.channelErrorMessage}</FormHelperText>

                            </FormControl>
                        </div>


                    </DialogContent>
                    <DialogActions>
                        <Button onClick={handleClose} variant="dialogSecondary" style={{ width: 120 }}>
                            Cancel
                        </Button>
                        <Button onClick={() => this.handleOk(false)} variant="dialogPrimary" style={{ width: 120 }} >
                            OK
                        </Button>
                    </DialogActions>
                    {this.state.showHelpDialog && (
                        <DialogEx open={this.state.showHelpDialog} tag="wifiHelp"
                            style={{ userSelect: "none" }}
                            onEnterKey={()=>{ 
                                this.setState({ showHelpDialog: false });
                            }}
                        >
                            <DialogContent>
                                <Typography className={classes.pgraph} variant="h6" color="textPrimary">
                                    PiPedal Auto-Hotspot
                                </Typography>

                                <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                                    The PiPedal <b><i>Auto-Hotspot</i></b> feature allows you to connect to your Raspberry Pi even if you don't have
                                    access to a Wi-Fi router. For example, if you are performing at a live venue, you probably will not
                                    have access to a Wi-Fi router; but you can configure PiPedal so that your Raspberry Pi  automatically
                                    starts a Wi-Fi hotspot when you are not at home. The feature is primarily intended for use with the
                                    PiPedal Android client, but you may find it useful for other purposes as well.
                                </Typography>
                                <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                                    Raspberry Pi's are unable to run hotspots, and have another active Wi-Fi connection at the same time; so the auto-hotspot feature
                                    automatically turns the hotspot on, when your Raspberry Pi cannot otherwise be connected to, and can be configured to
                                    automatically turn the PiPedal hotspot off when you do want your Raspberry Pi to connect to another Wi-Fi access point.
                                    Which auto-start option you should select depends on how you normally connect to your Raspberry Pi when you are at home.
                                </Typography>
                                <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                                    If you normally connect to your Raspberry Pi using an ethernet connection, the <b><i>No ethernet connection</i></b> is a
                                    good choice. The PiPedal Wi-Fi hotspot will be available whenever the ethernet cable is unplugged. <b><i>Always on</i></b> is
                                    also a good choice, but may confuse your phone or tablet, since your Android device will now have to decide whether to auto-connect to your home Wi-Fi
                                    router, or to the Raspberry Pi hotspot. If you use the <b><i>No ethernet connection</i></b> option, your phone or tablet will
                                    never see the PiPedal hotspot and your Wi-Fi router at the same time.
                                </Typography>
                                <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                                    If you normally connect to your Raspberry Pi through a Wi-Fi router, <b><i>Not at home</i></b> is a good choice. The
                                    PiPedal hotspot will be automatically turned off whenever your home Wi-Fi router is in range, and automatically turned on
                                    when you are out of range of your home Wi-Fi router.
                                </Typography>
                                <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                                    If there are multiple locations, and multiple Wi-Fi routers you use with PiPedal on a regular basis, you can select
                                    the <b><i>No remembered Wi-Fi connections</i></b> option, but this is a riskier option. The PiPedal hotspot will be automatically turned on if there are no
                                    Wi-Fi access points in range that you have previously connected to from your Raspberry Pi, and will be automatically turned on otherwise.
                                    The risk is that you could find yourself unable to connect to your Raspberry Pi when performing
                                    at a local bar, after you have used your Rasberry Pi to connect to the Wi-Fi access point at the coffee shop nextdoor. (Public Wi-Fi access
                                    points usually won't work because devices that are connected to a public access point can't connect to each other).
                                    Will you ever do that? Probably not. But there is some risk that you might find yourself unable to connect at a live venue. Whether that's an
                                    acceptable risk is up to you.
                                </Typography>
                                <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                                    Typically, when you're away from home, there's no easy way to connect to your Raspberry Pi from a laptop in order to
                                    correct the problem. So you should carefully test that your auto-hotspot configuration works as expected before you adventure
                                    away from home with PiPedal.
                                </Typography>

                            </DialogContent>
                            <DialogActions>
                                <Button
                                    onClick={
                                        () => {
                                            this.setState({ showHelpDialog: false });
                                        }}
                                    variant="dialogSecondary" >
                                    Ok
                                </Button>
                            </DialogActions>

                        </DialogEx>

                    )}
                    {this.state.showWifiWarningDialog && (
                        <DialogEx open={this.state.showWifiWarningDialog} tag="wifiConfirm"
                            onEnterKey={()=>{ this.handleWarningDialogOk();}}
                            style={{ userSelect: "none" }}>
                            <DialogContent>
                                <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                                    Enabling the Wi-Fi hotspot will disable regular Wi-Fi network access on your Raspberry Pi when the PiPedal hotspot is active.
                                    If you are relying on Wi-Fi access to connect to your Raspberry Pi, consider carefully whether your autostart options
                                    will allow you to connect to your Raspberry Pi once applied. PiPedal
                                    <a href="https://rerdavies.github.io/pipedal/Configuring.html" target="_blank" rel="noreferrer">
                                        online documentation</a> provides a discussion of how to choose safe hotspot auto-start options.
                                </Typography>
                                <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                                    When you are connected to the PiPedal hotspot, you can connect to the PiPedal web server at http://10.48.0.1.
                                </Typography>
                                <Typography className={classes.pgraph} variant="body2" color="textPrimary" gutterBottom>
                                    Are you sure you want to continue?
                                </Typography>
                            </DialogContent>
                            <DialogActions>
                                <Button onClick={() => this.setState({ showWifiWarningDialog: false })} variant="dialogSecondary" style={{ width: 120 }}>
                                    Cancel
                                </Button>
                                <Button onClick={() => {
                                    this.handleWarningDialogOk();
                                }} variant="dialogPrimary" style={{ width: 120 }} >
                                    PROCEED
                                </Button>
                            </DialogActions>
                        </DialogEx>
                    )}
                </DialogEx >
            );
        }
    });

export default WifiConfigDialog;