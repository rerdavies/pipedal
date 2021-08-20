import React from 'react';
import { Theme, withStyles, WithStyles,createStyles } from '@material-ui/core/styles';


const styles = (theme: Theme) => createStyles({
});


interface XxxProps extends WithStyles<typeof styles> {
    theme: Theme,
};

interface XxxState {

};

const Xxx = withStyles(styles, { withTheme: true })(

    class extends React.Component<XxxProps, XxxState> {
        constructor(props: XxxProps)
        {
            super(props);
            this.state = {

            };
        }

        render() {
            return (
                <div/>
            );
        }
    }
);

export default Xxx;
