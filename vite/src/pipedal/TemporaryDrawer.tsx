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

import React, { Component } from 'react';
import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";
import {createStyles} from './WithStyles';


import IconButtonEx from './IconButtonEx';
import Drawer from '@mui/material/Drawer';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import { Theme } from '@mui/material/styles';
import {isDarkMode} from './DarkMode';

const drawerStyles = (theme: Theme) => {
    return createStyles({
    list: {
        width: 250,
    },
    fullList: {
        width: 'auto',
    },

    drawer_header: {
        color: theme.palette.primary.main,
        background: (isDarkMode()? theme.palette.background.default: 'white'),

    }
})};

type Anchor = 'top' | 'left' | 'bottom' | 'right';

type CloseEventHandler = () => void;



interface DrawerProps extends WithStyles<typeof drawerStyles> {
    title?: string;
    position: Anchor;
    is_open: boolean;
    onClose?: CloseEventHandler;
    children?: React.ReactNode;

}
type DrawerState = {
    is_open: boolean;
}

export const TemporaryDrawer = withStyles(
    class extends Component<DrawerProps, DrawerState>
    {
        constructor(props: DrawerProps) {
            super(props);
            this.state = { is_open: props.is_open };

        }

        toggleDrawer(anchor: Anchor, open: boolean): void {
            this.setState({ is_open: open });
        };


        fireClose() {
            let handler = this.props.onClose;
            if (handler != null) {
                handler();
            }
        }

        render() {
            const classes  = withStyles.getClasses(this.props);


            return (
                <div>
                    <React.Fragment>
                        <Drawer anchor={this.props.position} open={this.props.is_open} onClose={() => { this.fireClose(); }} >
                            <div 
                                className={this.props.position === 'top' || this.props.position === 'bottom' ? classes.fullList : classes.list}
                                role="presentation"
                                onClick={() => { this.fireClose(); }}
                                onKeyDown={() => { this.fireClose(); }}
                            >
                                <div style={{ display: "flex", flexFlow: "row nowrap", justifyContent: "flex-start", alignItems: "center", width: "100%"}}>

                                    <IconButtonEx tooltip="Back"
                                         style={{ flex: "0 0 auto" }} >
                                        <ArrowBackIcon style={{ fill: '#666' }} />
                                    </IconButtonEx>
                                    <img src="img/Pi-Logo-3.png" alt="" style={{height: 36}} />
                                </div>
                                {this.props.children}
                            </div>
                        </Drawer>
                    </React.Fragment>
                </div >
            );
        }
    },
    drawerStyles
);
