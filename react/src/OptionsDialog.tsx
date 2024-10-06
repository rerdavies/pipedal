/* eslint-disable @typescript-eslint/no-unused-vars */
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
import List from '@mui/material/List';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemText from '@mui/material/ListItemText';
import DialogTitle from '@mui/material/DialogTitle';
import DialogEx from './DialogEx';
import DialogContent from '@mui/material/DialogContent';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';



export interface OptionItem {
    key: string | number;
    text: string;
};

export interface OptionsDialogProps {
    open: boolean;
    title?: string;
    options: OptionItem[];
    value: string | number;
    onOk: (result: OptionItem) => void;
    minWidth?: number | string;
    onClose: () => void;
}


function OptionsDialog(props: OptionsDialogProps) {
    //const classes = useStyles();
    const { options, value, onOk, onClose, open,title,minWidth } = props;

    let initialSelection : OptionItem | undefined = undefined;
    for (let option of options)
    {
        if (option.key === value)
        {
            initialSelection = option;
            break;
        }
    }
    
    const [currentSelection, setCurrentSelection] = useState(initialSelection)

    const handleListItemClick = (item: OptionItem) => {
        setCurrentSelection(item);
        onOk(item);
    };
    const handleCancel = () => {
        onClose();
    };
    return (
        <DialogEx tag="options" onClose={handleCancel} aria-labelledby="select-option" open={open}  >
        {title && (
            <DialogTitle style={{paddingLeft: 8}}>
                <Typography variant="body2" color="textSecondary">
                    {title}
                </Typography>
            </DialogTitle>
        )}
        {title && (
            <Divider />
        )}

        <DialogContent style={{padding: 0, margin: 0}}>
            <List style={{ marginLeft: 3, marginRight: 3 ,minWidth: minWidth }}>
                {options.map(
                    (item: OptionItem,index) => {
                        return (
                            <ListItemButton onClick={() => handleListItemClick(item)} key={item.key} selected={item === currentSelection} >
                                <ListItemText primary={item.text} />
                            </ListItemButton>
                        );
                    }
                )}
            </List>
        </DialogContent>
        </DialogEx>
    );
}

export default OptionsDialog;
