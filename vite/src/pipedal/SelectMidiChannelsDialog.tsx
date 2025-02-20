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
import ListItemButton from '@mui/material/ListItemButton';
import DialogTitle from '@mui/material/DialogTitle';
import DialogActions from '@mui/material/DialogActions';
import FormControlLabel from '@mui/material/FormControlLabel';

import Checkbox from '@mui/material/Checkbox';
import {AlsaMidiDeviceInfo} from './AlsaMidiDeviceInfo';
import DialogEx from './DialogEx';



export interface SelectMidiChannelsDialogProps {
    open: boolean;
    selectedChannels: AlsaMidiDeviceInfo[];
    availableChannels: AlsaMidiDeviceInfo[];
    onClose: (selectedChannels: AlsaMidiDeviceInfo[] | null) => void;
}

function isChecked(selectedChannels: AlsaMidiDeviceInfo[], channel: AlsaMidiDeviceInfo): boolean {
    for (let i = 0; i < selectedChannels.length; ++i) {
        if (selectedChannels[i].equals(channel)) return true;
    }
    return false;
}
function addPort(availableChannels: AlsaMidiDeviceInfo[], selectedChannels: AlsaMidiDeviceInfo[], newChannel: AlsaMidiDeviceInfo)
:AlsaMidiDeviceInfo[]
 {
    let result: AlsaMidiDeviceInfo[] = [];
    for (let i = 0; i < availableChannels.length; ++i) {
        let channel = availableChannels[i];
        if (isChecked(selectedChannels, channel) || channel.equals(newChannel)) {
            result.push(channel);
        }
    }
    return result;
}

function removePort(selectedChannels: AlsaMidiDeviceInfo[], channel: AlsaMidiDeviceInfo) 
:AlsaMidiDeviceInfo[]
{
    let result: AlsaMidiDeviceInfo[] = [];
    for (let i = 0; i < selectedChannels.length; ++i) {
        if (!selectedChannels[i].equals(channel)) {
            result.push(selectedChannels[i]);
        }
    }
    return result;
}

function SelectMidiChannelsDialog(props: SelectMidiChannelsDialogProps) {
    //const classes = useStyles();
    const { onClose, selectedChannels, availableChannels, open } = props;
    const [currentSelection, setCurrentSelection] = useState(selectedChannels);



    let toggleSelect = (value: AlsaMidiDeviceInfo) => {
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
        <DialogEx tag="midiChannels" onClose={handleClose} aria-labelledby="select-channels-title" open={open}
            onEnterKey={handleOk}
        >
            <DialogTitle id="simple-dialog-title">Select MIDI Device</DialogTitle>
            <List>
                {availableChannels.map((channel) => (
                    <ListItemButton>
                        <FormControlLabel
                            control={
                                <Checkbox checked={isChecked(currentSelection, channel)} onClick={() => toggleSelect(channel)} key={channel.name} />
                            }
                            label={channel.description}
                        />
                    </ListItemButton>
                )

                )}
            </List>
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
