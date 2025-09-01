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

import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";

import { PiPedalModelFactory, PiPedalModel, State } from "./PiPedalModel";
import Utility from './Utility';
import SvgPathBuilder from './SvgPathBuilder';


const StandardItemSize = { width: 80, height: 110 };

export const FREQUENCY_RESPONSE_VECTOR_URI = "http://two-play.com/plugins/toob#frequencyResponseVector";

const DEFAULT_PLOT_WIDTH = StandardItemSize.width * 2 - 16;
const PLOT_HEIGHT = StandardItemSize.height - 12;

const styles = (theme: Theme) => createStyles({
    frame: {
        width: DEFAULT_PLOT_WIDTH, height: PLOT_HEIGHT, background: "#444",
        borderRadius: 6,
        marginTop: 12, marginLeft: 8, marginRight: 8,
        boxShadow: "1px 4px 8px #000 inset"
    },
});

interface ToobFrequencyResponseProps extends WithStyles<typeof styles> {
    instanceId: number;
    propertyName?: string;
    width?: number;

}
interface ToobFrequencyResponseState {
    minDb: number;
    maxDb: number;
    minF: number;
    maxF: number;
    path: string;
}

const ToobFrequencyResponseView =
    withStyles(
        class extends React.Component<ToobFrequencyResponseProps, ToobFrequencyResponseState>
        {
            model: PiPedalModel;

            customizationId: number = 1;
            pathRef: React.RefObject<SVGPathElement|null>;

            constructor(props: ToobFrequencyResponseProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    path: "",
                    minDb: -30,
                    maxDb: 5,
                    minF: 30,
                    maxF: 20000
                };

                this.pathRef = React.createRef();
                this.onPedalboardChanged = this.onPedalboardChanged.bind(this);
                this.onStateChanged = this.onStateChanged.bind(this);
            }

            onStateChanged() {
                if (this.model.state.get() === State.Ready) {
                    this.requestDeferred = false;
                    this.requestOutstanding = false;
                    this.updateAllWaveShapes(); // after a reconnect.
                }
            }
            onPedalboardChanged() {
                this.updateAllWaveShapes();
            }
            private mounted: boolean = false;
            componentDidMount() {
                this.mounted = true;
                this.model.state.addOnChangedHandler(this.onStateChanged);
                this.model.pedalboard.addOnChangedHandler(this.onPedalboardChanged);
                this.updateAllWaveShapes();
            }
            componentWillUnmount() {
                this.mounted = false;
                this.model.state.removeOnChangedHandler(this.onStateChanged);
                this.model.pedalboard.removeOnChangedHandler(this.onPedalboardChanged);
            }

            componentDidUpdate() {
                //this.updateFrequencyResponse();
            }
            requestOutstanding: boolean = false;
            requestDeferred: boolean = false;

            dbMinDefault: number = -35;
            dbMaxDefault: number = 5;
            dbTickSpacingOpt: number = 10;


            MIN_DB_AF: number = Math.pow(10, -192 / 20);
            fMin: number = 30;
            fMax: number = 20000;
            logMin: number = Math.log(this.fMin);
            logMax: number = Math.log(this.fMax);

            // Size of the SVG element.
            xMin: number = 0;
            xMax() {
                return this.props.width ? this.props.width: DEFAULT_PLOT_WIDTH;
            }
            yMin: number = 0;
            yMax: number = PLOT_HEIGHT;
            dbTickSpacing: number = this.dbTickSpacingOpt;

            toX(frequency: number): number {
                var logV = Math.log(frequency);
                return (this.xMax() - this.xMin) * (logV - this.logMin) / (this.logMax - this.logMin) + this.xMin;

            }
            toY(value: number): number {
                value = Math.abs(value);

                var db;
                if (value < this.MIN_DB_AF) {
                    db = -192.0;
                } else {
                    db = 20 * Math.log10(value);
                }
                var y = (db - this.dbMin) / (this.dbMax - this.dbMin) * (this.yMax - this.yMin) + this.yMin;
                return y;
            }

            onFrequencyResponseUpdated(data: number[]) {
                if (!this.mounted) return;
                let pathBuilder = new SvgPathBuilder();
               
                let dbMin = data[2];
                let dbMax = data[3];
                let xMax = this.xMax();

                let n = data.length-4;

                let toX_ = (bin: number): number => {
                    return (xMax - this.xMin) * bin/n;
    
                };
                let toY_ = (value: number): number => {
   
                    var db;
                    if (value < this.MIN_DB_AF) {
                        db = -192.0;
                    } else {
                        db = 20 * Math.log10(value);
                    }
                    var y = (db - dbMin) / (dbMax - dbMin) * (this.yMax - this.yMin) + this.yMin;
                    return y;
                };
    

                if (n >= 2) {
                    pathBuilder.moveTo(toX_(0), toY_(data[4]))
                    for (let i = 1; i < n; i++) {
                        pathBuilder.lineTo(toX_(i), toY_(data[i+4]));
                    }
                }
                this.currentPath = pathBuilder.toString();
                this.setState({ 
                    minF: data[0],
                    maxF: data[1],
                    maxDb: data[2],
                    minDb: data[3],
                    path: this.currentPath 
                });

            }

            private getPropertyUri()
            {
                return this.props.propertyName? this.props.propertyName: FREQUENCY_RESPONSE_VECTOR_URI;
            }
            updateAllWaveShapes() {
                if (this.requestOutstanding) { // throttling.
                    this.requestDeferred = true;
                    return;
                }
                this.requestOutstanding = true;
                this.model.getPatchProperty<any>(this.props.instanceId, this.getPropertyUri())
                    .then((json) => {
                        if (json && json.otype_ === "Vector" && json.vtype_ === "Float") {
                            this.onFrequencyResponseUpdated(json.value as number[]);
                            if (this.requestDeferred) {
                                Utility.delay(10) // take  breath
                                    .then(
                                        () => {
                                            this.requestOutstanding = false;
                                            this.requestDeferred = false;
                                            this.updateAllWaveShapes();
                                        }

                                    );
                            } else {
                                this.requestOutstanding = false;
                            }
                        }
                    }).catch(error => {
                        // assume the connection was lost. We'll get saved by a reconnect.
                        this.requestOutstanding = false;
                        this.requestDeferred = false;
                    });
            }

            currentPath: string = "";

            majorGridLine(x0: number, y0: number, x1: number, y1: number): React.ReactNode {
                return (<line key={"l" + this.nextKey++} x1={x0} y1={y0} x2={x1} y2={y1} fill='none' stroke="#FFF" strokeWidth="0.75" opacity="1" />);
            }

            gridLine(x0: number, y0: number, x1: number, y1: number): React.ReactNode {
                return (<line key={"l" + this.nextKey++} x1={x0} y1={y0} x2={x1} y2={y1} fill='none' stroke="#FFF" strokeWidth="0.25" opacity="1" />);
            }
            grid(): React.ReactNode[] {
                let result: React.ReactNode[] = [];

                let xMax = this.xMax();
                for (var db = Math.ceil(this.dbMax / this.dbTickSpacing) * this.dbTickSpacing; db < this.dbMin; db += this.dbTickSpacing) {
                    var y = (db - this.dbMin) / (this.dbMax - this.dbMin) * (this.yMax - this.yMin);
                    if (db === 0) {
                        result.push(
                            this.majorGridLine(this.xMin, y, xMax, y)
                        );

                    } else {
                        result.push(
                            this.gridLine(this.xMin, y, xMax, y)
                        );
                    }
                }
                let decade0 = Math.pow(10, Math.floor(Math.log10(this.fMin)));
                for (var decade = decade0; decade < this.fMax; decade *= 10) {
                    for (var i = 1; i <= 10; ++i) {
                        var f = decade * i;
                        if (f > this.fMin && f < this.fMax) {
                            var x = this.toX(f);
                            if (i === 10) {
                                result.push(this.majorGridLine(x, this.yMin, x, this.yMax));

                            } else {
                                result.push(this.gridLine(x, this.yMin, x, this.yMax));
                            }
                        }
                    }
                }
                return result;
            }

            dbMin: number = 5;
            dbMax: number = -35;

            private nextKey: number = 0;
            render() {
                // deliberately reversed to flip up and down.
                this.nextKey = 0;
                this.dbMax = this.state.minDb;
                this.dbMin = this.state.maxDb;

                this.fMin = this.state.minF;
                this.fMax = this.state.maxF;
                this.logMin = Math.log(this.fMin);
                this.logMax = Math.log(this.fMax);

                let xMax = this.xMax();

                const classes = withStyles.getClasses(this.props);
                return (
                    <div className={classes.frame} style={{width: xMax }} >
                        <svg width={xMax} height={PLOT_HEIGHT} viewBox={"0 0 " + xMax + " " + PLOT_HEIGHT} stroke="#0F8" fill="none" strokeWidth="2.5" opacity="0.6">
                            {this.grid()}
                            <path d={this.state.path} ref={this.pathRef} />
                        </svg>

                    </div>);
            }
        },
        styles
    );



export default ToobFrequencyResponseView;