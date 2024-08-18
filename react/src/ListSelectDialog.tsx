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
import Button from '@mui/material/Button';


import Radio from '@mui/material/Radio';
import List from '@mui/material/List';
import ListItem from '@mui/material/ListItem';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';

import DialogEx from './DialogEx';
import DialogActions from '@mui/material/DialogActions';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';


export interface ListSelectDialogProps {
    open: boolean,
    title: string,
    width: number,
    selectedItem: string,
    items: string[],
    onOk: (selectedItem: string) => void,
    onClose: () => void
};

export interface ListSelectDialogState {
    fullScreen: boolean;
};

export default class ListSelectDialog extends ResizeResponsiveComponent<ListSelectDialogProps, ListSelectDialogState> {

    refText: React.RefObject<HTMLInputElement>;

    constructor(props: ListSelectDialogProps) {
        super(props);
        this.state = {
            fullScreen: false
        };
        this.refText = React.createRef<HTMLInputElement>();
    }
    mounted: boolean = false;



    onWindowSizeChanged(width: number, height: number): void {
        this.setState({ fullScreen: height < 200 })
    }


    componentDidMount() {
        super.componentDidMount();
        this.mounted = true;
    }
    componentWillUnmount() {
        super.componentWillUnmount();
        this.mounted = false;
    }

    componentDidUpdate() {
    }

    render() {

        return (
            <DialogEx tag="list" onClose={()=>this.props.onClose()} open={this.props.open}>
                <List sx={{pt: 0}}>
                {
                    this.props.items.map(
                        (value: string, index: number) => {
                            return (
                                <ListItem key={index}  >
                                        <ListItemButton role={undefined} onClick={()=> this.props.onOk(value)} dense>
                                        <ListItemIcon>
                                            <Radio
                                                edge="start"
                                                checked={value === this.props.selectedItem}
                                            disableRipple
                                            />
                                        </ListItemIcon>
                                        <ListItemText primary={value} />
                                        </ListItemButton>
                                </ListItem>
                            );
                        }
                    )
                }
                </List>
                <DialogActions>
                    <Button variant="dialogSecondary" onClick={()=> this.props.onClose()} color="primary">
                        Cancel
                    </Button>
                </DialogActions>
            </DialogEx>
        );
    }
}