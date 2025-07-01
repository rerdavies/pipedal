/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

import React, { SyntheticEvent, useEffect } from "react";
import FormGroup from "@mui/material/FormGroup";
import FormControlLabel from "@mui/material/FormControlLabel";
import DialogContent from "@mui/material/DialogContent";
import TextField, { StandardTextFieldProps } from "@mui/material/TextField";
import DialogTitle from "@mui/material/DialogTitle";
import DialogEx from "./DialogEx";
import Toolbar from "@mui/material/Toolbar";
import IconButtonEx from "./IconButtonEx";
import ArrowBackIcon from "@mui/icons-material/ArrowBack";
import Typography from "@mui/material/Typography";
import Slider, { SliderProps } from "@mui/material/Slider";
import Checkbox from "@mui/material/Checkbox";
import TimebaseSelectorDialog from "./TimebaseselectorDialog";
import Timebase, { TimebaseUnits } from "./Timebase";
import useWindowSize from "./UseWindowSize";
import Button from "@mui/material/Button";
import PlayArrow from "@mui/icons-material/PlayArrow";
import StopIcon from '@mui/icons-material/Stop';
import { LoopParameters } from './Timebase';


export interface LoopDialogProps {
    isOpen: boolean;
    onClose: () => void;
    onSetLoop: (value: LoopParameters) => void;
    value: LoopParameters;
    playing: boolean;
    onPreview: () => void;
    onCancelPlaying: () => void;
    duration: number;
    timebase: Timebase;
    onTimebaseChange: (timebase: Timebase) => void; // Optional callback for timebase changes
    sampleRate: number;

};


interface TimeEditProps extends StandardTextFieldProps {
    timebase: Timebase;
    sampleRate: number;
    value: number;
    onValueChange?: (e: React.ChangeEvent<HTMLTextAreaElement | HTMLInputElement>, value: number) => void;
    onBlur: () => void;
    max: number;
};



function parseTime(timebase: Timebase, sampleRate: number, duration: number, timeString: string): number {

    switch (timebase.units) {
        case TimebaseUnits.Samples:
            {
                const samples = parseInt(timeString, 10);
                if (isNaN(samples)) {
                    return samples;
                }
                let result = samples / sampleRate; // Convert samples to seconds    
                if (result < 0) {
                    result = 0;
                }
                if (result > duration) {
                    result = duration;
                }
                return result;

            }
        case TimebaseUnits.Seconds:
            {
                const parts = timeString.split(':');
                let hours = 0;
                let minutes = 0;
                let seconds = 0;

                if (parts.length === 1) {
                    hours = 0;
                    minutes = 0;
                    seconds = parseFloat(parts[0]);
                    if (isNaN(seconds)) {
                        throw new Error("Invalid seconds format. Use 'mm:ss' or 'mm:ss.mmmm'.");
                    }
                } else if (parts.length === 2) {
                    hours = 0;
                    minutes = parseInt(parts[0], 10);
                    seconds = parseFloat(parts[1]);
                    if (isNaN(minutes) || isNaN(seconds)) {
                        throw new Error("Invalid minutes or seconds format. Use 'mm:ss' or 'mm:ss.mmmm'.");
                    }
                } else if (parts.length === 3) {
                    hours = parseInt(parts[0]);
                    minutes = parseInt(parts[1], 10);
                    seconds = parseFloat(parts[2]);
                    if (isNaN(hours) || isNaN(minutes) || isNaN(seconds)) {
                        throw new Error("Invalid hours, minutes or seconds format. Use 'hh:mm:ss' or 'mm:ss.mmmm'.");
                    }
                } else {
                    throw new Error("Invalid time format. Use 'hh:mm:ss' or 'mm:ss.mmmm'.");
                }

                let result = hours * 60 * 60 + minutes * 60 + seconds; // Convert total time to seconds
                if (result < 0) {
                    result = 0;
                }
                if (result > duration) {
                    result = duration;
                }
                return result;

            }
        case TimebaseUnits.Beats:
            {
                const parts = timeString.split(':');
                let bar = 0;
                let beat = 0;




                if (parts.length === 1) {
                    bar = 1;
                    beat = parseFloat(parts[0]);
                    if (isNaN(beat)) {
                        throw new Error("Invalid seconds format. Use 'bar:beat' or 'bar:beat.fraction'.");
                    }
                    if (beat < 1 || beat >= timebase.timeSignature.numerator + 1) {
                        throw new Error(`Beat number must be between 1 and ${timebase.timeSignature.numerator + 1}.`);
                    }
                } else if (parts.length === 2) {
                    bar = parseInt(parts[0], 10);
                    if (bar < 1) {
                        throw new Error("Bar number must be 1 or greater.");
                    }
                    beat = parseFloat(parts[1]);
                    if (isNaN(bar) || isNaN(beat)) {
                        throw new Error("Invalid bar or beat format. Use 'bar:beat' or 'bar:beat.fraction'.");
                    }
                    if (beat < 1 || beat >= timebase.timeSignature.numerator + 1) {
                        throw new Error(`Beat number must be between 1 and ${timebase.timeSignature.numerator + 1}.`);
                    }
                } else {
                    throw new Error("Invalid time format. Use 'hh:mm:ss' or 'mm:ss.mmmm'.");
                }
                const actualBeat = (bar - 1) * timebase.timeSignature.numerator + (beat - 1); // Convert bar and beat to actual beat number
                let result = actualBeat * 60 / timebase.tempo; // Convert beats to seconds based on tempo
                if (result < 0) {
                    result = 0;
                }
                if (result > duration) {
                    result = duration;
                }
                return result;
            }
    }
}

export function formatTime(timebase: Timebase, sampleRate: number, seconds: number): string {
    switch (timebase.units) {
        case TimebaseUnits.Samples:
            {
                return Math.round(seconds * sampleRate).toString();
            }
            break;
        case TimebaseUnits.Seconds:
            {
                const hours = Math.floor(seconds / 3600);
                const minutes = Math.floor(seconds / 60);
                const secs = Math.floor(seconds % 60);
                const mmillis = Math.floor((seconds % 1) * 10000);
                if (hours == 0) {
                    return `${minutes}:${secs.toString().padStart(2, '0')}.${mmillis.toString().padStart(4, '0')}`;
                } else {
                    return `${hours}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}.${mmillis.toString().padStart(4, '0')}`;
                }
                break;
            }
        case TimebaseUnits.Beats:
            {
                let beats = timebase.tempo / 60.0 * seconds;
                const bars = Math.floor(beats / timebase.timeSignature.numerator);
                const beat = (beats - bars * timebase.timeSignature.numerator);
                return `${bars + 1}:${(beat + 1).toFixed(4)}`;
            }
            break;
        default:
            throw new Error("Unsupported timebase units");
    }
}




interface SliderWithPreviewProps extends SliderProps {
    style?: React.CSSProperties;
    value: number | number[];
    onChange: (event: Event | SyntheticEvent, value: number | number[], thumb?: number) => void;
    onPreview?: (event: Event | SyntheticEvent, value: number | number[], thumb?: number) => void;
    onCommitPreviewValue?: (event: Event | SyntheticEvent, value: number | number[], thumb?: number) => void;
}
interface PreviewArguments {
    newValue: number | number[];
    thumb?: number;
};

function SliderWithPreview(props: SliderWithPreviewProps) {
    const { value, onChange, onPreview, onCommitPreviewValue, style, ...extra } = props;
    const [previewArguments, setPreviewArguments] =
        React.useState<PreviewArguments | null>(null);

    const [pointerDown, setPointerDown] = React.useState(false);


    // code to handle a bug in crhome relating to mouseup events, that causes zero values to not be set.

    const handleMouseUp = (e: MouseEvent) => {
        if (pointerDown && e.button === 0) { // Left mouse button
            setPointerDown(false);
            if (previewArguments) {
                if (onCommitPreviewValue) {
                    onCommitPreviewValue(e as Event, previewArguments.newValue, previewArguments.thumb);
                }
                setPreviewArguments(null);
            }
        }
    };

    useEffect(() => {
        document.addEventListener('mouseup', handleMouseUp);

        // Cleanup: Remove the event listener when the component unmounts
        return () => {
            document.removeEventListener('mouseup', handleMouseUp);
        };
    }, []); // Re-run effect when tempValue changes to ensure latest value is committed


    return (
        <Slider
            {...extra}
            style={style}
            step={0.0001}
            value={previewArguments ? previewArguments.newValue : props.value}
            onChange={(event, newValue, thumb) => {
                if (pointerDown) {
                    if (props.onPreview) {
                        props.onPreview(event, newValue, thumb);
                    }
                    setPreviewArguments({ newValue, thumb });
                } else {
                    if (props.onChange) {
                        props.onChange(event, newValue, thumb);
                    }
                    return;
                }
            }}

            onChangeCommitted={(event, newValue) => {
                if (pointerDown) {
                    if (props.onPreview) {
                        props.onPreview(event as SyntheticEvent, newValue, 0);
                    }
                    setPreviewArguments({ newValue: newValue, thumb: 0 });
                } else {
                    if (props.onChange) {
                        props.onChange(event, newValue, 0);
                    }
                    return;
                }
            }}
            onMouseDown={(e) => {
                if (e.button == 0) {
                    setPointerDown(true);
                }
            }}
            onMouseUp={(e) => {
                if (e.button == 0) { // Left mouse button   
                    setPointerDown(false);
                    if (previewArguments) {
                        if (onCommitPreviewValue) {
                            onCommitPreviewValue(e, previewArguments.newValue, previewArguments.thumb);
                        }
                        setPreviewArguments(null);
                    }
                }

            }}
            onMouseMove={(e) => {
                if (pointerDown && previewArguments) {
                    if (props.onPreview) {
                        if (e.clientX < 0) // bug in crhrome
                        {
                            props.onPreview(e, previewArguments.newValue, previewArguments.thumb);

                        }
                    }
                }
            }
            }
        />
    );
}


function TimeEdit(props: TimeEditProps) {
    let { value, onValueChange, onBlur, timebase, sampleRate, max, ...extra } = props;
    const [text, setText] = React.useState(formatTime(timebase, props.sampleRate, props.value));
    const [error, setError] = React.useState(false);
    const [focus, setFocus] = React.useState(false);
    // slice props.

    React.useEffect(() => {
        if (!focus) {
            setText(formatTime(timebase, sampleRate, props.value));
        }
    },
        [props.value]);

    React.useEffect(() => {
        if (!focus) {
            setText(formatTime(props.timebase, sampleRate, props.value));
        }
    }, [props.timebase, sampleRate]);



    return (
        <TextField
            {...extra}
            variant="standard"
            spellCheck="false"
            error={error}
            value={text}
            onBlur={() => {
                setText(formatTime(props.timebase, sampleRate, props.value));
                setFocus(false);
                onBlur();
            }}
            onChange={(e) => {
                try {
                    setText(e.target.value);
                    let val = parseTime(props.timebase, sampleRate, max, e.target.value);
                    if (!isNaN(val)) {
                        if (val > max) {
                            val = max;
                        }
                        if (props.onValueChange) {
                            props.onValueChange(e, val);
                        }
                        setError(false);
                    } else {
                        setError(true);
                    }
                } catch (err) {
                    setError(true);
                }
            }}
            onFocus={(e) => {
                setFocus(true);
                setText(formatTime(props.timebase, sampleRate, props.value));
                setError(false);
                e.target.select();
            }
            }
            inputProps={{
                style: { textAlign: "center" },
                autoComplete: "off",
                spellCheck: "false",
                autoCorrect: "off",
                autoCapitalize: "off",
            }}
        />

    );
}



export default function LoopDialog(props: LoopDialogProps) {
    const [start, setStart] = React.useState(props.value.start);
    const [loopEnable, setLoopEnable] = React.useState(props.value.loopEnable);
    const [loopStart, setLoopStart] = React.useState(props.value.loopStart);
    const [loopEnd, setLoopEnd] = React.useState(props.value.loopEnd);


    const [windowSize] = useWindowSize(); 
    const fullScreen = windowSize.height < 500;

    function cancelPlaying() {
        props.onCancelPlaying();
    }
    function commitResults() {
        let tLoopStart = loopStart;
        let tLoopEnd = loopEnd;
        if (tLoopEnd < tLoopStart) {
            let tmp = tLoopStart;
            tLoopStart = tLoopEnd;
            tLoopEnd = tmp;
            setLoopStart(tLoopStart);
            setLoopEnd(tLoopEnd);
        }
        if (props.onSetLoop) {
            props.onSetLoop({ start: start, loopEnable: loopEnable, loopStart: tLoopStart, loopEnd: tLoopEnd });
        }
    }
    React.useEffect(() => {
        setStart(props.value.start);
        setLoopEnable(props.value.loopEnable);
        setLoopStart(props.value.loopStart);
        setLoopEnd(props.value.loopEnd);
    }, [props.value]);

    return (
        <DialogEx
            tag="LoopDialog"
            onClose={() => {
                commitResults();
                props.onClose();
            }}

            maxWidth="xl"
            open={props.isOpen}
            onEnterKey={() => {
            }}
            fullScreen={fullScreen}
            sx={{
                "& .MuiDialog-container": {
                    "& .MuiPaper-root": {
                        width: "100%",
                        maxWidth: fullScreen ? undefined : "500px",  // Set your width here
                    },
                },
            }}
        >
            <DialogTitle style={{ paddingTop: 0 }}>
                <Toolbar style={{ padding: 0 }}>
                    <IconButtonEx
                        tooltip="Back"
                        edge="start"
                        color="inherit"
                        aria-label="back"
                        style={{ opacity: 0.6 }}
                        onClick={() => {
                            commitResults();
                            props.onClose();
                        }}
                    >
                        <ArrowBackIcon style={{ width: 24, height: 24 }} />
                    </IconButtonEx>
                    <Typography noWrap component="div" sx={{ flexGrow: 1 }}>
                        Loop
                    </Typography>
                    <div style={{ flexGrow: 1 }} />

                    <TimebaseSelectorDialog

                        onTimebaseChange={(timebase) => {
                            props.onTimebaseChange(timebase);        // Handle timebase change if needed
                        }}
                        timebase={props.timebase} // Pass the current timebase if needed
                    />
                </Toolbar>
            </DialogTitle>
            <DialogContent style={{}}>
                <div style={{
                    position: "relative",
                    display: "flex", justifyContent: "stretch", flexFlow: "column nowrap", marginLeft: 24, marginRight: 24,
                    marginBottom: 8

                }}>
                    <Typography display="block" variant="body1" gutterBottom style={{}}>
                        Start
                    </Typography>
                    <SliderWithPreview
                        style={{
                            width: "100%"
                        }}
                        value={start}
                        min={0}
                        max={props.duration}

                        onChange={(e, v, thumb) => {
                            let val = Array.isArray(v) ? v[0] : v
                            setStart(val);
                        }}
                        onPreview={(e, v, thumb) => {
                            let val = Array.isArray(v) ? v[0] : v

                            setStart(val);
                        }}
                        onCommitPreviewValue={(e, v, thumb) => {
                            let val = Array.isArray(v) ? v[0] : v

                            setStart(val);
                            cancelPlaying();

                        }}
                        valueLabelFormat={(v) => formatTime(props.timebase, props.sampleRate, v)}
                    />

                    <TimeEdit variant="standard" spellCheck="false"
                        value={start}
                        max={props.duration}
                        timebase={props.timebase}
                        sampleRate={props.sampleRate}


                        onBlur={() => {
                            if (start > props.duration) {
                                setStart(props.duration);
                            }
                        }}
                        onValueChange={(e, val) => {
                            setStart(val);
                            cancelPlaying();
                        }
                        }
                        style={{ width: 120, alignSelf: "center", textAlign: "center" }} />
                    <FormGroup style={{ alignSelf: "start", marginTop: 8, marginBottom: 8, marginLeft: 0 }}>
                        <FormControlLabel
                            labelPlacement="start"
                            style={{ marginLeft: 0 }}
                            control={<Checkbox checked={loopEnable}
                                onChange={(e, checked) => {
                                    setLoopEnable(checked);
                                    cancelPlaying();
                                }}

                            />} label="Enable loop" />
                    </FormGroup>

                    <SliderWithPreview
                        style={{
                            width: "100%",
                            opacity: loopEnable ? 1 : 0.4,
                        }}
                        disabled={!loopEnable}
                        value={
                            [loopStart, loopEnd]
                        }
                        min={0}
                        max={props.duration}
                        onChange={(e, v, thumb) => {
                            if (!Array.isArray(v)) throw new Error("Expected array for loop start and end");

                            setLoopStart(v[0]);
                            setLoopEnd(v[1]);
                        }}
                        onPreview={(e, v, thumb) => {
                            if (!Array.isArray(v)) throw new Error("Expected array for loop start and end");

                            setLoopStart(v[0]);
                            setLoopEnd(v[1]);
                        }}
                        onCommitPreviewValue={(e, v, thumb) => {
                            if (!Array.isArray(v)) throw new Error("Expected array for loop start and end");

                            setLoopStart(v[0]);
                            setLoopEnd(v[1]);
                            cancelPlaying();

                        }}

                    />
                    <div style={{ display: "flex", justifyContent: "space-evenly", columnGap: 8 }}>
                        <TimeEdit variant="standard" spellCheck="false"
                            value={loopStart}
                            disabled={!loopEnable}
                            max={props.duration}
                            timebase={props.timebase}
                            sampleRate={props.sampleRate}

                            style={{
                                maxWidth: 120, textAlign: "center", opacity: loopEnable ? 1 : 0.4,
                                flex: "1 1 auto"
                            }}
                            onBlur={() => {
                            }}
                            onValueChange={(e, val) => {
                                setLoopStart(val);
                                cancelPlaying();


                            }}
                        />
                        <TimeEdit variant="standard" spellCheck="false"
                            max={props.duration}

                            value={loopEnd}
                            sampleRate={props.sampleRate}

                            disabled={!loopEnable}

                            timebase={props.timebase}

                            style={{
                                maxWidth: 120, textAlign: "center",
                                opacity: loopEnable ? 1 : 0.4,
                                flex: "1 1 auto"
                            }}

                            onBlur={() => {
                            }}
                            onValueChange={(e, val) => {
                                setLoopEnd(val);
                                cancelPlaying();

                            }}
                        />
                    </div>
                    <Button variant="dialogSecondary" style={{ alignSelf: "end", marginTop: 16, width: 120 }}
                        startIcon={
                            props.playing ? (<StopIcon />) : (<PlayArrow />)
                        }
                        onClick={() => {
                            commitResults();
                            props.onPreview();
                        }}>
                        Preview
                    </Button>
                </div>

            </DialogContent>
        </DialogEx>
    )
}

