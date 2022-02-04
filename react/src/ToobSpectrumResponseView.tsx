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
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';

import { PiPedalModelFactory, PiPedalModel, State,ListenHandle } from "./PiPedalModel";
import { StandardItemSize } from './PluginControlView';
import Utility from './Utility';
import { setInterval,clearInterval } from 'timers';


const SPECTRUM_RESPONSE_VECTOR_URI = "http://two-play.com/plugins/toob#spectrumResponse";

const PLOT_WIDTH = StandardItemSize.width*3-16;
const PLOT_HEIGHT = StandardItemSize.height-12;

const styles = (theme: Theme) => createStyles({
    frame: {
        width: PLOT_WIDTH, height: PLOT_HEIGHT, background: "#444", 
        borderRadius: 6,
        marginTop: 12, marginLeft: 8, marginRight: 8,
        boxShadow: "1px 4px 8px #000 inset"
    },
});

interface ToobSpectrumResponseProps extends WithStyles<typeof styles> {
    instanceId: number;

};
interface ToobSpectrumResponseState {
    path: string;
    minF: number;
    maxF: number;
};

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

                let plugin = this.model.pedalBoard.get()?.maybeGetItem(this.props.instanceId);
                if (plugin)
                {
                    minF = plugin.getControlValue("minF");
                    maxF = plugin.getControlValue("maxF");
                }


                this.state = {
                    path: "",
                    minF: minF,
                    maxF: maxF
                };

                this.onPedalBoardChanged = this.onPedalBoardChanged.bind(this);
                this.onStateChanged = this.onStateChanged.bind(this);
            }

            onStateChanged()
            {
                if (this.model.state.get() === State.Ready)
                {
                    this.requestDeferred = false;
                    this.requestOutstanding = false;

                    let plugin = this.model.pedalBoard.get()?.maybeGetItem(this.props.instanceId);
                    if (plugin)
                    {
                        let minF = plugin.getControlValue("minF");
                        let maxF = plugin.getControlValue("maxF");
                        this.setState({minF: minF, maxF: maxF});
                    }
    
                }
                this.isReady = this.model.state.get() === State.Ready;

                this.maybeStartClock(); // after a reconnect.
            }
            onPedalBoardChanged() 
            {
                this.maybeStartClock();
                let plugin = this.model.pedalBoard.get()?.maybeGetItem(this.props.instanceId);
                if (plugin)
                {
                    let minF = plugin.getControlValue("minF");
                    let maxF = plugin.getControlValue("maxF");
                    this.setState({minF: minF, maxF: maxF});
                }
            }

            mounted: boolean = false;
            isReady: boolean = false;
            componentDidMount()
            {
                this.model.state.addOnChangedHandler(this.onStateChanged);
                this.model.pedalBoard.addOnChangedHandler(this.onPedalBoardChanged);
                this.mounted = true;
                this.isReady = this.model.state.get() === State.Ready;
                this.maybeStartClock();
            }
            componentWillUnmount()
            {
                this.mounted = false;
                this.maybeStartClock();
                this.model.state.removeOnChangedHandler(this.onStateChanged);
                this.model.pedalBoard.removeOnChangedHandler(this.onPedalBoardChanged);
            }

            componentDidUpdate() {
            }
            requestOutstanding: boolean = false;
            requestDeferred: boolean = false;

            // Size of the SVG element.
            xMin: number = 0;
            xMax: number = PLOT_WIDTH+4;
            yMin: number = 0;
            yMax: number = PLOT_HEIGHT;

            onSpectrumResponseUpdated(svgPath: string) {
                this.setState({path: svgPath});

            }

            isTicking : boolean = false;
            hClock?: NodeJS.Timeout;
            hAtomOutput?: ListenHandle;

            maybeStartClock() {
                let  shouldTick = this.isReady && this.mounted;

                if (shouldTick !== this.isTicking)
                {
                    this.isTicking = shouldTick;
                    if (shouldTick)
                    {
                        this.hAtomOutput = this.model.listenForAtomOutput(this.props.instanceId,
                            (instanceId,atomOutput)=> {
                                this.onSpectrumResponseUpdated(atomOutput.value as string);
                                if (this.requestDeferred) {
                                    Utility.delay(30) // take  breath
                                        .then(
                                            () => {
                                                this.requestOutstanding = false;
                                                this.requestDeferred = false;
                                                this.requestSpectrum();
                                            }
        
                                        );
                                } else {
                                    this.requestOutstanding = false;
                                }
        

                            });
                        this.hClock = setInterval(()=> {
                                this.requestSpectrum();
                            },
                            1000/15);
                    } else {
                        if (this.hAtomOutput)
                        {
                            this.model.cancelListenForAtomOutput(this.hAtomOutput);
                            this.hAtomOutput = undefined;
                        }
                        if (this.hClock)
                        {
                            clearInterval(this.hClock);
                            this.hClock = undefined;
                        }
                    }
                }


            }
            requestSpectrum() {
                if (this.requestOutstanding) { // throttling.
                    this.requestDeferred = true;
                    return;
                }
                this.requestOutstanding = true;
                this.model.getLv2Parameter<string>(this.props.instanceId, SPECTRUM_RESPONSE_VECTOR_URI)
                    .then((data) => {
                    }).catch(error => {
                        // assume the connection was lost. We'll get saved by a reconnect.
                    });
            }

            numPoints = 200;

            minF = 60;
            maxF = 18000;
            logMinF = Math.log(this.minF);
            logMaxF = Math.log(this.maxF);

            toX(f: number): number {
                let width = PLOT_WIDTH-8;
                return  (Math.log(f)-this.logMinF)*width/(this.logMaxF-this.logMinF);
            }
            grid(): React.ReactNode[] {
                let result: React.ReactNode[] = [];
                let width = PLOT_WIDTH-8;
                let height = PLOT_HEIGHT-8;

                for (let db = -20; db >= -100; db -= 20)
                {
                    let y = height*db/-100;
                    result.push(
                        (
                            <line x1={0} y1={y} x2={width} y2={y} key={"db" + db} />
                        )
                    );
                }

                let minF = this.state.minF;
                let maxF = this.state.maxF;
                this.minF = minF;
                this.maxF = maxF;
                this.logMinF = Math.log(this.minF);
                this.logMaxF = Math.log(this.maxF);
    
                for (let decade = 10; decade < 100000; decade *= 10)
                {
                    for (let minorTick = 1; minorTick <= 5; ++ minorTick)
                    {
                        let f = decade*minorTick;
                        if (f > minF && f < maxF)
                        {
                            let x = this.toX(f);
                            if (minorTick === 1)
                            {
                                result.push(
                                    (
                                        <line x1={x} y1={0} x2={x} y2={height} key={"hgrid"+f} strokeWidth={1}/>
                                    )
                                );
                            } else {
                                result.push(
                                    (
                                        <line x1={x} y1={0} x2={x} y2={height} key={"hgrid"+f} />
                                    )
                                );
                            }
        
                        }

                    }
                }

                return result;
            }

            render() {
                let classes = this.props.classes;
                return (
                    <div className={classes.frame} style={{position: "relative"}} >
                        <div style={{position: "absolute", left: 4, top: 4}}>
                            <svg width={PLOT_WIDTH-8} height={PLOT_HEIGHT-8}  stroke="#888" strokeWidth="0.5" opacity="1"
                                >
                                {this.grid()}
                            </svg>
                        </div>
                        <div style={{position: "absolute", left: 4, top: 4}}>
                            <svg width={PLOT_WIDTH-8} height={PLOT_HEIGHT-8} preserveAspectRatio='none' viewBox={"0 0 " + this.numPoints + " " + 1000} stroke="none" fill="#0FC" strokeWidth="2.5" opacity="0.6"
                                >
                                <path d={this.state.path}  />
                            </svg>
                        </div>

                    </div>);
            }
        }
    );



export default ToobSpectrumResponseView;