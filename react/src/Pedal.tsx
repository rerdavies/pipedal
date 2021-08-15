import React, { Component, ReactNode } from 'react';
import { withStyles, createStyles, WithStyles } from '@material-ui/core';
import { Theme } from '@material-ui/core/styles/createMuiTheme';
import { Lv2Plugin } from './Lv2Plugin'



const pedalStyles = ({ palette }: Theme) => createStyles({
});

interface PedalProps extends WithStyles<typeof pedalStyles> {
    plugin: Lv2Plugin;
    children?: React.ReactChild | React.ReactChild[];

};

type PedalState = {
    
};

export const TemporaryDrawer = withStyles(pedalStyles)(
    class extends Component<PedalProps, PedalState>
    {
        state: PedalState;
    
        constructor(props: PedalProps) {
            super(props);
            this.state = { 
            };
        }
        render() : ReactNode {
            return (
                <div>
                    
                </div>
            );
        }

    }
);