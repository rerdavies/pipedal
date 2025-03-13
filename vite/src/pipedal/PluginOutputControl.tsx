// Copyright (c) 2023 Robin Davies
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
import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";
import { UiControl } from './Lv2Plugin';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, MonitorPortHandle, State } from './PiPedalModel';
import { isDarkMode } from './DarkMode';
import GxTunerControl from './GxTunerControl';
import Units from './Units';
import ControlTooltip from './ControlTooltip';
import { css } from '@emotion/react';



function makeLedGradient(color: string) {
    if (color === "green") {
        if (isDarkMode()) {
            return "radial-gradient(circle at center, #0F0 0, #0F0 20%, #333 100%)";
        } else {
            return "radial-gradient(circle at center, #4F4 0, #4F4 40%, #464 100%)";
        }
    }
    if (color === "blue") {
        if (isDarkMode()) {
            return "radial-gradient(circle at center, #00F 0, #00F 20%, #333 100%)";
        } else {
            return "radial-gradient(circle at center, #44F 0, #44F 40%, #446 100%)";
        }
    }
    if (color === "yellow") {
        if (isDarkMode()) {
            return "radial-gradient(circle at center, #FF0 0, #FF0 20%, #333 100%)";
        } else {
            return "radial-gradient(circle at center, #FF4 0, #FF4 40%, #664 100%)";
        }
    }
    if (isDarkMode()) {
        return "radial-gradient(circle at center, #F000 0, #F00 20%, #333 100%)";
    } else {
        return "radial-gradient(circle at center, #F44 0, #F44 40%, #644 100%)";
    }

}



const styles = (theme: Theme) => createStyles({
    frame: css({
        position: "relative",
        margin: "12px"
    }),
    switchTrack: css({
        height: '100%',
        width: '100%',
        borderRadius: 14 / 2,
        zIndex: -1,
        transition: theme.transitions.create(['opacity', 'background-color'], {
            duration: theme.transitions.duration.shortest
        }),
        backgroundColor: theme.palette.secondary.main,
        opacity: theme.palette.mode === 'light' ? 0.38 : 0.3
    }),
    displayValue: css({
        position: "absolute",
        top: 0,
        left: 0,
        right: 0,
        bottom: 4,
        textAlign: "center",
        background: "white",
        color: "#666",
        // zIndex: -1,
    }),
    controlFrame: css({
        display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "space-between", height: 116
    }),

    titleSection: css({
        flex: "0 0 auto", alignSelf:"stretch", marginBottom: 8, marginLeft: 0, marginRight: 0
    }),

    midSection: css({
        flex: "1 1 1", display: "flex",flexFlow: "column nowrap",alignContent: "center",justifyContent: "center"
    }),

    editSection: css({
        flex: "0 0 28", position: "relative", width: 60, height: 28, minHeight: 28
    }),
    editSectionNoContent: css({
        flex: "0 0 28", position: "relative", width: 1, height: 28, minHeight: 28         
    })

});


export interface PluginOutputControlProps extends WithStyles<typeof styles> {
    uiControl: UiControl;
    instanceId: number;
}
type PluginOutputControlState = {
    hasValue: boolean;
    value: number;
};

const PluginOutputControl =
    withStyles(
        class extends Component<PluginOutputControlProps, PluginOutputControlState> {

            private model: PiPedalModel;
            private vuRef: React.RefObject<HTMLDivElement|null>;
            private progressRef: React.RefObject<HTMLDivElement|null>;
            private dbVuRef: React.RefObject<HTMLDivElement|null>;
            private dbVuTelltaleRef: React.RefObject<HTMLDivElement|null>;
            private lampRef: React.RefObject<HTMLDivElement|null>;


            constructor(props: PluginOutputControlProps) {
                super(props);
                this.vuRef = React.createRef<HTMLDivElement>();
                this.progressRef = React.createRef<HTMLDivElement>();
                this.dbVuRef = React.createRef<HTMLDivElement>();
                this.dbVuTelltaleRef = React.createRef<HTMLDivElement>();
                this.lampRef = React.createRef<HTMLDivElement>();
                this.state = {
                    hasValue: false,
                    value: 0
                };
                this.model = PiPedalModelFactory.getInstance();
                this.onConnectionStateChanged = this.onConnectionStateChanged.bind(this);
            }

            private onConnectionStateChanged(state: State) {
                if (state === State.Ready) {
                    this.unsubscribe();
                    this.subscribe();
                }
            }

            private mounted: boolean = false;
            componentWillUnmount() {
                if (super.componentWillUnmount) {
                    super.componentWillUnmount();
                }
                this.mounted = false;
                this.model.state.removeOnChangedHandler(this.onConnectionStateChanged);
                this.unsubscribe();
            }
            componentDidMount(): void {
                this.mounted = true;
                this.subscribe();
                this.model.state.addOnChangedHandler(this.onConnectionStateChanged);

                if (super.componentDidMount) {
                    super.componentDidMount();
                }
            }

            componentDidUpdate(prevProps: Readonly<PluginOutputControlProps>, prevState: Readonly<PluginOutputControlState>, snapshot?: any): void {
                if (prevProps.instanceId !== this.props.instanceId) {
                    this.unsubscribe();
                    this.subscribe();
                }
            }

            private PROGRESS_WIDTH = 40-4;
            private VU_HEIGHT = 60 - 4;
            private DB_VU_HEIGHT = 60 - 4;
            private animationHandle: number | undefined = undefined;

            private dbVuTelltale = -96.0;
            private dbVuValue = -96.0;

            private dbVuHoldTime = 0.0;

            private requestDbVuAnimation() {
                if (!this.animationHandle) {
                    this.animationHandle = requestAnimationFrame(
                        () => {
                            let value = this.dbVuValue;
                            let range = this.vuMap(value);
                            let top = range - this.VU_HEIGHT;
                            if (top > 0) top = 0;

                            if (value > this.dbVuTelltale) {
                                this.dbVuTelltale = value;
                                this.dbVuHoldTime = Date.now() + 2000;
                            }
                            if (this.dbVuRef.current) {
                                this.dbVuRef.current.style.marginTop = top + "px";
                            }
                            this.animationHandle = undefined;
                            this.updateDbVuTelltale();
                        }
                    )
                }
            }

            updateDbVuTelltale() {
                let telltaleDone = true;
                if (this.dbVuHoldTime !== 0) {
                    telltaleDone = false;
                    let t = Date.now();
                    if (t >= this.dbVuHoldTime) {
                        let dt = t - this.dbVuHoldTime;
                        let telltaleValue = this.dbVuTelltale - 30 * dt / 1000;
                        if (telltaleValue < -200) {
                            telltaleValue = -200;
                            telltaleDone = true;
                        }
                        this.dbVuTelltale = telltaleValue;
                        this.dbVuHoldTime = t;

                    }
                    let y = this.dbVuMap(this.dbVuTelltale);
                    if (y < 0) y = 0;

                    if (this.dbVuTelltaleRef.current) {
                        let telltaleStyle = this.dbVuTelltaleRef.current.style;
                        telltaleStyle.marginTop = y + "px";
                        let telltaleColor = "#0C0";
                        if (this.dbVuTelltale >= 0) {
                            telltaleColor = "#F00";
                        } else if (this.dbVuTelltale >= -10) {
                            telltaleColor = "#FF0";
                        }
                        telltaleStyle.background = telltaleColor;
                    }
                }
                if (!telltaleDone) {
                    this.requestDbVuAnimation();
                }
            }
            private updateValue(value: number) {
                if (this.lampRef.current) {
                    let control = this.props.uiControl;
                    let range = (value - control.min_value) / (control.max_value - control.min_value);
                    this.lampRef.current.style.opacity = range + "";
                } else if (this.dbVuRef.current) {
                    this.dbVuValue = value;
                    this.requestDbVuAnimation();
                } else if (this.progressRef.current) {
                    let control = this.props.uiControl;
                    let range = (value - control.min_value) / (control.max_value - control.min_value);
                    let w = this.PROGRESS_WIDTH * range;

                    if (!this.animationHandle) {
                        this.animationHandle = requestAnimationFrame(
                            () => {
                                if (this.progressRef.current) {
                                    this.progressRef.current.style.width = w + "px";
                                }
                                this.animationHandle = undefined;
                            }
                        )
                    }

                } 
                else if (this.vuRef.current) {
                    let control = this.props.uiControl;
                    let range = (value - control.min_value) / (control.max_value - control.min_value);
                    let top = this.VU_HEIGHT - range * this.VU_HEIGHT;

                    if (!this.animationHandle) {
                        this.animationHandle = requestAnimationFrame(
                            () => {
                                if (this.vuRef.current) {
                                    this.vuRef.current.style.marginTop = top + "px";
                                }
                                this.animationHandle = undefined;
                            }
                        )
                    }

                } else {
                    if (this.mounted) {
                        this.setState({ hasValue: true, value: value });
                    }
                }
            }

            private monitorPortHandle?: MonitorPortHandle;

            private subscribe(): void {
                this.unsubscribe();
                this.monitorPortHandle = this.model.monitorPort(
                    this.props.instanceId,
                    this.props.uiControl.symbol,
                    1 / 15.0, // update rate.
                    (value) => {
                        this.updateValue(value);
                    }
                );

            }
            private unsubscribe(): void {
                if (this.monitorPortHandle) {
                    this.model.unmonitorPort(this.monitorPortHandle)
                    this.monitorPortHandle = undefined;
                }
            }

            isShortSelectOrText(control: UiControl) {
                if (control.scale_points.length === 0) {
                    return true;
                }
                for (let scale_point of control.scale_points) {
                    if (scale_point.label.length > 12) return false;
                }
                return true;
            }

            formatDisplayValue(uiControl: UiControl | undefined, value: number): string {
                if (!uiControl) return "";

                return uiControl.formatDisplayValue(value);
            }


            dbVuMap(value: number): number {
                let control = this.props.uiControl;
                let y = (control.max_value - value) * this.DB_VU_HEIGHT / (control.max_value - control.min_value);
                return y;
            }

            vuMap(value: number): number {
                let control = this.props.uiControl;
                let y = (control.max_value - value) * this.VU_HEIGHT / (control.max_value - control.min_value);
                return y;
            }
            render() {

                const classes = withStyles.getClasses(this.props);
                let control: UiControl = this.props.uiControl;


                let text = "";
                if (this.state.hasValue) {
                    let value = this.state.value;
                    if (control.isOnOffSwitch()) {
                        text = value === 0 ? "OFF" : "ON";
                    } else {
                        text = this.formatDisplayValue(control, value);
                    }
                }

                let isText = control.isOutputText();

                let item_width: number | undefined = isText ? 160 : 80;
                if (isText) {
                    if (this.isShortSelectOrText(control)) {
                        item_width = 80;
                    }
                }
                if (control.isTuner()) {
                    item_width = undefined;
                    return (
                        <div style={{ display: "flex", flexDirection: "column", width: item_width, margin: 8, height: 98 }}>
                            <GxTunerControl instanceId={this.props.instanceId} symbolName={control.symbol}
                                valueIsMidi={control.units === Units.midiNote}
                            />
                        </div >

                    );
                } else if (control.isProgress()) {
                    item_width = undefined;
                    return (
                        <div className={classes.controlFrame} 
                            style={{ width: item_width }}>
                            {/* TITLE SECTION */}
                            <div className={classes.titleSection} 
                                style={{ width: "100%"  }}>
                                <ControlTooltip uiControl={control}>
                                    <Typography  variant="caption" display="block" style={{
                                        width: "100%",
                                        textAlign: "center"
                                    }}> {control.name === "" ? "\u00A0" : control.name}</Typography>
                                </ControlTooltip>
                            </div>
                            {/* CONTROL SECTION */}

                            <div className={classes.midSection} 
                                style={{ flex: "1 1 1", display: "flex", justifyContent: "center", alignItems: "start", flexFlow: "row nowrap" }}>
                                <div style={{ width: this.PROGRESS_WIDTH+2, height: 12, marginLeft: 8, marginRight: 8, background: "#181818", }}>
                                    <div style={{ height:  8, width: this.PROGRESS_WIDTH,  overflow: "hidden", position: "absolute", 
                                        margin: "1px 1px 1px 1px", background: "#282828" }}>
                                        <div ref={this.progressRef} style={{ height:  10, width: this.PROGRESS_WIDTH,   position: "absolute", marginTop: 0, background: "#0C0" }} />
                                        <div style={{ height:  10, width: this.PROGRESS_WIDTH,   position: "absolute", 
                                            boxShadow: "inset 0px 2px 4px #000D", background: "transparent"
                                          }} />
                                    </div>
                                </div>

                            </div>
                            <div className={classes.editSectionNoContent}>

                            </div>

                        </div >

                    );

                } else if (control.isDbVu()) {
                    item_width = undefined;
                    let redLevel = this.dbVuMap(0);
                    let yellowLevel = this.dbVuMap(-10);
                    return (
                        <div className={classes.controlFrame} 
                            style={{ width: item_width }}>
                            {/* TITLE SECTION */}
                            <div className={classes.titleSection} 
                                style={{ width: "100%"  }}>
                                <ControlTooltip uiControl={control}>
                                    <Typography  variant="caption" display="block" style={{
                                        width: "100%",
                                        textAlign: "center"
                                    }}> {control.name === "" ? "\u00A0" : control.name}</Typography>
                                </ControlTooltip>
                            </div>
                            {/* CONTROL SECTION */}

                            <div className={classes.midSection} 
                                style={{ flex: "1 1 1", display: "flex", justifyContent: "center", alignItems: "start", flexFlow: "row nowrap" }}>
                                <div style={{ width: 8, height: this.DB_VU_HEIGHT + 4, background: "#000", }}>
                                    <div style={{ height: this.DB_VU_HEIGHT, width: 4, overflow: "hidden", position: "absolute", margin: 2 }}>
                                        <div style={{ width: 4, height: redLevel, position: "absolute", marginTop: 0, background: "#F00" }} />
                                        <div style={{ width: 4, height: (yellowLevel - redLevel), position: "absolute", marginTop: redLevel, background: "#CC0" }} />
                                        <div style={{ width: 4, height: (this.DB_VU_HEIGHT - yellowLevel), position: "absolute", marginTop: yellowLevel, background: "#0A0" }} />

                                        <div ref={this.dbVuRef} style={{ width: 4, position: "absolute", marginTop: 0, height: this.VU_HEIGHT, background: "#000" }} />
                                        <div ref={this.dbVuTelltaleRef} style={{ width: 4, position: "absolute", marginTop: 100, height: 3, background: "#C00" }} />
                                    </div>
                                </div>

                            </div>
                            <div className={classes.editSectionNoContent}>

                            </div>

                        </div >

                    );
                }
                else if (control.isVu()) {
                    item_width = undefined;
                    return (
                        <div className={classes.controlFrame} style={{ width: item_width}}>
                            {/* TITLE SECTION */}
                            <div className={classes.titleSection} 
                                style={{ width: "100%" }}>
                                <ControlTooltip uiControl={control}>
                                    <Typography noWrap display="block"  variant="caption" style={{
                                        width: item_width,
                                        textAlign: "center"
                                    }}> {control.name === "" ? "\u00A0" : control.name}</Typography>
                                </ControlTooltip>
                            </div>
                            {/* CONTROL SECTION */}

                            <div className={classes.midSection}
                                 style={{ display: "flex", justifyContent: "center", alignItems: "start", flexFlow: "row nowrap" }}>
                                <div style={{ width: 8, height: this.VU_HEIGHT + 4, background: "#000", }}>
                                    <div style={{ height: this.VU_HEIGHT, overflow: "hidden", position: "absolute", margin: 2 }}>
                                        <div ref={this.vuRef} style={{ width: 4, height: this.VU_HEIGHT, background: "#0C0", }} />
                                    </div>
                                </div>

                            </div>

                            {/* LABEL/EDIT SECTION*, strictly a placeholder for visual alignment purposes.*/}
                            <div className={classes.editSectionNoContent}>
                            </div>
                        </div >

                    );
                } else if (control.isLamp()) {
                    item_width = undefined;
                    let attachedLamp = control.name === "" || control.name === "\u00A0";

                    let ledGradient: string;
                    if (this.props.uiControl.pipedal_ledColor.length === 0) {
                        ledGradient = (isDarkMode() ? "radial-gradient(circle at center, #F00 0, #F00 20%, #333 100%)"
                            : "radial-gradient(circle at center, #F44 0, #F44 40%, #644 100%)");
                    } else {
                        ledGradient = makeLedGradient(this.props.uiControl.pipedal_ledColor);
                    }
                    return (
                        <div className={classes.controlFrame}
                            style={{width: attachedLamp? 15: item_width}}
                            >
                            {/* TITLE SECTION */}
                            <div className={classes.titleSection} 
                                    style={{ flex: "0 0 auto", width: "100%"}}>
                                <ControlTooltip uiControl={control}>
                                    <Typography noWrap variant="caption" display="block" style={{
                                        width: "100%",
                                        textAlign: "center"
                                    }}> {control.name === "" ? "\u00A0" : (control.name)}</Typography>
                                </ControlTooltip>
                            </div>
                            {/* CONTROL SECTION */}

                            <div className={classes.midSection} 
                                style={{
                                    display: "flex", justifyContent: "center", alignItems: "center", flexFlow: "row nowrap",
                                }}>
                                <div style={{
                                    width: 12, height: 12, background: isDarkMode() ? "#111" :
                                        "#444", borderRadius: 5, position: "relative"
                                }}>
                                    <div ref={this.lampRef} style={{
                                        width: 8, height: 8,
                                        background: ledGradient,
                                        opacity: 0, borderRadius: 3, margin: 2, position: "absolute"
                                    }} />
                                </div>

                            </div>

                            {/* LABEL/EDIT SECTION*, strictly a placeholder for visual alignment purposes.*/}
                            <div className={classes.editSectionNoContent}>
                            </div>
                        </div >
                    );
                } if (control.isOutputText()) {
                    return (
                        <div className={classes.controlFrame} 
                                style={{ display: "flex", flexDirection: "column", width: item_width }}>
                            {/* TITLE SECTION */}
                            <div className={classes.titleSection} 
                                style={{ flex: "0 0 auto", width: "100%" }}>
                                <ControlTooltip uiControl={control}>
                                    <Typography noWrap variant="caption" display="block" style={{
                                        width: "100%",
                                        textAlign: "left"
                                    }}> {control.name === "" ? "\u00A0": control.name}</Typography>
                                </ControlTooltip>
                            </div>

                            {/* CONTROL SECTION */}

                            <div className={classes.midSection} style={{
                                display: "flex", justifyContent: "center", 
                                flexFlow: "row nowrap", paddingTop: 6

                            }}>
                                <Typography variant="caption" display="block" noWrap style={{ width: "100%" }}>
                                    {text}
                                </Typography>
                            </div>

                            {/* LABEL/EDIT SECTION*, strictly a placeholder for visual alignment purposes.*/}
                            <div className={classes.editSectionNoContent}>
                            </div>
                        </div >
                    );

                } else {
                    return (
                        <div className={classes.controlFrame} 
                            style={{ display: "flex", flexDirection: "column", width: item_width  }}>
                            {/* TITLE SECTION */}
                            <div className={classes.titleSection} 
                                style={{ flex: "0 0 auto", width: "100%" }}>
                                <ControlTooltip uiControl={control}>
                                    <Typography noWrap variant="caption" display="block" style={{
                                        width: "100%",
                                        textAlign: "left"
                                    }}> {control.name}</Typography>
                                </ControlTooltip>
                            </div>
                            {/* CONTROL SECTION */}

                            <div className={classes.midSection} 
                                style={{ display: "flex", justifyContent: "start", alignItems: "center", flexFlow: "row nowrap" }}>
                                <Typography noWrap variant="caption" display="block" style={{ width: "100%" }}>
                                    {text}
                                </Typography>
                            </div>

                            {/* LABEL/EDIT SECTION*, strictly a placeholder for visual alignment purposes.*/}
                            <div className={classes.editSectionNoContent}>
                            </div>
                        </div >
                    );
                }
            }
        },
        styles
    );

export default PluginOutputControl;