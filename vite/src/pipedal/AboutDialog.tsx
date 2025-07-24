
import React, { SyntheticEvent, Component } from 'react';
import IconButtonEx from './IconButtonEx';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, State } from './PiPedalModel';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import Divider from '@mui/material/Divider';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';

import JackHostStatus from './JackHostStatus';
import { PiPedalError } from './PiPedalError';
import DialogEx from './DialogEx';



import Slide, { SlideProps } from '@mui/material/Slide';

interface AboutDialogProps  {
    open: boolean;
    onClose: () => void;
};

interface AboutDialogState {
    jackStatus?: JackHostStatus;

    openSourceNotices: string;
};


const Transition = React.forwardRef(function Transition(
    props: SlideProps, ref: React.Ref<unknown>
) {
    return (<Slide direction="up" ref={ref} {...props} />);
});


const AboutDialog = class extends Component<AboutDialogProps, AboutDialogState> {

    model: PiPedalModel;
    refNotices: React.RefObject<HTMLDivElement | null >;


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
               .catch(() => { /* ignore*/ });
        }
    }

    timerHandle?: number;

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
     componentDidUpdate(prevProps: Readonly<AboutDialogProps>, prevState: Readonly<AboutDialogState>, snapshot: unknown): void {
        super.componentDidUpdate?.(prevProps, prevState, snapshot);
        this.updateNotifications();
    }

      handleDialogClose() {
        this.props.onClose();
    }


    render() {
        let addressKey = 0;
        let serverVersion = this.model.serverVersion?.serverVersion ?? "";
        let nPos = serverVersion.indexOf(' ');
        if (nPos !== -1) {
            serverVersion = serverVersion.substring(nPos + 1);
        }
        return (
            <DialogEx tag="about" fullScreen open={this.props.open}
                onClose={() => { this.props.onClose() }} TransitionComponent={Transition}
                onEnterKey={() => { this.props.onClose() }}
                style={{ userSelect: "none" }}
            >

                <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                    <div style={{ flex: "0 0 auto" }}>
                        <AppBar style={{
                            position: 'relative',
                            top: 0, left: 0
                        }}  >
                            <Toolbar>
                                <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                >
                                    <ArrowBackIcon />
                                </IconButtonEx>
                                <Typography noWrap variant="h6"
                                    sx={{ marginLeft: 2, flex: 1 }}
                                >
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
                        <div id="debug_info" style={{
                            userSelect: "text",
                            cursor: "text",margin: 24
                        }}>
                            <div style={{ display: "flex", flexFlow: "row nowrap" }}>
                                <Typography noWrap display="block" variant="h6" color="textPrimary" style={{ flexGrow: 1, flexShrink: 1 }}>
                                    PiPedal <span style={{ fontSize: "0.7em" }}>
                                        {serverVersion
                                            + (this.model.serverVersion?.debug ? " (Debug)" : "")}
                                    </span>
                                </Typography>

                            </div>
                            <Typography noWrap display="block" variant="body2" style={{ marginBottom: 12 }}  >
                                Copyright &#169; 2022-2025 Robin Davies.
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
                            <div style={{ marginBottom: 16 }}>
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
                            <Typography noWrap display="block" variant="caption"  >
                                SERVER OS
                            </Typography>
                            <div style={{ marginBottom: 16 }}>
                                <Typography display="block" variant="body2" style={{ marginBottom: 0, marginLeft: 24 }}  >
                                    {this.model.serverVersion?.osVersion ?? ""}
                                </Typography>
                            </div>
                        </div><div style={{marginLeft: 24, marginRight: 24}}>

                            <Divider />
                            <Typography display="block" variant="caption"  >
                                LEGAL NOTICES
                            </Typography>
                            <div dangerouslySetInnerHTML={{ __html: this.state.openSourceNotices }} style={{ fontSize: "0.8em", maxWidth: 400 }}>
                            </div>

                        </div>
                    </div>
                </div>
            </DialogEx >

        );

    }

};


export default AboutDialog;