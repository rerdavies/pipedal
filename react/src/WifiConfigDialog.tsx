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
import Button from '@mui/material/Button';
import TextField from '@mui/material/TextField';
import DialogEx from './DialogEx';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogTitle';
import Switch from '@mui/material/Switch';
import FormControlLabel from '@mui/material/FormControlLabel';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import WifiConfigSettings from './WifiConfigSettings';
import NoChangePassword from './NoChangePassword';
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
import {PiPedalModel, PiPedalModelFactory} from './PiPedalModel';

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
    fullScreen: boolean;
    showWifiWarningDialog: boolean;
    wifiWarningGiven: boolean;
    enabled: boolean;
    name: string;
    newPassword: string;
    nameError: boolean;
    nameErrorMessage: string;
    passwordError: boolean;
    passwordErrorMessage: string;
    countryCode: string;
    channel: string;
    wifiChannels: WifiChannel[];
}



const WifiConfigDialog = withStyles(styles, { withTheme: true })( 
    class extends ResizeResponsiveComponent<WifiConfigProps, WifiConfigState> {

    refName: React.RefObject<HTMLInputElement>;
    refPassword: React.RefObject<HTMLInputElement>;
    model: PiPedalModel;

    constructor(props: WifiConfigProps) {
        super(props);
        this.model = PiPedalModelFactory.getInstance();
        this.state = {
            fullScreen: false,
            enabled: this.props.wifiConfigSettings.enable,
            name: this.props.wifiConfigSettings.hotspotName,
            newPassword: "",
            nameError: false,
            nameErrorMessage: NBSP,
            passwordError: false,
            showWifiWarningDialog: false,
            wifiWarningGiven: this.props.wifiConfigSettings.wifiWarningGiven,
            passwordErrorMessage: NBSP,
            countryCode: this.props.wifiConfigSettings.countryCode,
            channel: this.props.wifiConfigSettings.channel,
            wifiChannels: []
        };
        this.model.getWifiChannels(this.props.wifiConfigSettings.countryCode)
        .then((wifiChannels_) => {
            this.setState({wifiChannels: wifiChannels_});
        }).catch((err)=> {});

        this.refName = React.createRef<HTMLInputElement>();
        this.refPassword = React.createRef<HTMLInputElement>();
    }
    mounted: boolean = false;

    handleEnableChanged(e: any) {
        this.setState({ enabled: e.target.checked });
    }
    

    onWindowSizeChanged(width: number, height: number): void {
        this.setState({ fullScreen: height < 200 })
    }


    componentDidMount() {
        super.componentDidMount();
        this.mounted = true;
    }
    componentWillUnmount() {
        super.componentWillUnmount();
        this.mounted = false;
    }

    componentDidUpdate(prevProps: WifiConfigProps) {

        if (this.props.open && !prevProps.open)
        {
            this.setState({
                enabled: this.props.wifiConfigSettings.enable,
                name: this.props.wifiConfigSettings.hotspotName,
                newPassword: "",
                wifiWarningGiven: this.props.wifiConfigSettings.wifiWarningGiven,
                countryCode: this.props.wifiConfigSettings.countryCode,
                channel: this.props.wifiConfigSettings.channel
            });

            if (this.props.wifiConfigSettings.countryCode !== this.state.countryCode)
            {
                this.model.getWifiChannels(this.props.wifiConfigSettings.countryCode)
                .then((wifiChannels) => {
                    this.setState({wifiChannels: wifiChannels});
                })
                .catch((error) => {});

            }
    
        }

    }

    handleOk(wifiWarningGiven: boolean) {
        let hasError = false;
        if (this.state.enabled)
        {
            let name = this.state.name;
            if (name.length === 0) {
                this.setState({nameError: true, nameErrorMessage: "* Required"});
                hasError = true;
            } else if (name.length > 31) 
            {
                this.setState({nameError: true, nameErrorMessage: "> 31 characters"});
                hasError = true;
            }
            let password = this.state.newPassword;
            if (password.length > 0) {
                if (password.length < 8) {
                    this.setState({passwordError: true, passwordErrorMessage: "Less than 8 characters"});
                    hasError = true;

                } else if (password.length > 63) {
                    this.setState({passwordError: true, passwordErrorMessage: "> 63 characters"});
                    hasError = true;

                }

            }
        }
        if (this.state.enabled && (!this.props.wifiConfigSettings.hasPassword) && this.state.newPassword.length === 0)
        {
            this.setState({passwordError: true, passwordErrorMessage: "* Required"});
            hasError = true;
        }
        if (!hasError)
        {
            let wifiConfigSettings = new WifiConfigSettings();
            wifiConfigSettings.valid = true;

            wifiConfigSettings.enable = this.state.enabled;
            wifiConfigSettings.hotspotName = this.state.name;
            if (this.state.newPassword.length === 0 || !this.state.enabled)
            {
                wifiConfigSettings.hasPassword = false;
            } else {
                wifiConfigSettings.hasPassword = true;
                wifiConfigSettings.password = this.state.newPassword;
            }
            
            if (!this.props.wifiConfigSettings.wifiWarningGiven && !wifiWarningGiven) {
                this.setState({showWifiWarningDialog: true});
            } else {
                wifiConfigSettings.wifiWarningGiven = true;
                this.preventPasswordPrompt();
                setTimeout(()=> {
                    this.props.onOk(wifiConfigSettings);
                });
            }
        }
    
    }
    preventPasswordPrompt()
    {
        let passwordInput = this.refPassword.current;
        if (passwordInput)
        {
            passwordInput.value = "";
            passwordInput.type = "text";
        }
    }
    handleChannelChange(e: any)
    {
        let value = e.target.value as string;
        this.setState({channel: value });
    }
    handleCountryChanged(e: any)
    {
        let value = e.target.value as string;

        this.setState({countryCode: value});
        this.model.getWifiChannels(value)
        .then(wifiChannels => {
            this.setState({wifiChannels: wifiChannels});
        })
        .catch((error)=> { });
    }

    render() {
        let props = this.props;
        let classes = this.props.classes;
        let { open, onClose} = props;

        const handleClose = () => {
            this.preventPasswordPrompt();
            setTimeout(()=> {
                onClose();
            });
        };

        return (
            <DialogEx tag="WifiConfigDialog" open={open} fullWidth onClose={handleClose} style={{userSelect: "none"}}
                fullScreen={this.state.fullScreen}
            >
                { this.state.fullScreen && (
                    <DialogTitle>Wi-fi Hotspot</DialogTitle>
                )}
                <DialogContent>

                    <FormControlLabel
                        control={(
                            <Switch 
                                checked={this.state.enabled}
                                onChange={(e: any) => this.handleEnableChanged(e)}
                                color="secondary"
                            />
                        )}
                        label="Enable"
                    />
                    <TextField style={{ marginBottom: 16, marginTop: 16 }}

                        autoComplete="off"
                        id="name"
                        spellCheck="false"
                        label="SSID"
                        type="text"
                        fullWidth
                        error={this.state.nameError}
                        helperText={this.state.nameErrorMessage}
                        value={this.state.name}
                        onChange={(e) => this.setState({name: e.target.value, nameError: false, nameErrorMessage: NBSP})}
                        inputRef={this.refName}
                        disabled={!this.state.enabled}
                    />
                    <div style={{ marginBottom: 16 }}>
                        <input type="password" style={{display:"none"}}/>
                        <NoChangePassword 
                            inputRef={this.refPassword}
                            onPasswordChange={(text): void => { this.setState({ newPassword: text, passwordError: false, passwordErrorMessage: NBSP }); }}
                            hasPassword={this.props.wifiConfigSettings.hasPassword}
                            label="WEP Passphrase"
                            error={this.state.passwordError}
                            helperText={this.state.passwordErrorMessage}
                            defaultValue={this.state.newPassword}
                            disabled={!this.state.enabled}
                        />
                    </div>
                    <div style={{ marginBottom: 16}}>
                        <FormControl>
                            <InputLabel htmlFor="countryCodeSelect">Country</InputLabel>
                            <Select variant="standard" id="countryCodeSelect" value={this.state.countryCode} style={{width: 220}} 
                                onChange={(event)=>this.handleCountryChanged(event)} >
                                {Object.entries(this.model.countryCodes).map(([key,value])=> {
                                    return (
                                        <MenuItem value={key}>{value}</MenuItem>
                                    );
                                })}
                            </Select>
                        </FormControl>

                    </div>
                    <div style={{ marginBottom: 24}}>
                        <FormControl>
                            <InputLabel htmlFor="channelSelect">Channel</InputLabel>
                            <Select variant="standard" id="channelSelect" value={this.state.channel} style={{width: 220}} onChange={(e)=>{
                                this.handleChannelChange(e);
                            }}>
                                {this.state.wifiChannels.map((channel)=> {
                                    return (
                                        <MenuItem value={channel.channelId}>{channel.channelName}</MenuItem>
                                    )
                                })}
                            </Select>
                        </FormControl>

                    </div>

                </DialogContent>
                <DialogActions>
                    <Button onClick={handleClose} color="primary" style={{ width: 120 }}>
                        Cancel
                    </Button>
                    <Button onClick={()=> this.handleOk(false)} color="secondary" style={{ width: 120 }} >
                        OK
                    </Button>
                </DialogActions>
                <DialogEx open={this.state.showWifiWarningDialog} tag="wifiConfirm" 
                    style={{userSelect: "none"}}>
                    <DialogContent>
                        <Typography className={classes.pgraph} variant="body2" color="textPrimary">
                            Enabling the Wi-Fi hotspot will disable regular Wi-Fi network access on the PiPedal device. Once 
                            enabled, connect to the hotspot and launch http://172.24.1.1 or http://{this.state.name}.local to access the PiPedal web app again.
                        </Typography>
                        <Typography className={classes.pgraph} variant="body2" color="textPrimary" gutterBottom>
                            Are you sure you want to proceed?
                        </Typography>
                    </DialogContent>
                    <DialogActions>
                        <Button onClick={()=> this.setState({showWifiWarningDialog: false})} color="primary" style={{ width: 120 }}>
                            Cancel
                        </Button>
                        <Button onClick={()=> {
                            this.setState({showWifiWarningDialog: false});
                            this.handleOk(true);
                        }} color="secondary" style={{ width: 120 }} >
                            PROCEED
                        </Button>
                    </DialogActions>
                </DialogEx>
            </DialogEx>
        );
    }
});

export default WifiConfigDialog;