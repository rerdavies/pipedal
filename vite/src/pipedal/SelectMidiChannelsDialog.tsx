// Copyright (c) 2022 Robin Davies
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

import Checkbox from '@mui/material/Checkbox';
import { AlsaSequencerConfiguration, AlsaSequencerPortSelection } from './AlsaSequencer';
import DialogEx from './DialogEx';

import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';

export interface SelectMidiChannelsDialogProps {
    open: boolean;
    onClose: () => void;
}

interface DialogItem {
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
    const [allPorts, setAllPorts] = useState<DialogItem[] | null>(null);
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
            let result: DialogItem[] = [];
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
                            sortOrder: port.sortOrder,
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

    const isChecked = (value: DialogItem) => {
        if (availablePorts === null || configuration === null) {
            return false;
        }
        return configuration.connections.some((port) => port.id === value.id);
    };
    const setChecked = (value_: DialogItem, checked: boolean) => {
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
        newConfiguration.connections = newConnections;
        setConfiguration(newConfiguration);
    };
    let toggleSelect = (value: DialogItem) => {
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


    return (
        <DialogEx tag="midiChannels" onClose={handleClose} aria-labelledby="select-midi-inputs" 
            open={open && readyToDisplay}
            fullWidth maxWidth="xs"
            onEnterKey={handleClose}
        >
            <DialogTitle id="select-midi-inputs">Select MIDI Inputs</DialogTitle>
            <DialogContent dividers>
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
