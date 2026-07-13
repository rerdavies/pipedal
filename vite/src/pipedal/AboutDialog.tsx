
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

interface AboutDialogProps {
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
    refNotices: React.RefObject<HTMLDivElement | null>;


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
                .catch(_error => { /* ignore*/ });
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
    componentDidUpdate(prevProps: Readonly<AboutDialogProps>, prevState: Readonly<AboutDialogState>, snapshot: any): void {
        super.componentDidUpdate?.(prevProps, prevState, snapshot);
        this.updateNotifications();
    }

    handleDialogClose(_e: SyntheticEvent) {
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
                            cursor: "text", margin: 24
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
                                Copyright &#169; Robin Davies.
                            </Typography>
                            {this.model.isAndroidHosted() && (
                                <Typography noWrap display="block" variant="body2" style={{ marginBottom: 0 }}  >
                                    {this.model.getAndroidHostVersion()}
                                </Typography>
                            )}

                            {this.model.isAndroidHosted() && (
                                <Typography noWrap display="block" variant="body2" style={{ marginBottom: 12 }}  >
                                    Copyright &#169; Robin Davies.
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
                        </div>
                        <div style={{ marginLeft: 24, marginRight: 24 }}>
                            <Divider />
                            <Typography display="block" variant="caption"   >
                                LEGAL NOTICES
                            </Typography>
                            <div style={{ marginLeft: 24, marginTop: 32 }}>
                                <Divider style={{ marginBottom: 24 }} />

                                <Typography display="block" variant="body2" style={{ marginTop: 8 }}>
                                    The following files are provided under a T3K license. Under an arrangement with Tone3000.com,
                                    permission has been granted to Robin Davies to distribute these files for use ONLY with PiPedal.
                                </Typography>
                                <ul style={{ marginTop: 8, marginBottom: 16, listStyle: "none", paddingLeft: 24 }}>

                                    <li>
                                        <Typography display="inline" variant="body2">
                                            /etc/pipedal/config/default_presets/presets/Factory Presets.piBank (contains
                                            compressed versions of the files listed below)
                                        </Typography>
                                    </li>
                                    <li>
                                        <Typography display="inline" variant="body2">
                                            Files installed in "/var/pipedal/audio_uploads/NeuralAmpModels/Factory Models/"
                                        </Typography>
                                    </li>
                                    <li>
                                        <Typography display="inline" variant="body2">
                                            Files installed in "/var/pipedal/audio_uploads/IRs/Factory IRs/"
                                        </Typography>
                                    </li>
                                </ul>
                                <Typography display="block" variant="body2" style={{ marginBottom: 12 }}>
                                    Please contact the original authors for permission if you would like to distribute these files for
                                    other purposes, or if you would like to distribute these files in a forked version of PiPedal.
                                </Typography>
                                <Typography display="block" variant="body2" style={{ marginBottom: 12 }}>
                                    See the following files for detailed attributions:
                                </Typography>
                                <ul style={{ marginTop: 8, marginBottom: 16, listStyle: "none", paddingLeft: 24 }}>
                                    <li>
                                        <Typography display="inline" variant="body2">
                                            /var/pipedal/audio_uploads/NeuralAmpModels/Factory Models/README.md<br />
                                        </Typography>
                                    </li>
                                    <li>
                                        <Typography display="inline" variant="body2">
                                            /var/pipedal/audio_uploads/IRs/Factory IRs/README.md<br />
                                        </Typography>
                                    </li>
                                </ul>
                                <Typography display="block" variant="body2" style={{ marginBottom: 12 }}>
                                    &copy; 2dor - <a href="https://tone3000.com/t2dor" target="_blank" rel="noreferrer">https://tone3000.com/t2dor</a><br />
                                    &copy; amalgamaudio - <a href="https://tone3000.com/amalgamaudio" target="_blank" rel="noreferrer">https://tone3000.com/amalgamaudio</a><br />
                                    &copy; kenazmusic - <a href="https://tone3000.com/kenazmusic" target="_blank" rel="noreferrer">https://tone3000.com/kenazmusic</a><br />
                                    &copy; outmodedelectronics - <a href="https://tone3000.com/outmodedelectronics" target="_blank" rel="noreferrer">https://tone3000.com/outmodedelectronics</a><br />
                                    &copy; scottcorgan - <a href="https://tone3000.com/scottcorgan" target="_blank" rel="noreferrer">https://tone3000.com/scottcorgan</a><br />
                                    &copy; tone3000 - <a href="https://tone3000.com/tone3000" target="_blank" rel="noreferrer">https://tone3000.com/tone3000</a><br />
                                </Typography>

                                <div style={{ marginLeft: 64, marginRight: 64 }}>
                                    <Typography display="block" variant="subtitle2" style={{ marginTop: 8 }}>
                                        T3K License
                                    </Typography>
                                    <Typography display="block" variant="body2" style={{ marginTop: 4, marginBottom: 12 }}>
                                        Users may download and use the data file in software and publish the resulting outputs without
                                        royalties or restrictions. However, they may not upload, republish, or distribute the data file
                                        without the author&apos;s permission.
                                    </Typography>
                                </div>


                                <Divider style={{ marginTop: 24, marginBottom: 24 }} />
                                <Typography display="block" variant="body2" style={{ marginTop: 8 }}>
                                    PiPedal factory presets are licensed under a <a href="https://creativecommons.org/licenses/by/4.0/"
                                        target="_blank"
                                        rel="noreferrer">CC-BY-4.0 license</a>.
                                </Typography>
                                <Typography display="block" variant="body2" style={{ marginTop: 8, marginBottom: 8 }}>
                                    &copy; Robin E.R. Davies<br />
                                    &copy; Andrew Curtis<br />
                                </Typography>

                                <Divider style={{ marginTop: 24, marginBottom: 24 }} />

                                <Typography display="block" variant="body2" style={{ marginBottom: 12 }}>
                                    Installed files in the  "/var/pipedal/audio_uploads/CabIR" directory are provided
                                    under a <a href="https://creativecommons.org/licenses/by/4.0/"
                                        target="_blank"
                                        rel="noreferrer">CC-BY-4.0 license</a>. 
                                    Refer to the following file for detailed attributions:
                                </Typography>
                                <ul style={{ marginTop: 8, marginBottom: 16, listStyle: "none", paddingLeft: 24 }}>
                                    <li>
                                        <Typography display="inline" variant="body2">
                                            /var/pipedal/audio_uploads/CabIR/LICENSE.md<br />
                                        </Typography>
                                    </li>
                                </ul>
                                <Typography display="block" variant="body2" style={{ marginBottom: 12 }}>
                                    &copy; Kristoffer Ekstrand, Adventure Kid Research &amp; Technology<br />
                                </Typography>


                                <Divider style={{ marginTop: 24, marginBottom: 24 }} />
                                <Typography display="block" variant="body2" style={{ marginBottom: 12 }}>
                                    Installed files in the  "/var/pipedal/audio_uploads/ConvolutionReverb" directory are provided
                                    under a <a href="https://creativecommons.org/licenses/by/4.0/"
                                        target="_blank"
                                        rel="noreferrer">CC-BY-4.0 license</a>. See the following file for detailed attributions:
                                </Typography>
                                <ul style={{ marginTop: 8, marginBottom: 16, listStyle: "none", paddingLeft: 24 }}>
                                    <li>
                                        <Typography display="inline" variant="body2">
                                            /var/pipedal/audio_uploads/ConvolutionReverb/LICENSE.md<br />
                                        </Typography>
                                    </li>
                                </ul>
                                <Typography display="block" variant="body2" style={{ marginBottom: 12 }}>
                                    &copy; Greg Hopkins - <a href="https://hopkinsmedia.services/ir" target="_blank" rel="noreferrer">https://hopkinsmedia.services/ir</a><br />
                                    &copy; www.openairlib.net, University of York - <a href="https://www.openairlib.net/" target="_blank" rel="noreferrer">https://www.openairlib.net/</a><br />
                                    &copy; www.openairlib.net, Audiolab University of York, Alex Duffell, Aishwarya Sridhar, Zhong Li<br />
                                    &copy; www.openairlib.net, Audio Lab University of York, Andrew Chadwick, Simon Shelley<br />
                                    &copy; www.openairlib.net, AudioLab University of York, www.ncem.co.uk<br />
                                </Typography>
                                <Divider />



                                <div dangerouslySetInnerHTML={{ __html: this.state.openSourceNotices }} style={{ fontSize: "0.8em", maxWidth: 400 }}>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </DialogEx >

        );

    }

};


export default AboutDialog;