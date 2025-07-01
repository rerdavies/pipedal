

import React from 'react';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import DialogEx from './DialogEx';
import Link from '@mui/material/Link';
import Toolbar from '@mui/material/Toolbar';
import IconButtonEx from './IconButtonEx';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';




export interface Tone3000DialogProps {
    open: boolean;
    onClose: () => void;
}

function Tone3000Dialog(props: Tone3000DialogProps) {
    const { open, onClose } = props;

    const model: PiPedalModel = PiPedalModelFactory.getInstance();

    let [openAuthDialog, setOpenAuthDialog] = React.useState(!model.hasTone3000Auth.get());


    React.useEffect(()=> {
        let authListener = (value: boolean) => {
            setOpenAuthDialog(!value);
        }
        model.hasTone3000Auth.addOnChangedHandler(authListener);
        return () => {
            model.hasTone3000Auth.removeOnChangedHandler(authListener);
        }
    });

    function RedirectUrl() {
        let baseUrl = new URL(window.location.href);
        let result = "http://" + baseUrl.hostname + ":" + baseUrl.port + "/";
        return result;
    }
    function AuthUrl() {
        //return "https://www.tone3000.com/api/v1/auth?redirect_url=" + encodeURIComponent(RedirectUrl()) + "&opt_only=true";
        return "https://www.tone3000.com/api/v1/auth?redirect_url=" + encodeURIComponent(RedirectUrl()) + "&otp_only=true";
    }
    return (
        <DialogEx
            tag="tone3000"
            fullScreen={true}
            open={open}
            onEnterKey={() => { onClose(); }}
            onClose={() => { onClose(); }}
            aria-labelledby="tone3000-dialog-title"
            aria-describedby="tone3000-dialog-description"
        >
            <DialogTitle id="tone3000-dialog-title">
                <Toolbar style={{ padding: 0 }}>
                    <IconButtonEx
                        tooltip="Close"
                        edge="start"
                        color="inherit"
                        aria-label="cancel"
                        style={{ opacity: 0.6 }}
                        onClick={() => { onClose(); }}
                    >
                        <ArrowBackIcon style={{ width: 24, height: 24 }} />
                    </IconButtonEx>

                    <Typography noWrap component="div" sx={{ flexGrow: 1 }}>
                        TONE3000 Models
                    </Typography>

                </Toolbar>
            </DialogTitle>
            <DialogContent>
                <Typography variant="body1">
                    This is a placeholder for the Tone 3000 dialog.
                </Typography>
            </DialogContent>
            <DialogActions>
                <Button onClick={onClose} color="primary">
                    Close
                </Button>
            </DialogActions>
            {
                openAuthDialog && (
                    <Dialog
                        fullScreen={true}
                        open={openAuthDialog}>

                        <DialogTitle id="tone3000-auth-dialog-title">
                            <Toolbar style={{ padding: 0 }}>
                                <IconButtonEx
                                    tooltip="Close"
                                    edge="start"
                                    color="inherit"
                                    aria-label="cancel"
                                    style={{ opacity: 0.6 }}
                                    onClick={() => { onClose(); }}
                                >
                                    <ArrowBackIcon style={{ width: 24, height: 24 }} />
                                </IconButtonEx>

                                <Typography noWrap component="div" sx={{ flexGrow: 1 }}>
                                    TONE3000 Authorization
                                </Typography>

                            </Toolbar>
                        </DialogTitle>
                        <DialogContent>
                            <div style={{ display: "flex", flexFlow: "row nowrap", alignItems: "start" }}>
                                <div style={{ flex: "1 1 1px" }} />
                                <div style={{
                                    maxWidth: 500, marginLeft: "auto", marginRight: "auto",
                                    display: "flex", flexFlow: "column nowrap", gap: 16, alignItems: "start",
                                }}>
                                    <Typography variant="body1" component="div" display="block" >
                                        The <Link href="https://www.tone3000.com" target="_blank">TONE3000 website</Link> provides an
                                        online library of models for use with TooB Neural Amp Modeller. You can download
                                        models from the TONE3000 website onto your local machine and then upload them to
                                        PiPedal; or, more conveniently, you can browse the TONE3000 model database directly, and download models directly
                                        to the PiPedal server from the TONE3000 model database in a single step.
                                    </Typography>
                                    <Typography variant="body1" component="div" display="block" >
                                        In order to access the TONE3000 database, you must first obtain an access token from the
                                        TONE3000 website. Clicking on the button will take you to an external website in
                                        order to complete the authorization process.
                                    </Typography>
                                    <Button variant="contained" color="primary" style={{ alignSelf: "end", marginRight: 32 }}
                                        onClick={() => {
                                            window.open(AuthUrl(), "_blank");
                                        }}
                                    >
                                        Continue to TONE3000
                                    </Button>

                                    <Typography variant="body2" component="div" display="block" style={{ marginTop: 32 }}>
                                        Privacy statement: PiPedal will only have access to your authorization token
                                        which does not contain personally identifying information, and which is stored locally on
                                        your PiPedal server. Please refer to
                                        the <Link href="https://www.tone3000.com/privacy" target="_blank">TONE3000 privacy policy</Link> for
                                        information on how your data is used by TONE3000.
                                    </Typography>
                                </div>
                                <div style={{ flex: "2 2 1px" }} />
                            </div>


                        </DialogContent>
                    </Dialog>
                )}
        </DialogEx>

    );
}

export default Tone3000Dialog;