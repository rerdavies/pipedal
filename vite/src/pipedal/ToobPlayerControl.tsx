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

import React, { useEffect } from 'react';
import { styled } from '@mui/material/styles';
import LoopDialog from './LoopDialog';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import ButtonEx from './ButtonEx';
import IconButton from '@mui/material/IconButton';
import Pause from '@mui/icons-material/Pause';
import PlayArrow from '@mui/icons-material/PlayArrow';
import FastForward from '@mui/icons-material/FastForward';
import FastRewind from '@mui/icons-material/FastRewind';
import { PiPedalModelFactory, State } from './PiPedalModel';
import ButtonTooltip from './ButtonTooltip';
import { pathFileNameOnly } from './FileUtils';
import ButtonBase from '@mui/material/ButtonBase';
import FilePropertyDialog from './FilePropertyDialog';
import JsonAtom from './JsonAtom';
import { UiFileProperty } from './Lv2Plugin';
import { Divider } from '@mui/material';
import useWindowSize from './UseWindowSize';
import { getAlbumArtUri } from './AudioFileMetadata';
import RepeatIcon from '@mui/icons-material/Repeat';
import Timebase, { LoopParameters, TimebaseUnits } from './Timebase';
import ControlSlider from './ControlSlider';

let Player__seek = "http://two-play.com/plugins/toob-player#seek"
const AUDIO_FILE_PROPERTY_URI = "http://two-play.com/plugins/toob-player#audioFile";
const LOOP_PROPERTY_URI = "http://two-play.com/plugins/toob-player#loop";
class PluginState {
    // Must match ProcessorState values in Lv2AudioFileProcessor.hpp
    // Not an enum because it has to be json serializable.
    static Idle = 0;
    static Recording = 1;
    static StoppingRecording = 2;
    static CuePlayingThenPlay = 3;
    static CuePlayingThenPause = 4;
    static Paused = 5;
    static Playing = 6;
    static Error = 7;
};

const useWallpaper = false;
const WallPaper = styled('div')({
    position: 'absolute',
    width: '100%',
    height: '100%',
    top: 0,
    left: 0,
    overflow: 'hidden',
    background: 'linear-gradient(rgb(255, 38, 142) 0%, rgb(255, 105, 79) 100%)',
    transition: 'all 500ms cubic-bezier(0.175, 0.885, 0.32, 1.275) 0s',
    '&::before': {
        content: '""',
        width: '140%',
        height: '140%',
        position: 'absolute',
        top: '-40%',
        right: '-50%',
        background:
            'radial-gradient(at center center, rgb(62, 79, 249) 0%, rgba(62, 79, 249, 0) 64%)',
    },
    '&::after': {
        content: '""',
        width: '140%',
        height: '140%',
        position: 'absolute',
        bottom: '-50%',
        left: '-30%',
        background:
            'radial-gradient(at center center, rgb(247, 237, 225) 0%, rgba(247, 237, 225, 0) 70%)',
        transform: 'rotate(30deg)',
    },
});

function tidyHundredths(value: number): string {
    if (value === 0) {
        return "";
    } else if (value % 10 == 0) {
        return `.${(value / 10).toString()}`;
    } else {
        return `.${value.toString()}`;
    }

}
export function formatTimeCompact(timebase: Timebase, sampleRate: number, seconds: number): string {
    switch (timebase.units) {
        case TimebaseUnits.Samples:
            {
                return Math.round(seconds * sampleRate).toString();
            }
            break;
        case TimebaseUnits.Seconds:
            {
                const t = Math.round(seconds * 100);
                let hundredths = t % 100;
                let secs = Math.floor(t / 100);
                let minutes = Math.floor(secs / 60);
                let hours = Math.floor(minutes / 60);
                minutes = minutes % 60;
                secs = secs % 60;

                if (hours == 0) {
                    return `${minutes}:${secs.toString().padStart(2, '0')}${tidyHundredths(hundredths)}`;
                } else {
                    return `${hours}:${minutes}:${secs.toString().padStart(2, '0')}${tidyHundredths(hundredths)}`;
                }
                break;
            }
        case TimebaseUnits.Beats:
            {
                let t = Math.round(seconds * timebase.tempo / 60.0 * 100.0);
                let hundredths = t % 100;
                let beats = Math.floor(t / 100.0);
                let bars = Math.floor(beats / timebase.timeSignature.numerator);
                let beat = (beats - bars * timebase.timeSignature.numerator);

                return `${bars + 1}:${(beat + 1)}${tidyHundredths(hundredths)}`;
            }
            break;
        default:
            throw new Error("Unsupported timebase units");
    }
}

function timebaseEqual(a: Timebase, b: Timebase): boolean {
    if (a.units !== b.units) {
        return false;
    }
    if (a.tempo !== b.tempo) {
        return false;
    }
    if (a.timeSignature.numerator !== b.timeSignature.numerator) {
        return false;
    }
    if (a.timeSignature.denominator !== b.timeSignature.denominator) {
        return false;
    }
    return true;
}
function getAlbumLine(album: string, artist: string, albumArtist: string): string {
    if (artist === "") {
        artist = albumArtist;
    }
    let joiner = (artist !== "" && album !== "") ? " - " : "";
    return album + joiner + artist;

}

interface WidgetProps {
    noBorders: boolean;
    children: React.ReactNode;
}

const WidgetBorders = styled('div')(({ theme }) => ({

    padding: 16,
    borderRadius: 16,
    minWidth: 300,
    maxWidth: 800,
    width: "70%",
    margin: 'auto',
    position: 'relative',
    zIndex: 1,
    backgroundColor: 'rgba(255,255,255,0.4)',
    boxShadow: "1px 4px 12px rgba(0,0,0,0.2)",
    ...theme.applyStyles('dark', {
        backgroundColor: useWallpaper ? 'rgba(0,0,0,0.6)' : '#282828',
    }),
}));

const WidgetNoBorders = styled('div')(({ theme }) => ({

    padding: 16,
    borderRadius: 0,
    width: "100%",
    height: "100%",
    margin: 0,
    position: 'relative',
    zIndex: 1,
    backgroundColor: 'rgba(255,255,255,0.4)',
    ...theme.applyStyles('dark', {
        backgroundColor: useWallpaper ? 'rgba(0,0,0,0.6)' : theme.mainBackground,
    }),
}));
// const WidgetNoWallpaper = styled('div')(({ theme }) => ({

//     padding: 16,
//     borderRadius: 0,
//     width: "100%",
//     height: "100%",
//     margin: 0,
//     position: 'relative',
//     zIndex: 1,

// }));


function Widget(props: WidgetProps) {
    // if (!useWallpaper) {
    //     return(<WidgetNoWallpaper>{props.children} </WidgetNoWallpaper>);
    // }
    if (props.noBorders) {
        return (<WidgetNoBorders>{props.children} </WidgetNoBorders>);
    } else {
        return (<WidgetBorders>{props.children} </WidgetBorders>);
    }
}

const CoverImage = styled('div')({
    width: 60,
    height: 60,
    objectFit: 'cover',
    overflow: 'hidden',
    flexShrink: 0,
    borderRadius: 8,
    backgroundColor: 'rgba(0,0,0,0.08)',
    '& > img': {
        width: '100%',
    },
});



export interface ToobPlayerControlProps {
    instanceId: number;
    extraControls: React.ReactElement[];
}

export default function ToobPlayerControl(
    props: ToobPlayerControlProps
) {

    const model = PiPedalModelFactory.getInstance();
    const sampleRate = model.jackConfiguration.get().sampleRate == 0 ?
        48000 :
        model.jackConfiguration.get().sampleRate;


    const defaultCoverArt = "/img/default_album.jpg";
    const [serverConnected, setServerConnected] = React.useState(model.state.get() == State.Ready);
    const [duration, setDuration] = React.useState(0.0);
    const [start, setStart] = React.useState(0.0);
    const [loopStart, setLoopStart] = React.useState(0.0);
    const [loopEnable, setLoopEnable] = React.useState(false);
    const [loopEnd, setLoopEnd] = React.useState(0.0);
    const [pluginState, setPluginState] = React.useState(0.0);
    const [coverArt, setCoverArt] = React.useState(defaultCoverArt);
    const [audioFile, setAudioFile] = React.useState("");
    const [position, setPosition] = React.useState(0.0);
    //let position = 0;
    const [title, setTitle] = React.useState("");
    const [album, setAlbum] = React.useState("");
    const [artist, setArtist] = React.useState("");
    const [albumArtist, setAlbumArtist] = React.useState("");
    const [showFileDialog, setShowFileDialog] = React.useState(false);
    const [showLoopDialog, setShowLoopDialog] = React.useState(false);
    const [timebase, setTimebase] = React.useState<Timebase>(
        {
            units: TimebaseUnits.Seconds,
            tempo: 120.0,
            timeSignature: { numerator: 4, denominator: 4 }
        });
    const [loopParameters, setLoopParameters] = React.useState<LoopParameters>({
        start: 0.0,
        loopEnable: false,
        loopStart: 0.0,
        loopEnd: 0.0
    });


    const [size] = useWindowSize();
    const width = size.width;
    const height = size.height;

    const HORIZONTAL_CONTROL_SCROLL_HEIGHT_BREAK = 500;

    const useHorizontalLayout = height < HORIZONTAL_CONTROL_SCROLL_HEIGHT_BREAK && width > 573 && width > height;
    //const useVerticalScroll = width < 573;
    const noBorders = width < 420 || height < 720;
    let useQuadMixPanel = width < 720;
    if (noBorders) {
        useQuadMixPanel = width < 370;
    }

    function SelectFile() {
        setShowFileDialog(true);
    }
    function onAudioFileChanged(path: string) {
        setAudioFile(path);
        if (path === "") {
            setTitle("");
            setAlbum("");
            setArtist("");
            setAlbumArtist("");
            setCoverArt(defaultCoverArt);
            return;
        }
        model.getAudioFileMetadata(path)
            .then((metadata) => {
                let strTrack: string = "";
                if (metadata.track > 0) {
                    let disc = (metadata.track / 1000);
                    if (disc >= 1) {
                        strTrack = (metadata.track % 1000).toString() + '/' + Math.floor(disc).toString();
                    } else {
                        strTrack = metadata.track.toString();
                    }
                    strTrack += ". ";
                }
                let coverArtUri = getAlbumArtUri(model, metadata, path);
                setCoverArt(coverArtUri);
                setTitle(strTrack + metadata.title);
                setAlbum(metadata.album);
                setArtist(metadata.artist);
                setAlbumArtist(metadata.albumArtist);
            })
            .catch((e) => {
                setTitle("#error" + e.message);
                setAlbum("");
            });

    }
    function ControlCluster() {
        return (
            <Box
                sx={(theme) => ({
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    mt: -1,
                    '& svg': {
                        color: '#000',
                        ...theme.applyStyles('dark', {
                            color: '#fff',
                        }),
                    },
                })}
            >
                <ButtonTooltip title="Previous track">
                    <IconButton
                        style={{ opacity: (pluginState === PluginState.Idle ? 0.2 : 0.7) }}
                        onClick={() => {
                            if (position > start + 3.0) {
                                model.sendPedalboardControlTrigger(
                                    props.instanceId,
                                    "stop",
                                    1
                                );
                            } else {
                                onPreviousTrack();
                            }
                        }}
                    >
                        <FastRewind fontSize="large" />
                    </IconButton>
                </ButtonTooltip>
                <ButtonTooltip title="Play/Pause">
                    <IconButton

                        style={{ opacity: (pluginState === PluginState.Idle ? 0.2 : 0.7) }}
                        onClick={() => {
                            if (pluginState != PluginState.Idle) {
                                if (paused) {
                                    model.sendPedalboardControlTrigger(
                                        props.instanceId,
                                        "play",
                                        1
                                    );
                                } else {
                                    model.sendPedalboardControlTrigger(
                                        props.instanceId,
                                        "pause",
                                        1
                                    );
                                }
                            }
                        }}
                    >
                        {paused ? (
                            <PlayArrow sx={{ fontSize: '3rem' }} />
                        ) : (
                            <Pause sx={{ fontSize: '3rem' }} />
                        )}
                    </IconButton>
                </ButtonTooltip>
                <ButtonTooltip title="Next track">
                    <IconButton
                        style={{ opacity: (pluginState == PluginState.Idle ? 0.2 : 0.7) }}
                        onClick={() => { onNextTrack(); }}
                    >
                        <FastForward fontSize="large" />
                    </IconButton>
                </ButtonTooltip>
            </Box>

        );
    }
    function FilePanel() {
        return (
            <ButtonBase style={{ display: "block", width: "100%", borderRadius: 10, textAlign: "left" }}
                onClick={() => { SelectFile() }}
            >
                <Box sx={{ display: 'flex', alignItems: 'center', padding: "2px" }}>
                    <CoverImage>
                        <img style={{ opacity: pluginState === PluginState.Idle ? 0.3 : 1.0 }}
                            onDragStart={(e) => {
                                e.preventDefault();
                            }}
                            alt="Cover Art"
                            src={
                                coverArt
                            }
                        />
                    </CoverImage>
                    <Box sx={{ ml: 1.5, minWidth: 0 }}>
                        {pluginState === PluginState.Idle ? (
                            <Typography variant="caption" noWrap>
                                Tap to select
                            </Typography>
                        ) : (
                            <div>
                                <Typography variant="body1" noWrap>
                                    {titleLine}
                                </Typography>
                                <Typography variant="body2" noWrap sx={{}}>
                                    {albumLine}
                                </Typography>
                            </div>

                        )}
                    </Box>
                </Box>
            </ButtonBase>
        );

    }
    function loopButtonText() {
        if (start === 0 && !loopEnable) {
            return "Set loop";

        } else if (loopEnable) {
            return `${formatTimeCompact(timebase, sampleRate, start)} [${formatTimeCompact(timebase, sampleRate, loopStart)} - ${formatTimeCompact(timebase, sampleRate, loopEnd)}]`;
        } else {
            return `Start: ${formatTimeCompact(timebase, sampleRate, start)}`;
        }
    }
    function getUiFileProperty(uri: string): UiFileProperty {
        let pedalboardItem = model.pedalboard.get().getItem(props.instanceId);
        let uiPlugin = model.getUiPlugin(pedalboardItem.uri);
        if (!uiPlugin) {
            throw "uiPlugin not found.";
        }
        for (let property of uiPlugin.fileProperties) {
            if (property.patchProperty === uri) {
                return property;
            }
        }
        throw "FileProperty not found.";
    }

    function onLoopPropertyChanged(loopSettingsJson: string) {
        try {
            let atomObject = JSON.parse(loopSettingsJson);
            let loopParameters: LoopParameters = atomObject.loopParameters as LoopParameters;
            let newTimebase: Timebase | undefined = atomObject.timebase as (Timebase | undefined);;
            if (newTimebase !== undefined) {
                if (!timebaseEqual(timebase, newTimebase)) {
                    setTimebase(newTimebase);
                }
                setLoopParameters(loopParameters);
                setLoopEnable(loopParameters.loopEnable);
                setStart(loopParameters.start);
                setLoopStart(loopParameters.loopStart);
                setLoopEnd(loopParameters.loopEnd);
            } else {
                throw new Error("Invalid loop settings.");
            }
        } catch (e) {
            console.warn("Unable to parse loop settings.");
            setTimebase(
                {
                    units: TimebaseUnits.Seconds,
                    tempo: 120.0,
                    timeSignature: { numerator: 4, denominator: 4 }
                })
            setLoopEnable(false);
            setStart(0.0);
            setLoopStart(0.0);
            setLoopEnd(0.0);
        }

    }
    function onNextTrack() {
        model.getNextAudioFile(audioFile)
            .then((file) => {
                let json = JsonAtom.Path(file);
                model.setPatchProperty(
                    props.instanceId,
                    AUDIO_FILE_PROPERTY_URI,
                    json);
            })
            .catch(() => {
            });

    }
    function onPreviousTrack() {
        model.getPreviousAudioFile(audioFile)
            .then((file) => {
                let json = JsonAtom.Path(file);
                model.setPatchProperty(
                    props.instanceId,
                    AUDIO_FILE_PROPERTY_URI,
                    json);
            })
            .catch(() => {
            });
    }
    function OnSeek(value: number) {
        model.setPatchProperty(props.instanceId, Player__seek, value)
            .then(() => {
            }).catch((e) => {
                console.warn("Seek error. " + e.toString());
            });
    }
    function onStateChanged(value: State) {
        setServerConnected(value === State.Ready);
    }
    useEffect(() => {
        model.state.addOnChangedHandler(onStateChanged);
        if (model.state.get() !== State.Ready) {
            // wait for it.
            return () => {
                model.state.removeOnChangedHandler(onStateChanged);
            }
        }
        let durationHandle = model.monitorPort(props.instanceId, "duration", 1.0 / 15,
            (value) => {
                setDuration(value);
            }
        );
        let positionHandle = model.monitorPort(props.instanceId, "position", 1.0,
            (value) => {
                setPosition(value);
            }
        );

        let pluginStateHandle = model.monitorPort(props.instanceId, "state", 1.0 / 1000.0,
            (value) => {
                setPluginState(value);
            }
        );
        let filePropertyHandle = model.monitorPatchProperty(
            props.instanceId,
            AUDIO_FILE_PROPERTY_URI,
            (instanceId: number, propertyUri: string, atomObject: any) => {
                if (typeof (atomObject) === "object") {
                    let path = atomObject.value;
                    onAudioFileChanged(path);
                } else if (typeof (atomObject) === "string") {
                    onAudioFileChanged(atomObject as string);
                }
            }

        );
        let loopPropertyHandle = model.monitorPatchProperty(
            props.instanceId,
            LOOP_PROPERTY_URI,
            (instanceId: number, propertyUri: string, atomObject: any) => {
                if (typeof (atomObject) === "string") {
                    onLoopPropertyChanged(atomObject as string);
                }
            }

        );

        model.getPatchProperty(props.instanceId, AUDIO_FILE_PROPERTY_URI)
            .then((o) => {
                let path = o.value;
                onAudioFileChanged(path);
            });
        model.getPatchProperty(props.instanceId, LOOP_PROPERTY_URI)
            .then((o) => {
                onLoopPropertyChanged(o as string);
            })


        return () => {
            model.state.removeOnChangedHandler(onStateChanged);
            model.unmonitorPort(durationHandle);
            model.unmonitorPort(pluginStateHandle);
            model.unmonitorPort(positionHandle);
            // model.unmonitorPort(loopEnableHandle);
            // model.unmonitorPort(startHandle);
            // model.unmonitorPort(loopStartHandle);
            // model.unmonitorPort(loopEndHandle);
            model.cancelMonitorPatchProperty(filePropertyHandle);
            model.cancelMonitorPatchProperty(loopPropertyHandle);
        };
    },
        [serverConnected]
    );
    const titleLine = title !== "" ? title : pathFileNameOnly(audioFile);
    const albumLine = getAlbumLine(album, artist, albumArtist);

    const paused = (pluginState === PluginState.Idle
        || pluginState === PluginState.CuePlayingThenPause
        || pluginState === PluginState.Paused
        || pluginState === PluginState.Error);

    function handlePreview() {
        if (paused) {
            model.sendPedalboardControlTrigger(
                props.instanceId,
                "play",
                1
            );
        } else {
            model.sendPedalboardControlTrigger(
                props.instanceId,
                "stop",
                1
            );
        }
    }
    function handleCancelPlaying() {
        if (!paused) {
            model.sendPedalboardControlTrigger(
                props.instanceId,
                "stop",
                1
            );
        }
    }
    function LinearMixPanel() {
        return (
            <Box style={{ width: "100%" }}  >
                <div style={{ display: "flex", flex: "0 0 auto", justifyContent: "center", alignItems: 'center', flexFlow: "row nowrap" }}>
                    {props.extraControls}
                </div>
            </Box>
        );

    }
    function QuadMixPanel() {
        return (
            <div style={{ display: "flex", width: "100%", flex: "0 0 auto", justifyContent: "center", alignItems: "center", flexFlow: "row nowrap" }}>
                <div style={{ display: "flex", flex: "0 0 auto", gap: 8, alignItems: "center", flexFlow: "column nowrap" }}>
                    {props.extraControls[0]}
                    {props.extraControls[1]}
                </div>
                <div style={{ display: "flex", flex: "0 0 auto", gap: 8, alignItems: "center", flexFlow: "column nowrap" }}>
                    {props.extraControls[2]}
                    {props.extraControls[3]}
                </div>

            </div>
        );
    }
    function SliderCluster() {
        return (

            <div style={{ position: "relative", display: "flex", flexFlow: "column nowrap" }}>
                <div style={{ marginBottom: 16 }}>
                    <ControlSlider
                        instanceId={props.instanceId}
                        controlKey="position"
                        duration={duration}
                        onPreviewValue={(value) => {
                        }}
                        onValueChanged={(value) => {
                            OnSeek(value);
                        }}
                    />
                </div>
                <div
                    style={{
                        position: "absolute",
                        width: "100%",
                        display: "flex", flexFlow: "row nowrap", alignItems: "center",
                        justifyContent: "center",
                        bottom: 0
                    }}
                >
                    {/**
                     * 
                        sx={{
                            '& .MuiTouchRipple-root': {
                            },
                            '& .MuiTouchRipple-ripple': {
                                transform: 'scale(1.9) !important',
                            }
                        }}
                     */}
                    <ButtonEx tooltip="Loop Settings" variant="dialogSecondary"
                        style={{
                            flex: "0 1 auto", maxWidth: 200,
                            textTransform: "none", padding: "4px 8px"
                        }}
                        startIcon={(<RepeatIcon />)}
                        onClick={() => {
                            setShowLoopDialog(true);
                        }}
                    >
                        <Typography noWrap variant="caption">
                            {loopButtonText()
                            }
                        </Typography>
                    </ButtonEx>
                </div>
            </div >
        );
    }


    function VerticalWidget() {
        return (
            <div style={{
                position: "relative", width: "100%", height: "100%",
                display: "flex", flexFlow: "column nowrap"
            }} >
                <div style={{ flex: "1 1 1px" }} />

                <Widget noBorders={noBorders}>
                    {
                        FilePanel()
                    }
                    {SliderCluster()}
                    {
                        ControlCluster()
                    }
                    <Divider style={{ marginTop: 8, marginBottom: 8 }} />
                    {useQuadMixPanel ? QuadMixPanel()
                        :
                        LinearMixPanel()
                    }
                </Widget>
                <div style={{ flex: "2 2 1px" }} />
            </div>
        );
    }

    function HorizontalWidget() {
        return (
            <div style={{
                position: "relative", width: "100%", height: "100%",
                display: "flex", flexFlow: "column nowrap"
            }} >

                <Widget noBorders={true}>
                    <div style={{
                        display: "flex", flexFlow: "row nowrap", alignItems: "center", justifyContent: "stretch",
                        width: "100%"
                    }}>
                        <div style={{
                            display: "flex", alignItems: "stretch", flexFlow: "column nowrap", flex: "1 1 100%"

                        }}>
                            <div style={{ display: "flex", flexFlow: "row nowrap", flex: "1 1 auto" }}>
                                <div style={{ position: "relative", display: "flex", flexFlow: "row nowrap", flex: "1 1 auto" }}>
                                    {
                                        FilePanel()
                                    }
                                </div>
                            </div>
                            <div style={{ flex: "0 0 auto" }}>
                                {
                                    SliderCluster()
                                }
                            </div>
                        </div>
                        <div style={{
                            position: "relative", display: "flex", flexFlow: "row nowrap", flex: "0 0  auto"

                        }}>
                            {
                                ControlCluster()
                            }
                        </div>
                        <div style={{ flex: "0 0 auto" }}>
                            {LinearMixPanel()}
                        </div>
                    </div>


                </Widget>
                <div style={{ flex: "2 2 1px" }} />
            </div >
        );
    }
    return (
        <div id="toobPlayerViewFrame" style={{
            width: '100%', height: "100%", overflow: 'hidden', position: 'relative',
        }}>
            {useWallpaper && (<WallPaper />)}
            {
                useHorizontalLayout ?
                    HorizontalWidget() : VerticalWidget()
            }
            {showLoopDialog && (
                <LoopDialog isOpen={showLoopDialog}
                    sampleRate={
                        sampleRate
                    }
                    playing={!paused}
                    onPreview={() => { handlePreview(); }}
                    value={loopParameters}
                    onCancelPlaying= {()=> { handleCancelPlaying();}}
                    timebase={timebase}
                    onTimebaseChange={(newTimebase) => {
                        setTimebase(newTimebase);
                        // model.setPatchProperty(
                        //     props.instanceId,
                        //     "timebase",
                        //     newTimebase
                        // );
                    }}

                    onClose={() => {
                        setShowLoopDialog(false);
                    }}
                    onSetLoop={(loop: LoopParameters) => {
                        let loopSettings = {
                            timebase: timebase,
                            loopParameters: loop
                        };
                        model.setPatchProperty(
                            props.instanceId,
                            LOOP_PROPERTY_URI,
                            JSON.stringify(loopSettings));

                        setStart(loop.start);
                        setLoopEnable(loop.loopEnable);
                        setLoopStart(loop.loopStart);
                        setLoopEnd(loop.loopEnd);
                    }}
                    duration={duration}
                />
            )}
            {showFileDialog && (
                <FilePropertyDialog open={showFileDialog}
                    fileProperty={getUiFileProperty(AUDIO_FILE_PROPERTY_URI)}
                    instanceId={props.instanceId}
                    selectedFile={audioFile}
                    onCancel={() => {
                        setShowFileDialog(false);
                    }}
                    onApply={(fileProperty, selectedFile) => {
                        model.setPatchProperty(
                            props.instanceId,
                            AUDIO_FILE_PROPERTY_URI,
                            JsonAtom.Path(selectedFile)
                        )
                            .then(() => {

                            })
                            .catch((error) => {
                                model.showAlert("Unable to complete the operation. " + error);
                            });

                    }}
                    onOk={(fileProperty, selectedFile) => {

                        model.setPatchProperty(
                            props.instanceId,
                            fileProperty.patchProperty,
                            JsonAtom.Path(selectedFile)
                        )
                            .then(() => {

                            })
                            .catch((error) => {
                                model.showAlert("Unable to complete the operation. " + error);
                            });
                        setShowFileDialog(false);
                    }
                    }
                />

            )}
        </div>
    );
}