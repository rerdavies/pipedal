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
                                <Toolbar className={classes.drawer_header} style={{backgroundImage: 'url("img/ic_drawer.svg")' }} >
                                    <IconButton style={{ marginLeft: -24  }}
                                    >
                                        <ArrowBackIcon />
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
