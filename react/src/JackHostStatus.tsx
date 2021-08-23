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

import React from 'react';
import Typography from '@material-ui/core/Typography';


const RED_COLOR = "#C00";
const GREEN_COLOR = "#666";



function tempDisplay(mC: number): string
{
    return (mC/1000).toFixed(1) + "\u00B0C"; // degrees C.
}
function cpuDisplay(cpu: number): string
{
    return cpu.toFixed(1)+"%";
}

export default class JackHostStatus {
    deserialize(input: any): JackHostStatus
    {
        this.active = input.active;
        this.restarting = input.restarting;
        this.underruns = input.underruns;
        this.cpuUsage = input.cpuUsage;
        this.msSinceLastUnderrun = input.msSinceLastUnderrun;
        this.temperaturemC = input.temperaturemC;
        return this;
    }
    hasTemperature() : boolean {
        return this.temperaturemC >= -100000;
    }
    active: boolean = false;
    restarting: boolean = false;
    underruns: number = 0;
    cpuUsage: number = 0;
    msSinceLastUnderrun: number = -5000*1000;
    temperaturemC: number = -1000000;

    static getDisplayView(label: string,status?: JackHostStatus): React.ReactNode {
        if (!status) {
            return (<div style={{whiteSpace: "nowrap"}}>
                <Typography variant="caption" color="textSecondary">{label}</Typography>
                <Typography variant="caption">&nbsp;</Typography>
            </div>);
        }
        if (status.restarting)
        {
            return (
                <div style={{whiteSpace: "nowrap"}}>
                    <Typography variant="caption" color="textSecondary">{label}</Typography>
                    <span style={{color: RED_COLOR}}>
                        <Typography variant="caption" color="inherit">Restarting&nbsp;&nbsp;</Typography>
                    </span>
                    {
                        status.temperaturemC > -100000 &&
                        (
                            <span style={{color: status.temperaturemC > 75000? RED_COLOR: GREEN_COLOR}}>
                                <Typography variant="caption" color="inherit">{tempDisplay(status.temperaturemC)}</Typography>
                            </span>
                        )
                    }

                </div>
            );

        } else if (!status.active) {
            return (
                <div style={{whiteSpace: "nowrap"}}>
                    <Typography variant="caption" color="textSecondary">{label}</Typography>

                    <span style={{color: RED_COLOR}}>
                        <Typography variant="caption" color="inherit">Stopped&nbsp;&nbsp;</Typography>
                    </span>
                    {
                        status.temperaturemC > -100000 &&
                        (
                            <span style={{color: status.temperaturemC > 75000? RED_COLOR: GREEN_COLOR}}>
                                <Typography variant="caption" color="inherit">{tempDisplay(status.temperaturemC)}</Typography>
                            </span>
                        )
                    }
                </div>
            );
        } else {
            let underrunError = status.msSinceLastUnderrun < 15*1000;
            return (
                <div style={{whiteSpace: "nowrap"}}>
                    <Typography variant="caption" color="textSecondary">{label}</Typography>
                    <span style={{color: underrunError? RED_COLOR: GREEN_COLOR}}>
                        <Typography variant="caption" color="inherit">
                            XRuns:&nbsp;{status.underruns+""}&nbsp;&nbsp;
                        </Typography>
                    </span>
                    <span style={{color: underrunError? RED_COLOR: GREEN_COLOR}}>
                        <Typography variant="caption" color="inherit">
                            CPU:&nbsp;{cpuDisplay(status.cpuUsage)}&nbsp;&nbsp;
                        </Typography>
                    </span>

                    <span style={{color: GREEN_COLOR}}>
                        <Typography variant="caption" color="inherit">{tempDisplay(status.temperaturemC)}</Typography>
                    </span>

                </div>
            );


        }
    }

};