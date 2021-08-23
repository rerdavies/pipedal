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

import { createStyles, WithStyles, withStyles, Theme } from '@material-ui/core/styles';

import Button from '@material-ui/core/Button';
import DialogActions from '@material-ui/core/DialogActions';
import Dialog from '@material-ui/core/Dialog';
import JackServerSettings from './JackServerSettings';


import InputLabel from '@material-ui/core/InputLabel';
import FormControl from '@material-ui/core/FormControl';
import Select from '@material-ui/core/Select';
import DialogContent from '@material-ui/core/DialogContent';
import MenuItem from '@material-ui/core/MenuItem';
import { nullCast } from './Utility';
import Typography from '@material-ui/core/Typography';

interface JackServerSettingsDialogState {
    latencyText: string;
};

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
    onApply: (sampleRate: number, bufferSize: number, numberOfBuffers: number) => void;
}
    
function getLatencyText(sampleRate: number, bufferSize: number, numberOfBuffers: number): string {
    let ms = bufferSize * numberOfBuffers / sampleRate * 1000;
    return ms.toFixed(1) + "ms";
}

const JackServerSettingsDialog = withStyles(styles)(
    class extends Component<JackServerSettingsDialogProps, JackServerSettingsDialogState> {
        constructor(props: JackServerSettingsDialogProps) {
            super(props);
            let jackServerSettings = props.jackServerSettings;

            this.state = {
                latencyText:
                    getLatencyText(
                        jackServerSettings.sampleRate,
                        jackServerSettings.bufferSize,
                        jackServerSettings.numberOfBuffers)
            };
        }

        updateLatency(e: any): void {
            setTimeout(()=> {
                let sampleRate: number = parseInt(nullCast<any>(document.getElementById('jsd_sampleRate')).value);
                let bufferSize: number = parseInt(nullCast<any>(document.getElementById('jsd_bufferSize')).value);
                let bufferCount: number = parseInt(nullCast<any>(document.getElementById('jsd_bufferCount')).value);
                this.setState({
                    latencyText: getLatencyText(sampleRate, bufferSize, bufferCount)
                });
            },0);
        }
        handleApply() {
            let sampleRate = parseInt(nullCast<any>(document.getElementById('jsd_sampleRate')).value);
            let bufferSize = parseInt(nullCast<any>(document.getElementById('jsd_bufferSize')).value);
            let bufferCount = parseInt(nullCast<any>(document.getElementById('jsd_bufferCount')).value);

            this.props.onApply(sampleRate,bufferSize,bufferCount);
        };
            

        render() {
            const classes = this.props.classes;

            const { onClose, jackServerSettings, open } = this.props;

            const handleClose = () => {
                onClose();
            };

            return (
                <Dialog onClose={handleClose} aria-labelledby="select-channels-title" open={open}>
                    <DialogContent>
                        <FormControl className={classes.formControl}>
                            <InputLabel htmlFor="sampleRate">Sample rate</InputLabel>
                            <Select
                                onChange={(e) => this.updateLatency(e)}
                                defaultValue={jackServerSettings.sampleRate}
                                inputProps={{
                                    name: 'Sample Rate',
                                    id: 'jsd_sampleRate',
                                    style: {
                                        textAlign: "right"
                                    }
                                }}
                            >
                                <MenuItem value={44100}>44100</MenuItem>
                                <MenuItem value={48000}>48000</MenuItem>
                                <MenuItem value={88200}>88200</MenuItem>
                                <MenuItem value={96000}>96000</MenuItem>
                                <MenuItem value={176400}>176400</MenuItem>
                                <MenuItem value={192000}>192000</MenuItem>
                            </Select>
                        </FormControl>
                        <div style={{ display: "inline", whiteSpace: "nowrap" }}>
                            <FormControl className={classes.formControl}>
                                <InputLabel htmlFor="bufferSize">Buffer size</InputLabel>
                                <Select
                                    onChange={(e) => this.updateLatency(e)}
                                    defaultValue={jackServerSettings.bufferSize}
                                    inputProps={{
                                        name: 'Buffer size',
                                        id: 'jsd_bufferSize',
                                    }}
                                >
                                    <MenuItem value={16}>16</MenuItem>
                                    <MenuItem value={32}>32</MenuItem>
                                    <MenuItem value={64}>64</MenuItem>
                                    <MenuItem value={128}>128</MenuItem>
                                    <MenuItem value={256}>256</MenuItem>
                                    <MenuItem value={512}>512</MenuItem>
                                    <MenuItem value={1024}>1024</MenuItem>
                                </Select>
                            </FormControl>
                            <FormControl className={classes.formControl}>
                                <InputLabel htmlFor="numberofBuffers">Buffers</InputLabel>
                                <Select
                                    onChange={(e) => this.updateLatency(e)}
                                    defaultValue={jackServerSettings.numberOfBuffers}
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
                        <Typography display="block" variant="caption" style={{ textAlign: "left", marginTop: 8, marginLeft: 24 }}
                            color="textSecondary">
                            Latency: {this.state.latencyText}
                        </Typography>
                    </DialogContent>

                    <DialogActions>
                        <Button onClick={handleClose} color="primary">
                            Cancel
                        </Button>
                        <Button onClick={()=> this.handleApply()} color="secondary">
                            OK
                        </Button>
                    </DialogActions>

                </Dialog>
            );
        }
    });

export default JackServerSettingsDialog;
