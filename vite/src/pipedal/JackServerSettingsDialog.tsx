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


import { Component } from 'react';

import { Theme } from '@mui/material/styles';

import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";

import Button from '@mui/material/Button';
import DialogActions from '@mui/material/DialogActions';
import DialogEx from './DialogEx';
import JackServerSettings from './JackServerSettings';


import InputLabel from '@mui/material/InputLabel';
import FormControl from '@mui/material/FormControl';
import Select from '@mui/material/Select';
import DialogContent from '@mui/material/DialogContent';
import MenuItem from '@mui/material/MenuItem';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';

import AlsaDeviceInfo from './AlsaDeviceInfo';

const  MIN_BUFFER_SIZE = 16;
const  MAX_BUFFER_SIZE = 2048;


interface BufferSetting {
    bufferSize: number;
    numberOfBuffers: number;
};

const INVALID_DEVICE_ID = "_invalid_";
interface JackServerSettingsDialogState {
    latencyText: string;
    jackServerSettings: JackServerSettings;
    alsaDevices?: AlsaDeviceInfo[];
    okEnabled: boolean;
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

function getLatencyText(settings?: JackServerSettings ): string {
    if (!settings)
    {
        return "\u00A0";
    }

    if (!settings.valid) return "\u00A0";
    let ms = settings.bufferSize * settings.numberOfBuffers / settings.sampleRate * 1000;
    return ms.toFixed(1) + "ms";
}

function getValidBufferCounts(bufferSize: number, alsaDeviceInfo?: AlsaDeviceInfo): number[]
{
    if (!alsaDeviceInfo)
    {
        return [];
    }
    let result: number[] = [];
    if (bufferSize * 2 >= alsaDeviceInfo.minBufferSize)
    {
        result =  [2,3,4,5,6];
    } else {
        let minBuffers = Math.ceil(alsaDeviceInfo.minBufferSize/bufferSize/2-0.0001);
        result = [minBuffers*2,minBuffers*3, minBuffers*4, minBuffers*5, minBuffers*6];
    }
    for (let i = 0; i < result.length; ++i)
    {
        let selectedSize = result[i]*bufferSize;
        if (selectedSize < alsaDeviceInfo.minBufferSize || selectedSize > alsaDeviceInfo.maxBufferSize)
        {
            result.splice(i,1);
            --i;
        }
    }

    return result;
}

function intersectArrays(a: number[], b: number[]): number[] {
    return a.filter((v) => b.indexOf(v) !== -1);
}

function getValidBufferCountsMultiple(bufferSize: number, inDevice?: AlsaDeviceInfo, outDevice?: AlsaDeviceInfo): number[] {
    let c1 = getValidBufferCounts(bufferSize, inDevice);
    if (!outDevice) return c1;
    return intersectArrays(c1, getValidBufferCounts(bufferSize, outDevice));
}

function getValidBufferSizesMultiple(inDevice?: AlsaDeviceInfo, outDevice?: AlsaDeviceInfo): number[] {
    let s1 = getValidBufferSizes(inDevice);
    if (!outDevice) return s1;
    return intersectArrays(s1, getValidBufferSizes(outDevice));
}
function getValidBufferSizes(alsaDeviceInfo? : AlsaDeviceInfo): number[]
{
    if (!alsaDeviceInfo )
    {
        return [];
    }
    let result: number[] = [];
    for (let i = MIN_BUFFER_SIZE; i <= MAX_BUFFER_SIZE; i *= 2)
    {
        if (getValidBufferCounts(i,alsaDeviceInfo).length !== 0)
        {
            result.push(i);
        }
    }
    return result;
}
function getBestBuffers(
    inDevice: AlsaDeviceInfo | undefined,
    outDevice: AlsaDeviceInfo | undefined,
    bufferSize: number,
    numberOfBuffers: number,
    bufferSizeChanging: boolean = false
) : BufferSetting {
    if (!inDevice && !outDevice)
    {
        return { bufferSize:bufferSize, numberOfBuffers: numberOfBuffers};
    }
    const device = inDevice || outDevice;
    if (!device)
    {
        return { bufferSize:bufferSize, numberOfBuffers:numberOfBuffers};
    }

    // If the numberOfbuffers is fine as is, don't change the number of Buffers.
    // set default values. Otherwise, choose the best buffer count from available buffer counts.
    let validBuffercounts = getValidBufferCountsMultiple(bufferSize,inDevice,outDevice);
    const minSize = Math.max(inDevice?.minBufferSize ?? MIN_BUFFER_SIZE, outDevice?.minBufferSize ?? MIN_BUFFER_SIZE);
    const maxSize = Math.min(inDevice?.maxBufferSize ?? MAX_BUFFER_SIZE, outDevice?.maxBufferSize ?? MAX_BUFFER_SIZE);
    let ix = validBuffercounts.indexOf(numberOfBuffers);
    if (ix !== -1) {
    if (bufferSize*numberOfBuffers <= maxSize
            && bufferSize*numberOfBuffers >= minSize)
        {
            return {bufferSize: bufferSize, numberOfBuffers: numberOfBuffers};
        }
    }
    // if numberOfBuffers is not valid, and the user has just selected a new buffers size,
    // then choose from available buffer counts.
    if (bufferSizeChanging)
    {
        if (validBuffercounts.length !== 0)
        {
            // prefer the one thats divisible by 3.
            for (let i = 0; i < validBuffercounts.length; ++i)
            {
                if (validBuffercounts[i] % 3 === 0)
                {
                    numberOfBuffers = validBuffercounts[i];
                    return {bufferSize: bufferSize, numberOfBuffers:numberOfBuffers};
                }
            }
            numberOfBuffers = validBuffercounts[0];
            return {bufferSize: bufferSize, numberOfBuffers:numberOfBuffers};
        }
    }
    // otherwise select a sensible starting value.

    // favored default: 64x3.
  if (64*3 >= minSize && 64*3 <= maxSize)
    {
        return { bufferSize: 64, numberOfBuffers: 3};
    }
    // if that isn't possible then minBufferSize/2 x 4.
 bufferSize = minSize/2;
    numberOfBuffers = 4;
    // otherwise, minBufferSize/2 x 2.
    if (bufferSize*numberOfBuffers > maxSize)
    {
        numberOfBuffers = 2;
    }
    return { bufferSize: bufferSize, numberOfBuffers: numberOfBuffers};

};

function isOkEnabled(jackServerSettings: JackServerSettings, alsaDevices?: AlsaDeviceInfo[])
{
    if (!jackServerSettings.valid) return false;
    if (!alsaDevices) return false;
	const inDevice = alsaDevices.find(d => d.id === jackServerSettings.alsaInputDevice);
    const outDevice = alsaDevices.find(d => d.id === jackServerSettings.alsaOutputDevice);
    if (!inDevice || !outDevice) return false;

    const deviceBufferSize = jackServerSettings.bufferSize * jackServerSettings.numberOfBuffers;
    const minSize = Math.max(inDevice.minBufferSize, outDevice.minBufferSize);
    const maxSize = Math.min(inDevice.maxBufferSize, outDevice.maxBufferSize);
    if (deviceBufferSize < minSize || deviceBufferSize > maxSize) return false;

    const validBufferCounts = getValidBufferCountsMultiple(jackServerSettings.bufferSize, inDevice, outDevice);
    if (validBufferCounts.indexOf(jackServerSettings.numberOfBuffers) === -1) return false;

    const sampleRates = intersectArrays(inDevice.sampleRates, outDevice.sampleRates);
    if (sampleRates.indexOf(jackServerSettings.sampleRate) === -1) return false;

    return true;
}


const JackServerSettingsDialog = withStyles(
    class extends Component<JackServerSettingsDialogProps, JackServerSettingsDialogState> {

        model: PiPedalModel;

        constructor(props: JackServerSettingsDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();
            

            this.state = {
                latencyText: getLatencyText(props.jackServerSettings),
                jackServerSettings: props.jackServerSettings.clone(), // invalid, but not nullish
                alsaDevices: undefined,
                okEnabled: false
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
                                latencyText: getLatencyText(settings),
                                okEnabled: isOkEnabled(settings,devices)
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
           if (!alsaDevices || alsaDevices.length === 0) {
                result.valid = false;
                result.alsaInputDevice = INVALID_DEVICE_ID;
                result.alsaOutputDevice = INVALID_DEVICE_ID;
                return result;
            }

            let inDevice = alsaDevices.find(d => d.id === result.alsaInputDevice);
            let outDevice = alsaDevices.find(d => d.id === result.alsaOutputDevice);
            if (!inDevice) {
                inDevice = alsaDevices[0];
                result.alsaInputDevice = inDevice.id;
            }
            if (!outDevice) {
                outDevice = alsaDevices[0];
                result.alsaOutputDevice = outDevice.id;
            }
            
			if (result.sampleRate === 0) result.sampleRate = 48000;
			 let sampleRates = intersectArrays(inDevice.sampleRates, outDevice.sampleRates);
            if (sampleRates.length === 0) sampleRates = inDevice.sampleRates;
            let bestSr = sampleRates[0];
            let bestErr = 1e36;
            for (let sr of sampleRates) {
                let err = (sr - result.sampleRate) * (sr - result.sampleRate);
                if (err < bestErr) { bestErr = err; bestSr = sr; }
            }
            result.sampleRate = bestSr;

            let bestBuffers =  getBestBuffers(inDevice, outDevice, result.bufferSize, result.numberOfBuffers);
            result.bufferSize = bestBuffers.bufferSize;
            result.numberOfBuffers = bestBuffers.numberOfBuffers;

            result.valid = !!result.alsaInputDevice && !!result.alsaOutputDevice;
            return result;
        }

        componentDidMount() {
            this.mounted = true;
            if (this.props.open) {
                this.requestAlsaInfo();
            }

        }
        componentDidUpdate(oldProps: JackServerSettingsDialogProps) {
            if ((this.props.open && !oldProps.open) && this.mounted) {
                let settings = this.applyAlsaDevices(this.props.jackServerSettings.clone(), this.state.alsaDevices);
                this.setState({ 
                    jackServerSettings: settings,
                    latencyText: getLatencyText(settings),
                    okEnabled:  isOkEnabled(settings,this.state.alsaDevices)
                 });
                if (!this.state.alsaDevices) {
                    this.requestAlsaInfo();
                }
            }
        }

        componentWillUnmount() {
            this.mounted = false;

        }
       getSelectedDevice(deviceId: string): AlsaDeviceInfo | undefined {
            if (!this.state.alsaDevices) return undefined;
            for (let i = 0; i < this.state.alsaDevices.length; ++i) {
                if (this.state.alsaDevices[i].id === deviceId) {
                    return this.state.alsaDevices[i];
                }

            }
             return undefined;
        }

        handleRateChanged(e: any) {
            let rate = e.target.value as number;
             let inDev = this.getSelectedDevice(this.state.jackServerSettings.alsaInputDevice);
            let outDev = this.getSelectedDevice(this.state.jackServerSettings.alsaOutputDevice);
            if (!inDev && !outDev) return;
            let settings = this.state.jackServerSettings.clone();
            settings.sampleRate = rate;

            this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings),
                okEnabled: isOkEnabled(settings,this.state.alsaDevices)
            });
        }
        handleSizeChanged(e: any) {
            let size = e.target.value as number;

            let inDev = this.getSelectedDevice(this.state.jackServerSettings.alsaInputDevice);
            let outDev = this.getSelectedDevice(this.state.jackServerSettings.alsaOutputDevice);
            if (!inDev && !outDev) return;
            let settings = this.state.jackServerSettings.clone();
            settings.bufferSize = size;
              let bestBufferSetting = getBestBuffers(inDev,outDev,settings.bufferSize,settings.numberOfBuffers,true);
            settings.bufferSize = bestBufferSetting.bufferSize;
            settings.numberOfBuffers = bestBufferSetting.numberOfBuffers;

            this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings),
                okEnabled: isOkEnabled(settings,this.state.alsaDevices)
            });
        }
        handleNumberOfBuffersChanged(e: any) {
            let bufferCount = e.target.value as number;

            let inDev = this.getSelectedDevice(this.state.jackServerSettings.alsaInputDevice);
            let outDev = this.getSelectedDevice(this.state.jackServerSettings.alsaOutputDevice);
            if (!inDev && !outDev) return;
            let settings = this.state.jackServerSettings.clone();
            settings.numberOfBuffers = bufferCount;

            this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings),
                okEnabled: isOkEnabled(settings,this.state.alsaDevices)
            });
        }

        handleApply() {
            if (this.state.okEnabled)
            {
                this.props.onApply(this.state.jackServerSettings.clone());
            }
        };
		handleInputDeviceChanged(e: any) {
			const d = e.target.value as string;
			let s = this.state.jackServerSettings.clone();
			s.alsaInputDevice = d;
			this.setState({
			jackServerSettings: s,
			okEnabled: isOkEnabled(s, this.state.alsaDevices)
			});
		}
		
		handleOutputDeviceChanged(e: any) {
			const d = e.target.value as string;
			let s = this.state.jackServerSettings.clone();
			s.alsaOutputDevice = d;
			this.setState({
			jackServerSettings: s,
			okEnabled: isOkEnabled(s, this.state.alsaDevices)
			});
		}
        render() {
            const classes = withStyles.getClasses(this.props);

            const { onClose, open } = this.props;

            const handleClose = () => {
                onClose();
            };
           
			let selectedInputDevice = this.getSelectedDevice(this.state.jackServerSettings.alsaInputDevice);
            let selectedOutputDevice = this.getSelectedDevice(this.state.jackServerSettings.alsaOutputDevice);

            let bufferSizes: number[] = getValidBufferSizesMultiple(selectedInputDevice, selectedOutputDevice);
            let bufferCounts = getValidBufferCountsMultiple(this.state.jackServerSettings.bufferSize, selectedInputDevice, selectedOutputDevice);
            let bufferSizeDisabled = !(selectedInputDevice || selectedOutputDevice);
            let bufferCountDisabled = !(selectedInputDevice || selectedOutputDevice);
            let sampleRates = selectedInputDevice && selectedOutputDevice ?
                    intersectArrays(selectedInputDevice.sampleRates, selectedOutputDevice.sampleRates)
                    : (selectedInputDevice ? selectedInputDevice.sampleRates : (selectedOutputDevice ? selectedOutputDevice.sampleRates : []));

            return (
                <DialogEx tag="jack" onClose={handleClose} aria-labelledby="select-channels-title" open={open}
                    onEnterKey={() => {
                        this.handleApply();
                    }}

                >
                    <DialogContent>
                        <div>
						 {/* Audio Input Device */}
                            <FormControl className={classes.formControl}>
							<InputLabel htmlFor="jsd_inputDevice">Input Device</InputLabel>
						<Select
						id="jsd_inputDevice"
						value={this.state.jackServerSettings.alsaInputDevice}
						onChange={e => this.handleInputDeviceChanged(e)}
						disabled={!this.state.alsaDevices || this.state.alsaDevices.length === 0}
						style={{ width: 220 }}
						>
						{this.state.alsaDevices?.map(d => (
								<MenuItem key={d.id} value={d.id}>{d.name}</MenuItem>
						)) || <MenuItem value="" disabled>Loading...</MenuItem>}
						</Select>
						</FormControl>
						
						{/* Audio Output Device */}
						<FormControl className={classes.formControl}>
							<InputLabel htmlFor="jsd_outputDevice">Output Device</InputLabel>
							  <Select
								id="jsd_outputDevice"
								value={this.state.jackServerSettings.alsaOutputDevice}
								onChange={e => this.handleOutputDeviceChanged(e)}
								disabled={!this.state.alsaDevices || this.state.alsaDevices.length === 0}
								style={{ width: 220 }}
								>
								{this.state.alsaDevices?.map(d => (
										<MenuItem key={d.id} value={d.id}>{d.name}</MenuItem>
								)) || <MenuItem value="" disabled>Loading...</MenuItem>}
								</Select>
						</FormControl>
                        </div><div>
                            <FormControl className={classes.formControl}>
                                <InputLabel htmlFor="jsd_sampleRate">Sample rate</InputLabel>
                                <Select variant="standard"
                                    onChange={(e) => this.handleRateChanged(e)}
                                    value={this.state.jackServerSettings.sampleRate}
                                     disabled={sampleRates.length === 0}
                                    inputProps={{
                                        id: 'jsd_sampleRate',
                                        style: {
                                            textAlign: "right"
                                        }
                                    }}
                                >
                                 {sampleRates.map((sr) => {
                                            return (<MenuItem value={sr}>{sr}</MenuItem> );
                                    })}
                                </Select>
                            </FormControl>
                            <div style={{ display: "inline", whiteSpace: "nowrap" }}>
                                <FormControl className={classes.formControl}>
                                    <InputLabel htmlFor="bufferSize">Buffer size</InputLabel>
                                    <Select variant="standard"
                                        onChange={(e) => this.handleSizeChanged(e)}
                                        value={this.state.jackServerSettings.bufferSize}
                                        disabled={bufferSizeDisabled}
                                        inputProps={{
                                            name: 'Buffer size',
                                            id: 'jsd_bufferSize',
                                        }}
                                    >
                                        {bufferSizes.map((buffSize) =>
                                        (
                                            <MenuItem key={"b"+buffSize} value={buffSize}>{buffSize}</MenuItem>

                                        )
                                        )}
                                    </Select>
                                </FormControl>
                                <FormControl className={classes.formControl}>
                                    <InputLabel htmlFor="numberofBuffers">Buffers</InputLabel>
                                    <Select variant="standard"
                                        onChange={(e) => this.handleNumberOfBuffersChanged(e)}
                                        value={this.state.jackServerSettings.numberOfBuffers}
                                        disabled={bufferCountDisabled}
                                        inputProps={{
                                            name: 'Number of buffers',
                                            id: 'jsd_bufferCount',
                                        }}
                                    >
                                        {
                                            bufferCounts.map((bufferCount)=>
                                            {
                                                return (
                                                    <MenuItem key={bufferCount} value={bufferCount}>{bufferCount.toString()}</MenuItem>

                                                );
                                            })
                                        }
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
                        <Button variant="dialogSecondary" onClick={handleClose} >
                            Cancel
                        </Button>
                        <Button variant="dialogPrimary" onClick={() => this.handleApply()} 
                            disabled={!this.state.okEnabled}>
                            OK
                        </Button>
                    </DialogActions>

                </DialogEx>
            );
        }
    },
    styles);

export default JackServerSettingsDialog;
