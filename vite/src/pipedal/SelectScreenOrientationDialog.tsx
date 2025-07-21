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

import List from '@mui/material/List';
import ListItemText from '@mui/material/ListItemText';
import DialogEx from './DialogEx';

import { ListItemButton } from '@mui/material';
import ScreenOrientation from './ScreenOrientation';



export interface SelectScreenOrientationDialogProps {
    open: boolean;
    screenOrientation: ScreenOrientation;
    onClose: (screenOrientation: ScreenOrientation | null) => void;
}


function SelectScreenOrientationDialog(props: SelectScreenOrientationDialogProps) {
    //const classes = useStyles();
    const { onClose, screenOrientation, open } = props;
    
    const handleCancel = () => {
        onClose(null);
    }
    const handleOk = (value: ScreenOrientation) => {
        onClose(value);
    };

    const handleListItemClick = (value: ScreenOrientation) => {
        handleOk(value);
    };

    return (
        <DialogEx tag="channels" onClose={handleCancel} aria-labelledby="select-channels-title" open={open}
            onEnterKey={() => { }}
        >

            <List style={{ marginLeft: 0, marginRight: 0 }}>
                <ListItemButton onClick={() => handleListItemClick(ScreenOrientation.SystemDefault)} key={"0"} 
                   selected={screenOrientation === ScreenOrientation.SystemDefault} >
                    <ListItemText primary={"System default"} />
                </ListItemButton>
                <ListItemButton onClick={() => handleListItemClick(ScreenOrientation.Portrait)} key={"1"} 
                   selected={screenOrientation === ScreenOrientation.Portrait} >
                    <ListItemText primary={"Portrait"} />
                </ListItemButton>
                <ListItemButton onClick={() => handleListItemClick(ScreenOrientation.Landscape)} key={"2"} 
                   selected={screenOrientation === ScreenOrientation.Landscape} >
                    <ListItemText primary={"Landscape"} />
                </ListItemButton>
            </List>
        </DialogEx>
    );
}

export default SelectScreenOrientationDialog;
