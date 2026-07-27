// Copyright (c) 2026 Robin Davies
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
import { useState } from 'react';
import Button from '@mui/material/Button';
import List from '@mui/material/List';
import ListItemButton from '@mui/material/ListItemButton';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import FormControlLabel from '@mui/material/FormControlLabel';
import Typography from '@mui/material/Typography';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';

import Checkbox from '@mui/material/Checkbox';
import { AlsaSequencerConfiguration, AlsaSequencerPortSelection } from './AlsaSequencer';
import DialogEx from './DialogEx';

import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';

export interface SelectMidiChannelsDialogProps {
    open: boolean;
    onClose: () => void;
}

interface DeviceListItem {
    id: string;
    name: string;
    sortOrder: number
    offline: boolean;
};

function SelectMidiChannelsDialog(props: SelectMidiChannelsDialogProps) {
    //const classes = useStyles();
    const { open, onClose } = props;
    const [availablePorts, setAvailablePorts] = useState<AlsaSequencerPortSelection[] | null>(null);
    const [configuration, setConfiguration] = useState<AlsaSequencerConfiguration | null>(null);
    const [allPorts, setAllPorts] = useState<DeviceListItem[] | null>(null);
    const [model] = useState<PiPedalModel>(PiPedalModelFactory.getInstance());
    const [changed, setChanged] = useState<boolean>(false);
    const [ readyToDisplay, setReadyToDisplay ] = useState<boolean>(false);


    React.useEffect(() => {
        if (open) {
            setReadyToDisplay(false);
            model.getAlsaSequencerPorts().then((ports) => {
                setAvailablePorts(ports);
            }).catch((error) => {
                model.showAlert(error);
                setReadyToDisplay(true);
                setAvailablePorts(null);
            });
            model.getAlsaSequencerConfiguration().then((config) => {
                setConfiguration(config);
            }).catch((error) => {
                model.showAlert(error);
                setReadyToDisplay(true);
                setConfiguration(null);
            });
            return () => {
            }
        } else {
            return () => { };
        }
    }, [open]);
    React.useEffect(() => {
        if (availablePorts !== null && configuration !== null) {
            let result: DeviceListItem[] = [];
            setReadyToDisplay(true);
            for (let port of availablePorts) {
                result.push({
                    id: port.id,
                    name: port.name,
                    sortOrder: port.sortOrder,
                    offline: false
                });
            }

            // include ports that have been previously selected but are not in the current list of available ports
            for (let port of configuration.connections) {
                if (!availablePorts.some((p) => p.id === port.id)) {
                    result.push(
                        {
                            id: port.id,
                            name: port.name,
                            sortOrder: port.sortOrder+100,
                            offline: true
                        }
                    );
                }
            }
            result.sort((a, b) => {
                return a.sortOrder - b.sortOrder;
            });
            setAllPorts(result);
        } else {
            setAllPorts(null);
        }

    }, [availablePorts, configuration]);

    const isChecked = (value: DeviceListItem) => {
        if (availablePorts === null || configuration === null) {
            return false;
        }
        return configuration.connections.some((port) => port.id === value.id);
    };
    const setChecked = (value_: DeviceListItem, checked: boolean) => {
        if (availablePorts === null || configuration === null) {
            return;
        }
        let value = new AlsaSequencerPortSelection();
        value.id = value_.id;
        value.name = value_.name;
        value.sortOrder = value_.sortOrder;
        let newConnections = configuration.connections.slice();
        if (checked) {
            newConnections.push(value);
        } else {
            newConnections = newConnections.filter((port) => port.id !== value.id);
        }
        let newConfiguration = new AlsaSequencerConfiguration();
        newConfiguration.midiChannel = configuration.midiChannel;
        newConfiguration.connections = newConnections;
        setConfiguration(newConfiguration);
    };
    let toggleSelect = (value: DeviceListItem) => {
        if (availablePorts === null || configuration === null) {
            return;
        }
        if (!isChecked(value)) {
            setChecked(value, true);
        } else {
            setChecked(value, false);
        }
        setChanged(true);
    };

    const handleClose = (): void => {
        onClose();
    };
    const handleOk = (): void => {
        if (changed && configuration !== null) {
            model.setAlsaSequencerConfiguration(configuration);
        }
        onClose();
    };
    const handleChannelChanged = (channel: number) => {
        if (configuration !== null) {
            let newConfiguration = new AlsaSequencerConfiguration();
            newConfiguration.midiChannel = channel;
            newConfiguration.connections = configuration.connections.slice();
            setConfiguration(newConfiguration);
            setChanged(true);
        }
    }


    return (
        <DialogEx tag="midiChannels" onClose={handleClose} aria-labelledby="select-midi-inputs" 
            open={open && readyToDisplay}
            fullWidth maxWidth="xs"
            onEnterKey={handleClose}
        >
            <DialogTitle id="select-midi-inputs">Select MIDI Inputs</DialogTitle>
            <DialogContent dividers>
                <div style={{ marginLeft: 16, marginRight: 16, marginTop: 8, marginBottom: 8 }}>
                    <Typography display="block" variant="caption">MIDI Channel</Typography>
                    <Select variant='standard' value={configuration? configuration.midiChannel.toString(): ""} style={{ width: 100 }}
                        sx={{ '& .MuiSelect-select': { textAlign: 'right' } }} disabled={!readyToDisplay} 
                        onChange={(event) => handleChannelChanged(parseInt(event.target.value))}>
                        <MenuItem key={-1} value={-1} sx={{ justifyContent: 'flex-end' }}>OMNI</MenuItem>
                        <MenuItem key={0} value={0} sx={{ justifyContent: 'flex-end' }}>0</MenuItem>
                        <MenuItem key={1} value={1} sx={{ justifyContent: 'flex-end' }}>1</MenuItem>
                        <MenuItem key={2} value={2} sx={{ justifyContent: 'flex-end' }}>2</MenuItem>
                        <MenuItem key={3} value={3} sx={{ justifyContent: 'flex-end' }}>3</MenuItem>
                        <MenuItem key={4} value={4} sx={{ justifyContent: 'flex-end' }}>4</MenuItem>
                        <MenuItem key={5} value={5} sx={{ justifyContent: 'flex-end' }}>5</MenuItem>
                        <MenuItem key={6} value={6} sx={{ justifyContent: 'flex-end' }}>6</MenuItem>
                        <MenuItem key={7} value={7} sx={{ justifyContent: 'flex-end' }}>7</MenuItem>
                        <MenuItem key={8} value={8} sx={{ justifyContent: 'flex-end' }}>8</MenuItem>
                        <MenuItem key={9} value={9} sx={{ justifyContent: 'flex-end' }}>9</MenuItem>
                        <MenuItem key={10} value={10} sx={{ justifyContent: 'flex-end' }}>10</MenuItem>
                        <MenuItem key={11} value={11} sx={{ justifyContent: 'flex-end' }}>11</MenuItem>
                        <MenuItem key={12} value={12} sx={{ justifyContent: 'flex-end' }}>12</MenuItem>
                        <MenuItem key={13} value={13} sx={{ justifyContent: 'flex-end' }}>13</MenuItem>
                        <MenuItem key={14} value={14} sx={{ justifyContent: 'flex-end' }}>14</MenuItem>
                        <MenuItem key={15} value={15} sx={{ justifyContent: 'flex-end' }}>15</MenuItem>
                    </Select>
                </div>
                <List>
                    {allPorts !== null && allPorts.length === 0 && (
                        <Typography variant="body2" style={{ marginLeft: 32, marginRight: 24, marginTop: 8, marginBottom: 16 }}>
                            No MIDI devices found.
                        </Typography>)}
                    {allPorts != null && allPorts.map((port) => (
                        <ListItemButton key={port.id}>
                            <FormControlLabel
                                control={
                                    <Checkbox
                                        checked={isChecked(port)}
                                        onClick={() => toggleSelect(port)} />
                                }
                                label={
                                    (
                                    <div style={{ display: "flex", flexFlow: "row nowrap", alignItems: "center" }}>
                                        <Typography 
                                            color={ port.offline ? "textSecondary" : "textPrimary" }
                                            noWrap variant="body2"
                                            style={{ flex: "1 1 auto" }}
                                        >
                                            {port.name}
                                        </Typography>
                                        {port.offline && (
                                            <Typography color="textSecondary" variant="body2" >
                                                (offline)
                                            </Typography>
                                        )}
                                    </div>
                                    )}                                        
                            />
                        </ListItemButton>
                    )

                    )}
                </List>
            </DialogContent>
            <DialogActions>
                <Button onClick={handleClose} variant="dialogSecondary" >
                    Cancel
                </Button>
                <Button onClick={handleOk} variant="dialogPrimary"   >
                    OK
                </Button>
            </DialogActions>
        </DialogEx>
    );
}

export default SelectMidiChannelsDialog;
