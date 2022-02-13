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


import React, { Component } from 'react';

import { Theme } from '@mui/material/styles';

import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';

import Button from '@mui/material/Button';
import DialogActions from '@mui/material/DialogActions';
import Dialog from '@mui/material/Dialog';
import JackServerSettings from './JackServerSettings';


import InputLabel from '@mui/material/InputLabel';
import FormControl from '@mui/material/FormControl';
import Select from '@mui/material/Select';
import DialogContent from '@mui/material/DialogContent';
import MenuItem from '@mui/material/MenuItem';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';

import AlsaDeviceInfo from './AlsaDeviceInfo';

const INVALID_DEVICE_ID = "_invalid_";
interface JackServerSettingsDialogState {
    latencyText: string;
    jackServerSettings: JackServerSettings;
    alsaDevices?: AlsaDeviceInfo[];
}

const styles = (theme: Theme) =>
    createStyles({
        formControl: {
            margin: theme.spacing(1),
            minWidth: 120,
        },
        selectEmpty: {
            marginTop: theme.spacing(2),
        },
    });
export interface JackServerSettingsDialogProps extends WithStyles<typeof styles> {
    open: boolean;
    jackServerSettings: JackServerSettings;
    onClose: () => void;
    onApply: (jackServerSettings: JackServerSettings) => void;
}

function getLatencyText(settings: JackServerSettings ): string {
    if (!settings.valid) return "\u00A0";
    let ms = settings.bufferSize * settings.numberOfBuffers / settings.sampleRate * 1000;
    return ms.toFixed(1) + "ms";
}

const JackServerSettingsDialog = withStyles(styles)(
    class extends Component<JackServerSettingsDialogProps, JackServerSettingsDialogState> {

        model: PiPedalModel;

        constructor(props: JackServerSettingsDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();
            

            this.state = {
                latencyText: "\u00A0",
                jackServerSettings: props.jackServerSettings.clone() // invalid, but not nullish
            };
        }
        mounted: boolean = false;

        requestAlsaInfo() {
            this.model.getAlsaDevices()
                .then((devices) => {
                    if (this.mounted) {
                        if (this.props.open) {
                            let settings = this.applyAlsaDevices(this.props.jackServerSettings, devices);
                            this.setState({
                                alsaDevices: devices,
                                jackServerSettings: settings,
                                latencyText: getLatencyText(settings)
                            });
                        } else {
                            this.setState({ alsaDevices: devices });
                        }
                    }
                })
                .catch((error) => {

                });
        }

        applyAlsaDevices(jackServerSettings: JackServerSettings, alsaDevices?: AlsaDeviceInfo[]) {
            let result = jackServerSettings.clone();
            if (!alsaDevices) {
                return result;
            }
            if (alsaDevices.length === 0) {
                result.valid = false;
                result.alsaDevice = INVALID_DEVICE_ID;
                return result;
            }

            let selectedDevice: AlsaDeviceInfo | undefined = undefined;
            for (let i = 0; i < alsaDevices.length; ++i) {
                if (alsaDevices[i].id === result.alsaDevice) {
                    selectedDevice = alsaDevices[i]
                    break;
                }
            }
            if (!selectedDevice) {
                selectedDevice = alsaDevices[0];
                result.alsaDevice = selectedDevice.id;

            }
            if (result.sampleRate === 0) result.sampleRate = 48000;
            if (result.bufferSize === 0) result.bufferSize = 64;
            if (result.numberOfBuffers === 0) result.numberOfBuffers = 3;
            result.sampleRate = selectedDevice.closestSampleRate(result.sampleRate);
            result.bufferSize = selectedDevice.closestBufferSize(result.bufferSize);
            result.valid = true;
            return result;
        }

        componentDidMount() {
            this.mounted = true;
            this.requestAlsaInfo();

        }
        componentDidUpdate(oldProps: JackServerSettingsDialogProps) {
            if (this.props.open && !oldProps.open) {
                let settings = this.applyAlsaDevices(this.props.jackServerSettings.clone(), this.state.alsaDevices);
                this.setState({ 
                    jackServerSettings: settings,
                    latencyText: getLatencyText(settings)
                 });
                this.requestAlsaInfo();
            }
        }

        componentWillUnmount() {
            this.mounted = false;

        }
        getSelectedDevice(deviceId: string): AlsaDeviceInfo | null {
            if (!this.state.alsaDevices) return null;
            for (let i = 0; i < this.state.alsaDevices.length; ++i) {
                if (this.state.alsaDevices[i].id === deviceId) {
                    return this.state.alsaDevices[i];
                }

            }
            return null;
        }

        handleDeviceChanged(e: any) {
            let device = e.target.value as string;

            let selectedDevice = this.getSelectedDevice(device);
            if (!selectedDevice) return;
            let settings = this.state.jackServerSettings.clone();
            settings.alsaDevice = device;
            settings.sampleRate = selectedDevice.closestSampleRate(settings.sampleRate);
            settings.bufferSize = selectedDevice.closestBufferSize(settings.bufferSize);

            this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings)
            });
        }
        handleRateChanged(e: any) {
            let rate = e.target.value as number;
            console.log("New rate: " + rate);
            let selectedDevice = this.getSelectedDevice(this.state.jackServerSettings.alsaDevice);
            if (!selectedDevice) return;
            let settings = this.state.jackServerSettings.clone();
            settings.sampleRate = rate;

            this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings)
            });
        }
        handleSizeChanged(e: any) {
            let size = e.target.value as number;

            let selectedDevice = this.getSelectedDevice(this.state.jackServerSettings.alsaDevice);
            if (!selectedDevice) return;
            let settings = this.state.jackServerSettings.clone();
            settings.bufferSize = size;

            this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings)
            });
        }
        handleNumberOfBuffersChanged(e: any) {
            let bufferCount = e.target.value as number;

            let selectedDevice = this.getSelectedDevice(this.state.jackServerSettings.alsaDevice);
            if (!selectedDevice) return;
            let settings = this.state.jackServerSettings.clone();
            settings.numberOfBuffers = bufferCount;

            this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings)
            });
        }

        handleApply() {

            this.props.onApply(this.state.jackServerSettings.clone());
        };
        getBufferSizes(alsaDevice: AlsaDeviceInfo): number[] {
            let result: number[] = [];
            let max = alsaDevice.maxBufferSize;
            if (max > 2048) max = 2048;

            for (let i = 16; i <= max; i *= 2) {
                if (i >= alsaDevice.minBufferSize) {
                    result.push(i);
                }
            }
            return result;
        }


        render() {
            const classes = this.props.classes;

            const { onClose, open } = this.props;

            const handleClose = () => {
                onClose();
            };
            let waitingForDevices = !this.state.alsaDevices
            let noDevices = this.state.alsaDevices && this.state.alsaDevices.length === 0;
            let selectedDevice: AlsaDeviceInfo | undefined = undefined;
            if (this.state.alsaDevices) {
                for (let device of this.state.alsaDevices) {
                    if (device.id === this.state.jackServerSettings.alsaDevice) {
                        selectedDevice = device;
                        break;
                    }
                }
            }
            let bufferSizes: number[] = [];
            if (selectedDevice) {
                bufferSizes = this.getBufferSizes(selectedDevice);
            }

            return (
                <Dialog onClose={handleClose} aria-labelledby="select-channels-title" open={open}>
                    <DialogContent>
                        <div>
                            <FormControl className={classes.formControl}>
                                <InputLabel htmlFor="jsd_device">Device</InputLabel>
                                <Select onChange={(e) => this.handleDeviceChanged(e)}
                                    value={this.state.jackServerSettings.alsaDevice}
                                    style={{width: 220}}
                                    inputProps={{
                                        name: "Device",
                                        id: "jsd_device",
                                    }}
                                >
                                    {(noDevices && !waitingForDevices) &&
                                        (
                                            <MenuItem value={INVALID_DEVICE_ID}>No suitable devices.</MenuItem>
                                        )
                                    }
                                    {((!noDevices) && !waitingForDevices) && (
                                        this.state.alsaDevices!.map((device) =>
                                        (
                                            <MenuItem key={device.id} value={device.id}>{device.name}</MenuItem>
                                        )

                                        )
                                    )}
                                </Select>
                            </FormControl>
                        </div><div>
                            <FormControl className={classes.formControl}>
                                <InputLabel htmlFor="jsd_sampleRate">Sample rate</InputLabel>
                                <Select
                                    onChange={(e) => this.handleRateChanged(e)}
                                    value={this.state.jackServerSettings.sampleRate}
                                    inputProps={{
                                        id: 'jsd_sampleRate',
                                        style: {
                                            textAlign: "right"
                                        }
                                    }}
                                >
                                    {selectedDevice &&
                                        selectedDevice.sampleRates.map((sr) => {
                                            return ( <MenuItem value={sr}>{sr}</MenuItem> );
                                        })
                                    }
                                </Select>
                            </FormControl>
                            <div style={{ display: "inline", whiteSpace: "nowrap" }}>
                                <FormControl className={classes.formControl}>
                                    <InputLabel htmlFor="bufferSize">Buffer size</InputLabel>
                                    <Select
                                        onChange={(e) => this.handleSizeChanged(e)}
                                        value={this.state.jackServerSettings.bufferSize}
                                        inputProps={{
                                            name: 'Buffer size',
                                            id: 'jsd_bufferSize',
                                        }}
                                    >
                                        {bufferSizes.map((buffSize) =>
                                        (
                                            <MenuItem value={buffSize}>{buffSize}</MenuItem>

                                        )
                                        )}
                                    </Select>
                                </FormControl>
                                <FormControl className={classes.formControl}>
                                    <InputLabel htmlFor="numberofBuffers">Buffers</InputLabel>
                                    <Select
                                        onChange={(e) => this.handleNumberOfBuffersChanged(e)}
                                        value={this.state.jackServerSettings.numberOfBuffers}
                                        inputProps={{
                                            name: 'Number of buffers',
                                            id: 'jsd_bufferCount',
                                        }}
                                    >
                                        <MenuItem value={2}>2</MenuItem>
                                        <MenuItem value={3}>3</MenuItem>
                                        <MenuItem value={4}>4</MenuItem>
                                    </Select>
                                </FormControl>
                            </div>
                        </div>
                        <Typography display="block" variant="caption" style={{ textAlign: "left", marginTop: 8, marginLeft: 24 }}
                            color="textSecondary">
                            Latency: {this.state.latencyText}
                        </Typography>
                    </DialogContent>

                    <DialogActions>
                        <Button onClick={handleClose} color="primary">
                            Cancel
                        </Button>
                        <Button onClick={() => this.handleApply()} color="secondary" disabled={
                            (!this.state.alsaDevices) || !this.state.jackServerSettings.valid}>
                            OK
                        </Button>
                    </DialogActions>

                </Dialog>
            );
        }
    });

export default JackServerSettingsDialog;
