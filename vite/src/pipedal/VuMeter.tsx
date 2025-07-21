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

import React, { Component } from 'react';
import { Theme } from '@mui/material/styles';
import WithStyles, {withTheme} from './WithStyles';
import { withStyles } from "tss-react/mui";
import { css } from '@emotion/react';

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

}

class TelltaleState {
    telltaleDb: number = MIN_DB;
    lastTelltaleTime: number = 0;
    telltaleHoldTime: number = 0;
    telltaleHoldValue: number = MIN_DB;

    reset() : void {
        this.telltaleHoldTime = 0;
        this.lastTelltaleTime = 0;
        this.telltaleDb = MIN_DB;
        this.telltaleHoldValue = MIN_DB;
    }
    getTelltaleHoldValue(currentDb: number): number {
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
        this.telltaleHoldValue = holdValue;

        if (currentDb > holdValue) {
            this.telltaleDb = currentDb;
            this.telltaleHoldTime = t + TELLTALE_HOLD_MS;
            return currentDb;
        } else {
            return holdValue;
        }
    }

}

const LEFT_X = BORDER_THICKNESS;
const RIGHT_X = BORDER_THICKNESS + CHANNEL_WIDTH + BORDER_THICKNESS;
const TOP_Y = BORDER_THICKNESS;
const VU_BAR_HEIGHT = DISPLAY_HEIGHT-2*BORDER_THICKNESS;

const styles = (theme: Theme) => ({
    frame: css({
        position: "relative",
        width: CHANNEL_WIDTH + 2*BORDER_THICKNESS,
        borderTop: BORDER_THICKNESS + "px black solid",
        borderBottom: BORDER_THICKNESS + "px black solid",
        height: DISPLAY_HEIGHT,
        background: "black",
        overflow: "hidden",
        marginLeft: (CHANNEL_WIDTH+BORDER_THICKNESS)/2,
        marginRight: (CHANNEL_WIDTH+BORDER_THICKNESS)/2
    }),
    frameStereo: css({
        position: "relative",
        width: 2*CHANNEL_WIDTH + 3*BORDER_THICKNESS,
        borderTop: BORDER_THICKNESS + "px black solid",
        borderBottom: BORDER_THICKNESS + "px black solid",
        height: DISPLAY_HEIGHT,
        background: "black",
        overflow: "hidden",
    }),
    monoTextFrame: css({
        display: "flex",flexFlow: "column nowrap", alignItems: "center", textAlign: "center", overflow: "clip",
    }),
    stereoTextFrame: css({
        display: "flex",flexFlow: "column nowrap", alignItems: "center", textAlign: "center",overflow: "clip",
    }),
    vuTextFrame: css({
        color: theme.palette.text.secondary,
        display: "div",
        position: "relative",
        overflow: "clip",
        marginTop: 4,
        textAlign: "center",
        width: 20, height: 16,
        fontSize: "11px",
        justifyContent: "center",
        alignItems: "center"
    }),
    frameL: css({
        position: "absolute",
        top:  TOP_Y ,
        left: LEFT_X,
        width: CHANNEL_WIDTH,
        height: VU_BAR_HEIGHT,
        overflow: "clip"
    }),
    frameR: css({
        position: "absolute",
        overflow: "clip",
        top:  TOP_Y,
        left: RIGHT_X,
        width: CHANNEL_WIDTH,
        height: VU_BAR_HEIGHT,
    }),
    yellowBar: css({
        top: 0,
        left: 0,
        width: CHANNEL_WIDTH,
        height: VU_BAR_HEIGHT,
        position: "absolute",
        background: "#CC0",
        transform: "translateY(" + VU_BAR_HEIGHT + "px)"
    }),
    greenBar: css({
        top: 0,
        left: 0,
        width: CHANNEL_WIDTH,
        height: VU_BAR_HEIGHT,
        position: "absolute",
        background: "#0C2",
        transform: "translateY(" + VU_BAR_HEIGHT + "px)"
    }),
    redBar: css({
        top: 0,
        left: 0,
        width: CHANNEL_WIDTH,
        height: VU_BAR_HEIGHT,
        position: "absolute",
        background: "#F00",
        transform: "translateY(" + VU_BAR_HEIGHT + "px)"    
    }),
    indicatorBar: css({
        position: "absolute",
        left: 0,
        width: CHANNEL_WIDTH,
        height: TELLTALE_HEIGHT + "px",
        top: 0,
        background: "#F00",
        transform: "translateY(" + VU_BAR_HEIGHT + "px)"    


    }),
});



interface VuMeterProps extends WithStyles<typeof styles> {
    instanceId: number;
    display: "input" | "output";
    theme: Theme;
    displayText?: boolean;
}

interface VuMeterState {
    isStereo: boolean;
}

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
    withTheme(withStyles(
        class extends Component<VuMeterProps, VuMeterState>
        {
            divRef: React.RefObject<HTMLDivElement|null> = React.createRef();
            textRef: React.RefObject<HTMLDivElement|null> = React.createRef();
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
            requestedStereoState: boolean = false;

            private currentDisplayValue: string | null = null;
            updateText(telltaleDb: number)
            {

                if (this.textRef.current) {
                    let displayValue: string; 
                    if (telltaleDb <= MIN_DB)
                    {
                        displayValue = "-";
                    } else {
                        let iDb = Math.round(telltaleDb);
                        if (iDb > 0)
                        {
                            displayValue = "+" + Math.round(telltaleDb).toString() + "\u00A0"
                        } else if (iDb < 0) {
                            displayValue = Math.round(telltaleDb).toString() + "\u00A0";
                        } else {
                            if (telltaleDb === 0) {
                                displayValue = "0";
                            } else if (telltaleDb > 0)
                            {
                                displayValue = "+0\u00A0";
                            } else {
                                displayValue = "-0\u00A0";
                            }
                        }
                    }
                    
                    if (this.currentDisplayValue !== displayValue)  
                    {
                        this.currentDisplayValue = displayValue;
                        this.textRef.current.innerText = displayValue;
                    }
                }
            }

            private currentVuInfo: VuUpdateInfo | null = null;

            private animationFrameHandle: number | null = null;
            onVuChanged(vuInfo: VuUpdateInfo): void {
                this.currentVuInfo = vuInfo;
                if (this.animationFrameHandle == null)
                {
                    this.animationFrameHandle = window.requestAnimationFrame(() => {
                        this.animationFrameHandle = null;
                        this.animateVuUpdate();
                    });
                }
            }
            private animateVuUpdate() : void
            {
                let value: number;
                let valueR: number;
                let vuInfo = this.currentVuInfo;
                if (!vuInfo) {
                    return; // no vu info.
                }
                if (!this.divRef.current) { // unmounted?
                    return;
                } 

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
                    if (isStereo !== this.requestedStereoState) // guard against overrunning the layout engine.
                    {
                        this.requestedStereoState = isStereo;

                        this.setState({
                            isStereo: isStereo
                        });
                        return; // we can't proceed without stereo layout.
                    }
                }

                let childNodes = this.divRef.current!.childNodes;

                let leftFrameChildNodes = childNodes[0].childNodes;

                let vuData: VuChannelData = {
                    value: value,
                    redDiv: leftFrameChildNodes[0] as HTMLDivElement,
                    yellowDiv: leftFrameChildNodes[1] as HTMLDivElement,
                    greenDiv: leftFrameChildNodes[2] as HTMLDivElement,
                    telltaleDiv: leftFrameChildNodes[3] as HTMLDivElement
                };
                this.updateChannel(vuData,this.telltaleStateL);
                if (this.state.isStereo) {
                    let rightFrameChildren = childNodes[1].childNodes;
                    vuData = {
                        value: valueR,
                        redDiv: rightFrameChildren[0] as HTMLDivElement,
                        yellowDiv: rightFrameChildren[1] as HTMLDivElement,
                        greenDiv: rightFrameChildren[2] as HTMLDivElement,
                        telltaleDiv: rightFrameChildren[3] as HTMLDivElement
                    };
                    this.updateChannel(vuData, this.telltaleStateR);
                }
                if (this.props.displayText)
                {
                    if (this.state.isStereo)
                    {
                        this.updateText(Math.max(this.telltaleStateL.telltaleHoldValue,this.telltaleStateR.telltaleHoldValue));
                    } else {
                        this.updateText(this.telltaleStateL.telltaleHoldValue);
                    }
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
                    vuData.greenDiv.style.transform = `translateY(${y}px)`;
                    vuData.yellowDiv.style.transform = `translateY(${INVISIBLE_Y})`;
                    vuData.redDiv.style.transform = `translateY(${INVISIBLE_Y})`;
                } else {
                    vuData.greenDiv.style.transform = `translateY(${this.yYellow}px)`;
                    if (y >= this.yZero) {
                        vuData.yellowDiv.style.transform = `translateY(${y}px)`;
                        vuData.redDiv.style.transform = `translateY(${INVISIBLE_Y})`;
                    } else {
                        vuData.yellowDiv.style.transform = `translateY(${this.yZero}px)`;
                        vuData.redDiv.style.transform = `translateY(${y}px)`;
                    }
                }
                let dbTelltale = telltaleState.getTelltaleHoldValue(db);
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
                vuData.telltaleDiv.style.transform = `translateY(${yTelltale}px)`;


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
                let displayText = this.props.displayText??false;
                const classes = withStyles.getClasses(this.props);
                if (displayText)
                {
                    return (
                        <div className={this.state.isStereo? classes.stereoTextFrame: classes.monoTextFrame}>
                            {
                                this.renderVus()
                            }
                            <div ref={this.textRef} className={classes.vuTextFrame}
                            />
                        </div>
                    );
                } else {
                    return this.renderVus();
                }

            }

            renderVus() {

                const classes = withStyles.getClasses(this.props);
                if (!this.state.isStereo) {
                    return (<div className={classes.frame} ref={this.divRef}
                    >
                        <div className={classes.frameL}>
                            <div className={classes.redBar} />
                            <div className={classes.yellowBar} />
                            <div className={classes.greenBar} />
                            <div className={classes.indicatorBar} />
                        </div>
                    </div>);
                } else {
                    return (<div className={classes.frameStereo} ref={this.divRef}
                    >
                        <div className={classes.frameL}>
                            <div className={classes.redBar} />
                            <div className={classes.yellowBar} />
                            <div className={classes.greenBar} />
                            <div className={classes.indicatorBar} />
                        </div>
                        <div className={classes.frameR}>   
                            <div className={classes.redBar} />
                            <div className={classes.yellowBar} />
                            <div className={classes.greenBar} />
                            <div className={classes.indicatorBar} />
                        </div>

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
        },
        styles
    ));

export default VuMeter;



