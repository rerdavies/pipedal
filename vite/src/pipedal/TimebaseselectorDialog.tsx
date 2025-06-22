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

import React, { useState } from 'react';
import DialogEx from './DialogEx';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import Button from '@mui/material/Button';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import TextField, { StandardTextFieldProps } from '@mui/material/TextField';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';
import Toolbar from '@mui/material/Toolbar';
import IconButtonEx from './IconButtonEx';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import Timebase, { TimebaseUnits } from './Timebase';

interface TimebaseSelectorDialogProps {
    timebase: Timebase;
    onTimebaseChange: (timebase: Timebase) => void;
}

interface NumericEditProps extends StandardTextFieldProps {
    value: number;
    onValueChange: (value: number) => void;
    parse: (value: string) => number;
    min: number;
    max: number;
    step?: number;
    style?: React.CSSProperties;
}

function NumericEdit(props: NumericEditProps) {
    let { value, min, max, parse, step, onValueChange,style, ...extras } = props;
    let [text, setText] = React.useState(value.toString());
    let [error, setError] = React.useState(false);
    let [focus, setFocus] = React.useState(false);

    React.useEffect(() => {
        if (!focus) {
            setText(value.toString());
            setError(false);
        }
    }, [value]);

    const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        let newValue = parse(event.target.value);
        if (!isNaN(newValue) && newValue >= min && newValue <= max) {
            setText(event.target.value);
            onValueChange(newValue);
            setError(false);
        } else {
            setText(event.target.value);
            setError(true);
        }
    };
    return (
        <TextField
            {...extras}
            style={style}
            variant="standard"
            type="number"
            value={text}
            onChange={handleChange}
            inputProps={{
                min: props.min,
                max: props.max,
                step: props.step,
                style: { textAlign: 'center' }
            }}
            error={error}
            onFocus={(e) => {
                setFocus(true);
                e.target.select();
            }}
            onBlur={() => {
                setFocus(false);
                let newValue = parse(text);
                if (isNaN(newValue)) {
                    setText(value.toString());
                } else if (newValue < min) {
                    newValue = min;
                    setText(value.toString());
                    onValueChange(newValue);
                } else if (newValue > max) {
                    newValue = max;
                    setText(newValue.toString());
                    onValueChange(value)

                } else {
                    setText(newValue.toString());
                    onValueChange(newValue);
                }
                setError(false);
            }}
        />
    );
}


export default function TimebaseSelectorDialog(props: TimebaseSelectorDialogProps) {
    const { timebase, onTimebaseChange } = props;
    const [open, setOpen] = useState(false);
    const [editingTimebase, setEditingTimebase] = useState<Timebase>({ ...timebase });

    const handleOpen = () => {
        setEditingTimebase({ ...timebase });
        setOpen(true);
    };

    const handleClose = () => {
        setOpen(false);
    };
    const enableBeatControls = editingTimebase.units === TimebaseUnits.Beats;

    const handleChange = (timebase: Timebase) => {
        onTimebaseChange(timebase);
    };

    const handleTimebaseTypeChange = (event: any) => {
        let value = {
            ...editingTimebase,
            units: event.target.value as TimebaseUnits
        };
        setEditingTimebase(value);
        handleChange(value);

    };

    const handleTempoChange = (tempo: number) => {
        let value = {
            ...editingTimebase,
            tempo: tempo
        };
        setEditingTimebase(value);
        handleChange(value);
    };

    const handleTimeSignatureChange = (field: 'numerator' | 'denominator', numVal: number) => {
        let value = {
            ...editingTimebase,
            timeSignature: {
                ...editingTimebase.timeSignature,
                [field]: numVal
            }
        };
        setEditingTimebase(value);
        handleChange(value);
    };

    const getTimebaseTypeLabel = (timebase: Timebase): string => {
        switch (timebase.units) {
            case TimebaseUnits.Seconds: return 'Seconds';
            case TimebaseUnits.Samples: return 'Samples';
            case TimebaseUnits.Beats:
                {

                    return timebase.tempo.toString() +
                        " bpm (" +
                        timebase.timeSignature.numerator.toString() +
                        "/" + timebase.timeSignature.denominator.toString() + ")";
                }
            default: return 'Unknown';
        }
    };
    return (
        <>
            <Button variant="dialogSecondary" onClick={handleOpen}
                style={{ textTransform: 'none' }}
            >
                Units: {getTimebaseTypeLabel(timebase)}
            </Button>

            <DialogEx tag="timebasesettings"
                open={open} onClose={handleClose} maxWidth="sm"
                onEnterKey={() => { }}
            >
                <DialogTitle style={{ paddingTop: 0 }}>
                    <Toolbar style={{ padding: 0 }}>
                        <IconButtonEx
                            tooltip="Back"
                            edge="start"
                            color="inherit"
                            aria-label="back"
                            style={{ opacity: 0.6 }}
                            onClick={() => { handleClose(); }}
                        >
                            <ArrowBackIcon style={{ width: 24, height: 24 }} />
                        </IconButtonEx>
                        <Typography noWrap component="div" sx={{ flexGrow: 1 }}>
                            Timebase Settings
                        </Typography>
                        <div style={{ flexGrow: 1 }} />

                    </Toolbar>
                </DialogTitle>
                <DialogContent>
                    <Box sx={{
                        paddingLeft: 0, paddingRight: 0,
                        paddingTop: 0, paddingBottom: 1
                    }}>
                        <div style={{ display: "flex", flexFlow: "column nowrap", alignItems: "start" }}>
                            <Typography variant="caption">
                                Units
                            </Typography>
                            <Select variant="standard"
                                style={{ width: 180 }}
                                value={editingTimebase.units}
                                label="Timebase"
                                onChange={handleTimebaseTypeChange}
                            >
                                <MenuItem value={TimebaseUnits.Seconds}>Seconds</MenuItem>
                                <MenuItem value={TimebaseUnits.Samples}>Samples</MenuItem>
                                <MenuItem value={TimebaseUnits.Beats}>Beats</MenuItem>
                            </Select>

                            <div style={{ opacity: enableBeatControls ? 1 : 0.4, }}
                            >
                                <Typography display="block" variant="caption" style={{ paddingTop: 24 }}>
                                    Tempo (BPM)
                                </Typography>
                                <NumericEdit
                                    parse={(value: string) => parseFloat(value)}
                                    min={10}
                                    max={400}
                                    step={1}
                                    onValueChange={handleTempoChange}
                                    value={editingTimebase.tempo}
                                    disabled={!enableBeatControls}

                                    variant="standard"
                                    type="number"
                                    style={{ width: 120, }}
                                />
                                <Typography display="block" variant="caption" style={{ paddingTop: 24 }}>
                                    Time Signature
                                </Typography>
                                <div style={{
                                    display: "flex", flexFlow: "row nowrap",
                                    alignItems: "center", justifyContent: "start"
                                }}>

                                    <NumericEdit
                                        parse={(value: string) => parseInt(value)}
                                        min={1}
                                        max={32}
                                        step={1}
                                        onValueChange={(value) => handleTimeSignatureChange('numerator', value)}
                                        value={editingTimebase.timeSignature.numerator}
                                        style={{ width: 80 }}
                                        variant="standard"
                                        type="number"
                                        disabled={!enableBeatControls}
                                    />
                                    <Typography variant="body2">&nbsp;/&nbsp;</Typography>
                                    <Select variant="standard"
                                        style={{ width: 80 }}
                                        value={editingTimebase.timeSignature.denominator}
                                        disabled={!enableBeatControls}

                                        label="Timebase"
                                        sx={{
                                            '& .MuiSelect-select': {
                                                textAlign: 'center'
                                            }
                                        }}
                                        onChange={(e) => {
                                            handleTimeSignatureChange('denominator', parseInt(e.target.value.toString()));
                                        }
                                        }
                                    >
                                        <MenuItem value={2} sx={{ justifyContent: 'center' }}>2</MenuItem>
                                        <MenuItem value={4} sx={{ justifyContent: 'center' }}>4</MenuItem>
                                        <MenuItem value={8} sx={{ justifyContent: 'center' }}>8</MenuItem>
                                    </Select>

                                </div>
                            </div>
                        </div>
                    </Box>
                </DialogContent >
            </DialogEx >
        </>
    );
}


