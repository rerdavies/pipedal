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

import { PiPedalModelFactory, PiPedalModel, State, ControlValueChangedHandle } from "./PiPedalModel";
import { StandardItemSize } from './PluginControlView';
import Utility from './Utility';
import SvgPathBuilder from './SvgPathBuilder';


const WAVESHAPE_VECTOR_URI = "http://two-play.com/plugins/toob#waveShape";

const PLOT_WIDTH = StandardItemSize.height - 12;
const PLOT_HEIGHT = StandardItemSize.height - 12;

const styles = (theme: Theme) => createStyles({
    frame: {
        width: PLOT_WIDTH, height: PLOT_HEIGHT, background: "#444",
        borderRadius: 6,
        marginTop: 12, marginLeft: 8, marginRight: 8,
        boxShadow: "1px 4px 8px #000 inset"
    },
});

interface SvgRect 
{
    x: number;
    y: number;
    width: number;
    height: number;
    color: string;

}

interface ToobWaveShapeProps extends WithStyles<typeof styles> {
    instanceId: number;
    controlNumber: number;
    controlKeys: string[];

    vuMin: number;
    vuMax: number;
    vuMinPeak: number;
    vuMaxPeak: number;

    vuOutMin: number;
    vuOutMax: number;
    vuOutMinPeak: number;
    vuOutMaxPeak: number;

};
interface ToobWaveShapeState {
    data: number[];
};

const HOLD_TIME: number = 1.6; // seconds.
const DECAY_RATE = 1.0; // full range/second.

export class BidirectionalVuPeak {
    minValue: number = 0;
    maxValue: number = 0;
    minPeak: number = 0;
    maxPeak: number = 0;
    holdTime: number = 1E120; // don't decay until we have an actual peak.
    lastTime: number = 0;

    update(time: number, minValue: number, maxValue: number)
    {
        // decay peak.
        if (time > this.holdTime)
        {
            let dt = time-this.lastTime;
            let decay = dt*DECAY_RATE;
            this.maxPeak -= decay;
            if (this.maxPeak < 0) this.maxPeak = 0;

            this.minPeak += decay;
            if (this.minPeak > 0) this.minPeak = 0;
            this.lastTime = time;

        }
        this.minValue = minValue;
        this.maxValue = maxValue;
        let resetPeakHold = false;
        if (minValue < this.minPeak)
        {
            this.minPeak = minValue;
            resetPeakHold = true;
        }
        if (maxValue > this.maxPeak)
        {
            this.maxPeak = maxValue;
            resetPeakHold = true;
        }
        if (resetPeakHold)
        {
            this.holdTime = time+HOLD_TIME;
            this.lastTime = this.holdTime;
        }
    }

}


const ToobWaveShapeView =
    withStyles(styles, { withTheme: true })(
        class extends React.Component<ToobWaveShapeProps, ToobWaveShapeState>
        {
            model: PiPedalModel;

            customizationId: number = 1;

            constructor(props: ToobWaveShapeProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    data: []
                };

                this.onPedalBoardChanged = this.onPedalBoardChanged.bind(this);
                this.onStateChanged = this.onStateChanged.bind(this);
                this.onControlValueChanged = this.onControlValueChanged.bind(this);
                this.isReady = this.model.state.get() === State.Ready;
            }

            isReady: boolean;
            onStateChanged() {
                let isReady = this.model.state.get() === State.Ready;
                if (this.isReady !== isReady) {
                    this.isReady = isReady;

                    if (isReady) {
                        this.requestDeferred = false;
                        this.requestOutstanding = false;
                        this.updateWaveShape(); // after a reconnect.
                    }
                }
            }
            onPedalBoardChanged() {
                this.updateWaveShape();
            }
            _valueChangedHandle?: ControlValueChangedHandle;


            mounted: boolean = false;

            componentDidMount() {
                this.mounted = true;
                this.model.state.addOnChangedHandler(this.onStateChanged);
                this.model.pedalBoard.addOnChangedHandler(this.onPedalBoardChanged);
                this._valueChangedHandle = this.model.addControlValueChangeListener(
                    this.props.instanceId,
                    this.onControlValueChanged);

                this.isReady = this.model.state.get() === State.Ready;
                this.updateWaveShape();
            }

            lastVuMin: number = -1000;
            lastVuMax: number = -1000;

            componentDidUpdate() {
            }

            componentWillUnmount() {
                this.mounted = false;
                if (this._valueChangedHandle) {
                    this.model.removeControlValueChangeListener(this._valueChangedHandle);
                    this._valueChangedHandle = undefined;
                }
                this.model.state.removeOnChangedHandler(this.onStateChanged);
                this.model.pedalBoard.removeOnChangedHandler(this.onPedalBoardChanged);
            }

            onControlValueChanged(key: string, value: number) {
                for (let i = 0; i < this.props.controlKeys.length; ++i) {
                    if (this.props.controlKeys[i] === (key)) {
                        this.updateWaveShape();
                    }
                }
            }

            requestOutstanding: boolean = false;
            requestDeferred: boolean = false;

            // Size of the SVG element.
            xMin: number = 4;
            xMax: number = PLOT_WIDTH - 10;
            yMin: number = 2;
            yMax: number = PLOT_HEIGHT - 10;
            vuHeight = 4;
            nPoints: number = 0;

            indexToX(value: number): number {
                return (this.xMax - this.xMin) * value / this.nPoints + this.xMin;
            }
            toY(value: number): number {
                let yMid = (this.yMin + this.yMax) / 2;
                return yMid - yMid * value;
            }
            toX(value: number): number {
                let xMid = (this.xMin + this.xMax)/2;
                return xMid+ (this.xMax-this.xMin)*0.5*value;
            }


            onWaveShapeUpdated(data: number[]) {
                if (!this.mounted) {
                    return;
                }
                this.setState({data: data});
            }

            updateWaveShape() {
                if (!this.isReady)
                    return;
                if (this.requestOutstanding) { // throttling.
                    this.requestDeferred = true;
                    return;
                }
                this.requestOutstanding = true;
                this.model.getLv2Parameter<number[]>(this.props.instanceId, WAVESHAPE_VECTOR_URI + this.props.controlNumber)
                    .then((data) => {
                        this.onWaveShapeUpdated(data);
                        if (this.requestDeferred) {
                            Utility.delay(10) // take  breath
                                .then(
                                    () => {
                                        this.requestOutstanding = false;
                                        this.requestDeferred = false;
                                        this.updateWaveShape();
                                    }

                                );
                        } else {
                            this.requestOutstanding = false;
                        }
                    }).catch(error => {
                        // assume the connection was lost. We'll get saved by a reconnect.
                        this.requestOutstanding = false;
                        this.requestDeferred = false;
                    });
            }

            currentPath: string = "";

            redColor: string = "#F00";
            yellowColor: string = "#FF0";
            greenColor: string = "#0F0";

            horizontalVuMeter(minValue: number, maxValue: number, minPeak: number, maxPeak: number): React.ReactNode[]
            {
                let result: React.ReactNode[] = [];
                let greenXMin = this.toX(-0.5);
                let greenXMax = this.toX(0.5);
                if (minValue < -1.0) minValue = -1.0;
                if (maxValue > 1.0) maxValue = 1.0;
                if (minPeak < -1.0) minPeak = -1;
                if (maxPeak > 1.0) maxPeak = 1.0;
                let nKey = 0;

                let yTop = this.yMax+2;

                let minX = this.toX(minValue);
                let maxX = this.toX(maxValue);
                let minPeakX = this.toX(minPeak);
                let maxPeakX = this.toX(maxPeak);
                if (minX < greenXMin)
                {
                    result.push( ( <rect x={minX} y={yTop} 
                                        width={greenXMin-minX} height={this.vuHeight} 
                                        fill={this.yellowColor} stroke="none" key={"hr"+ nKey++} />));
                    minX = greenXMin;
                }
                if (maxX > greenXMax)
                {
                    result.push( ( <rect x={greenXMax} y={yTop} 
                        width={maxX-greenXMax} height={this.vuHeight} 
                        fill={this.yellowColor} stroke="none" key={"hr"+ nKey++} />))
                    maxX = greenXMax;

                }
                if (maxX > minX)
                {
                    result.push( ( <rect x={minX} y={yTop} 
                        width={maxX-minX} height={this.vuHeight} 
                        fill={this.greenColor} stroke="none" key={"hr"+ nKey++} />))
                }
                if (minPeakX+4 < maxPeakX)
                {
                    let minColor = this.peakColor(minPeak);
                    let maxColor = this.peakColor(maxPeak);
                    result.push( ( <rect x={minPeakX} y={yTop} 
                        width={2} height={this.vuHeight} 
                        fill={minColor} stroke="none" key={"hr"+ nKey++} />))
                    result.push( ( <rect x={maxPeakX-2} y={yTop} 
                        width={2} height={this.vuHeight} 
                        fill={maxColor} stroke="none" key={"hr"+ nKey++} />))
                }

                return result;
            }
            verticalVuMeter(minValue: number, maxValue: number, minPeak: number, maxPeak: number): React.ReactNode[]
            {
                let result: React.ReactNode[] = [];
                let greenYMin = this.toY(-0.5);
                let greenYMax = this.toY(0.5);
                if (minValue < -1.0) minValue = -1.0;
                if (maxValue > 1.0) maxValue = 1.0;
                if (minPeak < -1.0) minPeak = -1;
                if (maxPeak > 1.0) maxPeak = 1.0;
                let nKey = 0;

                let xLeft = this.xMax+2;

                let minY = this.toY(minValue);
                let maxY = this.toY(maxValue);
                let minPeakY = this.toY(minPeak);
                let maxPeakY = this.toY(maxPeak);
                if (minY > greenYMin)
                {
                    result.push( ( <rect y={greenYMin} x={xLeft} 
                                        height={minY-greenYMin} width={4} 
                                        fill={this.yellowColor} stroke="none" key={"vr"+ nKey++} />));
                    minY = greenYMin;
                }
                if (maxY < greenYMax)
                {
                    result.push( ( <rect y={maxY} x={xLeft} 
                        height={greenYMax-maxY} width={4} 
                        fill={this.yellowColor} stroke="none" key={"vr"+ nKey++} />))
                    maxY = greenYMax;   
                }
                if (minY > maxY)
                {
                    result.push( ( <rect y={maxY} x={xLeft} 
                        height={minY-maxY} width={4} 
                        fill={this.greenColor} stroke="none" key={"vr"+ nKey++} />))
                }
                if (minPeakY-4 > maxPeakY)
                {
                    let minColor = this.peakColor(minPeak);
                    let maxColor = this.peakColor(maxPeak);
                    result.push( ( <rect y={minPeakY-4} x={xLeft} 
                        width={4} height={2} 
                        fill={minColor} stroke="none" key={"vr"+ nKey++} />))
                    result.push( ( <rect y={maxPeakY} x={xLeft} 
                        width={4} height={2} 
                        fill={maxColor} stroke="none" key={`vr${nKey++}`} />))
                }

                return result;
            }
            peakColor(value: number): string {
                if (value >= 1.0 || value <= -1.0) return this.redColor;
                if (value >= 0.5 || value <= -0.5) return this.yellowColor;
                return this.greenColor;
            }

            render() {
                let classes = this.props.classes;


                let data = this.state.data;
                let pathBuilder = new SvgPathBuilder();
                this.nPoints = data.length;
                if (data.length > 2) {
                    pathBuilder.moveTo(this.indexToX(0), this.toY(data[0]));
                    for (let i = 1; i < data.length; ++i) {
                        pathBuilder.lineTo(this.indexToX(i), this.toY(data[i]));
                    }
                }
                let currentPath = pathBuilder.toString();



                return (
                    <div className={classes.frame} >
                        <svg width={PLOT_WIDTH} height={PLOT_HEIGHT} viewBox={"0 0 " + PLOT_WIDTH + " " + PLOT_HEIGHT} >
                            { this.horizontalVuMeter(this.props.vuMin,this.props.vuMax,
                                this.props.vuMinPeak,this.props.vuMaxPeak)
                            }
                            { this.verticalVuMeter(this.props.vuOutMin,this.props.vuOutMax,
                                this.props.vuOutMinPeak,this.props.vuOutMaxPeak)
                            }
                            <path d={currentPath} stroke="#0F8" fill="none" strokeWidth="2.5" opacity="0.6" />
                        </svg>

                    </div>);
            }
        }
    );



export default ToobWaveShapeView;