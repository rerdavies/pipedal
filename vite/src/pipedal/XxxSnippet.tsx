
import React from 'react';
import { withStyles } from "tss-react/mui";
import { css } from '@emotion/react';

import { Theme } from "@mui/material";




// const styles : (theme: Theme) => Record<string, CSSObject>
const styles 
 = ((theme: Theme) => {
    return {
        root: {
            color: theme.palette.primary.main
        },
        frame: css({
            margin: "12px",
            color: "red",
            position: "absolute",
            height: '100%',
            width: '100%',
            borderRadius: 14 / 2,
            zIndex: -1,
            transition: theme.transitions.create(['opacity', 'background-color'], {
                duration: theme.transitions.duration.shortest
            }),
            backgroundColor: theme.palette.secondary.main,
            opacity: theme.palette.mode === 'light' ? 0.38 : 0.3,
            //position: "absolute"

         })

    }
});

export interface DummyProps  {
    className?: string;
    classes?: Partial<Record<keyof ReturnType<typeof styles>, string>>;

};


class DummyBase extends React.Component<DummyProps> {
    render() {
        const classes = withStyles.getClasses(this.props);
        return (<div className={classes.root} style={{position: "absolute"}}/>);
    }
}

const Dummy = withStyles(
    DummyBase,
    styles
);

export default Dummy;

