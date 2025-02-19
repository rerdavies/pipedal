/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
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

import React from 'react';
import IconButton from '@mui/material/IconButton';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';

import Button from '@mui/material/Button';
import TextField from '@mui/material/TextField';
import DialogEx from './DialogEx';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import Switch from '@mui/material/Switch';
import FormControlLabel from '@mui/material/FormControlLabel';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import WifiDirectConfigSettings from './WifiDirectConfigSettings';

//import { withStyles } from "tss-react/mui";
import MenuItem from '@mui/material/MenuItem';
import WifiChannel from './WifiChannel';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';


declare let crypto: any;

const NBSP = "\u00A0";

export interface WifiDirectConfigProps  {
    open: boolean,
    wifiDirectConfigSettings: WifiDirectConfigSettings,
    onOk: (wifiDirectConfigSettings: WifiDirectConfigSettings) => void,
    onClose: () => void
}

export interface WifiDirectConfigState {
    fullScreen: boolean;
    enabled: boolean;
    name: string;
    pin: string;
    nameError: boolean;
    nameErrorMessage: string;
    pinError: boolean;
    pinErrorMessage: string;
    countryCode: string;
    channel: string;
    controlWidth: number;
    landscapeLayout: boolean;
    controlSize: ("medium" | "small" | undefined);
    wifiChannels: WifiChannel[];
}



const WifiDirectConfigDialog = 
    class extends ResizeResponsiveComponent<WifiDirectConfigProps, WifiDirectConfigState> {

        refName: React.RefObject<HTMLInputElement|null>;
        refPassword: React.RefObject<HTMLInputElement|null>;
        model: PiPedalModel;

        constructor(props: WifiDirectConfigProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();
            this.state = {
                fullScreen: this.useFullScreen(),
                enabled: this.props.wifiDirectConfigSettings.enable,
                name: this.props.wifiDirectConfigSettings.hotspotName,
                pin: this.props.wifiDirectConfigSettings.pin,
                nameError: false,
                nameErrorMessage: NBSP,
                pinError: false,
                pinErrorMessage: NBSP,
                countryCode: this.props.wifiDirectConfigSettings.countryCode,
                channel: this.props.wifiDirectConfigSettings.channel,
                controlWidth: 260,
                controlSize: this.windowSize.height < 600 ? "small" : "medium",
                landscapeLayout: this.useLandscapeLayout(),

                wifiChannels: []
            };
            this.model.getWifiChannels(this.props.wifiDirectConfigSettings.countryCode)
                .then((wifiChannels_) => {
                    this.setState({ wifiChannels: wifiChannels_ });
                }).catch((err) => { });

            this.refName = React.createRef<HTMLInputElement>();
            this.refPassword = React.createRef<HTMLInputElement>();
        }

        useLandscapeLayout(): boolean {
            return (this.windowSize.height < 690 && this.windowSize.width > this.windowSize.height && this.windowSize.width > 690);
        }
        mounted: boolean = false;

        handleEnableChanged(e: any) {
            this.setState({ enabled: e.target.checked });
        }


        onWindowSizeChanged(width: number, height: number): void {
            this.setState({
                fullScreen: this.useFullScreen(),
                controlSize: height < 600 ? "small" : "medium",
                landscapeLayout: this.useLandscapeLayout()
            })
        }

        useFullScreen(): boolean {
            return this.windowSize.height < 390;
        }


        componentDidMount() {
            super.componentDidMount();
            this.mounted = true;
        }
        componentWillUnmount() {
            super.componentWillUnmount();
            this.mounted = false;
        }

        componentDidUpdate(prevProps: WifiDirectConfigProps) {

            if (this.props.open && !prevProps.open) {
                this.setState({
                    enabled: this.props.wifiDirectConfigSettings.enable,
                    name: this.props.wifiDirectConfigSettings.hotspotName,
                    pin: this.props.wifiDirectConfigSettings.pin,
                    countryCode: this.props.wifiDirectConfigSettings.countryCode,
                    channel: this.props.wifiDirectConfigSettings.channel
                });

                if (this.props.wifiDirectConfigSettings.countryCode !== this.state.countryCode) {
                    this.model.getWifiChannels(this.props.wifiDirectConfigSettings.countryCode)
                        .then((wifiChannels) => {
                            this.setState({ wifiChannels: wifiChannels });
                        })
                        .catch((error) => { });

                }

            }

        }
        utf8Encode: TextEncoder = new TextEncoder();

        validateName(name: string): void {
            if (name.length === 0) {
                throw new RangeError("Required.");
            }
            let bytes = this.utf8Encode.encode("DIRECT-XX-" + name);
            if (bytes.length > 31) {
                throw new RangeError("Too long.");
            }
        }
        validatePin(pin: string) {
            if (pin.length === 0) throw new Error("Required.");
            if (pin.length !== 8) {
                throw new RangeError("Must be exactly 8 digits.");
            }
            for (let i = 0; i < pin.length; ++i) {
                let c = pin.charCodeAt(i);
                if (c < '0'.charCodeAt(0) || c > '9'.charCodeAt(0)) {
                    throw new RangeError("Must be exactly 8 digits.");
                }
            }
        }

        handleOk() {
            let nameError = false;
            let nameErrorMessage = NBSP;
            let pinError = false;
            let pinErrorMessage = NBSP;
            if (this.state.enabled) {
                let name = this.state.name;
                try {
                    this.validateName(name);
                } catch (e: any) {
                    nameError = true; nameErrorMessage = "* " + e.message;
                }
                let pin = this.state.pin;
                try {
                    this.validatePin(pin);
                } catch (e: any) {
                    pinError = true; pinErrorMessage = "* " + e.message;
                }

                let error = pinError || nameError || this.state.countryCode === "";
                if (error) {
                    this.setState({ nameError: nameError, nameErrorMessage: nameErrorMessage, pinError: pinError, pinErrorMessage: pinErrorMessage });
                } else {
                    let wifiDirectConfigSettings = new WifiDirectConfigSettings();
                    wifiDirectConfigSettings.valid = true;

                    wifiDirectConfigSettings.enable = this.state.enabled;
                    wifiDirectConfigSettings.hotspotName = this.state.name;
                    wifiDirectConfigSettings.countryCode = this.state.countryCode;
                    wifiDirectConfigSettings.channel = this.state.channel;
                    wifiDirectConfigSettings.pin = this.state.pin;
                    this.props.onOk(wifiDirectConfigSettings);
                }
            } else {
                let wifiDirectConfigSettings = new WifiDirectConfigSettings();
                wifiDirectConfigSettings.valid = true;
                wifiDirectConfigSettings.enable = false;
                this.props.onOk(wifiDirectConfigSettings);

            }

        }
        preventPasswordPrompt() {
        }
        handleChannelChange(e: any) {
            let value = e.target.value as string;
            this.setState({ channel: value });
        }
        handleCountryChanged(e: any) {
            let value = e.target.value as string;

            this.setState({ countryCode: value });
            this.model.getWifiChannels(value)
                .then(wifiChannels => {
                    this.setState({ wifiChannels: wifiChannels });
                })
                .catch((error) => { });
        }

        generateRandomPin(): string {
            // prefer crypto if it's available.
            let zero = "0".charCodeAt(0);
            try {
                let randomValue = new Int32Array(8);
                crypto.getRandomValues(randomValue);

                let s: string = "";
                for (let i = 0; i < 8; ++i) {
                    let k = randomValue[i];
                    if (k < 0) k = -k;
                    k = k % 10;

                    s += String.fromCharCode(zero + k);
                }
                return s;

            } catch (e) {

            }
            let zero2 = "a".charCodeAt(0);
            let s2 = "";
            for (let i = 0; i < 8; ++i) {
                let k = Math.floor(Math.random() * 10);
                if (k < 0) k = -k;
                k = k % 10;

                s2 += String.fromCharCode(zero2 + k);
            }
            return s2;
        }

        onGenerateRandomPin() {
            let newPin = this.generateRandomPin();
            this.setState({ pin: newPin, pinError: false, pinErrorMessage: NBSP });
            if (this.refPassword.current) {
                this.refPassword.current.value = newPin;
            }
        }

        render() {
            let props = this.props;
            let { open, onClose } = props;

            const handleClose = () => {
                this.preventPasswordPrompt();
                // let preventPasswordPrompt changes settle (it's HARD to prevent a password prompt)
                setTimeout(() => {
                    onClose();
                }, 100);
            };

            return (
                <DialogEx tag="p2pConfig" open={open} onClose={handleClose} style={{ userSelect: "none", }}
                    fullScreen={this.state.fullScreen} fullWidth={this.useLandscapeLayout()} onEnterKey={()=>{}}
                >
                    {this.state.landscapeLayout && (
                        <DialogContent >
                            <div style={{ display: "flex", gap: 16, alignItems: "left" }}>
                                <IconButton
                                    edge="start"
                                    color="inherit"
                                    onClick={() => { this.props.onClose(); }}
                                    aria-label="back"
                                    size="large">
                                    <ArrowBackIcon htmlColor="#888" />
                                </IconButton>
                                <FormControlLabel
                                    control={(
                                        <Switch
                                            checked={this.state.enabled}
                                            onChange={(e: any) => this.handleEnableChanged(e)}
                                            color="primary"
                                        />
                                    )}
                                    label="Wi-Fi Direct Hotspot"
                                />
                            </div>
                            <div style={{ marginTop: 16, display: 'flex', alignItems: 'center', gap: 16 }}>
                                <TextField
                                    required

                                    sx={{ maxWidth: this.state.controlWidth }}
                                    autoComplete="off"
                                    id="name"
                                    spellCheck="false"
                                    label="Service Name"
                                    type="text"
                                    size={this.state.controlSize}
                                    fullWidth
                                    error={this.state.nameError}
                                    helperText={this.state.nameErrorMessage}
                                    value={this.state.name}
                                    onChange={(e) => this.setState({ name: e.target.value, nameError: false, nameErrorMessage: NBSP })}
                                    inputRef={this.refName}
                                    variant="filled"
                                />

                                <TextField
                                    required
                                    inputRef={this.refPassword}
                                    sx={{ maxWidth: this.state.controlWidth }}

                                    size={this.state.controlSize}
                                    autoComplete="off"
                                    onChange={(e): void => { this.setState({ pin: e.target.value, pinError: false, pinErrorMessage: NBSP }); }}
                                    label="Pin"
                                    inputProps={{ inputMode: 'numeric' }}
                                    fullWidth
                                    error={this.state.pinError}
                                    helperText={this.state.pinErrorMessage}
                                    defaultValue={this.state.pin}
                                    disabled={!this.state.enabled}
                                    variant="filled"
                                />
                            </div>
                            <div style={{ width: this.state.controlWidth * 2 }}>
                                <div style={{ marginTop: 0, marginBottom: 16, width: this.state.controlWidth, marginLeft: "auto", marginRight: 0, textAlign: "right" }}>
                                    <Button size="small" disabled={!this.state.enabled}
                                        onClick={() => this.onGenerateRandomPin()}
                                    >Generate random pin</Button>
                                </div>
                            </div>
                            <div style={{ marginTop: 0, display: 'flex', alignItems: 'stretch', gap: 16 }}>
                                <TextField required select label="Country" size={this.state.controlSize} value={this.state.countryCode} variant="filled"
                                    sx={{ width: this.state.controlWidth }}
                                    disabled={!this.state.enabled}
                                    onChange={(event) => { this.handleCountryChanged(event); }}
                                >
                                    {Object.entries(this.model.countryCodes).map(([key, value]) => {
                                        return (
                                            <MenuItem value={key}>{value}</MenuItem>
                                        );
                                    })}
                                </TextField>

                                <TextField select id="channelSelect" size={this.state.controlSize} label="Channel"
                                    value={this.state.channel} variant="filled" sx={{ width: this.state.controlWidth }}
                                    disabled={!this.state.enabled}
                                    onChange={(event) => { this.handleChannelChange(event); }}
                                >
                                    {this.state.wifiChannels.map((channel) => {
                                        let t = channel.channelId;
                                        if (t.startsWith("a")) t = t.substring(1);
                                        if (t.startsWith("g")) t = t.substring(1);
                                        let name = channel.channelName;
                                        if (name.startsWith("1 ") || name.startsWith("6 ") || name.startsWith("11 ")) {
                                            name += " *Recommended";
                                        }
                                        return (
                                            <MenuItem value={t}>{name}</MenuItem>
                                        )
                                    })}
                                </TextField>

                            </div>
                        </DialogContent>
                    )}
                    {(!this.state.landscapeLayout) && (
                        <DialogContent sx={{ minHeight: 96 }} >
                            <div>
                                <FormControlLabel
                                    control={(
                                        <Switch
                                            checked={this.state.enabled}
                                            onChange={(e: any) => this.handleEnableChanged(e)}
                                            color="secondary"
                                        />
                                    )}
                                    label="Wi-Fi Direct Hotspot"
                                />
                            </div><div style={{ marginTop: 16 }}>
                                <TextField
                                    required

                                    sx={{ maxWidth: this.state.controlWidth }}
                                    autoComplete="off"
                                    id="name"
                                    spellCheck="false"
                                    label="Service Name"
                                    type="text"
                                    size={this.state.controlSize}
                                    fullWidth
                                    error={this.state.nameError}
                                    helperText={this.state.nameErrorMessage}
                                    value={this.state.name}
                                    onChange={(e) => this.setState({ name: e.target.value, nameError: false, nameErrorMessage: NBSP })}
                                    inputRef={this.refName}
                                    variant="filled"
                                />
                            </div>
                            <div>
                                <TextField
                                    inputRef={this.refPassword}
                                    sx={{ maxWidth: this.state.controlWidth }}
                                    style={{ maxWidth: this.state.controlWidth }}
                                    size={this.state.controlSize}
                                    autoComplete="off"
                                    onChange={(e): void => { this.setState({ pin: e.target.value, pinError: false, pinErrorMessage: NBSP }); }}
                                    label="Pin"
                                    inputProps={{ inputMode: 'numeric' }}
                                    fullWidth
                                    error={this.state.pinError}
                                    helperText={this.state.pinErrorMessage}
                                    defaultValue={this.state.pin}
                                    disabled={!this.state.enabled}
                                    variant="filled"
                                />
                            </div>
                            <div style={{ marginTop: 0, marginBottom: 16, width: this.state.controlWidth, marginLeft: "auto", marginRight: 0, textAlign: "right" }}>
                                <Button size="small" disabled={!this.state.enabled}
                                    onClick={() => this.onGenerateRandomPin()}
                                >Generate random pin</Button>
                            </div>
                            <div style={{ marginBottom: 16 }}>
                                <TextField select label="Country" size={this.state.controlSize} value={this.state.countryCode} variant="filled" sx={{ width: this.state.controlWidth }}
                                    disabled={!this.state.enabled}
                                    onChange={(event) => { this.handleCountryChanged(event); }}
                                >
                                    {Object.entries(this.model.countryCodes).map(([key, value]) => {
                                        return (
                                            <MenuItem value={key}>{value}</MenuItem>
                                        );
                                    })}
                                </TextField>



                            </div>
                            <div style={{ marginBottom: 24 }}>
                                <TextField select id="channelSelect" size={this.state.controlSize} label="Channel" value={this.state.channel} variant="filled" sx={{ width: this.state.controlWidth }}
                                    disabled={!this.state.enabled}
                                    onChange={(event) => { this.handleChannelChange(event); }}
                                >
                                    {this.state.wifiChannels.map((channel) => {
                                        let t = channel.channelId;
                                        if (t.startsWith("a")) t = t.substring(1);
                                        if (t.startsWith("g")) t = t.substring(1);
                                        let name = channel.channelName;
                                        if (name.startsWith("1 ") || name.startsWith("6 ") || name.startsWith("11 ")) {
                                            name += " *Recommended";
                                        }
                                        return (
                                            <MenuItem value={t}>{name}</MenuItem>
                                        )
                                    })}
                                </TextField>

                            </div>
                        </DialogContent>
                    )}

                    <DialogActions>
                        <Button onClick={handleClose} variant="dialogSecondary"  style={{ width: 120 }}>
                            Cancel
                        </Button>
                        <Button onClick={() => this.handleOk()} variant="dialogPrimary"  style={{ width: 120 }} >
                            OK
                        </Button>
                    </DialogActions>
                </DialogEx>
            );
        }
    };

export default WifiDirectConfigDialog;