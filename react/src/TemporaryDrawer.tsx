// Copyright (c) 2021 Robin Davies
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
import { withStyles, createStyles, WithStyles } from '@material-ui/core';

import Toolbar from '@material-ui/core/Toolbar';
import IconButton from '@material-ui/core/Toolbar';
import Drawer from '@material-ui/core/Drawer';
import ArrowBackIcon from '@material-ui/icons/ArrowBack';
import { Theme } from '@material-ui/core/styles/createMuiTheme';


const drawerStyles = ({ palette }: Theme) => createStyles({
    list: {
        width: 250,
    },
    fullList: {
        width: 'auto',
    },

    drawer_header: {
        color: 'white',
        background: palette.primary.main,

    }
});

type Anchor = 'top' | 'left' | 'bottom' | 'right';

type CloseEventHandler = () => void;



interface DrawerProps extends WithStyles<typeof drawerStyles> {
    title?: string;
    position: Anchor;
    is_open: boolean;
    onClose?: CloseEventHandler;
    children?: React.ReactChild | React.ReactChild[];

};
type DrawerState = {
    is_open: boolean;
}

export const TemporaryDrawer = withStyles(drawerStyles)(
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
            const { classes } = this.props;


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
                                <Toolbar className={classes.drawer_header} 
                                    style={{backgroundImage: 'url("img/ic_drawer_2.png")', border: "red solid", borderWidth: "0px 0px 3px 0px" }} 
                                >
                                    <IconButton style={{ marginLeft: -24  }}
                                    >
                                        <ArrowBackIcon style={{ fill: '#666' }} />
                                    </IconButton>
                                </Toolbar>
                                {this.props.children}
                            </div>
                        </Drawer>
                    </React.Fragment>
                </div >
            );
        }
    }
);
