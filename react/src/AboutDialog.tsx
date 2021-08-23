
import React, { SyntheticEvent, Component } from 'react';
import IconButton from '@material-ui/core/IconButton';
import Typography from '@material-ui/core/Typography';
import { PiPedalModel, PiPedalModelFactory, State } from './PiPedalModel';
import { TransitionProps } from '@material-ui/core/transitions/transition';
import Slide from '@material-ui/core/Slide';
import Dialog from '@material-ui/core/Dialog';
import AppBar from '@material-ui/core/AppBar';
import Toolbar from '@material-ui/core/Toolbar';
import { Theme, withStyles, WithStyles, createStyles } from '@material-ui/core/styles';
import ArrowBackIcon from '@material-ui/icons/ArrowBack';
import Divider from '@material-ui/core/Divider';
import JackHostStatus from './JackHostStatus';


interface AboutDialogProps extends WithStyles<typeof styles> {
    open: boolean;
    onClose: () => void;


};

interface AboutDialogState {
    jackStatus?: JackHostStatus;
};


const styles = (theme: Theme) => createStyles({
    dialogAppBar: {
        position: 'relative',
        top: 0, left: 0
    },
    dialogTitle: {
        marginLeft: theme.spacing(2),
        flex: 1,
    },
    sectionHead: {
        marginLeft: 24,
        marginRight: 24,
        marginTop: 16,
        paddingBottom: 12

    },
    textBlock: {
        marginLeft: 24,
        marginRight: 24,
        marginTop: 16,
        paddingBottom: 0,
        opacity: 0.95

    },
    textBlockIndented: {
        marginLeft: 40,
        marginRight: 24,
        marginTop: 16,
        paddingBottom: 0,
        opacity: 0.95

    },

    setting: {
        minHeight: 64,
        width: "100%",
        textAlign: "left",
        paddingLeft: 22,
        paddingRight: 22
    },
    primaryItem: {

    },
    secondaryItem: {

    }


});


const Transition = React.forwardRef(function Transition(
    props: TransitionProps & { children?: React.ReactElement },
    ref: React.Ref<unknown>,
) {
    return <Slide direction="up" ref={ref} {...props} />;
});


const AboutDialog = withStyles(styles, { withTheme: true })(

    class extends Component<AboutDialogProps, AboutDialogState> {

        model: PiPedalModel;



        constructor(props: AboutDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();

            this.handleDialogClose = this.handleDialogClose.bind(this);
            this.state = {
                jackStatus: undefined

            };
        }

        mounted: boolean = false;
        subscribed: boolean = false;

        tick() {
            if (this.model.state.get() === State.Ready) {
                this.model.getJackStatus()
                    .then(jackStatus => {
                        this.setState({jackStatus: jackStatus});
                    })
                    .catch(error => { /* ignore*/ });
            }
        }

        timerHandle?: NodeJS.Timeout;

        updateNotifications() {
            let subscribed = this.mounted && this.props.open;
            if (subscribed !== this.subscribed) {
                if (subscribed) {
                    this.timerHandle = setInterval(() => this.tick(), 1000);
                } else {
                    if (this.timerHandle) {
                        clearInterval(this.timerHandle);
                        this.timerHandle = undefined;
                    }
                }
                this.subscribed = subscribed;
            }
        }


        componentDidMount() {
            this.mounted = true;
            this.updateNotifications();
        }
        componentWillUnmount() {
            this.mounted = false;
            this.updateNotifications();
        }
        componentDidUpdate() {
            this.updateNotifications();
        }

        handleDialogClose(e: SyntheticEvent) {
            this.props.onClose();
        }


        render() {
            let classes = this.props.classes;

            return (
                <Dialog fullScreen open={this.props.open}
                    onClose={() => { this.props.onClose() }} TransitionComponent={Transition}>

                    <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                        <div style={{ flex: "0 0 auto" }}>
                            <AppBar className={classes.dialogAppBar} >
                                <Toolbar>
                                    <IconButton edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                    >
                                        <ArrowBackIcon />
                                    </IconButton>
                                    <Typography variant="h6" className={classes.dialogTitle}>
                                        About
                                    </Typography>
                                </Toolbar>
                            </AppBar>
                        </div>
                        <div style={{
                            flex: "1 1 auto", position: "relative", overflow: "hidden",
                            overflowX: "hidden", overflowY: "auto", margin: 22
                        }}
                        >
                            <div>
                                <Typography display="block" variant="h6" color="textPrimary">
                                    PiPedal <span style={{fontSize: "0.7em"}}>
                                        {(this.model.serverVersion? this.model.serverVersion.serverVersion : "")
                                        + (this.model.serverVersion?.debug ? " (Debug)": "")}
                                         </span>
                                </Typography>
                                <Typography display="block" variant="body2" style={{marginBottom: 12}}  >
                                    Copyright &#169; 2021 Robin Davies. All rights reserved.
                                </Typography>
                                <Divider/>
                                <Typography display="block" variant="caption"  >
                                    JACK AUDIO STATUS
                                </Typography>
                                <div style={{paddingTop: 8, paddingBottom: 24}}>
                                    {
                                        JackHostStatus.getDisplayView("",this.state.jackStatus)
                                    }
                                </div>
                                <Divider/>
                                <Typography display="block" variant="caption"  >
                                    LEGAL NOTICES
                                </Typography>
                                <Typography display="block" variant="caption" style={{paddingTop: 12, paddingBottom: 12}} >
                                    Pi Pedal uses the following open-source components.
                                </Typography>
                                <Typography display="block" variant="caption" style={{paddingTop: 12, paddingBottom: 12}} >
                                    TBD
                                </Typography>

                            </div>
                        </div>
                    </div>
                </Dialog >

            );

        }

    }

);

export default AboutDialog;