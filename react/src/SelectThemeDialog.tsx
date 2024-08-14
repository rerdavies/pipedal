/* eslint-disable @typescript-eslint/no-unused-vars */
// Copyright (c) 2023 Robin Davies
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
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import DialogEx from './DialogEx';
import FormControlLabel from '@mui/material/FormControlLabel';
import FormControl from '@mui/material/FormControl';

import RadioGroup from '@mui/material/RadioGroup';
import Radio from '@mui/material/Radio';
import {ColorTheme} from './DarkMode';



export interface SelectThemesDialogProps {
    open: boolean;
    defaultTheme: ColorTheme;
    onClose: () => void;
    onOk: (value: ColorTheme) => void;
}

function SelectThemesDialog(props: SelectThemesDialogProps) {
    //const classes = useStyles();
    const { onClose, defaultTheme, open,onOk } = props;
    const [ selectedTheme, setSelectedTheme ] = useState(defaultTheme);
    const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        let value = ColorTheme.Light;
        if ((event.target as HTMLInputElement).value === '1')
        {
            value = ColorTheme.Dark;
        } else if ((event.target as HTMLInputElement).value === '2')
        {
            value = ColorTheme.System;
        }        
        setSelectedTheme(value);
    };
    const handleClose = (): void => {
        onClose();
    };
    const handleOk = (): void => {
        onOk(selectedTheme)
    };


    return (
        <DialogEx tag="theme" onClose={handleClose} open={open}>
            <DialogTitle id="simple-dialog-title">Theme</DialogTitle>
            <DialogContent>
                <FormControl>
                    <RadioGroup
                        defaultValue={defaultTheme}
                        onChange={handleChange}
                    >
                        <FormControlLabel value={ColorTheme.Light} control={<Radio size='small' />} label="Light" />
                        <FormControlLabel value={ColorTheme.Dark} control={<Radio size='small' />} label="Dark" />
                        <FormControlLabel value={ColorTheme.System} control={<Radio size='small' />} label="Use system setting" />
                    </RadioGroup>
                </FormControl>
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

export default SelectThemesDialog;
