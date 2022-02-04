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
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';
import { PiPedalModel, State,PiPedalModelFactory, VuUpdateInfo, VuSubscriptionHandle } from './PiPedalModel';


const DISPLAY_HEIGHT = 110;
const BORDER_THICKNESS = 2;
const TELLTALE_HEIGHT = 4;
const CHANNEL_WIDTH = 4;
const TELLTALE_HOLD_MS = 2000;
const TELLTALE_DECAY_RATE = -20 / 1000.0; // -20db in one second.


const INTERIOR_DISPLAY_HEIGHT = DISPLAY_HEIGHT - BORDER_THICKNESS * 2;


const MIN_DB = -35;
const MAX_DB = 6;
const DB_YELLOW = -12;



interface VuChannelData {
    value: number;
    redDiv: HTMLDivElement;
    yellowDiv: HTMLDivElement;
    greenDiv: HTMLDivElement;
    telltaleDiv: HTMLDivElement;

};

class TelltaleState {
    telltaleDb: number = MIN_DB;
    lastTelltaleTime: number = 0;
    telltaleHoldTime: number = 0;

    reset() : void {
        this.telltaleHoldTime = 0;
        this.lastTelltaleTime = 0;
        this.telltaleDb = MIN_DB;
    }
    getValue(currentDb: number): number {
        let t = new Date().getTime();

        let holdValue: number;
        if (t < this.telltaleHoldTime) {
            holdValue = this.telltaleDb;
        } else {
            if (this.telltaleHoldTime === 0) {
                holdValue = MIN_DB;
            } else {
                holdValue = this.telltaleDb + (t - this.telltaleHoldTime) * TELLTALE_DECAY_RATE;
                if (holdValue < MIN_DB) {
                    this.telltaleDb = holdValue = MIN_DB;
                    this.telltaleHoldTime = t + 100 * 1000; // wait for a long time.
                }
            }
        }

        if (currentDb > holdValue) {
            this.telltaleDb = currentDb;
            this.telltaleHoldTime = t + TELLTALE_HOLD_MS;
            return currentDb;
        } else {
            return holdValue;
        }
    }

};

const LEFT_X = BORDER_THICKNESS;
const RIGHT_X = BORDER_THICKNESS + + CHANNEL_WIDTH + BORDER_THICKNESS;

const styles = ({ palette }: Theme) => createStyles({
    frame: {
        position: "relative",
        width: CHANNEL_WIDTH + 2*BORDER_THICKNESS,
        borderTop: BORDER_THICKNESS + "px black solid",
        borderBottom: BORDER_THICKNESS + "px black solid",
        height: DISPLAY_HEIGHT,
        background: "black",
        overflow: "hidden",
        marginLeft: (CHANNEL_WIDTH+BORDER_THICKNESS)/2,
        marginRight: (CHANNEL_WIDTH+BORDER_THICKNESS)/2
    },
    frameStereo: {
        position: "relative",
        width: 2*CHANNEL_WIDTH + 3*BORDER_THICKNESS,
        borderTop: BORDER_THICKNESS + "px black solid",
        borderBottom: BORDER_THICKNESS + "px black solid",
        height: DISPLAY_HEIGHT,
        background: "black",
        overflow: "hidden",
    },
    redFrameL: {
        position: "absolute",
        top: "100%",
        left: LEFT_X,
        width: CHANNEL_WIDTH,
        height: DISPLAY_HEIGHT - 2,
        background: "#F00"
    },
    yellowFrameL: {
        top: "100%",
        left: LEFT_X,
        width: CHANNEL_WIDTH,
        position: "absolute",
        height: DISPLAY_HEIGHT - 2,
        background: "#CC0"
    },
    greenFrameL: {
        top: "100%",
        left: LEFT_X,
        width: CHANNEL_WIDTH,
        position: "absolute",
        height: DISPLAY_HEIGHT - 2,
        background: "#0C2"
    },
    indicatorL: {
        position: "absolute",
        left: LEFT_X,
        width: CHANNEL_WIDTH,
        height: TELLTALE_HEIGHT + "px",
        top: "100%",
        background: "#F00"

    },
    redFrameR: {
        position: "absolute",
        top: "100%",
        left: RIGHT_X,
        width: CHANNEL_WIDTH,
        height: DISPLAY_HEIGHT - 2,
        background: "#F00"
    },
    yellowFrameR: {
        top: "100%",
        left: RIGHT_X,
        width: CHANNEL_WIDTH,
        position: "absolute",
        height: DISPLAY_HEIGHT - 2,
        background: "#CC0"
    },
    greenFrameR: {
        top: "100%",
        left: RIGHT_X,
        width: CHANNEL_WIDTH,
        position: "absolute",
        height: DISPLAY_HEIGHT - 2,
        background: "#0C2"
    },
    indicatorR: {
        position: "absolute",
        left: RIGHT_X,
        width: CHANNEL_WIDTH,
        height: TELLTALE_HEIGHT + "px",
        top: "100%",
        background: "#F00"

    }

});



interface VuMeterProps extends WithStyles<typeof styles> {
    instanceId: number;
    display: "input" | "output";
    theme: Theme;
};

interface VuMeterState {
    isStereo: boolean;
};

function aToDb(value: number): number {
    if (value === 0) return -96;
    return Math.log10(Math.abs(value)) * 20;
}

function dbToY(db: number): number {
    if (db < MIN_DB) db = MIN_DB;
    if (db > MAX_DB) db = MAX_DB;

    let y = INTERIOR_DISPLAY_HEIGHT - (db - MIN_DB) / (MAX_DB - MIN_DB) * INTERIOR_DISPLAY_HEIGHT;;
    return y;
}

export const VuMeter =
    withStyles(styles, { withTheme: true })(
        class extends Component<VuMeterProps, VuMeterState>
        {
            divRef: React.RefObject<HTMLDivElement> = React.createRef();
            model: PiPedalModel;

            yZero: number;
            yYellow: number;

            constructor(props: VuMeterProps) {
                super(props);
                this.state = {
                    isStereo: false
                };
                this.model = PiPedalModelFactory.getInstance();

                this.yZero = dbToY(0);
                this.yYellow = dbToY(DB_YELLOW);

                this.onVuChanged = this.onVuChanged.bind(this);
                this.onStateChanged = this.onStateChanged.bind(this);

            }


            telltaleStateL: TelltaleState = new TelltaleState();
            telltaleStateR: TelltaleState = new TelltaleState();

            vuInfo?: VuUpdateInfo;
            requesttedStereoState: boolean = false;

            onVuChanged(vuInfo: VuUpdateInfo): void {
                let value: number;
                let valueR: number;

                this.vuInfo = vuInfo;


                let isStereo: boolean;

                if (this.props.display === "input") {
                    value = vuInfo.inputMaxValueL;
                    valueR = vuInfo.inputMaxValueR;
                    isStereo = vuInfo.isStereoInput;
                } else {
                    isStereo = vuInfo.isStereoOutput;
                    value = vuInfo.outputMaxValueL;
                    valueR = vuInfo.outputMaxValueR;
                }
                if (this.state.isStereo !== isStereo)
                {
                    if (isStereo !== this.requesttedStereoState) // guard against overrunning the layout engine.
                    {
                        this.requesttedStereoState = isStereo;

                        this.setState({
                            isStereo: isStereo
                        });
                        return; // we can't procede without stereo layout.
                    }
                }

                let childNodes = this.divRef.current!.childNodes;

                let vuData: VuChannelData = {
                    value: value,
                    redDiv: childNodes[0] as HTMLDivElement,
                    yellowDiv: childNodes[1] as HTMLDivElement,
                    greenDiv: childNodes[2] as HTMLDivElement,
                    telltaleDiv: childNodes[3] as HTMLDivElement
                };
                this.updateChannel(vuData,this.telltaleStateL);
                if (this.state.isStereo) {
                    vuData = {
                        value: valueR,
                        redDiv: childNodes[4] as HTMLDivElement,
                        yellowDiv: childNodes[5] as HTMLDivElement,
                        greenDiv: childNodes[6] as HTMLDivElement,
                        telltaleDiv: childNodes[7] as HTMLDivElement
                    };
                    this.updateChannel(vuData, this.telltaleStateR);
                }
            }

            resetTelltales() {
                this.telltaleStateL.reset();
                this.telltaleStateR.reset();
            }
            updateChannel(vuData: VuChannelData, telltaleState: TelltaleState) {
                let db = aToDb(vuData.value);
                if (db > MAX_DB) db = MAX_DB;
                if (db < MIN_DB) db = MIN_DB;

                let y = dbToY(db);
                let INVISIBLE_Y = INTERIOR_DISPLAY_HEIGHT + "px";
                if (y >= this.yYellow) {
                    // green only.
                    vuData.greenDiv.style.top = y + "px";
                    vuData.yellowDiv.style.top = INVISIBLE_Y;
                    vuData.redDiv.style.top = INVISIBLE_Y;
                } else {
                    vuData.greenDiv.style.top = this.yYellow + "px";
                    if (y >= this.yZero) {
                        vuData.yellowDiv.style.top = y + "px";
                        vuData.redDiv.style.top = INVISIBLE_Y;
                    } else {
                        vuData.yellowDiv.style.top = this.yZero + "px";
                        vuData.redDiv.style.top = y + "px";
                    }
                }
                let dbTelltale = telltaleState.getValue(db);
                let yTelltale = dbToY(dbTelltale);

                if (yTelltale < this.yZero) {
                    vuData.telltaleDiv.style.background = "#F00";
                } else if (yTelltale < this.yYellow) {
                    vuData.telltaleDiv.style.background = "#CC0";
                } else {
                    vuData.telltaleDiv.style.background = "#0F2";
                }
                if (yTelltale > INTERIOR_DISPLAY_HEIGHT - TELLTALE_HEIGHT) {
                    // non-zero and zero are different displays.
                    // zero: nothing.. tiny: show the telltale.
                    if (vuData.value !== 0) {
                        yTelltale = INTERIOR_DISPLAY_HEIGHT - TELLTALE_HEIGHT;
                    } else {
                        yTelltale = INTERIOR_DISPLAY_HEIGHT;
                    }
                }
                vuData.telltaleDiv.style.top = yTelltale + "px";


            }
            subscriptionHandle?: VuSubscriptionHandle;

            subscribedInstanceId?: number;

            addVuSubscription() {
                this.removeVuSubscription();

                if (this.model.state.get() === State.Ready)
                {
                    if (this.props.instanceId !== -1) {
                        this.subscribedInstanceId = this.props.instanceId;
                        this.subscriptionHandle = this.model.addVuSubscription(this.props.instanceId, this.onVuChanged);
                        this.resetTelltales();
                    }
                }
            }
            removeVuSubscription() {
                if (this.subscriptionHandle) {
                    this.model.removeVuSubscription(this.subscriptionHandle);
                    this.subscriptionHandle = undefined;
                    this.subscribedInstanceId = undefined;
                }
            }

            componentDidUpdate()
            {
                if (this.subscribedInstanceId)
                {
                    if (this.props.instanceId !== this.subscribedInstanceId)
                    {
                        this.removeVuSubscription();
                        this.addVuSubscription();
                    }
                }

            }


            render() {
                let classes = this.props.classes;

                if (!this.state.isStereo) {
                    return (<div className={classes.frame} ref={this.divRef}
                    >
                        <div className={classes.redFrameL} />
                        <div className={classes.yellowFrameL} />
                        <div className={classes.greenFrameL} />
                        <div className={classes.indicatorL} />
                    </div>);
                } else {
                    return (<div className={classes.frameStereo} ref={this.divRef}
                    >
                        <div className={classes.redFrameL} />
                        <div className={classes.yellowFrameL} />
                        <div className={classes.greenFrameL} />
                        <div className={classes.indicatorL} />

                        <div className={classes.redFrameR} />
                        <div className={classes.yellowFrameR} />
                        <div className={classes.greenFrameR} />
                        <div className={classes.indicatorR} />

                    </div>);

                }
            }

            onStateChanged(state: State) {
                // initial connection or reconnect
                if (state === State.Ready)
                {
                    this.addVuSubscription(); // re-subscribe.
                } else {
                    this.removeVuSubscription();
                }
            }

            componentDidMount() {
                this.model.state.addOnChangedHandler(this.onStateChanged);

                this.addVuSubscription();
            }
            componentWillUnmount() {
                this.removeVuSubscription();
                this.model.state.removeOnChangedHandler(this.onStateChanged);
            }
        }
    );

export default VuMeter;



