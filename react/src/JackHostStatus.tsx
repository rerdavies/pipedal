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
import Typography from '@mui/material/Typography';
import {isDarkMode} from './DarkMode';

const RED_COLOR = isDarkMode()? "#F88":"#C00";
const GREEN_COLOR = isDarkMode()? "rgba(255,255,255,0.7)": "#666";



function tempDisplay(mC: number): string {
    return (mC / 1000).toFixed(1) + "\u00B0C"; // degrees C.
}
function cpuDisplay(cpu: number): string {
    return cpu.toFixed(1) + "%";
}

function fmtCpuFreq(freq: number): string
{
    if (freq >= 100000000)
    {
        return (freq/1000000).toFixed(1) + " GHz";
    }
    if (freq >= 10000000)
    {
        return (freq/1000000).toFixed(2) + " GHz";
    }
    if (freq >= 1000000)
    {
        return (freq/1000000).toFixed(3) + " GHz";
    }
    if (freq >= 1000)
    {
        return (freq/1000).toFixed(3) + " MHz";
    }
    return freq + " KHz";
}

export default class JackHostStatus {
    deserialize(input: any): JackHostStatus {
        this.active = input.active;
        this.restarting = input.restarting;
        this.errorMessage = input.errorMessage;
        this.underruns = input.underruns;
        this.cpuUsage = input.cpuUsage;
        this.msSinceLastUnderrun = input.msSinceLastUnderrun;
        this.temperaturemC = input.temperaturemC;
        this.cpuFreqMax = input.cpuFreqMax;
        this.cpuFreqMin = input.cpuFreqMin;
        this.governor = input.governor;
        return this;
    }
    hasTemperature(): boolean {
        return this.temperaturemC >= -100000;
    }
    active: boolean = false;
    errorMessage: string = "";
    restarting: boolean = false;
    underruns: number = 0;
    cpuUsage: number = 0;
    msSinceLastUnderrun: number = -5000 * 1000;
    temperaturemC: number = -1000000;
    cpuFreqMax: number = 0;
    cpuFreqMin: number = 0;
    governor: string = "";

    static getCpuInfo(label: string, status?: JackHostStatus): React.ReactNode {
        if (!status) {
            return (<div style={{ whiteSpace: "nowrap" }}>
                <Typography variant="caption" color="inherit">{label}</Typography>
                <Typography variant="caption">&nbsp;</Typography>
            </div>);
        }
        return (<div style={{ whiteSpace: "nowrap" }}>
            <Typography variant="caption" color="inherit">{label}</Typography>
            <Typography variant="caption" color="inherit">
                {
                    (status.cpuFreqMax === status.cpuFreqMin)?
                    (
                        <span> {status.governor} {fmtCpuFreq(status.cpuFreqMax)}  </span>
                    )
                    :(
                        <span> {status.governor} {fmtCpuFreq(status.cpuFreqMax)}-{fmtCpuFreq(status.cpuFreqMax)}  </span>

                    )
                }
            </Typography>
        </div>);


    }
    static getDisplayView(label: string, status?: JackHostStatus): React.ReactNode {
        if (!status) {
            return (<div style={{ whiteSpace: "nowrap" }}>
                <Typography variant="caption" color="inherit">{label}</Typography>
                <Typography variant="caption">&nbsp;</Typography>
            </div>);
        }
        if (status.restarting) {
            return (
                <div style={{ whiteSpace: "nowrap" }}>
                    <Typography variant="caption" color="inherit">{label}</Typography>
                    <span style={{ color: RED_COLOR }}>
                        <Typography variant="caption" color="inherit">Restarting&nbsp;&nbsp;</Typography>
                    </span>
                    {
                        status.temperaturemC > -100000 &&
                        (
                            <span style={{ color: status.temperaturemC > 75000 ? RED_COLOR : GREEN_COLOR }}>
                                <Typography variant="caption" color="inherit">{tempDisplay(status.temperaturemC)}</Typography>
                            </span>
                        )
                    }
                </div>
            );

        } else if (!status.active) {
            return (
                <div style={{ whiteSpace: "nowrap" }}>
                    <Typography variant="caption" color="inherit">{label}</Typography>

                    <span style={{ color: RED_COLOR }}>
                        <Typography variant="caption" color="inherit">{status.errorMessage === "" ? "Stopped" : status.errorMessage}&nbsp;&nbsp;</Typography>
                    </span>
                    {
                        status.temperaturemC > -100000 &&
                        (
                            <span style={{ color: status.temperaturemC > 75000 ? RED_COLOR : GREEN_COLOR }}>
                                <Typography variant="caption" color="inherit">{tempDisplay(status.temperaturemC)}</Typography>
                            </span>
                        )
                    }
                </div>
            );
        } else {
            let underrunError = status.msSinceLastUnderrun < 15 * 1000;
            return (
                <div style={{ whiteSpace: "nowrap" }}>
                    <Typography variant="caption" color="inherit">{label}</Typography>
                    <span style={{ color: underrunError ? RED_COLOR : GREEN_COLOR }}>
                        <Typography variant="caption" color="inherit">
                            XRuns:&nbsp;{status.underruns + ""}&nbsp;&nbsp;
                        </Typography>
                    </span>
                    <span style={{ color: underrunError ? RED_COLOR : GREEN_COLOR }}>
                        <Typography variant="caption" color="inherit">
                            CPU:&nbsp;{cpuDisplay(status.cpuUsage)}&nbsp;&nbsp;
                        </Typography>
                    </span>

                    <span style={{ color: GREEN_COLOR }}>
                        <Typography variant="caption" color="inherit">{tempDisplay(status.temperaturemC)}</Typography>
                    </span>

                </div>
            );


        }
    }

};