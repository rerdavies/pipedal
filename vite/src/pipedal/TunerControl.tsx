// Copyright (c) Robin E.R. Davies
// Copyright (c) Fulgencio Ruiz Rubio.
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

import React, {Component, createRef} from 'react';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";
import { MonitorPortHandle, PiPedalModel, State, PiPedalModelFactory } from "./PiPedalModel";
import {isDarkMode} from './DarkMode';

const DEFAULT_FREQUENCY_PORT_NAME = "FREQ";

const DIAL_VIEWBOX_WIDTH = 220;
const DIAL_VIEWBOX_HEIGHT = 100;
const NEEDLE_CY = 320;
const CENTS_PER_MARK = 2;
const NEEDLE_OUTER_RADIUS = 0.98;
const TICK_OUTER_RADIUS = 0.96;
const TICK_INNER_RADIUS = 0.93;
const TICK_INNER_ZERO_RADIUS = 0.85;
const TICK_INNER_CENTER_RADIUS = 0.80;
const DIAL_ANGLE_DEGREES = 20;

const CX = DIAL_VIEWBOX_WIDTH / 2;
const NEEDLE_OUTER_Y = NEEDLE_CY - NEEDLE_OUTER_RADIUS * NEEDLE_CY;
const NEEDLE_INNER_Y = NEEDLE_CY - 0.1 * NEEDLE_CY;

function centsToColor(absCents: number): string {
    const t = Math.min(absCents / 10, 1);
    if (t <= 0.5) {
        const u = t * 2;
        const r = Math.round(76 + (249 - 76) * u);
        const g = Math.round(175 + (168 - 175) * u);
        const b = Math.round(80 + (37 - 80) * u);
        return `rgb(${r},${g},${b})`;
    } else {
        const u = (t - 0.5) * 2;
        const r = Math.round(249 + (244 - 249) * u);
        const g = Math.round(168 + (67 - 168) * u);
        const b = Math.round(37 + (54 - 37) * u);
        return `rgb(${r},${g},${b})`;
    }
}

const styles = (_theme: Theme) =>
    createStyles({});

interface PitchInfo {
    valid: boolean;
    name: string;
    fractionText: string;
    fraction: number;
    
    semitoneCents: number
}

const FLAT = "\u{266d}";
const SHARP = "#";
const HALF_FLAT = "\uD834\uDD33";
const HALF_SHARP = "\uD834\uDD32";

const NOTES_12TET: string[] = [
    "C", "C" + SHARP, "D", "E"+FLAT,"E", "F", "F" + SHARP, "G", "A"+FLAT,"A","B"+FLAT,"B"
];
const NOTES_19TET: string[] = [
    "C", "C" + SHARP, "D"+FLAT, "D", "D" + SHARP,"E"+FLAT,"E", "E" + SHARP, "F", "F" + SHARP, "G"+FLAT,"G","G" + SHARP,"A"+FLAT,"A","A" + SHARP,"B"+FLAT,"B","C"+FLAT
];
const NOTES_24TET: string[] = [
    "C","C"+HALF_SHARP, "C" + SHARP,"D"+HALF_FLAT, "D", "D"+HALF_SHARP, "E"+FLAT, "E"+HALF_FLAT, "E","E"+HALF_SHARP,"F", "F"+HALF_SHARP, "F" + SHARP, "G"+HALF_FLAT,"G","G"+HALF_SHARP, "A"+FLAT,"A"+HALF_FLAT,"A","A"+HALF_SHARP,"B"+FLAT,"B"+HALF_FLAT,"B","C"+HALF_FLAT
];

interface TunerControlProps extends WithStyles<typeof styles> {
    instanceId: number;
    valueIsMidi: boolean;
    symbolName?: string;
}
type TunerControlState = {
    pitchInfo: PitchInfo
    tet: number;
    refFrequency: number;
};

const TunerControl =
    withStyles(
        class extends Component<TunerControlProps, TunerControlState>
        {
            model: PiPedalModel;

            private needleRef: React.RefObject<SVGGElement | null>;

            private cachedSemitoneCents: number = -1;
            private cachedTickElements: React.ReactNode[] = [];

            private lastValue: number = -1;
            private currentNeedleCents: number = 0;
            private targetNeedleCents: number = 0;
            private lastValidCents: number = 0;
            private lastValidTime: number = 0;
            private animationIdle: boolean = true;
            private rafHandle?: number = undefined;

            constructor(props: TunerControlProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();

                this.needleRef = createRef<SVGGElement>();

                this.state = {
                 pitchInfo: {
                     valid: false,
                     name: "",
                     fractionText: "",
                     fraction: 0,
                     semitoneCents: 100},
                 tet: 12,
                 refFrequency: 440   

                };
                this.onFrequencyUpdated = this.onFrequencyUpdated.bind(this);
                this.onStateChanged = this.onStateChanged.bind(this);
            }

            frequencyPortName() : string {
                if (this.props.symbolName) { return this.props.symbolName;}
                return DEFAULT_FREQUENCY_PORT_NAME;
            }

            noteToPitchInfo(value: number) : PitchInfo
            {
                let aOffset = 69;

                let tet = this.state.tet;
                let refFrequency = this.state.refFrequency;
                let names: string[];
                let semitoneCents = 100;
                let valid = false;

                let hz = -1;
                let midiNote = -1;

                switch (tet)
                {
                    case 12:
                    default:
                        aOffset = 69;
                        names = NOTES_12TET;
                        semitoneCents = 100;
                        break;
                    case 19:
                        aOffset = 4*19+13;
                        names = NOTES_19TET;
                        semitoneCents = 100*12.0/19;
                        break;
                    case 24:
                        aOffset = 4*24+18;
                        semitoneCents = 50;
                        names = NOTES_24TET;
                        break;
                }
                let name = "";
                let fractionText = "";
                let fraction = 0;

                if (this.props.valueIsMidi)
                {
                    midiNote = value;
                } else {
                    hz = value;
                    if (hz < 65)
                    {
                        hz = -1;
                        midiNote = -1;
                    } else {
                        midiNote = Math.log2(hz/refFrequency)*tet + aOffset;
                    }
                }

                if (midiNote > 0)
                {
                    let note = midiNote;
                    let noteNumber = Math.round(note);

                    let octave = Math.floor((noteNumber)/tet);

                    let nameIndex = noteNumber -octave* tet;
                    if (nameIndex < 0) nameIndex += tet;
                    name = names[ nameIndex ] + (octave-1); 
                    valid = true;
                    fraction = note-noteNumber;
                    if (fraction >= 0) {
                        fractionText = "+" + fraction.toFixed(2).substring(1);
                    } else {
                        fractionText = "\u2212" + fraction.toFixed(2).substring(2);
                    }
                }
                return {
                    valid: valid,
                    name: name,
                    fraction: fraction,
                    fractionText: fractionText,
                    semitoneCents: semitoneCents
                };
            }

            onFrequencyUpdated(value: number): void
            {
                if (value === this.lastValue && value <= 0 && this.animationIdle) return;

                this.lastValue = value;

                let pitchInfo = this.noteToPitchInfo(value);
                this.setState({  
                    pitchInfo: pitchInfo
                });

                if (pitchInfo.valid) {
                    let maxCents = pitchInfo.semitoneCents/2;
                    if (maxCents > 25) maxCents = 25;
                    this.targetNeedleCents = pitchInfo.semitoneCents*pitchInfo.fraction;
                    if (this.targetNeedleCents > maxCents) this.targetNeedleCents = maxCents;
                    if (this.targetNeedleCents < -maxCents) this.targetNeedleCents = -maxCents;
                    this.lastValidCents = this.targetNeedleCents;
                    this.lastValidTime = performance.now();
                    this.animationIdle = false;
                } else {
                    this.updateDecayTarget();
                }
                this.startAnimationLoop();
            }

            private updateDecayTarget(): void {
                let maxCents = this.state.pitchInfo.semitoneCents/2;
                if (maxCents > 25) maxCents = 25;
                const NEEDLE_DECAY_RATE_PER_MS = -50*7/1000;
                let time = performance.now();
                let dt = time - this.lastValidTime;
                this.targetNeedleCents = this.lastValidCents + (dt * NEEDLE_DECAY_RATE_PER_MS);
                if (this.targetNeedleCents <= -maxCents) {
                    this.targetNeedleCents = -maxCents;
                    this.animationIdle = true;
                } else {
                    this.animationIdle = false;
                }
            }

            private setNeedleAngle(cents: number): void {
                const range = this.state.pitchInfo.semitoneCents;
                const clampedRange = range > 50 ? 50 : range;
                const angleDeg = DIAL_ANGLE_DEGREES * (cents * 2 / clampedRange);
                const g = this.needleRef.current;
                if (g) {
                    g.setAttribute('transform', `rotate(${angleDeg},${CX},${NEEDLE_CY})`);
                }
            }

            private startAnimationLoop(): void {
                if (this.rafHandle !== undefined) return;

                const LERP_FACTOR = 0.12;
                const EPSILON = 0.05;

                const animate = () => {
                    if (!this.state.pitchInfo.valid) {
                        this.updateDecayTarget();
                    }

                    let diff = this.targetNeedleCents - this.currentNeedleCents;
                    if (Math.abs(diff) < EPSILON && this.animationIdle) {
                        this.currentNeedleCents = this.targetNeedleCents;
                        this.rafHandle = undefined;
                        this.setNeedleAngle(this.currentNeedleCents);
                        return;
                    }
                    this.currentNeedleCents += diff * LERP_FACTOR;
                    this.setNeedleAngle(this.currentNeedleCents);
                    this.rafHandle = requestAnimationFrame(animate);
                };

                this.rafHandle = requestAnimationFrame(animate);
            }

            private cancelAnimationLoop(): void {
                if (this.rafHandle !== undefined) {
                    cancelAnimationFrame(this.rafHandle);
                    this.rafHandle = undefined;
                }
            }

            subscribedInstanceId: number = -1;

            monitorHandle?: MonitorPortHandle;

            addSubscription() {
                this.subscribedInstanceId = this.props.instanceId;
                this.monitorHandle = this.model.monitorPort(this.props.instanceId,this.frequencyPortName(),1.0/30,this.onFrequencyUpdated);
            }
            removeSubscription() {
                this.subscribedInstanceId = -1;
                if (this.monitorHandle) {
                    this.model.unmonitorPort(this.monitorHandle);
                    this.monitorHandle = undefined;
                }
            }
            updateSubscription() {
                if (this.subscribedInstanceId !== this.props.instanceId)
                {
                    this.removeSubscription();
                    this.addSubscription();
                    
                }
            }

            onStateChanged(state: State)
            {
                if (state === State.Ready)
                {
                    this.removeSubscription();
                    this.updateSubscription();
                }
            }
            componentDidMount()
            {
                this.model.state.addOnChangedHandler(this.onStateChanged);
                this.addSubscription();
                // Set initial needle angle
                this.setNeedleAngle(0);
            }
            componentDidUpdate() {
                this.updateSubscription();
            }
            componentWillUnmount()
            {
                this.model.state.removeOnChangedHandler(this.onStateChanged);
                this.removeSubscription();
                this.cancelAnimationLoop();
            }

            private buildTickCache(): void {
                this.cachedSemitoneCents = this.state.pitchInfo.semitoneCents;
                const pitchInfo = this.state.pitchInfo;
                this.cachedTickElements = [];
                const range = pitchInfo.semitoneCents > 50 ? 50 : pitchInfo.semitoneCents;
                let maxCents = Math.min(
                    Math.floor(pitchInfo.semitoneCents/2 / CENTS_PER_MARK)*CENTS_PER_MARK,
                    30
                );
                for (let cents = -maxCents; cents <= maxCents + 0.01; cents += CENTS_PER_MARK) {
                    this.cachedTickElements.push(this.makeTick(pitchInfo, cents, "tick-" + cents));

                    const absCents = Math.abs(cents);
                    if (absCents < 0.001 || Math.abs(absCents % 10) < 0.001) {
                        const angleRad = (DIAL_ANGLE_DEGREES * Math.PI/180) * (cents*2/(range));
                        const textR = 0.98 * NEEDLE_CY;
                        const tx = textR * Math.sin(angleRad) + CX;
                        const ty = -textR * Math.cos(angleRad) + NEEDLE_CY;
                        const label = Math.round(cents).toString();
                        this.cachedTickElements.push(
                            <text key={"label-" + cents} x={tx} y={ty} textAnchor="middle"
                                  fill={isDarkMode() ? "#999" : "#666"}
                                  fontSize="6" fontFamily="arial,roboto,helvetica,sans">
                                {label}
                            </text>
                        );
                    }
                }
            }

            private makeTick(pitchInfo: PitchInfo, cents: number, key: string): React.ReactNode
            {
                const absCents = Math.abs(cents);
                const isCenter = absCents < 0.001;
                const isTenCent = !isCenter && Math.abs(absCents % 10) < 0.001;

                let r0: number, r1: number, width: number;
                if (isCenter) {
                    r0 = TICK_OUTER_RADIUS;
                    r1 = TICK_INNER_CENTER_RADIUS;
                    width = 3;
                } else if (isTenCent) {
                    r0 = TICK_OUTER_RADIUS;
                    r1 = TICK_INNER_ZERO_RADIUS;
                    width = 2;
                } else {
                    r0 = TICK_OUTER_RADIUS;
                    r1 = TICK_INNER_RADIUS;
                    width = 1.5;
                }
                let range = pitchInfo.semitoneCents;
                if (range > 50) {
                    range = 50;
                }
                r0 *= NEEDLE_CY;
                r1 *= NEEDLE_CY;
                let angleRad = (DIAL_ANGLE_DEGREES * Math.PI/180) * (cents*2/range);

                let sin_ = Math.sin(angleRad);
                let cos_ = -Math.cos(angleRad);
                let cx = CX;
                let cy = NEEDLE_CY;
                let d = `M${r0*sin_+cx},${r0*cos_+cy}L${r1*sin_+cx},${r1*cos_+cy}`;

                const tickColor = isDarkMode() ? "#b5b5b5" : "#666";
                const tickMax = Math.min(
                    Math.floor(pitchInfo.semitoneCents/2 / CENTS_PER_MARK)*CENTS_PER_MARK,
                    30
                );
                const opacity = 1 - Math.pow(absCents / tickMax, 2) * 1.2;
                return (<path key={key} d={d} stroke={tickColor} strokeWidth={width+""} opacity={opacity} />);
            }

            renderDial(pitchInfo: PitchInfo) 
            {
                let bounds =  "0,0," + DIAL_VIEWBOX_WIDTH+ "," +DIAL_VIEWBOX_HEIGHT;

                const needleColor = isDarkMode() ? "#FFF" : "#303030";

                // Build tick cache on first render or when semitoneCents changes
                if (this.cachedSemitoneCents !== pitchInfo.semitoneCents) {
                    this.buildTickCache();
                }

                const absCents = Math.abs(pitchInfo.fraction * pitchInfo.semitoneCents);
                const textColor = centsToColor(absCents);

                return (
                    <svg viewBox={bounds} width="100%" height="100%" style={{position: "absolute", top: 0, left: 0}} preserveAspectRatio="xMidYMid meet">
                        <defs>
                            <filter id="glow" x="-20%" y="-20%" width="140%" height="140%">
                                <feGaussianBlur stdDeviation="1.5" result="blur"/>
                                <feMerge>
                                    <feMergeNode in="blur"/>
                                    <feMergeNode in="SourceGraphic"/>
                                </feMerge>
                            </filter>
                        </defs>
                        { this.cachedTickElements }
                        <g ref={this.needleRef}>
                            <polygon
                                points={[
                                    [CX, NEEDLE_OUTER_Y],
                                    [CX+3, 100],
                                    [CX+1.5, NEEDLE_INNER_Y],
                                    [CX-1.5, NEEDLE_INNER_Y],
                                    [CX-3, 100]
                                ].map(p => p.join(',')).join(' ')}
                                fill={needleColor} />
                        </g>
                        <text x={CX-15} y={98} textAnchor="end" fill={textColor}
                              fontSize="25" fontWeight="700" filter="url(#glow)"
                              style={{fontFamily: "Pirulen, arial, roboto, helvetica, sans"}}>
                            {pitchInfo.name}
                        </text>
                        <text x={CX+15} y={98} textAnchor="start" fill={textColor}
                              fontSize="25" fontWeight="700" filter="url(#glow)"
                              style={{fontFamily: "Pirulen, arial, roboto, helvetica, sans"}}>
                            {pitchInfo.fractionText}
                        </text>
                    </svg>
                )
            }

            render() {
                return (
                    <div style={{
                        width: "100%",
                        height: "100%",
                        position: "relative",
                        boxShadow: isDarkMode() ? 
                            "5px 5px 6px rgba(0,0,0,0.8) inset":
                            "1px 5px 6px #888 inset",
                        background: isDarkMode()? "rgba(255,255,255,0.07)" : ""
                    }}>
                        { this.renderDial(this.state.pitchInfo) }
                    </div>
                );
            }
        },
        styles
    );

export default TunerControl;
