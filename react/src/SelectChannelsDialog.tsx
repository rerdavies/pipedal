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

import { useState } from 'react';
import Button from '@mui/material/Button';
import List from '@mui/material/List';
import ListItem from '@mui/material/ListItem';
import ListItemText from '@mui/material/ListItemText';
import DialogTitle from '@mui/material/DialogTitle';
import DialogActions from '@mui/material/DialogActions';
import Dialog from '@mui/material/Dialog';
import FormControlLabel from '@mui/material/FormControlLabel';

import Checkbox from '@mui/material/Checkbox';



export interface SelectChannelsDialogProps {
    open: boolean;
    selectedChannels: string[];
    availableChannels: string[];
    onClose: (selectedChannels: string[] | null) => void;
}

function isChecked(selectedChannels: string[], channel: string): boolean {
    for (let i = 0; i < selectedChannels.length; ++i) {
        if (selectedChannels[i] === channel) return true;
    }
    return false;
}
function addPort(availableChannels: string[], selectedChannels: string[], newChannel: string) {
    let result: string[] = [];
    for (let i = 0; i < availableChannels.length; ++i) {
        let channel = availableChannels[i];
        if (isChecked(selectedChannels, channel) || channel === newChannel) {
            result.push(channel);
        }
    }
    return result;
}

function removePort(selectedChannels: string[], channel: string) {
    let result: string[] = [];
    for (let i = 0; i < selectedChannels.length; ++i) {
        if (selectedChannels[i] !== channel) {
            result.push(selectedChannels[i]);
        }
    }
    return result;
}

function SelectChannelsDialog(props: SelectChannelsDialogProps) {
    //const classes = useStyles();
    const { onClose, selectedChannels, availableChannels, open } = props;
    const [currentSelection, setCurrentSelection] = useState(selectedChannels);


    let showCheckboxes = availableChannels.length > 2;
    if (showCheckboxes) {

        let toggleSelect = (value: string) => {
            if (!isChecked(currentSelection, value)) {
                setCurrentSelection(addPort(availableChannels, currentSelection, value));
            } else {
                setCurrentSelection(removePort(currentSelection, value));
            }
        };

        const handleClose = (): void => {
            onClose(null);
        };
        const handleOk = (): void => {
            onClose(currentSelection);
        };


        return (
            <Dialog onClose={handleClose} aria-labelledby="select-channels-title" open={open}>
                <DialogTitle id="simple-dialog-title">Select Channels</DialogTitle>
                <List>
                    {availableChannels.map((channel) => (
                        <ListItem button >
                            <FormControlLabel
                                control={
                                    <Checkbox checked={isChecked(currentSelection, channel)} onClick={() => toggleSelect(channel)} key={channel} />
                                }
                                label={channel}
                            />
                        </ListItem>
                    )

                    )}
                </List>
                <DialogActions>
                    <Button onClick={handleClose} color="primary">
                        Cancel
                    </Button>
                    <Button onClick={handleOk} color="primary" disabled={currentSelection.length === 0 || currentSelection.length > 2} >
                        OK
                    </Button>
                </DialogActions>
            </Dialog>
        );

    } else {

        const handleClose = () => {
            onClose(selectedChannels);
        };

        const handleListItemClick = (value: string) => {
            switch (value) {
                case "Stereo":
                    onClose(availableChannels);
                    break;
                case "Left":
                    onClose([availableChannels[0]]);
                    break;
                case "Right":
                    onClose([availableChannels[1]]);
                    break;
                case "None":
                default:
                    onClose([availableChannels[1]]);
            }
        };
        let selectionKey = "None";
        if (currentSelection.length === 2) {
            selectionKey = "Stereo";
        } else if (currentSelection.length === 1) {
            if (currentSelection[0] === availableChannels[0]) {
                selectionKey = "Left";
            } else {
                selectionKey = "Right";
            }
        }

        return (
            <Dialog onClose={handleClose} aria-labelledby="select-channels-title" open={open}>
                
                <List style={{ marginLeft: 0, marginRight: 0}}>
                    <ListItem button onClick={() => handleListItemClick("Stereo")} key={"Stereo"} selected={selectionKey === "Stereo"} >
                        <ListItemText primary={"Stereo"} />
                    </ListItem>
                    <ListItem button onClick={() => handleListItemClick("Left")} key={"LeftChannel"} selected={selectionKey === "Left"} >
                        <ListItemText primary={"Mono (left channel only)"} />
                    </ListItem>
                    <ListItem button onClick={() => handleListItemClick("Right")} key={"RightChannel"} selected={selectionKey === "Right"}>
                        <ListItemText primary={"Mono (right channel only"} />
                    </ListItem>
                </List>
            </Dialog>
        );
    }
}

export default SelectChannelsDialog;
