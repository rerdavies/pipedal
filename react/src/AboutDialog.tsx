
import React, { SyntheticEvent, Component } from 'react';
import IconButton from '@mui/material/IconButton';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, State } from './PiPedalModel';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import Divider from '@mui/material/Divider';
import ArrowBackIcon from '@mui/icons-material//ArrowBack';

import JackHostStatus from './JackHostStatus';
import { PiPedalError } from './PiPedalError';
import DialogEx from './DialogEx';

import { Theme, createStyles } from '@mui/material/styles';
import { withStyles, WithStyles } from '@mui/styles';
import Slide, { SlideProps } from '@mui/material/Slide';


interface AboutDialogProps extends WithStyles<typeof styles> {
    open: boolean;
    onClose: () => void;


};

interface AboutDialogState {
    jackStatus?: JackHostStatus;

    openSourceNotices: string;
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
    props: SlideProps, ref: React.Ref<unknown>
) {
    return (<Slide direction="up" ref={ref} {...props} />);
});


const AboutDialog = withStyles(styles, { withTheme: true })(

    class extends Component<AboutDialogProps, AboutDialogState> {

        model: PiPedalModel;
        refNotices: React.RefObject<HTMLDivElement>;


        constructor(props: AboutDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();
            this.refNotices = React.createRef();
            this.handleDialogClose = this.handleDialogClose.bind(this);
            this.state = {
                jackStatus: undefined,
                openSourceNotices: ""

            };
        }

        mounted: boolean = false;
        subscribed: boolean = false;

        tick() {
            if (this.model.state.get() === State.Ready) {
                this.model.getJackStatus()
                    .then(jackStatus => {
                        this.setState({ jackStatus: jackStatus });
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

        startFossRequest() {
            if (this.state.openSourceNotices === "") {
                fetch("var/notices.txt")
                    .then((request) => {
                        if (!request.ok) {
                            throw new PiPedalError("notices request failed.");
                        }
                        return request.text();
                    })
                    .then((text) => {
                        if (this.mounted) {
                            this.setState({ openSourceNotices: text });
                        }

                    })
                    .catch((err) => {
                        // ok in debug builds. File doesn't get placed until install time.
                        console.log("Failed to fetch open-source notices. " + err.toString());
                    });
            }
        }

        componentDidMount() {
            super.componentDidMount?.();
            this.mounted = true;
            this.updateNotifications();
            this.startFossRequest();
        }
        componentWillUnmount() {
            super.componentWillUnmount?.();
            this.mounted = false;
            this.updateNotifications();
        }
        componentDidUpdate(prevProps: Readonly<AboutDialogProps>, prevState: Readonly<AboutDialogState>, snapshot: any): void {
            super.componentDidUpdate?.(prevProps,prevState,snapshot);
            this.updateNotifications();
        }

        handleDialogClose(e: SyntheticEvent) {
            this.props.onClose();
        }


        render() {
            let classes = this.props.classes;
            let addressKey = 0;
            return (
                <DialogEx tag="about" fullScreen open={this.props.open}
                    onClose={() => { this.props.onClose() }} TransitionComponent={Transition}
                    style={{ userSelect: "none" }}
                >

                    <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                        <div style={{ flex: "0 0 auto" }}>
                            <AppBar className={classes.dialogAppBar} >
                                <Toolbar>
                                    <IconButton edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                    >
                                        <ArrowBackIcon />
                                    </IconButton>
                                    <Typography noWrap variant="h6" className={classes.dialogTitle}>
                                        About
                                    </Typography>
                                </Toolbar>
                            </AppBar>
                        </div>
                        <div style={{
                            flex: "1 1 auto", position: "relative", overflow: "hidden",
                            overflowX: "hidden", overflowY: "auto", userSelect: "text"
                        }}
                        >
                            <div style={{ margin: 24 }}>
                                <Typography noWrap display="block" variant="h6" color="textPrimary">
                                    PiPedal <span style={{ fontSize: "0.7em" }}>
                                        {(this.model.serverVersion ? this.model.serverVersion.serverVersion : "")
                                            + (this.model.serverVersion?.debug ? " (Debug)" : "")}
                                    </span>
                                </Typography>
                                <Typography noWrap display="block" variant="body2" style={{ marginBottom: 12 }}  >
                                    Copyright &#169; 2022 Robin Davies.
                                </Typography>
                                {this.model.isAndroidHosted() && (
                                    <Typography noWrap display="block" variant="body2" style={{ marginBottom: 0 }}  >
                                        {this.model.getAndroidHostVersion()}
                                    </Typography>
                                )}

                                {this.model.isAndroidHosted() && (
                                    <Typography noWrap display="block" variant="body2" style={{ marginBottom: 12 }}  >
                                        Copyright &#169; 2022-2024 Robin Davies.
                                    </Typography>
                                )}
                                <Divider />
                                <Typography noWrap display="block" variant="caption"  >
                                    ADDRESSES
                                </Typography>
                                <div style={{marginBottom: 16}}>
                                {
                                    this.model.serverVersion?.webAddresses.map((address) =>
                                    (
                                        <Typography key={addressKey++} display="block" variant="body2" style={{ marginBottom: 0, marginLeft: 24 }}  >
                                            {address}
                                        </Typography>
                                    ))

                                }
                                </div>


                                <Divider />
                                <Typography display="block" variant="caption"  >
                                    LEGAL NOTICES
                                </Typography>
                                <div dangerouslySetInnerHTML={{ __html: this.state.openSourceNotices }} style={{ fontSize: "0.8em",maxWidth: 400 }}>
                                </div>

                            </div>
                        </div>
                    </div>
                </DialogEx >

            );

        }

    }

);

export default AboutDialog;