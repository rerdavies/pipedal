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
import { Theme } from '@mui/material/styles';

import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';

import { PiPedalModelFactory, PiPedalModel, State, ListenHandle } from "./PiPedalModel";
import { StandardItemSize } from './PluginControlView';




import SvgPathBuilder from './SvgPathBuilder'

const SPECTRUM_RESPONSE_URI = "http://two-play.com/plugins/toob#spectrumResponse";
const SPECTRUM_ENABLE_URI = "http://two-play.com/plugins/toob#spectrumEnable";

const PLOT_WIDTH = StandardItemSize.width * 3 - 16;
const PLOT_HEIGHT = StandardItemSize.height - 12;

const styles = (theme: Theme) => createStyles({
    frame: {
        width: PLOT_WIDTH, height: PLOT_HEIGHT, background: "#333",
        borderRadius: 6,
        marginTop: 12, marginLeft: 8, marginRight: 8,
        overflow: "hidden"
    },
    frameShadow: {
        width: PLOT_WIDTH, height: PLOT_HEIGHT, 
        borderRadius: 6,
        boxShadow: "1px 4px 8px #000 inset"
    },
});

interface ToobSpectrumResponseProps extends WithStyles<typeof styles> {
    instanceId: number;

}
interface ToobSpectrumResponseState {
    path: string;
    holdPath: string;
    minF: number;
    maxF: number;
}

const ToobSpectrumResponseView =
    withStyles(styles, { withTheme: true })(
        class extends React.Component<ToobSpectrumResponseProps, ToobSpectrumResponseState>
        {
            model: PiPedalModel;

            customizationId: number = 1;

            constructor(props: ToobSpectrumResponseProps) {
                super(props);


                this.model = PiPedalModelFactory.getInstance();

                let minF = 60;
                let maxF = 18000;

                let plugin = this.model.pedalboard.get()?.maybeGetItem(this.props.instanceId);
                if (plugin) {
                    minF = plugin.getControlValue("minF");
                    maxF = plugin.getControlValue("maxF");
                }


                this.state = {
                    path: "",
                    holdPath: "",
                    minF: minF,
                    maxF: maxF
                };

                this.onPedalboardChanged = this.onPedalboardChanged.bind(this);
                this.onStateChanged = this.onStateChanged.bind(this);
            }

            onStateChanged() {
                if (this.model.state.get() === State.Ready) {
                    this.requestDeferred = false;
                    this.requestOutstanding = false;

                    let plugin = this.model.pedalboard.get()?.maybeGetItem(this.props.instanceId);
                    if (plugin) {
                        let minF = plugin.getControlValue("minF");
                        let maxF = plugin.getControlValue("maxF");
                        this.setState({ minF: minF, maxF: maxF });
                    }

                }
                this.isReady = this.model.state.get() === State.Ready;

                this.maybeStartMonitoring(); // after a reconnect.
            }
            onPedalboardChanged() {
                // rebind our monitoring hooks.
                this.maybeStartMonitoring(false);
                this.maybeStartMonitoring(true);

                let plugin = this.model.pedalboard.get()?.maybeGetItem(this.props.instanceId);
                if (plugin) {
                    let minF = plugin.getControlValue("minF");
                    let maxF = plugin.getControlValue("maxF");
                    this.setState({ minF: minF, maxF: maxF });
                }
            }

            mounted: boolean = false;
            isReady: boolean = false;
            componentDidMount() {
                this.model.state.addOnChangedHandler(this.onStateChanged);
                this.model.pedalboard.addOnChangedHandler(this.onPedalboardChanged);
                this.mounted = true;
                this.isReady = this.model.state.get() === State.Ready;
                this.maybeStartMonitoring();
            }
            componentWillUnmount() {
                this.mounted = false;
                this.maybeStartMonitoring();
                this.model.state.removeOnChangedHandler(this.onStateChanged);
                this.model.pedalboard.removeOnChangedHandler(this.onPedalboardChanged);
            }

            componentDidUpdate() {
            }
            requestOutstanding: boolean = false;
            requestDeferred: boolean = false;

            // Size of the SVG element.
            xMin: number = 0;
            xMax: number = PLOT_WIDTH + 4;
            yMin: number = 0;
            yMax: number = PLOT_HEIGHT;

            onSpectrumResponseUpdated(svgPath: string, holdSvgPath: string) {
                this.setState({ path: svgPath, holdPath: holdSvgPath });

            }

            private isMonitoring: boolean = false;
            private hClock?: NodeJS.Timeout;
            private hAtomOutput?: ListenHandle;

            private maybeStartMonitoring(wantsMonitoring: boolean = true) {
                let shouldMonitor = this.isReady && this.mounted && wantsMonitoring;

                if (shouldMonitor !== this.isMonitoring) {
                    this.isMonitoring = shouldMonitor;
                    if (shouldMonitor) {
                        this.hAtomOutput = this.model.monitorPatchProperty(
                            this.props.instanceId,
                            SPECTRUM_RESPONSE_URI,
                            (instanceId, propertyUri, json) => {
                                if (propertyUri === SPECTRUM_RESPONSE_URI && json.otype_ === "Tuple") {
                                    let values = json.value as string[];
                                    this.onSpectrumResponseUpdated(values[0], values[1]);
                                }
                            }
                        );
                        this.model.setPatchProperty(this.props.instanceId, SPECTRUM_ENABLE_URI, true)
                        .catch((error)=> {});
                    } else {

                        this.model.setPatchProperty(this.props.instanceId, SPECTRUM_ENABLE_URI, false)
                        .catch((error)=> {}); // e.g. plugin not found.



                        if (this.hAtomOutput) {
                            this.model.cancelMonitorPatchProperty(this.hAtomOutput);
                            this.hAtomOutput = undefined;
                        }
                    }
                }
            }

            numPoints = 200;

            minF = 60;
            maxF = 18000;
            logMinF = Math.log(this.minF);
            logMaxF = Math.log(this.maxF);

            toX(f: number): number {
                let width = PLOT_WIDTH - 8;
                return (Math.log(f) - this.logMinF) * width / (this.logMaxF - this.logMinF);
            }

            private minorGrid(): string {
                
                let ss = new SvgPathBuilder();

                let width = PLOT_WIDTH;
                let height = PLOT_HEIGHT;

                for (let db = -20; db >= -100; db -= 20) {
                    let y = height * db / -100;
                    ss.moveTo(0,y);
                    ss.lineTo(width,y);
                }

                let minF = this.state.minF;
                let maxF = this.state.maxF;
                this.minF = minF;
                this.maxF = maxF;
                this.logMinF = Math.log(this.minF);
                this.logMaxF = Math.log(this.maxF);

                for (let decade = 10; decade < 100000; decade *= 10) {
                    for (let minorTick = 1; minorTick <= 5; ++minorTick) {
                        let f = decade * minorTick;
                        if (f > minF && f < maxF) {
                            let x = this.toX(f);
                            if (minorTick === 1) {
                                ss.moveTo(x,0);
                                ss.lineTo(x,height);
                            } else {
                                ss.moveTo(x,0);
                                ss.lineTo(x,height);
                            }

                        }

                    }
                }
                return ss.toString();
            }

            private majorGrid(): string {
                
                let ss = new SvgPathBuilder();

                let height = PLOT_HEIGHT;

                let minF = this.state.minF;
                let maxF = this.state.maxF;
                this.minF = minF;
                this.maxF = maxF;
                this.logMinF = Math.log(this.minF);
                this.logMaxF = Math.log(this.maxF);

                for (let decade = 10; decade < 100000; decade *= 10) {
                    if (decade > minF && decade < maxF)
                    {
                        let x = this.toX(decade);
                        ss.moveTo(x,0);
                        ss.lineTo(x,height);
                    }
                }
                return  ss.toString();
            }


            render() {
                let classes = this.props.classes;
                return (
                    <div className={classes.frame} style={{ position: "relative" }} >
                        {/* <div style={{ position: "absolute", left: 0, top: 0 }}>
                            <svg fill="#660" width={PLOT_WIDTH } height={PLOT_HEIGHT} preserveAspectRatio='none' 
                                    viewBox={"0 0 " + this.numPoints + " " + 1000} stroke="none" strokeWidth="2.5" opacity="1.0"
                            >
                                <path d={this.state.holdPath} />
                            </svg>
                        </div> */}

                        <div style={{ position: "absolute", left: 0, top: 0 }}>
                            <svg fill="#0C4" width={PLOT_WIDTH } height={PLOT_HEIGHT } preserveAspectRatio='none' 
                                viewBox={"0 0 " + this.numPoints + " " + 1000} stroke="none" strokeWidth="2.5" opacity="1.0"
                            >
                                <path d={this.state.path} />
                            </svg>
                        </div>
                        <div style={{ position: "absolute", left: 0, top: 0 }}>
                            <svg width={PLOT_WIDTH } height={PLOT_HEIGHT} stroke="#FFF" strokeWidth="0.9" opacity="0.3"
                            >
                                <path d={this.minorGrid()} />
                            </svg>
                        </div>
                        <div style={{ position: "absolute", left: 0, top: 0 }}>
                            <svg width={PLOT_WIDTH} height={PLOT_HEIGHT} stroke="#FFF" strokeWidth="0.9" opacity="0.5"
                            >
                                <path d={this.majorGrid()} />
                            </svg>
                        </div>
                        <div className={classes.frameShadow} style={{ position: "absolute", left: 0, top: 0, right: 0, bottom: 0 }}>
                        </div>


                    </div>);
            }
        }
    );



export default ToobSpectrumResponseView;