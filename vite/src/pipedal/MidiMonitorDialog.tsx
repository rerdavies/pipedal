// Copyright (c) 2026 Thomas Rapolani
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

import { useEffect, useRef, useState } from 'react';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import PauseIcon from '@mui/icons-material/Pause';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import DeleteIcon from '@mui/icons-material/Delete';

import DialogEx from './DialogEx';
import IconButtonEx from './IconButtonEx';
import { ListenHandle, MidiMessage, PiPedalModelFactory } from './PiPedalModel';

// Keep the list bounded: a controller sending continuous CCs would otherwise
// grow it without limit while the dialog is open.
const MAX_ENTRIES = 200;

interface MidiMonitorEntry {
    key: number;
    time: string;
    channel: string;
    type: string;
    detail: string;
}

const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];

function noteName(note: number): string {
    // Middle C (note 60) is C4, matching the convention used elsewhere in the UI.
    return `${NOTE_NAMES[note % 12]}${Math.floor(note / 12) - 1}`;
}

function describe(message: MidiMessage): { channel: string, type: string, detail: string } {
    const status = message.cc0 & 0xF0;
    const channel = `${(message.cc0 & 0x0F) + 1}`;
    switch (status) {
        case 0x80:
            return { channel, type: "Note Off", detail: `${noteName(message.cc1)} (${message.cc1}) vel ${message.cc2}` };
        case 0x90:
            return message.cc2 === 0
                ? { channel, type: "Note Off", detail: `${noteName(message.cc1)} (${message.cc1}) vel 0` }
                : { channel, type: "Note On", detail: `${noteName(message.cc1)} (${message.cc1}) vel ${message.cc2}` };
        case 0xA0:
            return { channel, type: "Aftertouch", detail: `${noteName(message.cc1)} (${message.cc1}) ${message.cc2}` };
        case 0xB0:
            return { channel, type: "Control Change", detail: `CC ${message.cc1} = ${message.cc2}` };
        case 0xC0:
            return { channel, type: "Program Change", detail: `program ${message.cc1}` };
        case 0xD0:
            return { channel, type: "Channel Pressure", detail: `${message.cc1}` };
        case 0xE0:
            return { channel, type: "Pitch Bend", detail: `${((message.cc2 << 7) | message.cc1) - 8192}` };
        default:
            return {
                channel: "-",
                type: "System",
                detail: `0x${message.cc0.toString(16)} ${message.cc1} ${message.cc2}`
            };
    }
}

export interface MidiMonitorDialogProps {
    open: boolean;
    onClose: () => void;
}

export default function MidiMonitorDialog(props: MidiMonitorDialogProps) {
    const { open, onClose } = props;
    const [entries, setEntries] = useState<MidiMonitorEntry[]>([]);
    const [paused, setPaused] = useState(false);
    const pausedRef = useRef(paused);
    const nextKey = useRef(0);

    pausedRef.current = paused;

    useEffect(() => {
        if (!open) {
            return;
        }
        const model = PiPedalModelFactory.getInstance();
        let listenHandle: ListenHandle | undefined = model.monitorMidiEvents(
            (message: MidiMessage) => {
                if (pausedRef.current) {
                    return;
                }
                const described = describe(message);
                const entry: MidiMonitorEntry = {
                    key: nextKey.current++,
                    time: new Date().toLocaleTimeString(),
                    ...described
                };
                // Newest first, so the interesting end needs no scrolling.
                setEntries((previous) => [entry, ...previous].slice(0, MAX_ENTRIES));
            });

        return () => {
            if (listenHandle) {
                model.cancelListenForMidiEvent(listenHandle);
                listenHandle = undefined;
            }
        };
    }, [open]);

    if (!open) {
        return (<div />);
    }

    return (
        <DialogEx tag="midiMonitor" open={open} fullWidth fullScreen
            onClose={onClose}
            style={{ userSelect: "none" }}
            onEnterKey={() => { }}
        >
            <div style={{
                display: "flex", flexDirection: "column", flexWrap: "nowrap",
                width: "100%", height: "100%", overflow: "hidden"
            }}>
                <div style={{ flex: "0 0 auto" }}>
                    <AppBar style={{ position: "relative" }}>
                        <Toolbar>
                            <IconButtonEx tooltip="Back" edge="start" color="inherit"
                                onClick={onClose} aria-label="back" size="large">
                                <ArrowBackIcon />
                            </IconButtonEx>
                            <Typography variant="h6" style={{ flex: "1 1 auto", marginLeft: 16 }}>
                                MIDI Monitor
                            </Typography>
                            <IconButtonEx tooltip={paused ? "Resume" : "Pause"} color="inherit"
                                onClick={() => setPaused(!paused)}
                                aria-label={paused ? "resume" : "pause"} size="large">
                                {paused ? <PlayArrowIcon /> : <PauseIcon />}
                            </IconButtonEx>
                            <IconButtonEx tooltip="Clear" color="inherit"
                                onClick={() => setEntries([])} aria-label="clear" size="large">
                                <DeleteIcon />
                            </IconButtonEx>
                        </Toolbar>
                    </AppBar>
                </div>
                <div style={{ overflow: "auto", flex: "1 1 auto", width: "100%", padding: 16 }}>
                    {entries.length === 0 && (
                        <Typography variant="body2" color="textSecondary">
                            {paused
                                ? "Paused."
                                : "Waiting for MIDI input. Every incoming message is shown, including ones that cannot be bound to."}
                        </Typography>
                    )}
                    {entries.map((entry) => (
                        <div key={entry.key} style={{
                            display: "flex", flexDirection: "row", flexWrap: "nowrap",
                            gap: 16, alignItems: "baseline"
                        }}>
                            <Typography variant="caption" color="textSecondary"
                                style={{ flex: "0 0 auto", minWidth: 76 }}>
                                {entry.time}
                            </Typography>
                            <Typography variant="body2" style={{ flex: "0 0 auto", minWidth: 40 }}>
                                {entry.channel === "-" ? "-" : `Ch ${entry.channel}`}
                            </Typography>
                            <Typography variant="body2" style={{ flex: "0 0 auto", minWidth: 130 }}>
                                {entry.type}
                            </Typography>
                            <Typography variant="body2" color="textSecondary" noWrap
                                style={{ flex: "1 1 auto", minWidth: 0 }}>
                                {entry.detail}
                            </Typography>
                        </div>
                    ))}
                </div>
            </div>
        </DialogEx>
    );
}
