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
import IconButtonEx from '@mui/icons-material/Refresh';
import RefreshIcon from '@mui/icons-material/Refresh';
import Checkbox from '@mui/material/Checkbox';
import FormControlLabel from '@mui/material/FormControlLabel';
import JackHostStatus from './JackHostStatus';

import AlsaDeviceInfo from './AlsaDeviceInfo';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';

function filterDevices(devices: AlsaDeviceInfo[]): AlsaDeviceInfo[] {
    return devices.filter(d => {
        const name = (d.name + ' ' + d.longName).toLowerCase();
        return !(name.includes('hdmi') || name.includes('bcm2835'));
    });
}

function sortDevices(devices: AlsaDeviceInfo[]): AlsaDeviceInfo[] {
    function category(d: AlsaDeviceInfo): number {
        if (d.supportsCapture && d.supportsPlayback) return 0;
        if (d.longName.toLowerCase().includes("usb")) return 1;
        return 2;
    }
    let copy = [...devices];
    copy.sort((a,b) => {
        let ca = category(a);
        let cb = category(b);
        if (ca !== cb) return ca - cb;
        return a.name.localeCompare(b.name);
    });
    return copy;
}

const  MIN_BUFFER_SIZE = 16;
const  MAX_BUFFER_SIZE = 2048;


interface BufferSetting {
    bufferSize: number;
    numberOfBuffers: number;
};

// empty string used when no valid device is selected - the default.
const INVALID_DEVICE_ID = "";
interface JackServerSettingsDialogState {
    latencyText: string;
    jackServerSettings: JackServerSettings;
    alsaDevices?: AlsaDeviceInfo[];
    okEnabled: boolean;
    fullScreen: boolean;
    compactWidth: boolean;
    jackStatus?: JackHostStatus;
    showDeviceWarning: boolean;
    dontShowWarningAgain: boolean;
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
        inputLabel: {
            whiteSpace: "nowrap"
        },
        cpuStatusColor: {
            color: theme.palette.text.secondary,
            opacity: 0.7
        }
    });
export interface JackServerSettingsDialogProps extends WithStyles<typeof styles> {
    open: boolean;
    jackServerSettings: JackServerSettings;
    onClose: () => void;
    onApply: (jackServerSettings: JackServerSettings) => void;
}

function getLatencyText(settings?: JackServerSettings ): string {
    if (!settings) {
        return "\u00A0";
    }

    if (!settings.sampleRate || !settings.bufferSize || !settings.numberOfBuffers) {
        return "\u00A0";
    }

    let ms = (settings.bufferSize * settings.numberOfBuffers) / settings.sampleRate * 1000;
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

    // Build candidate sizes with 32 first if present.
    let validBufferSizes = getValidBufferSizesMultiple(inDevice, outDevice);
    validBufferSizes.sort((a,b)=>a-b);
    if (validBufferSizes.indexOf(32) !== -1) {
        validBufferSizes = [32, ...validBufferSizes.filter(v => v !== 32)];
    }

  function tryPick(count: number): BufferSetting | undefined {
        for (let bs of validBufferSizes) {
            const counts = getValidBufferCountsMultiple(bs, inDevice, outDevice);
            if (counts.indexOf(count) !== -1) {
                if (bs * count >= minSize && bs * count <= maxSize) {
                    return { bufferSize: bs, numberOfBuffers: count };
                }
            }
        }
        return undefined;
    }

    let result = tryPick(4);
    if (!result) result = tryPick(3);
    if (!result) result = tryPick(2);
    if (result) return result;

    // Fallback to previous behaviour.
    if (64*3 >= minSize && 64*3 <= maxSize) {
        return { bufferSize: 64, numberOfBuffers: 3 };
    }
    bufferSize = minSize/2;
    numberOfBuffers = 4;
    if (bufferSize * numberOfBuffers > maxSize) {
        numberOfBuffers = 2;
    }
   return { bufferSize: bufferSize, numberOfBuffers: numberOfBuffers };

};

function isOkEnabled(jackServerSettings: JackServerSettings, alsaDevices?: AlsaDeviceInfo[])
{
    if (!alsaDevices) return false;
    if (!jackServerSettings.alsaInputDevice || !jackServerSettings.alsaOutputDevice) return false;

    const inDevice = alsaDevices.find(d => d.id === jackServerSettings.alsaInputDevice && d.supportsCapture);
    const outDevice = alsaDevices.find(d => d.id === jackServerSettings.alsaOutputDevice && d.supportsPlayback);
    if (!inDevice || !outDevice) return false;

    const sampleRates = intersectArrays(inDevice.sampleRates, outDevice.sampleRates);
    if (sampleRates.indexOf(jackServerSettings.sampleRate) === -1) return false;

    const deviceBufferSize = jackServerSettings.bufferSize * jackServerSettings.numberOfBuffers;
    const minSize = Math.max(inDevice.minBufferSize, outDevice.minBufferSize);
    const maxSize = Math.min(inDevice.maxBufferSize, outDevice.maxBufferSize);
    if (deviceBufferSize < minSize || deviceBufferSize > maxSize) return false;

    const validBufferCounts = getValidBufferCountsMultiple(jackServerSettings.bufferSize, inDevice, outDevice);
    if (validBufferCounts.indexOf(jackServerSettings.numberOfBuffers) === -1) return false;

    return true;
}


const JackServerSettingsDialog = withStyles(
    class extends ResizeResponsiveComponent<JackServerSettingsDialogProps, JackServerSettingsDialogState> {

        model: PiPedalModel;
        ignoreClose: boolean = false;

        constructor(props: JackServerSettingsDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();


            this.suppressDeviceWarning = localStorage.getItem("suppressSeparateDeviceWarning") === "1";
            this.state = {
                latencyText: getLatencyText(props.jackServerSettings),
                jackServerSettings: props.jackServerSettings.clone(), // invalid, but not nullish
                alsaDevices: undefined,
                okEnabled: false,
                fullScreen: this.getFullScreen(),
                compactWidth: document.documentElement.clientWidth < 600,
                jackStatus: undefined,
                showDeviceWarning: false,
                dontShowWarningAgain: false
            };
        }
        mounted: boolean = false;
        statusTimer?: number = undefined;

        suppressDeviceWarning: boolean;
        /**
         * Copy of the settings when the dialog is opened. Pressing Apply only tests these settings temporarily. Closing the dialog without OK re-applies the saved copy so the audio driver returns to its previous configuration.
         */
        originalJackServerSettings?: JackServerSettings;

        getFullScreen() {
            return document.documentElement.clientWidth < 420 ||
                document.documentElement.clientHeight < 700;
        }

        onWindowSizeChanged(width: number, height: number): void {
            super.onWindowSizeChanged(width, height);
            const fullScreen = this.getFullScreen();
            const compactWidth = width < 600;
            if (fullScreen !== this.state.fullScreen || compactWidth !== this.state.compactWidth) {
                this.setState({ fullScreen, compactWidth });
            }
        }

        requestAlsaInfo() {
            this.model.getAlsaDevices()
                .then((devices) => {
                    if (this.mounted) {
                        if (this.props.open) {
                            let f = filterDevices(devices);
                            let settings = this.applyAlsaDevices(this.state.jackServerSettings, f);
                            this.setState({
                                alsaDevices: f,
                                jackServerSettings: settings,
                                latencyText: getLatencyText(settings),
                                okEnabled: isOkEnabled(settings, f)
                            });
                        } else {
                             this.setState({ alsaDevices: filterDevices(devices) });
                        }
                    }
                })
                .catch((error) => {
                    // Error requesting ALSA info.
                });
        }

       tickStatus() {
            this.model.getJackStatus()
                .then(status => {
                    if (this.mounted) {
                        this.setState({ jackStatus: status });
                    }
                })
                .catch(() => { });
        }
        startStatusTimer() {
            if (this.statusTimer) return;
            this.tickStatus();
            this.statusTimer = window.setInterval(() => this.tickStatus(), 1000);
        }
        stopStatusTimer() {
            if (this.statusTimer) {
                clearInterval(this.statusTimer);
                this.statusTimer = undefined;
            }
        }

        /** Persist the current settings to permanent storage. */
        saveSettings(settings?: JackServerSettings): void {
            const s = (settings ?? this.state.jackServerSettings).clone();
            // Fire and forget. Errors will be handled by PiPedalModel's internal error handling (e.g., showAlert).
            this.model.setJackServerSettings(s);
        }

        /**
         * Save the current settings temporarily so they can be restored if the
         * dialog is cancelled.
         */
        saveSettingsTemporary(settings?: JackServerSettings) {
            this.originalJackServerSettings = (settings ?? this.state.jackServerSettings).clone();
        }

        /** Apply the provided settings to the audio system. */
        applySettings(settings?: JackServerSettings): void {
            const s = (settings ?? this.state.jackServerSettings).clone();
            s.valid = true;
            // Fire and forget. Errors will be handled by PiPedalModel's internal error handling.
            this.model.applyJackServerSettings(s);
        }

        /**
         * Revert the dialog state to the persisted settings. The reverted
         * settings are applied immediately as well.
         */
        revertSettings() {
            this.model.getJackServerSettings()
                .then((s) => {
                    const applied = this.applyAlsaDevices(s.clone(), this.state.alsaDevices);
                    this.setState({
                        jackServerSettings: applied,
                        latencyText: getLatencyText(applied),
                        okEnabled: isOkEnabled(applied, this.state.alsaDevices)
                    });
                    this.applySettings(s);
                })
                .catch(() => { });
        }

        /**
         * Release currently used ALSA devices by switching to the dummy audio
         * driver. This allows testing alternative devices without conflicts.
         */
        releaseDevices() {
            const dummy = new JackServerSettings();
            dummy.useDummyAudioDevice();
            // Fire and forget.
            this.model.applyJackServerSettings(dummy);
        }

        applyAlsaDevices(jackServerSettings: JackServerSettings, alsaDevices?: AlsaDeviceInfo[]) {
            let result = jackServerSettings.clone();
           if (!alsaDevices || alsaDevices.length === 0) {
                result.valid = false;
                result.alsaInputDevice = INVALID_DEVICE_ID;
                result.alsaOutputDevice = INVALID_DEVICE_ID;
                return result;
            }

            let devices = filterDevices(alsaDevices);
            let inDevice = devices.find(d => d.id === result.alsaInputDevice && d.supportsCapture);
            let outDevice = devices.find(d => d.id === result.alsaOutputDevice && d.supportsPlayback);

            if (!inDevice || !outDevice) {
                const capture = devices.filter(d => d.supportsCapture);
                const playback = devices.filter(d => d.supportsPlayback);
                if (capture.length === 1 && playback.length === 1) {
                    inDevice = capture[0];
                    outDevice = playback[0];
                    result.alsaInputDevice = inDevice.id;
                    result.alsaOutputDevice = outDevice.id;
                } else if (capture.length === 1 && capture[0].supportsPlayback && playback.length === 0) {
                    inDevice = capture[0];
                    outDevice = capture[0];
                    result.alsaInputDevice = inDevice.id;
                    result.alsaOutputDevice = outDevice.id;
                }
            }

            if (!inDevice || !outDevice) {
                result.valid = false;
                if (!inDevice) result.alsaInputDevice = INVALID_DEVICE_ID;
                if (!outDevice) result.alsaOutputDevice = INVALID_DEVICE_ID;
                return result;
            }

            let sampleRates = intersectArrays(inDevice.sampleRates, outDevice.sampleRates);
            if (sampleRates.length !== 0 && sampleRates.indexOf(result.sampleRate) === -1) {
                let bestSr = sampleRates[0];
                let bestErr = 1e36;
                for (let sr of sampleRates) {
                    let err = (sr - result.sampleRate) * (sr - result.sampleRate);
                    if (err < bestErr) { bestErr = err; bestSr = sr; }
                }
                result.sampleRate = bestSr;
            }

            let bestBuffers =  getBestBuffers(inDevice, outDevice, result.bufferSize, result.numberOfBuffers);
            result.bufferSize = bestBuffers.bufferSize;
            result.numberOfBuffers = bestBuffers.numberOfBuffers;
            return result;
        }

        componentDidMount() {
            super.componentDidMount();
            this.mounted = true;
            this.ignoreClose = false;
            if (this.props.open) {
                this.requestAlsaInfo();
                this.startStatusTimer();
                this.saveSettingsTemporary(this.props.jackServerSettings);
            }
        }

        componentDidUpdate(oldProps: JackServerSettingsDialogProps) {
            if ((this.props.open && !oldProps.open) && this.mounted) {
               this.ignoreClose = false;
               this.saveSettingsTemporary(this.props.jackServerSettings);
                // When opening, preserve the current settings until ALSA device
                // information is loaded. If we don't have device info yet, just
                // clone the provided settings without applying device defaults.
                let settings = this.state.alsaDevices
                    ? this.applyAlsaDevices(this.props.jackServerSettings.clone(), this.state.alsaDevices)
                    : this.props.jackServerSettings.clone();

                this.setState({
                    jackServerSettings: settings,
                    latencyText: getLatencyText(settings),
                    okEnabled: isOkEnabled(settings, this.state.alsaDevices)
                });
                if (!this.state.alsaDevices) {
                    this.requestAlsaInfo();
                }
                 this.startStatusTimer();
            } else if (!this.props.open && oldProps.open) {
                this.stopStatusTimer();
                this.originalJackServerSettings = undefined;
            }
        }

        componentWillUnmount() {
            super.componentWillUnmount();
            this.mounted = false;
            this.stopStatusTimer();
            if (this.originalJackServerSettings) {
                // Revert any unapplied changes when the dialog is unmounted
                this.applySettings(this.originalJackServerSettings);
                this.originalJackServerSettings = undefined;
            }
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
            settings.valid = false;

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
                settings.valid = false;
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
            settings.valid = false;

            this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings),
                okEnabled: isOkEnabled(settings,this.state.alsaDevices)
            });
        }

        handleApply() {
            if (this.state.okEnabled) {
                this.applySettings(); // Fire and forget
                this.startStatusTimer();
            }
        };

        handleOk() {
            if (!this.state.okEnabled) return;

            const proceedWithOk = () => {
                this.ignoreClose = true; // Indicate that the closing is intentional
                this.releaseDevices(); // Fire and forget
                this.applySettings(); // Fire and forget
                this.saveSettings(); // Fire and forget
                this.originalJackServerSettings = undefined;
                if (this.props.onApply) {
                    this.props.onApply(this.state.jackServerSettings.clone());
                }
                this.props.onClose(); // Close the dialog
            };

            if (this.state.jackServerSettings.alsaInputDevice !== this.state.jackServerSettings.alsaOutputDevice
                && !this.suppressDeviceWarning) {
                this.setState({ showDeviceWarning: true });
                return;
            }

            proceedWithOk();
        };

        handleWarningProceed() {
            if (this.state.dontShowWarningAgain) {
                localStorage.setItem("suppressSeparateDeviceWarning", "1");
                this.suppressDeviceWarning = true;
            }
            this.setState({ showDeviceWarning: false, dontShowWarningAgain: false }, () => {
                this.ignoreClose = true;
                this.releaseDevices(); // Fire and forget
                this.applySettings(); // Fire and forget
                this.saveSettings(); // Fire and forget
                this.originalJackServerSettings = undefined;
                if (this.props.onApply) {
                    this.props.onApply(this.state.jackServerSettings.clone());
                }
                this.props.onClose(); // Close the dialog after the warning
            });
        }

        handleWarningCancel() {
            this.setState({ showDeviceWarning: false, dontShowWarningAgain: false });
        }

        handleWarningCheck(e: any, checked: boolean) {
            this.setState({ dontShowWarningAgain: checked });
        }
        handleInputDeviceChanged(e: any) {
                const d = e.target.value as string;
                let s = this.state.jackServerSettings.clone();
                s.alsaInputDevice = d;
                s.valid = false;
                let settings = this.applyAlsaDevices(s, this.state.alsaDevices);
                this.setState({
                jackServerSettings: settings,
                latencyText: getLatencyText(settings),
                okEnabled: isOkEnabled(settings, this.state.alsaDevices)
                });
        }

          handleOutputDeviceChanged(e: any) {
                const d = e.target.value as string;
                let s = this.state.jackServerSettings.clone();
                s.alsaOutputDevice = d;
                s.valid = false;
                let settings = this.applyAlsaDevices(s, this.state.alsaDevices);
                this.setState({
               jackServerSettings: settings,
                latencyText: getLatencyText(settings),
                okEnabled: isOkEnabled(settings, this.state.alsaDevices)
                });
        }

            render() {
            const classes = withStyles.getClasses(this.props);

            const { onClose, open } = this.props;

            //Ignore close rutine if the ignoreclose is true. (After OK or Proceed or Multi Device Warning)
            const handleClose = () => {
                 if (this.ignoreClose) {
                    return;
                } else {
                    this.releaseDevices(); // Fire and forget
                    if (this.originalJackServerSettings) {
                        // Revert any applied settings
                        this.applySettings(this.originalJackServerSettings); // Fire and forget
                        this.originalJackServerSettings = undefined;
                    }
                }
                onClose();
            };

            const sortedDevices = sortDevices(this.state.alsaDevices ?? []);

            let selectedInputDevice = this.getSelectedDevice(this.state.jackServerSettings.alsaInputDevice);
            let selectedOutputDevice = this.getSelectedDevice(this.state.jackServerSettings.alsaOutputDevice);

            const devicesSelected = (selectedInputDevice && selectedOutputDevice);

            let bufferSizes: number[] = devicesSelected ?
                    getValidBufferSizesMultiple(selectedInputDevice, selectedOutputDevice) : [];
            let bufferCounts = devicesSelected ?
                    getValidBufferCountsMultiple(this.state.jackServerSettings.bufferSize, selectedInputDevice, selectedOutputDevice) : [];
            let bufferSizeDisabled = !devicesSelected;
            let bufferCountDisabled = !devicesSelected;
            let sampleRates = devicesSelected && selectedInputDevice && selectedOutputDevice ?
                    intersectArrays(selectedInputDevice.sampleRates, selectedOutputDevice.sampleRates)
                     : [];
            let sampleRateOptions = sampleRates.length === 0 && this.state.jackServerSettings.sampleRate ? [this.state.jackServerSettings.sampleRate] : sampleRates;

            return (
             <>
                <DialogEx tag="jack" onClose={handleClose} aria-labelledby="select-channels-title" open={open}
                    onEnterKey={() => {
                        this.handleOk();
                    }}
                     fullScreen={this.state.fullScreen}

                >
                    <DialogContent>
                          <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
                          <div style={{ display: "flex", flexDirection: "row", alignItems: "center", gap: 8 }}>
                          <div style={{ display: "flex", flexDirection: this.state.compactWidth ? "column" : "row", gap: 8 }}>
                         {/* Audio Input Device */}
                                <FormControl variant="standard" className={classes.formControl}>
                                <InputLabel shrink className={classes.inputLabel} htmlFor="jsd_inputDevice">Input Device</InputLabel>
                                <Select variant="standard"
                                    id="jsd_inputDevice"
                                    value={this.state.jackServerSettings.alsaInputDevice}
                                    onChange={e => this.handleInputDeviceChanged(e)}
                                    disabled={!this.state.alsaDevices || this.state.alsaDevices.length === 0}
                                    style={{ width: 220 }}
                                >
                       {sortedDevices.filter(d => d.supportsCapture).map(d => (
                       <MenuItem key={d.id} value={d.id}>{d.name}</MenuItem>
                                                )) || <MenuItem value="" disabled>Loading...</MenuItem>}
                                </Select>
                            </FormControl>

                                                {/* Audio Output Device */}
                             <FormControl variant="standard" className={classes.formControl}>
                                <InputLabel shrink className={classes.inputLabel} htmlFor="jsd_outputDevice">Output Device</InputLabel>
                                <Select variant="standard"
                                    id="jsd_outputDevice"
                                    value={this.state.jackServerSettings.alsaOutputDevice}
                                    onChange={e => this.handleOutputDeviceChanged(e)}
                                    disabled={!this.state.alsaDevices || this.state.alsaDevices.length === 0}
                                    style={{ width: 220 }}
                                >
                                {sortedDevices.filter(d => d.supportsPlayback).map(d => (
                                               <MenuItem key={d.id} value={d.id}>{d.name}</MenuItem>
                                )) || <MenuItem value="" disabled>Loading...</MenuItem>}
                                 </Select>
                               </FormControl>
                                </div>
                                <IconButtonEx tooltip="Refresh devices" onClick={() => this.requestAlsaInfo()} aria-label="refresh-audio-devices">
                                    <RefreshIcon />
                                </IconButtonEx>
                            </div>
                        </div><div>
                        <FormControl variant="standard" className={classes.formControl}>
                                <InputLabel shrink className={classes.inputLabel} htmlFor="jsd_sampleRate">Sample rate</InputLabel>
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
                                   {sampleRateOptions.map((sr) => {
                                            return (<MenuItem value={sr}>{sr}</MenuItem> );
                                    })}
                                </Select>
                            </FormControl>
                            <div style={{ display: "inline", whiteSpace: "nowrap" }}>
                               <FormControl variant="standard" className={classes.formControl}>
                                    <InputLabel shrink className={classes.inputLabel} htmlFor="bufferSize">Buffer size</InputLabel>
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
                               <FormControl variant="standard" className={classes.formControl}>
                                    <InputLabel shrink className={classes.inputLabel} htmlFor="numberofBuffers">Buffers</InputLabel>
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
                           <div className={classes.cpuStatusColor} style={{ paddingLeft: 24 }}>
                             {JackHostStatus.getDisplayViewNoCpu("", this.state.jackStatus)}
                        </div>
                    </DialogContent>

                    <DialogActions>
                        <Button variant="dialogSecondary" onClick={handleClose}>
                            Cancel
                        </Button>
                     <Button variant="dialogSecondary" onClick={() => this.handleApply()} disabled={!this.state.okEnabled}>Apply</Button>
                        <Button variant="dialogPrimary" onClick={() => this.handleOk()} disabled={!this.state.okEnabled}>
                            OK
                        </Button>
                    </DialogActions>

                </DialogEx>
                  {this.state.showDeviceWarning && (
                    <DialogEx open={this.state.showDeviceWarning} tag="ioWarning"
                        onEnterKey={() => this.handleWarningProceed()}
                        onClose={() => this.handleWarningCancel()}
                        style={{ userSelect: "none" }}
                    >
                        <DialogContent>
                            <Typography variant="body2" color="textPrimary" gutterBottom>
                                Using different input and output devices may cause issues when streaming audio.
                                Are you sure you want to continue?
                            </Typography>
                            <FormControlLabel
                                control={<Checkbox checked={this.state.dontShowWarningAgain} onChange={(e,c)=>this.handleWarningCheck(e,c)} />}
                                label="Don't show me this message again"
                            />
                        </DialogContent>
                        <DialogActions>
                            <Button onClick={() => this.handleWarningCancel()} variant="dialogSecondary" style={{ width: 120 }}>Cancel</Button>
                            <Button onClick={() => this.handleWarningProceed()} variant="dialogPrimary" style={{ width: 120 }}>PROCEED</Button>
                        </DialogActions>
                    </DialogEx>
                )}
                </>
            );
        }
    },
    styles);

export default JackServerSettingsDialog;
