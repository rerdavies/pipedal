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

import React, {Component} from 'react';
import { createStyles, Theme, withStyles, WithStyles } from '@material-ui/core/styles';
import { MonitorPortHandle, PiPedalModel, State, PiPedalModelFactory } from "./PiPedalModel";
import SvgPathBuilder from './SvgPathBuilder'

//const char* model[] = {"12-TET","19-TET","24-TET", "31-TET", "53-TET"};
// set_adjustment(ui->widget[2]->adj,440.0, 440.0, 427.0, 453.0, 0.1, CL_CONTINUOS);

const FREQUENCY_PORT_NAME = "FREQ";

const DIAL_WIDTH= 220;
const DIAL_HEIGHT = 100;
const NEEDLE_CY = 320;
const CENTS_PER_MARK = 5;
const NEEDLE_OUTER_RADIUS = 0.98;
const TICK_OUTER_RADIUS = 0.96;
const TICK_INNER_RADIUS = 0.85;
const TICK_INNER_ZERO_RADIUS = 0.82;
const DIAL_ANGLE_RADIANS = 20*Math.PI/180;


const styles = (theme: Theme) =>
    createStyles({
        icon: {
            width: "24px",
            height: "24px",
            margin: "0px",
            opacity: "0.6"
        }
        });

interface PitchInfo {
    valid: boolean;
    name: string;
    fractionText: string;
    fraction: number;
    
    semitoneCents: number
};

const FLAT = "\u{266d}";
const SHARP = "#";
const DOUBLE_FLAT = "\uD834\uDD2B";
const DOUBLE_SHARP =  "\uD834\uDD2A";
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

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const NOTES_31TET: string[] = [
    "C", "D"+DOUBLE_FLAT, "C" + SHARP,"Db","C"+DOUBLE_SHARP,"D","E"+DOUBLE_FLAT,"D" + SHARP,"Eb","D"+DOUBLE_SHARP,"E",'Fb","E" + SHARP,"F","G"+DOUBLE_FLAT,"F" + SHARP,"Gb""Fx","G","A"+DOUBLE_FLAT,"G"+DOUBLE_SHARP,"A"'
];

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const NOTES_53TET = ["la","laa","lo","law","ta","teh","te","tu","tuh","ti","tih","to","taw","da","do","di","daw","ro","rih","ra","ru","ruh","reh","re ","ri","raw","ma","meh","me","mu","muh","mi","maa","mo","maw","fe","fa","fih","fu","fuh","fi","se","suh","su","sih","sol","si","saw","lo","leh","le","lu","luh"];

interface GxTunerControlProps extends WithStyles<typeof styles> {
    theme: Theme;
    instanceId: number;
};
type GxTunerControlState = {
    pitchInfo: PitchInfo
    tet: number;
    refFrequency: number;
};

const GxTunerControl =
    withStyles(styles, { withTheme: true })(
        class extends Component<GxTunerControlProps, GxTunerControlState>
        {
            model: PiPedalModel;


            refRoot: React.RefObject<HTMLDivElement>;

            constructor(props: GxTunerControlProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();

                this.refRoot =  React.createRef();

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


            noteToPitchInfo(hz: number) : PitchInfo
            {
                if (hz < 65) // ground hum. Ignore it.
                {
                    hz = 0;
                } 
                let tet = this.state.tet;
                let refFrequency = this.state.refFrequency;
                let aOffset: number;
                let names: string[];
                let semitoneCents = 100;
                let valid = false;


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
                if (hz !== 0)
                {

                    let note = Math.log2(hz/refFrequency)*tet + aOffset;
                    let noteNumber = Math.round(note);

                    let octave = Math.floor((noteNumber)/tet);

                    let nameIndex = noteNumber -octave* tet;
                    if (nameIndex < 0) nameIndex += tet;
                    name = names[ nameIndex ] + (octave-2); 
                    valid = true;
                    fraction = note-noteNumber;
                    if (fraction >= 0) {
                        fractionText = "+" + fraction.toFixed(2).substr(1);
                    } else {
                        fractionText = "\u2212" + fraction.toFixed(2).substr(2);

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

            lastValue: number = -1;
            onFrequencyUpdated(value: number): void
            {
                // suppress repeated zeros.
                if (value === this.lastValue && value === 0 && this.animationIdle) return;

                this.lastValue = value;

                let pitchInfo = this.noteToPitchInfo(value);
                this.setState({  
                    pitchInfo: pitchInfo
                });
            }

            subscribedInstanceId: number = -1;

            monitorHandle?: MonitorPortHandle;

            addSubscription() {
                this.subscribedInstanceId = this.props.instanceId;
                this.monitorHandle = this.model.monitorPort(this.props.instanceId,FREQUENCY_PORT_NAME,1.0/30,this.onFrequencyUpdated);
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
                // initial or reconnect
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
            }
            componentDidUpdate() {
                this.updateSubscription();
            }
            componentWillUnmount()
            {
                this.model.state.removeOnChangedHandler(this.onStateChanged);
                this.removeSubscription();
            }



            makeDialTick(pitchInfo: PitchInfo, cents: number): React.ReactNode {
                let isZeroTick = Math.abs(cents) < 0.001;
                let r0 = TICK_OUTER_RADIUS;
                let r1 = (isZeroTick? TICK_INNER_ZERO_RADIUS: TICK_INNER_RADIUS);
                let width = isZeroTick? 3: 2;
                return this.makeTick(pitchInfo,cents,r0,r1,"#666", width);
            }

            makeTick(pitchInfo: PitchInfo, cents: number, r0: number, r1: number, stroke: string,width: number): React.ReactNode
            {
                let range = pitchInfo.semitoneCents;
                if (range > 50) {
                    range = 50;
                }
                r0 *= NEEDLE_CY;
                r1 *= NEEDLE_CY;
                let angle = DIAL_ANGLE_RADIANS *  (cents*2/(range));

                let sin_ = Math.sin(angle);
                let cos_ = -Math.cos(angle);
                let cx = DIAL_WIDTH/2;
                let cy = NEEDLE_CY;
                let path = new SvgPathBuilder().moveTo(r0*sin_+cx,r0*cos_+cy).lineTo(r1*sin_+cx,r1*cos_+cy).toString();

                return (<path d={path} stroke={stroke} strokeWidth={width+""} />);
            }

            lastValidTime: number =  0;
            lastValidCents: number = -100;
            animationIdle: boolean = true;

            makeNeedle(pitchInfo: PitchInfo)
            {
                let maxCents = pitchInfo.semitoneCents/2;
                if (maxCents > 25) {
                    maxCents = 25;
                }
                let cents: number;
                if (!pitchInfo.valid)
                {
                    const NEEDLE_DECAY_RATE_PER_MS = -25/100;

                    // animate to zero position.
                    let time = new Date().getTime();
                    let dt = time-this.lastValidTime;

                    cents = this.lastValidCents+(dt*NEEDLE_DECAY_RATE_PER_MS);
                    if (cents <= -maxCents)
                    {
                        cents = -maxCents;
                        this.animationIdle = true;
                    } else {
                        this.animationIdle = false;
                    }
                } else {
                    cents = pitchInfo.semitoneCents*pitchInfo.fraction;
                    if (cents > maxCents) {
                        cents = maxCents;
                    }
                    if (cents < -maxCents)
                    {
                        cents = -maxCents;
                    }
                    this.lastValidCents = cents;
                    this.lastValidTime = new Date().getTime();
                }
                return this.makeTick(pitchInfo,cents,NEEDLE_OUTER_RADIUS,0.1,"#800",3);
            }
            
            renderDial(pitchInfo: PitchInfo) 
            {

                let bounds =  "0,0," + DIAL_WIDTH+ "," +DIAL_HEIGHT;
                
                let content: React.ReactNode[] = [];

                let maxCents = Math.floor(pitchInfo.semitoneCents/2 / CENTS_PER_MARK)*CENTS_PER_MARK;

                for (let cents = -maxCents; cents <= maxCents+ 0.01; cents += CENTS_PER_MARK) {
                    content.push(this.makeDialTick(pitchInfo,cents));
                }

                content.push(
                    this.makeNeedle(pitchInfo)
                );


                return (
                    <svg viewBox={bounds} width={DIAL_WIDTH} height={DIAL_HEIGHT} style={{position: "absolute"}} >
                        { content }
                    </svg>
                )
            }

            render() {

                return (<div ref={this.refRoot} style={{width: DIAL_WIDTH, height: DIAL_HEIGHT, fontSize: "2em", fontWeight: 700, position: "relative",
                    boxShadow: "1px 5px 6px #888 inset",
                     fontFamily: "arial,roboto,helvetica,sans"}}>
                         <div style={{position: "absolute", left: 0, bottom: 5, width: "50%",textAlign: "right"}}>
                             <span style={{ marginRight: 20, color: "#444", textAlign: "right" }}>{this.state.pitchInfo.name}</span>
                         </div>
                         <div style={{position: "absolute", right: 0, bottom: 5, width: "50%",textAlign: "left"}}>
                             <span style={{ marginLeft: 20, color: "#444", textAlign: "left" }}>{this.state.pitchInfo.fractionText}</span>
                         </div>

                    { this.renderDial(this.state.pitchInfo) }
                    </div>);
            }
        }
    );




export default GxTunerControl;