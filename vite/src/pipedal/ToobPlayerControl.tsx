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
import Button from '@mui/material/Button';
import Slider from '@mui/material/Slider';
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
import { LoopParameters, TimebaseUnits } from './Timebase';

let Player__seek = "http://two-play.com/plugins/toob-player#seek"
const AUDIO_FILE_PROPERTY_URI = "http://two-play.com/plugins/toob-player#audioFile";
class PluginState {
    // Must match PluginState values in ToobPlayer.hpp
    static Idle = 0.0;
    static CuePlaying = 1;
    static CuePlayPaused = 2.0;
    static Pausing = 3;
    static Paused = 4;
    static Playing = 5;
    static Error = 6;
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

function getAlbumLine(album: string, artist: string, albumArtist: string): string  {
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
        backgroundColor: useWallpaper?  'rgba(0,0,0,0.6)' : '#282828',
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

const TinyText = styled(Typography)({
    fontSize: '0.85rem',
    opacity: 0.9,
    fontWeight: 400,
    letterSpacing: 0.2,
});


export interface ToobPlayerControlProps {
    instanceId: number;
    extraControls: React.ReactElement[];
}

export default function ToobPlayerControl(
    props: ToobPlayerControlProps
) {

    const model = PiPedalModelFactory.getInstance();

    const defaultCoverArt = "/img/default_album.jpg";
    const [serverConnected, setServerConnected] = React.useState(model.state.get() == State.Ready);
    const [duration, setDuration] = React.useState(0.0);
    const [position, setPosition] = React.useState(0.0);
    const [start, setStart] = React.useState(0.0);
    const [loopStart, setLoopStart] = React.useState(0.0);
    const [loopEnable, setLoopEnable] = React.useState(false);
    const [loopEnd, setLoopEnd] = React.useState(0.0);
    const [dragging, setDragging] = React.useState(false);
    const [pluginState, setPluginState] = React.useState(0.0);
    const [sliderValue, setSliderValue] = React.useState(0.0);
    const [coverArt, setCoverArt] = React.useState(defaultCoverArt);
    const [audioFile, setAudioFile] = React.useState("");
    const [title, setTitle] = React.useState("");
    const [album, setAlbum] = React.useState("");
    const [artist, setArtist] = React.useState("");
    const [albumArtist, setAlbumArtist] = React.useState("");
    const [showFileDialog, setShowFileDialog] = React.useState(false);
    const [showLoopDialog, setShowLoopDialog] = React.useState(false);
    const [timebase,setTimebase] = React.useState(
        {
            units: TimebaseUnits.Seconds,
            tempo: 120.0,
            timeSignature: { numerator: 4, denominator: 4 }
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
    function formatDuration(value_: number) {
        let value = Math.ceil(value_);
        const minute = Math.floor(value / 60);
        const secondLeft = value - minute * 60;
        return `${minute}:${secondLeft < 10 ? `0${secondLeft}` : secondLeft}`;
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
                    let disc = (metadata.track /1000);
                    if (disc >= 1) {
                        strTrack = (metadata.track % 1000).toString() + '/' + Math.floor(disc).toString();
                    } else {
                        strTrack = metadata.track.toString();
                    }
                    strTrack += ". ";
                }
                let coverArtUri = getAlbumArtUri(model,metadata,path);
                setCoverArt(coverArtUri);
                setTitle(strTrack + metadata.title);
                setAlbum(metadata.album);
                setArtist(metadata.artist);
                setAlbumArtist(metadata.albumArtist);
            })
            .catch((e)=>{
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
                            if (position > 3) {
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
            return  `${formatDuration(start)} [${formatDuration(loopStart)} - ${formatDuration(loopEnd)}]`;   
        } else {
            return `Start: ${formatDuration(start)}`;
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
        setDragging(true);
        model.setPatchProperty(props.instanceId, Player__seek, value)
            .then(() => {
                setPosition(value);
                setDragging(false);
            }).catch((e) => {
                console.warn("Seek error. " + e.toString());
                setPosition(value);
                setDragging(false);
            });
        setSliderValue(value);
    }
    function onStateChanged(value: State) {
        setServerConnected(value === State.Ready);
    }
    useEffect(() => {
        model.state.addOnChangedHandler(onStateChanged);
        if (model.state.get() !== State.Ready) {
            // wait for it.
            return () => {
                model.state.removeOnChangedHandler
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
        // let startHandle = model.monitorPort(props.instanceId, "start", 1.0,
        //     (value) => {
        //         setStart(value);
        //     }
        // );
        // let loopStartHandle = model.monitorPort(props.instanceId, "loopStart", 1.0,
        //     (value) => {
        //         setLoopStart(value);
        //     }
        // );
        // let loopEnableHandle = model.monitorPort(props.instanceId, "loopEnable", 1.0,
        //     (value) => {
        //         setLoopEnable(value != 0);
        //     }
        // );
        // let loopEndHandle = model.monitorPort(props.instanceId, "loopEnd", 1.0,
        //     (value) => {
        //         setLoopEnd(value);
        //     }
        // );
        
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
                } else if (typeof(atomObject) === "string")
                {
                    onAudioFileChanged(atomObject as string);
                }
            }

        );

        model.getPatchProperty(props.instanceId, AUDIO_FILE_PROPERTY_URI)
            .then((o) => {
                let path = o.value;
                onAudioFileChanged(path);
            })


        return () => {
            model.state.removeOnChangedHandler(onStateChanged);
            model.unmonitorPort(durationHandle);
            model.unmonitorPort(positionHandle);
            model.unmonitorPort(pluginStateHandle);
            // model.unmonitorPort(loopEnableHandle);
            // model.unmonitorPort(startHandle);
            // model.unmonitorPort(loopStartHandle);
            // model.unmonitorPort(loopEndHandle);
            model.cancelMonitorPatchProperty(filePropertyHandle);
        };
    },
        [serverConnected]
    );
    const effectivePosition =
        (pluginState !== PluginState.Idle) ? Math.min(position, duration) : 0.0;
    const titleLine = title !== "" ? title : pathFileNameOnly(audioFile);
    const albumLine = getAlbumLine(album,artist,albumArtist);

    const paused = (pluginState === PluginState.Idle
        || pluginState === PluginState.CuePlayPaused
        || pluginState === PluginState.Paused);

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

            <div style={{ display: "flex", flexFlow: "column nowrap" }}>
                <Slider
                    value={dragging ? sliderValue : effectivePosition}
                    min={0}
                    step={1}
                    max={duration}
                    onChange={(_, val) => {
                        let v = val as number;
                        setPosition(v);
                        setSliderValue(v);
                    }}
                    onChangeCommitted={(e, value) => {
                        let v = value as number;
                        OnSeek(v);
                    }}
                    onPointerDown={(e) => {
                        setDragging(true);
                    }}
                    onPointerUp={(e) => {
                        // setDragging(false);
                    }}
                    disabled={duration == 0}
                    sx={(t) => ({
                        width: "100%",
                        color: 'rgba(0,0,0,0.87)',
                        height: 4,
                        '& .MuiSlider-thumb': {
                            width: 8,
                            height: 8,
                            transition: '0.3s cubic-bezier(.47,1.64,.41,.8)',
                            '&::before': {
                                boxShadow: '0 2px 12px 0 rgba(0,0,0,0.4)',
                            },
                            '&:hover, &.Mui-focusVisible': {
                                boxShadow: `0px 0px 0px 8px ${'rgb(0 0 0 / 16%)'}`,
                                ...t.applyStyles('dark', {
                                    boxShadow: `0px 0px 0px 8px ${'rgb(255 255 255 / 16%)'}`,
                                }),
                            },
                            '&.Mui-active': {
                                width: 20,
                                height: 20,
                            },
                        },
                        '& .MuiSlider-rail': {
                            opacity: 0.28,
                        },
                        ...t.applyStyles('dark', {
                            color: '#fff',
                        }),
                    })}
                />
                <div
                    style={{
                        flex: "0 0 auto",
                        width: "100%",
                        display: 'flex',
                        alignItems: 'top',
                        justifyContent: 'space-between',
                        marginTop: -4,
                    }}
                >
                    <TinyText>{formatDuration(dragging ? sliderValue : effectivePosition)}</TinyText>
                    <Button title="Loop" variant="dialogSecondary" 
                        style={{flex: "0 1 auto",width: 200,
                            textTransform: "none", fontSize: "0.9rem", fontWeight: 500, padding: "4px 8px"
                        }}
                        startIcon= {(<RepeatIcon />)}
                        onClick={() => {
                            setShowLoopDialog(true);
                        }}
                        >
                        {loopButtonText()
                        }
                    </Button>
                    <TinyText>{formatDuration(duration)}</TinyText>
                </div>
            </div>
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
                        model.jackConfiguration.get().sampleRate == 0? 
                            96000 : 
                            model.jackConfiguration.get().sampleRate
                    }
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
                    onSetLoop={(start, loopEnable,loopStart, loopEnd) => {

                        let loop: LoopParameters = { start: start, 
                            loopEnable: loopEnable, 
                            loopStart: loopStart, 
                            loopEnd: loopEnd };    

                        let loopSettings = {
                            timebase: timebase,
                            loopParameters: loop
                        };
                        model.setPatchProperty(
                            props.instanceId,
                            "http://two-play.com/plugins/toob-player#loopSettings",
                            loopSettings.toString());

                        setStart(start);
                        // model.setPedalboardControl(
                        //     props.instanceId,
                        //     "start",
                        //     start);
                        setLoopEnable(loopEnable);
                        // model.setPedalboardControl(
                        //     props.instanceId,
                        //     "loopEnable",
                        //     loopEnable? 1.0: 0.0);
                        setLoopStart(loopStart);
                        // model.setPedalboardControl(
                        //     props.instanceId,
                        //     "loopStart",
                        //     loopStart);
                        setLoopEnd(loopEnd);
                        // model.setPedalboardControl(
                        //     props.instanceId,
                        //     "loopEnd",
                        //     loopEnd     
                        // );
                    }}
                    start={start}
                    loopStart={loopStart}
                    loopEnable={loopEnable}
                    loopEnd={loopEnd}
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