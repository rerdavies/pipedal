// Copyright (c) 2025 Robin Davies
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

import ToobFrequencyResponseView, { FREQUENCY_RESPONSE_VECTOR_URI } from './ToobFrequencyResponseView';
import WithStyles from './WithStyles';
import { createStyles } from './WithStyles';
import SvgPathBuilder from './SvgPathBuilder';
import CircleUpIcon from './svg/expand_circle_up_24dp.svg?react';
import CircleDownIcon from './svg/expand_circle_down_24dp.svg?react';
import { withStyles } from "tss-react/mui";
import DialogEx from './DialogEx';
import DialogContent from '@mui/material/DialogContent';

import IControlViewFactory from './IControlViewFactory';
import { PiPedalModelFactory, PiPedalModel, State } from "./PiPedalModel";
import { PedalboardItem } from './Pedalboard';
import PluginControlView, { ICustomizationHost, ControlGroup, ControlViewCustomization } from './PluginControlView';
import { UiPlugin } from './Lv2Plugin';
import Typography from '@mui/material/Typography';
import IconButton from '@mui/material/IconButton/IconButton';
import { css } from '@emotion/react';




const styles = (theme: Theme) => createStyles({
    menuIcon: css({
        fill: (theme.palette.text.primary + "!important"), //theme.palette.text.primary,
        opacity: 0.6
    }),
    upIcon: css({
        fill: (theme.palette.text.primary + "!important"), //theme.palette.text.primary,
        opacity: 0.6,
        alignSelf: "flex-start"
    }),
});

interface Point { x: number, y: number };


function offsetFromEnd(pt0: Point, pt1: Point, distance: number) {
    let dx = pt1.x - pt0.x;
    let dy = pt1.y - pt0.y;
    let mag = Math.sqrt(dx * dx + dy * dy);
    let normX = dx / mag;
    let normY = dy / mag;
    return {
        x: pt1.x - normX * distance,
        y: pt1.y - normY * distance
    }
}


const getWindowSize = () => {
    return {
        width: document.documentElement.clientWidth,
        height: document.documentElement.clientHeight,
    };
};


interface UnpackedControls {
    eq: React.ReactNode;
    filters_loCut: React.ReactNode;
    filters_hiCut: React.ReactNode;
    low_level: React.ReactNode;
    low_freq: React.ReactNode;
    lmf_level: React.ReactNode;
    lmf_freq: React.ReactNode;
    lmf_q: React.ReactNode;
    hmf_level: React.ReactNode;
    hmf_freq: React.ReactNode;
    hmf_q: React.ReactNode;
    hi_level: React.ReactNode;
    hi_freq: React.ReactNode;
    gain: React.ReactNode;
}

interface ToobParametricEqViewProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;

}
interface ToobParametricEqViewState {
    windowSize: { width: number, height: number };
    maximized: boolean;

}

const ToobParametricEqView =
    withStyles(
        class extends React.Component<ToobParametricEqViewProps, ToobParametricEqViewState>
            implements ControlViewCustomization {
            model: PiPedalModel;

            customizationId: number = 943;

            constructor(props: ToobParametricEqViewProps) {
                super(props);

                this.handleResize = this.handleResize.bind(this);
                this.handleConnectionStateChanged = this.handleConnectionStateChanged.bind(this);

                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    windowSize: getWindowSize(),
                    maximized: false
                }
                let pluginInfo: UiPlugin | null = this.model.getUiPlugin(this.props.item.uri);
                if (pluginInfo === null) {
                    throw new Error("Plugin not fouund.");
                }
            }

            fullScreen() {
                return true;
            }

            horizontalScrollControls(host: ICustomizationHost, controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[] {
                // Implement horizontal scrolling behavior for tiny horizontal screens
                controls[0] = (
                    <div id="fixedGraph_xxx" key="fixedGraph" style={{ position: "absolute", top: 0, left: 0, padding: 5, background: "#F88" }}>
                        {controls[0] as React.ReactNode}
                    </div>
                );
                return controls;
            }

            unpackControls(host: ICustomizationHost, controls: (React.ReactNode | ControlGroup)[]): UnpackedControls {
                return {
                    eq: controls[0] as React.ReactNode,
                    filters_loCut: (controls[1] as ControlGroup).controls[0],
                    filters_hiCut: (controls[1] as ControlGroup).controls[1],
                    low_level: (controls[2] as ControlGroup).controls[0],
                    low_freq: (controls[2] as ControlGroup).controls[1],
                    lmf_level: (controls[3] as ControlGroup).controls[0],
                    lmf_freq: (controls[3] as ControlGroup).controls[1],
                    lmf_q: (controls[3] as ControlGroup).controls[2],
                    hmf_level: (controls[4] as ControlGroup).controls[0],
                    hmf_freq: (controls[4] as ControlGroup).controls[1],
                    hmf_q: (controls[4] as ControlGroup).controls[2],
                    hi_level: (controls[5] as ControlGroup).controls[0],
                    hi_freq: (controls[5] as ControlGroup).controls[1],
                    gain: controls[6] as React.ReactNode
                };
            }
            absolutePosition(x: number, y: number, el: React.ReactNode) {
                return (<div style={{ position: "absolute", left: x, top: y }}>{el}</div>);
            }
            private ROW_HEIGHT = 140;
            private BORDER_HEIGHT = 350;
            private DIAL_WIDTH = 63;
            private GRAPH_WIDTH = this.DIAL_WIDTH * 2;

            private smoothPath(pt0: Point, pt1: Point, pt2: Point, pt3: Point, curveRadius: number): string {
                let pathBuilder = new SvgPathBuilder();


                let ctl01 = offsetFromEnd(pt0, pt1, curveRadius);
                let ctl02 = offsetFromEnd(pt0, pt1, curveRadius / 3);
                let ctl03 = offsetFromEnd(pt2, pt1, curveRadius / 3);
                let ctl04 = offsetFromEnd(pt2, pt1, curveRadius);

                let ctl11 = offsetFromEnd(pt1, pt2, curveRadius);
                let ctl12 = offsetFromEnd(pt1, pt2, curveRadius / 3);
                let ctl13 = offsetFromEnd(pt3, pt2, curveRadius / 3);
                let ctl14 = offsetFromEnd(pt3, pt2, curveRadius);

                pathBuilder.moveTo(pt0.x, pt0.y);
                pathBuilder.lineTo(ctl01.x, ctl01.y);
                pathBuilder.bezierTo(
                    ctl02.x, ctl02.y,
                    ctl03.x, ctl03.y,
                    ctl04.x, ctl04.y
                );
                pathBuilder.lineTo(ctl11.x, ctl11.y);
                pathBuilder.bezierTo(
                    ctl12.x, ctl12.y,
                    ctl13.x, ctl13.y,
                    ctl14.x, ctl14.y
                );
                pathBuilder.lineTo(pt3.x, pt3.y);
                return pathBuilder.toString();
            }
            horizontalCompactControls(host: ICustomizationHost, controls: UnpackedControls) {
                let height = this.BORDER_HEIGHT;
                let row0 = height / 2 - this.ROW_HEIGHT;
                let row1 = row0 + this.ROW_HEIGHT;

                let borderTop = 0;
                let borderBottom = height - 8;

                let labelTop = row0 - 30;
                let labelBottom = row1 + this.ROW_HEIGHT - 5;

                let colPos = (group: number, col: number) => this.DIAL_WIDTH * (0.3 * group + col);
                let cpos0 = (group: number, col: number, el: React.ReactNode) => this.absolutePosition(colPos(group, col), row0, el);
                let cpos1 = (group: number, col: number, el: React.ReactNode) => this.absolutePosition(colPos(group, col) + this.DIAL_WIDTH / 2, row1, el);


                let label1 = (y: number, group: number, col0: number, col1: number, text: string, align: "left" | "center" | "right") => {
                    let left = colPos(group, col0);
                    let right = colPos(group, col1);
                    return (
                        <Typography variant="body1" noWrap style={{
                            fontSize: "0.85em", fontWeight: 700,
                            top: y, paddingLeft: 0, paddingRight: 0,
                            position: "absolute", left: left + 8, width: right - left,
                            opacity: 0.4, textAlign: align
                        }}>{text}</Typography>
                    );
                };
                let divider = (group: number, col0: number, y0: number, col1: number, y1: number) => {
                    let x0 = colPos(group, col0);
                    let x1 = colPos(group, col1);
                    return (
                        <svg width={x1 - x0 + 4} height={y1 - y0 + 4} style={{ position: "absolute", left: x0 - 2, top: y0 - 2 }}>
                            <line x1={2} y1={2} x2={2 + x1 - x0} y2={2 + y1 - y0} stroke="#888" strokeWidth={4} />
                        </svg>
                    );
                };
                let leftDivider = (group: number, col0: number, y0: number, col1: number, y1: number) => {
                    let x0 = colPos(group, col0);
                    let x1 = colPos(group, col1);
                    let xOrg = Math.min(x0, x1) - 2;
                    let yOrg = Math.min(y0, y1) - 2;
                    let width = Math.abs(x1 - x0) + 4;
                    let height = Math.abs(y1 - y0) + 4;

                    let bezierLength = 10;
                    let pt0 = { x: x0, y: y0 };
                    let pt1 = { x: x0, y: row1 - 18 };
                    let pt2 = { x: x1, y: row1 + 3 };
                    let pt3 = { x: x1, y: y1 };

                    let path = this.smoothPath(pt0, pt1, pt2, pt3, bezierLength);

                    return (
                        <svg width={width} height={height} style={{ position: "absolute", left: xOrg - 1, top: yOrg }}
                            viewBox={`${xOrg} ${yOrg} ${width} ${height}`}
                        >
                            <path d={path} fill="none" stroke="#888" strokeWidth="4px" />
                        </svg>
                    );
                };
                let graph = (
                    <ToobFrequencyResponseView width={this.GRAPH_WIDTH}
                        propertyName={FREQUENCY_RESPONSE_VECTOR_URI}
                        instanceId={this.props.instanceId} />
                );

                return [(<div style={{
                    width: "100%", height: "100%",
                    position: "relative", display: "flex", flexFlow: "column nowrap",
                    alignItems: "center", justifyContent: "center"
                }}>
                    <div style={{ flex: "1 1 0px", }} />
                    <div style={{
                        flex: "0 0 auto", border: "4px #888 solid", borderRadius: "8px 8px 8px 8px", position: "relative",
                        width: colPos(6, 8.5), height: height,
                    }}>
                        {label1(labelBottom, 1, 2, 3.0, "Lo Shelf", "center")}
                        {label1(labelTop, 2, 3, 5, "Low Mid", "center")}
                        {label1(labelBottom, 3, 4.5, 6.5, "High Mid", "center")}
                        {label1(labelTop, 4, 6.5, 7.5, "Hi Shelf", "center")}
                        {divider(1, 2.0, borderTop, 2.0, borderBottom)}
                        {divider(5, 7.5, borderTop, 7.5, borderBottom)}
                        {leftDivider(2, 3.0, borderTop, 3.0, borderBottom)}
                        {leftDivider(3, 5.0, borderTop, 4.5, borderBottom)}
                        {leftDivider(4, 6.5, borderTop, 6.5, borderBottom)}

                        {this.absolutePosition(0, row0, graph)}
                        {this.absolutePosition(colPos(0, 0), row1, controls.filters_loCut)}
                        {this.absolutePosition(colPos(0, 1), row1, controls.filters_hiCut)}

                        {cpos0(1, 2, controls.low_level)}
                        {cpos1(1, 2 - 0.5, controls.low_freq)}

                        {cpos0(2, 3.0, controls.lmf_q)}
                        {cpos1(2, 2.75, controls.lmf_freq)}
                        {cpos0(2, 4, controls.lmf_level)}

                        {cpos1(3, 4, controls.hmf_freq)}
                        {cpos0(3, 5.25, controls.hmf_level)}
                        {cpos1(3, 5, controls.hmf_q)}

                        {cpos0(4, 6.5, controls.hi_level)}
                        {cpos1(4, 6, controls.hi_freq)}

                        {cpos0(5, 7.5, controls.gain)}
                    </div>
                    <div style={{ flex: "5 5 0px", }} />

                </div>)];
            }
            verticalCompactControls(host: ICustomizationHost, controls: UnpackedControls) {
                let width = 380;

                let graph = (
                    <ToobFrequencyResponseView width={344}
                        propertyName={FREQUENCY_RESPONSE_VECTOR_URI}
                        instanceId={this.props.instanceId} />
                );
                let divider = () => {
                    return (
                        <div style={{ width: "100%", height: 1, background: "#8888" }} />
                    );
                }
                let vDivider = () => {
                    return (
                        <div style={{ width: 1, minWidth: 1, height: 164, background: "#888" }} />
                    );
                }
                let panel = (label: string, controls: React.ReactNode[]) => {
                    return (
                        <div style={{ display: "flex", flexFlow: "column nowrap", alignItems: "center", marginRight: 8, marginLeft: 8 }}>
                            <div style={{
                                width: controls.length * this.DIAL_WIDTH, height: 130, position: "relative",
                                overflow: "hidden", marginRight: 8
                            }}>
                                {controls.map((ctl, i) => {
                                    return (
                                        <div key={i} style={{ width: this.DIAL_WIDTH, position: "absolute", left: i * this.DIAL_WIDTH, top: 8 }}>
                                            {ctl}
                                        </div>
                                    );
                                })}
                            </div>
                            <Typography variant="body1" noWrap style={{
                                fontSize: "0.85em", fontWeight: 700, width: label !== "" ? 100 : 5, paddingTop: 4, paddingBottom: 8,
                                opacity: 0.4, textAlign: "center"
                            }}>{label}</Typography>
                        </div>
                    );
                }

                return [(<div key="tpq_cv" style={{
                    width: "100%", height: "100%",
                    position: "relative", display: "flex", flexFlow: "column nowrap",
                    alignItems: "center", justifyContent: "center"
                }}>
                    <div style={{ flex: "1 1 0px" }} />
                    <div style={{
                        display: "flex", flexFlow: "row wrap", alignItems: "center", width: width, justifyContent: "space-between",
                        flex: "0 0 auto", border: "4px #888 solid", borderRadius: "8px 8px 8px 8px", position: "relative",
                    }}>

                        {graph}
                        {(<div style={{ width: "100%", height: 16 }} />)}
                        {panel("", [
                            controls.filters_loCut,
                            controls.filters_hiCut,
                        ])}
                        {panel("", [
                            controls.gain
                        ])}
                        {divider()}
                        {panel("Low",
                            [controls.low_level,
                            controls.low_freq
                            ])
                        }
                        {vDivider()}
                        {panel("Low Mid",

                            [
                                controls.lmf_level,
                                controls.lmf_freq,
                                controls.lmf_q,
                            ])
                        }
                        {divider()}
                        {panel("High Mid",

                            [
                                controls.hmf_level,
                                controls.hmf_freq,
                                controls.hmf_q,
                            ])
                        }
                        {vDivider()}
                        {panel("High",
                            [controls.hi_level,
                            controls.hi_freq
                            ])
                        }
                    </div>
                    <div style={{ flex: "5 5 0px", }} />

                </div>)];
            }

            tinyControls(host: ICustomizationHost, controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[] {
                const classes = withStyles.getClasses(this.props);
                let isLandscape = host.isLandscapeGrid();
                let panelControls: React.ReactNode[] = [];

                let filterGroup = controls[1] as ControlGroup;
                let miscGroup: ControlGroup = {
                    name: "",
                    indexes: [],
                    controls: [
                        filterGroup.controls[0],
                        filterGroup.controls[1],
                        controls[controls.length - 1] as React.ReactNode
                    ]
                };
                panelControls.push(host.renderControlGroup(miscGroup, "vtc_miscGroup"));
                if (!isLandscape) {
                    panelControls.push(
                        (
                            <div key="ctlx_div" style={{ width: "100%", height: 0 }} />
                        )
                    );
                }

                for (let i = 2; i < controls.length - 1; ++i) {
                    let c = controls[i];
                    if (c instanceof ControlGroup) {
                        panelControls.push(
                            host.
                                renderControlGroup(c as ControlGroup, "vtc_cg" + i)
                        );
                    } else {
                        panelControls.push(c as React.ReactNode);
                    }
                }
                let panelStyle: React.CSSProperties;
                let scrollStyle: React.CSSProperties;
                let frameStyle: React.CSSProperties;

                if (isLandscape) {
                    panelStyle = { display: "flex", flexDirection: "row", alignItems: "stretch", width: "100%" };
                    scrollStyle = { flex: "1 1 auto", overflowX: "auto", overflowY: "hidden", whiteSpace: "nowrap" };
                    frameStyle = { display: "flex", flexFlow: "row nowrap", alignItems: "center", paddingTop: 8 };
                } else {
                    panelStyle = { display: "flex", flexDirection: "column", alignItems: "stretch", height: "100%" };
                    scrollStyle = { flex: "1 1 auto", overflowX: "hidden", overflowY: "auto", whiteSpace: "nowrap" };
                    frameStyle = { display: "flex", flexFlow: "row wrap", alignItems: "left", justifyContent: "left", gap: 12, paddingTop: 24, paddingBottom: 48 };
                }

                return [(
                    <div key={"vtc_container"} style={panelStyle}>
                        {host.isLandscapeGrid() ? (
                            <div key={"vtc_panel"} style={{ flex: " 0 0 auto", paddingBottom: 8, display: "flex", alignItems: "center", flexFlow: "row nowrap" }}>
                                <div style={{ alignSelf: "flex-start", marginTop: 8 }}>
                                    <IconButton 
                                        style={{marginTop: 24}}
                                        onClick={(e) => {
                                        e.stopPropagation();
                                        this.setState({ maximized: true });
                                    }}>
                                        <CircleUpIcon className={classes.upIcon} />
                                    </IconButton>
                                </div>
                                {controls[0] as React.ReactNode}
                            </div>
                        ) : (
                            <div key={"vtc_panel"} style={{ flex: " 0 0 auto", paddingBottom: 8, display: "flex", alignItems: "center", flexFlow: "row nowrap" }}>
                                {controls[0] as React.ReactNode}
                                <div style={{ alignSelf: "flex-start", marginTop: 8 }}>
                                    <IconButton onClick={(e) => {
                                        e.stopPropagation();
                                        this.setState({ maximized: true });
                                    }}>
                                        <CircleUpIcon className={classes.upIcon} />
                                    </IconButton>
                                </div>
                            </div>
                        )}
                        <div style={scrollStyle}>
                            <div style={frameStyle}>
                                {panelControls}
                            </div>
                        </div>
                    </div>
                )];
            }
            private dialogRef: HTMLDivElement | null = null;
            private panelRef: HTMLDivElement | null = null;

            dlgTickInAnimation(yOffset: number, startTime: number) {
                let elapsed = Date.now() - startTime;
                let pct = elapsed / 200;
                if (pct > 1) pct = 1;
                if (!this.dialogRef) {
                    return;
                }
                this.dialogRef.style.opacity = `${pct}`;
                this.dialogRef.style.transform = `translateY(${(1 - pct) * yOffset}px)`;
                if (pct >= 1) {
                    return;
                }
                window.requestAnimationFrame(() => {
                    this.dlgTickInAnimation(yOffset, startTime);
                });
            }
            dlgTickOutAnimation(yOffset: number, startTime: number) {
                if (!this.dialogRef || !this.panelRef) {
                    this.setState({ maximized: false });
                    return;
                }
                let elapsed = Date.now() - startTime;
                let pct = elapsed / 200;
                if (pct > 1) pct = 1;
                if (!this.dialogRef) {
                    return;
                }
                this.dialogRef.style.transform = `translateY(${(pct) * yOffset}px)`;
                this.dialogRef.style.opacity = `${1 - pct}`;
                if (pct >= 1) {
                    this.setState({ maximized: false });
                    return;
                }
                window.requestAnimationFrame(() => {
                    this.dlgTickOutAnimation(yOffset, startTime);
                });
            }
            dlgSlideInAnimation(yOffset: number) {
                if (!this.animateDialog) {
                    return;
                }
                this.animateDialog = false;
                window.requestAnimationFrame(() => {
                    if (this.dialogRef) {
                        let startTime = Date.now();
                        this.dlgTickInAnimation(yOffset, startTime);
                    }
                });
            }
            dlgSlideOutAnimation(yOffset: number) {
                if (!this.animateDialog) {
                    return;
                }
                this.animateDialog = false;

                window.requestAnimationFrame(() => {
                    if (this.dialogRef) {
                        let startTime = Date.now();
                        this.dlgTickInAnimation(yOffset, startTime);
                    }
                });
            }
            startSlideOutAnimation() {
                if (this.panelRef && this.dialogRef) {
                    let yOffset = this.panelRef.getBoundingClientRect().top;

                    window.requestAnimationFrame(() => {
                        if (this.dialogRef && this.dialogRef) {
                            let startTime = Date.now();
                            this.dlgTickOutAnimation(yOffset, startTime);
                        }
                    });
                } else {
                    this.setState({ maximized: false });
                }
            }
            checkTransitionStart() {
                if (this.dialogRef && this.panelRef) {
                    let panelRect = this.panelRef.getBoundingClientRect();
                    this.dlgSlideInAnimation(panelRect.top);
                }
            }
            setDialogRef(ref: HTMLDivElement | null) {
                this.dialogRef = ref;
                this.checkTransitionStart();
            };
            setPanelRef(ref: HTMLDivElement | null) {
                this.panelRef = ref;
                this.checkTransitionStart();
            }
            modifyControls(host: ICustomizationHost, controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[] {
                const classes = withStyles.getClasses(this.props);
                let unpackedControls = this.unpackControls(host, controls);

                if (this.state.maximized) {
                    return [(
                        <div ref={(instance: HTMLDivElement | null): void => { this.setPanelRef(instance); }}
                            style={{ position: "absolute", top: 0, left: 0, right: 0, bottom: 0 }}>
                            <DialogEx key="peq_dialog"
                                open={this.state.maximized}
                                onEnterKey={() => this.setState({ maximized: false })}
                                onClose={() => this.setState({ maximized: false })}
                                fullScreen={true}
                                tag="peq_dialog"

                                dlgRef={(instance: HTMLDivElement | null): void => {
                                    this.setDialogRef(instance);
                                }}
                            >

                                <DialogContent>
                                    {this.state.windowSize.width > this.state.windowSize.height ? (
                                        <div style={{
                                            width: "100%", height: "100%",
                                            display: "flex", flexFlow: "row nowrap"
                                        }}
                                        >
                                            <div>
                                                <IconButton onClick={(e) => {
                                                    e.stopPropagation();
                                                    this.startSlideOutAnimation();
                                                }} style={{ paddingTop: 48, paddingLeft: 8, paddingRight: 8 }}>
                                                    <CircleDownIcon className={classes.menuIcon} />
                                                </IconButton>
                                            </div>
                                            <div style={{ flex: "1 1 auto", height: "100%", position: "relative" }}>
                                                {this.horizontalCompactControls(host, unpackedControls)}
                                            </div>
                                        </div>

                                    ) : (
                                        <div style={{
                                            width: "100%", height: "100%",
                                            display: "flex", flexFlow: "column nowrap"
                                        }}
                                        >
                                            <div>
                                                <IconButton onClick={(e) => {
                                                    e.stopPropagation();
                                                    this.startSlideOutAnimation();
                                                }} style={{ paddingLeft: 8, paddingTop: 8, paddingBottom: 8 }}>
                                                    <CircleDownIcon className={classes.menuIcon} />
                                                </IconButton>
                                            </div>
                                            <div style={{ flex: "1 1 auto", width: "100%", position: "relative" }}>
                                                {this.verticalCompactControls(host, unpackedControls)}
                                            </div>
                                        </div>

                                    )
                                    }
                                </DialogContent>
                            </DialogEx>
                        </div>

                    )];
                }


                if (this.state.windowSize.width > 750 && this.state.windowSize.height > 600) {
                    return this.horizontalCompactControls(host, unpackedControls);
                }
                return this.tinyControls(host, controls);
            }

            private handleConnectionStateChanged(state: State) {
                if (state === State.Ready) {
                    this.unsubscribe();
                    this.subscribe();
                }
            }

            private handleResize() {
                this.setState({ windowSize: getWindowSize() });
            }

            subscribe() {
                this.subscribedId = this.props.instanceId;
            }
            unsubscribe() {
            }

            componentDidMount() {
                if (super.componentDidMount) {
                    super.componentDidMount();
                }
                this.subscribe();
                this.model.state.addOnChangedHandler(this.handleConnectionStateChanged);
                window.addEventListener("resize", this.handleResize);

            }
            componentWillUnmount() {
                this.unsubscribe();
                if (super.componentWillUnmount) {
                    super.componentWillUnmount();
                }
                this.model.state.removeOnChangedHandler(this.handleConnectionStateChanged);
                window.removeEventListener("resize", this.handleResize);

            }

            private subscribedId: number | null = null;
            private animateDialog: boolean = false;
            componentDidUpdate(oldProps: ToobParametricEqViewProps, oldState: ToobParametricEqViewState) {
                if (this.props.instanceId !== this.subscribedId) {
                    this.unsubscribe();
                    this.subscribe();
                }
                this.animateDialog = this.state.maximized != oldState.maximized;

            }


            render() {
                return (<PluginControlView
                    instanceId={this.props.instanceId}
                    item={this.props.item}
                    customization={this}
                    customizationId={this.customizationId}
                    showModGui={false}
                    onSetShowModGui={(instanceId: number, showModGui: boolean) => { }}

                />);
            }
        },
        styles
    );



class ToobParametricEqViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-parametric-eq";

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<ToobParametricEqView instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }


}
export default ToobParametricEqViewFactory;