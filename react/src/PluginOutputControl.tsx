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
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import { UiControl } from './Lv2Plugin';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, MonitorPortHandle,State } from './PiPedalModel';
import {isDarkMode} from './DarkMode';





const styles = (theme: Theme) => createStyles({
    frame: {
        position: "relative",
        margin: "12px"
    },
    switchTrack: {
        height: '100%',
        width: '100%',
        borderRadius: 14 / 2,
        zIndex: -1,
        transition: theme.transitions.create(['opacity', 'background-color'], {
            duration: theme.transitions.duration.shortest
        }),
        backgroundColor: theme.palette.secondary.main,
        opacity: theme.palette.mode === 'light' ? 0.38 : 0.3
    },
    displayValue: {
        position: "absolute",
        top: 0,
        left: 0,
        right: 0,
        bottom: 4,
        textAlign: "center",
        background: "white",
        color: "#666",
        // zIndex: -1,
    }
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
    withStyles(styles, { withTheme: true })(
        class extends Component<PluginOutputControlProps, PluginOutputControlState>
        {

            private model: PiPedalModel;
            private vuRef: React.RefObject<HTMLDivElement>;
            private lampRef: React.RefObject<HTMLDivElement>;


            constructor(props: PluginOutputControlProps) {
                super(props);
                this.vuRef = React.createRef<HTMLDivElement>();
                this.lampRef = React.createRef<HTMLDivElement>();
                this.state = {
                    hasValue: false,
                    value: 0
                };
                this.model = PiPedalModelFactory.getInstance();
                this.onConnectionStateChanged = this.onConnectionStateChanged.bind(this);
            }

            private onConnectionStateChanged(state: State)
            {
                if (state === State.Ready)
                {
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
                if (prevProps.instanceId !== this.props.instanceId)
                {
                    this.unsubscribe();
                    this.subscribe();
                }
            }

            private VU_HEIGHT = 48-2;
            private animationHandle: number | undefined = undefined;
            
            private updateValue(value: number) {
                if (this.lampRef.current)
                {
                    let control = this.props.uiControl;
                    let range = (value-control.min_value)/(control.max_value-control.min_value);
                    this.lampRef.current.style.opacity = range +"";
                }
                if (this.vuRef.current) {
                    let control = this.props.uiControl;
                    let range = (value-control.min_value)/(control.max_value-control.min_value);
                    let top = this.VU_HEIGHT-range*this.VU_HEIGHT;

                    if (!this.animationHandle)
                    {
                        this.animationHandle = requestAnimationFrame(
                            ()=> {
                                if (this.vuRef.current)
                                {
                                    this.vuRef.current.style.marginTop = top+"px";
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

            isShortSelect(control: UiControl) {
                for (let scale_point of control.scale_points) {
                    if (scale_point.label.length > 12) return false;
                }
                return true;
            }

            formatDisplayValue(uiControl: UiControl | undefined, value: number): string {
                if (!uiControl) return "";

                return uiControl.formatDisplayValue(value);
            }

            render() {

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

                let isSelect = control.isSelect();

                let item_width: number | undefined = isSelect ? 160 : 80;
                if (isSelect) {
                    if (this.isShortSelect(control)) {
                        item_width = 80;
                    }
                }
                if (control.isDial()) {
                    item_width = undefined;
                    return (
                        <div style={{ display: "flex", flexDirection: "column", width: item_width, margin: 8, height: 98 }}>
                            {/* TITLE SECTION */}
                            <div style={{ flex: "0 0 auto", width: "100%", marginBottom: 8, marginLeft: 0, marginRight: 0 }}>
                                <Typography variant="caption" display="block" style={{
                                    width: "100%",
                                    textAlign: "center"
                                }}> {control.name}</Typography>
                            </div>
                            {/* CONTROL SECTION */}

                            <div style={{ flex: "1 1 auto", display: "flex", justifyContent: "center", alignItems: "center", flexFlow: "row nowrap" }}>
                                <div style={{ width: 6, height: 48, background: "#000",  }}>
                                    <div style={{ height: 46, overflow: "hidden", position: "absolute", margin: 1 }}>
                                        <div ref={this.vuRef} style={{ width: 4, height: 48, background: "#0C0", }} />
                                    </div>
                                </div>

                            </div>

                            {/* LABEL/EDIT SECTION*, strictly a placeholder for visual alignment purposes.*/}
                            <div style={{ flex: "0 0 auto", position: "relative", width: 30, height: 27 }}>
                                &nbsp;
                            </div>
                        </div >

                    );
                } else if (control.toggled_property && control.scale_points.length === 0) {
                    item_width = undefined;
                    let attachedLamp = control.name === "" || control.name === "\u00A0" ;
                    return (
                        <div style={{ display: "flex", flexDirection: "column", width: item_width, 
                                marginTop: 8, marginBottom: 8, marginRight: 8,marginLeft: (attachedLamp? 0: 8), height: 98 
                            }}>
                            {/* TITLE SECTION */}
                            <div style={{ flex: "0 0 auto", width: "100%", marginBottom: 8, marginLeft: 0, marginRight: 0 }}>
                                <Typography variant="caption" display="block" style={{
                                    width: "100%",
                                    textAlign: "center"
                                }}> {control.name === "" ? "\u00A0": control.name}</Typography>
                            </div>
                            {/* CONTROL SECTION */}

                            <div style={{ flex: "1 1 auto", display: "flex", justifyContent: "left", alignItems: "start", flexFlow: "row nowrap" }}>
                                <div style={{ width: 12, height: 12, background: isDarkMode()? "#111" : "#444", borderRadius: 5, position: "relative" }}>
                                    <div ref={this.lampRef} style={{ width: 8, height: 8, 
                                        background: (isDarkMode()? "radial-gradient(circle at center, #F00 0, #F00 20%, #333 100%)"
                                                       : "radial-gradient(circle at center, #F44 0, #F44 40%, #644 100%)"),
                                        opacity: 0, borderRadius: 3,margin: 2,position: "absolute" }} />
                                </div>

                            </div>

                            {/* LABEL/EDIT SECTION*, strictly a placeholder for visual alignment purposes.*/}
                            <div style={{ flex: "0 0 auto", position: "relative", width: 30, height: 27 }}>
                                &nbsp;
                            </div>
                        </div >
                    );
                } else {
                    return (
                        <div style={{ display: "flex", flexDirection: "column", width: item_width, margin: 8, height: 98 }}>
                            {/* TITLE SECTION */}
                            <div style={{ flex: "0 0 auto", width: "100%", marginBottom: 8, marginLeft: 0, marginRight: 0 }}>
                                <Typography variant="caption" display="block" style={{
                                    width: "100%",
                                    textAlign: "left"
                                }}> {control.name}</Typography>
                            </div>
                            {/* CONTROL SECTION */}

                            <div style={{ flex: "1 1 auto", display: "flex", justifyContent: "start", alignItems: "center", flexFlow: "row nowrap" }}>
                                <Typography variant="caption" display="block" noWrap style={{ width: "100%" }}>
                                    {text}
                                </Typography>
                            </div>

                            {/* LABEL/EDIT SECTION*, strictly a placeholder for visual alignment purposes.*/}
                            <div style={{ flex: "0 0 auto", position: "relative", width: 40, height: 27 }}>
                                &nbsp;
                            </div>
                        </div >
                    );
                }
            }
        }
    );

export default PluginOutputControl;