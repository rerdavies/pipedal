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

import { PiPedalModelFactory, PiPedalModel, State } from "./PiPedalModel";
import { StandardItemSize } from './PluginControlView';
import Utility from './Utility';
import SvgPathBuilder from './SvgPathBuilder';


const FREQUENCY_RESPONSE_VECTOR_URI = "http://two-play.com/plugins/toob#frequencyResponseVector";

const PLOT_WIDTH = StandardItemSize.width*2-16;
const PLOT_HEIGHT = StandardItemSize.height-12;

const styles = (theme: Theme) => createStyles({
    frame: {
        width: PLOT_WIDTH, height: PLOT_HEIGHT, background: "#444", 
        borderRadius: 6,
        marginTop: 12, marginLeft: 8, marginRight: 8,
        boxShadow: "1px 4px 8px #000 inset"
    },
});

interface ToobFrequencyResponseProps extends WithStyles<typeof styles> {
    instanceId: number;

};
interface ToobFrequencyResponseState {
    path: string;
};

const ToobFrequencyResponseView =
    withStyles(styles, { withTheme: true })(
        class extends React.Component<ToobFrequencyResponseProps, ToobFrequencyResponseState>
        {
            model: PiPedalModel;

            customizationId: number = 1;
            pathRef: React.RefObject<SVGPathElement>;

            constructor(props: ToobFrequencyResponseProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    path: ""
                };

                this.pathRef = React.createRef();
                this.onPedalBoardChanged = this.onPedalBoardChanged.bind(this);
                this.onStateChanged = this.onStateChanged.bind(this);
            }

            onStateChanged()
            {
                if (this.model.state.get() === State.Ready)
                {
                    this.requestDeferred = false;
                    this.requestOutstanding = false;
                    this.updateAllWaveShapes(); // after a reconnect.
                }
            }
            onPedalBoardChanged() 
            {
                this.updateAllWaveShapes();
            }
            componentDidMount()
            {
                this.model.state.addOnChangedHandler(this.onStateChanged);
                this.model.pedalBoard.addOnChangedHandler(this.onPedalBoardChanged);
                this.updateAllWaveShapes();
            }
            componentWillUnmount()
            {
                this.model.state.removeOnChangedHandler(this.onStateChanged);
                this.model.pedalBoard.removeOnChangedHandler(this.onPedalBoardChanged);
            }

            componentDidUpdate() {
                //this.updateFrequencyResponse();
            }
            requestOutstanding: boolean = false;
            requestDeferred: boolean = false;

            dbMinOpt: number = -35;
            dbMaxOpt: number = 5;
            dbTickSpacingOpt: number = 10;


            MIN_DB_AF: number = Math.pow(10,-192/20);
            fMin: number = 30;
            fMax: number = 20000;
            logMin: number = Math.log(this.fMin);
            logMax: number = Math.log(this.fMax);
    
            // Size of the SVG element.
            xMin: number = 0;
            xMax: number = PLOT_WIDTH+4;
            yMin: number = 0;
            yMax: number = PLOT_HEIGHT;
            dbMin: number = this.dbMaxOpt; // deliberately reversed to flip up and down.
            dbMax: number = this.dbMinOpt;
            dbTickSpacing: number = this.dbTickSpacingOpt;
    
            toX(frequency: number): number 
            {
                var logV = Math.log(frequency);
                return (this.xMax-this.xMin) * (logV-this.logMin)/(this.logMax-this.logMin) + this.xMin;
    
            }
            toY(value: number): number {
                value = Math.abs(value);

                var db;
                if (value < this.MIN_DB_AF) {
                    db = -192.0;
                } else {
                    db = 20*Math.log10(value);
                }
                var y = (db-this.dbMin)/(this.dbMax-this.dbMin)*(this.yMax-this.yMin) + this.yMin;
                return y;
            }

            onFrequencyResponseUpdated(data: number[]) {
                let pathBuilder = new SvgPathBuilder();
                if (data.length > 2)
                {
                    pathBuilder.moveTo(this.toX(data[0]),this.toY(data[1]))
                    for (let i = 2; i < data.length; i += 2)
                    {
                        pathBuilder.lineTo(this.toX(data[i]),this.toY(data[i+1]));
                    }
                }
                this.currentPath = pathBuilder.toString();
                this.setState({path: this.currentPath});

            }

            updateAllWaveShapes() {
                if (this.requestOutstanding) { // throttling.
                    this.requestDeferred = true;
                    return;
                }
                this.requestOutstanding = true;
                this.model.getLv2Parameter<number[]>(this.props.instanceId, FREQUENCY_RESPONSE_VECTOR_URI)
                    .then((data) => {
                        this.onFrequencyResponseUpdated(data);
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
                    }).catch(error => {
                        // assume the connection was lost. We'll get saved by a reconnect.
                        this.requestOutstanding = false;
                        this.requestDeferred = false;
                    });
            }

            currentPath: string = "";

            gridLine(x0: number, y0: number, x1: number, y1: number): React.ReactNode {
                return (<line x1={x0} y1={y0} x2={x1} y2={y1} fill='none' stroke="#FFF" strokeWidth="0.25" opacity="1" />);
            }
            grid(): React.ReactNode[] {
                let result: React.ReactNode[] = [];

                
                for (var db = Math.ceil(this.dbMax/this.dbTickSpacing)*this.dbTickSpacing; db < this.dbMin; db += this.dbTickSpacing )
                {
                    var y = (db-this.dbMin)/(this.dbMax-this.dbMin)*(this.yMax-this.yMin);
                    result.push( 
                        this.gridLine(this.xMin,y,this.xMax,y)
                        );
                }
                let decade0 = Math.pow(10,Math.floor(Math.log10(this.fMin)));
                for (var decade = decade0; decade < this.fMax; decade *= 10)
                {
                    for (var i = 0; i < 10; ++i)
                    {
                        var f = decade*i;
                        if (f > this.fMin && f < this.fMax)
                        {
                            var x = this.toX(f);
                            result.push(this.gridLine(x,this.yMin,x,this.yMax));
                        }
                    }
                }
                return result;                
            }

            render() {
                let classes = this.props.classes;
                return (
                    <div className={classes.frame} >
                        <svg width={PLOT_WIDTH} height={PLOT_HEIGHT} viewBox={"0 0 " + PLOT_WIDTH + " " + PLOT_HEIGHT} stroke="#0F8" fill="none" strokeWidth="2.5" opacity="0.6">
                            { this.grid() }
                            <path d={this.state.path} ref={this.pathRef} />
                        </svg>

                    </div>);
            }
        }
    );



export default ToobFrequencyResponseView;