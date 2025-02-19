/* eslint-disable @typescript-eslint/no-unused-vars */

// Copyright (c) 2024 Robin E. R. Davies
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
import DialogTitle from '@mui/material/DialogTitle';
import DialogActions from '@mui/material/DialogActions';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import FormControlLabel from '@mui/material/FormControlLabel';
import ChannelBindingHelpDialog from './ChannelBindingsHelpDialog';

import IconButton from '@mui/material/IconButton';
import InfoOutlinedIcon from '@mui/icons-material/InfoOutlined';

import Checkbox from '@mui/material/Checkbox';
import { AlsaMidiDeviceInfo } from './AlsaMidiDeviceInfo';
import DialogEx from './DialogEx';
import MidiChannelBinding, { MidiDeviceSelection } from './MidiChannelBinding';
import DialogContent from '@mui/material/DialogContent';
import Typography from '@mui/material/Typography';
import Select, { SelectChangeEvent } from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';



export interface MidiChannelBindingDialogProps {
    open: boolean;
    midiChannelBinding: MidiChannelBinding;
    onClose: () => void;
    onChanged: (midiChannelBinding: MidiChannelBinding) => void;
    midiDevices: AlsaMidiDeviceInfo[];
}



function MidiChannelBindingDialog(props: MidiChannelBindingDialogProps) {
    const { onClose, midiChannelBinding, open } = props;

    const [currentChannel, setCurrentChannel] = useState(midiChannelBinding.channel);
    const [allowProgramChanges, setAllowProgramChanges] = useState(midiChannelBinding.acceptCommonMessages);
    const [helpDialog, setHelpDialog] = useState(false);

    const model: PiPedalModel = PiPedalModelFactory.getInstance();

    const handleClose = (): void => {
        onClose();
    };
    const handleOk = (): void => {
        onClose();
    };

    const generateDeviceSelect = () => {
        let result: React.ReactElement[] = [
            (<MenuItem key={0} value={0}>Any</MenuItem>),
            (<MenuItem key={1} value={1}>None</MenuItem>)
        ];
        let i = 2;
        for (let midiDevice of model.jackConfiguration.get().inputMidiDevices) {
            result.push((<MenuItem key={i} value={i}>{midiDevice.description}</MenuItem>));
            ++i;
        }
        return result;
    };

    const getDefaultDeviceSelect = (): number => {
        if (midiChannelBinding.deviceSelection === MidiDeviceSelection.DeviceAny as number) {
            return 0;
        }
        if (midiChannelBinding.deviceSelection === MidiDeviceSelection.DeviceNone as number) {
            return 1;
        }
        if (midiChannelBinding.midiDevices.length === 0) {
            return 0;
        }
        let selectedDevice = midiChannelBinding.midiDevices[0];

        let ix = 2;
        for (let midiDevice of model.jackConfiguration.get().inputMidiDevices) {
            if (midiDevice.name === selectedDevice) {
                return ix;
            }
            ++ix;
        }
        return 0; // any.
    }

    const [currentDeviceSelect, setCurrentDeviceSelect] = useState(getDefaultDeviceSelect())


    const handleDeviceSelecChange = (event: SelectChangeEvent) => {
        setCurrentDeviceSelect(parseInt(event.target.value));
    };
    const handleChannelChange = (event: SelectChangeEvent) => {
        setCurrentChannel(parseInt(event.target.value));
    };

    return (
        <DialogEx tag="midiChannelBinding" onClose={handleClose} aria-labelledby="select-channels-title" open={open}
            onEnterKey={()=>{}} fullWidth maxWidth="xs"
        >
            <DialogTitle id="simple-dialog-title">MIDI Channel Filters</DialogTitle>
            <DialogContent>
                <div style={{
                    display: "grid", alignItems: "center", columnGap: 24, rowGap: 24,
                    gridTemplateColumns: "1fr"
                }}>
                    <div >
                        <Typography display="block" variant="caption">MIDI Devices</Typography>
                        <Select variant='standard' value={currentDeviceSelect.toString()} fullWidth
                            onChange={handleDeviceSelecChange} >
                            {
                                generateDeviceSelect()
                            }
                        </Select>
                    </div>

                    <div>
                        <Typography display="block" variant="caption">Channels</Typography>
                        <Select variant='standard' label="Channels" value={currentChannel.toString()} fullWidth
                            onChange={handleChannelChange}>
                            <MenuItem value={-1}>Omni</MenuItem>
                            <MenuItem value={0}>Channel 1</MenuItem>
                            <MenuItem value={1}>Channel 2</MenuItem>
                            <MenuItem value={2}>Channel 3</MenuItem>
                            <MenuItem value={3}>Channel 4</MenuItem>
                            <MenuItem value={4}>Channel 5</MenuItem>
                            <MenuItem value={5}>Channel 6</MenuItem>
                            <MenuItem value={6}>Channel 7</MenuItem>
                            <MenuItem value={7}>Channel 8</MenuItem>
                            <MenuItem value={8}>Channel 9</MenuItem>
                            <MenuItem value={9}>Channel 10</MenuItem>
                            <MenuItem value={10}>Channel 11</MenuItem>
                            <MenuItem value={11}>Channel 12</MenuItem>
                            <MenuItem value={12}>Channel 13</MenuItem>
                            <MenuItem value={13}>Channel 14</MenuItem>
                            <MenuItem value={14}>Channel 15</MenuItem>
                            <MenuItem value={15}>Channel 16</MenuItem>
                        </Select>
                    </div>

                    <div style={{ display: "flex", flexFlow: "row nowrap", justifyContent: "center" }}>
                        <FormControlLabel control={
                            <Checkbox checked={allowProgramChanges}
                                onChange={(event) => {
                                    setAllowProgramChanges(event.target.checked);
                                }}
                            />
                        } label="Allow Program Changes"
                        />
                        {false&&( // wait until the implementation stabilizes before exposing the help dialog.
                            <IconButton
                                onClick={() => { setHelpDialog(true); }}
                                size="large">
                                <InfoOutlinedIcon color='inherit'
                                    style={{
                                        fill: "var(--mui-palette.text.primary)",
                                        opacity: 0.6                                        
                                    }}
                                />
                            </IconButton>
                        )}
                    </div>

                </div>

            </DialogContent>
            <DialogActions>
                <Button onClick={handleClose} variant="dialogSecondary" >
                    Cancel
                </Button>
                <Button onClick={handleOk} variant="dialogPrimary"   >
                    OK
                </Button>
            </DialogActions>

            <ChannelBindingHelpDialog open={helpDialog} onClose={()=>setHelpDialog(false)}
            />
        </DialogEx>
    );
}

export default MidiChannelBindingDialog;
