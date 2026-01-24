

import React from 'react';
import AppBar from '@mui/material/AppBar';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import DialogEx from './DialogEx';
import Toolbar from '@mui/material/Toolbar';
import IconButtonEx from './IconButtonEx';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';




export interface Tone3000DialogProps {
    open: boolean;
    onCancel: () => void;
    onDownload: (toneUrl: string) => void;
}

interface Tone3000AuthState {
    authToken: string;
    refreshToken: string;
    expiry: number;
};

// function getAuthToken(): Tone3000AuthState | null {

//     let authToken = window.localStorage.getItem("tone300_auth_token");
//     if (authToken === null) {
//         return null;
//     }
//     let refreshToken = window.localStorage.getItem("tone3000_refresh_token");
//     if (refreshToken === null) {
//         return null;
//     }       
//     let expiry = window.localStorage.getTime("tone3000_auth_expiry");
//     return { authToken: authToken, refreshToken: refreshToken, expiry: expiry };
// }

// function setAuthToken(authToken: string, refreshToken: string, expirySeconds: number) {
//     window.localStorage.setItem("tone3000_auth_token", authToken);
//     window.localStorage.setItem("tone3000_refresh_token", refreshToken);
//     window.localStorage.setItem("tone3000_auth_expiry", String(Date.now() + (expirySeconds * 1000)));
// }

// enum AuthState {
//     OnBoarding,
//     TokenExpired,
//     Authenticated
// }

function Tone3000Dialog(props: Tone3000DialogProps) {
    const { onCancel,onDownload } = props;

    function RedirectUrl() {
        let baseUrl = new URL(window.location.href);
        let result = "http://" + baseUrl.hostname + ":" + baseUrl.port + "/public/handleTone3000download.html";
        return result;
    }
    const appId = 'pipedal_app';
    const redirectUrl = RedirectUrl();

    let selectUrl = `https://www.tone3000.com/api/v1/select?app_id=${appId}&redirect_url=${redirectUrl}`;

    const model: PiPedalModel = PiPedalModelFactory.getInstance();
    if (model) {

    }

    React.useEffect(()=> {

        let messagehandler = (event: MessageEvent) => {
            // Handle messages received from the iframe
            if (event.origin !== "https://www.tone3000.com") {
                return;
            }
            const data = event.data;
            if (data.type === "tone3000_selection") {
                onDownload(data.toneUrl);
            }
        }
        window.addEventListener("message", messagehandler);

        return () => {
            window.removeEventListener("message", messagehandler);
        }
    });
    

    return (
        <DialogEx open={props.open} onClose={()=>{ onCancel()}} fullScreen={true} tag="tone3000Dlg" onEnterKey={()=>{}} >

            <div style={{display: "flex", flexDirection: "column", width: "100%", height: "100%"}}>
                    <div style={{ flex: "0 0 auto" }}>
                        <AppBar style={{
                            position: 'relative',
                            top: 0, left: 0
                        }}  >
                            <Toolbar>
                                <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={()=> onCancel()} 
                                aria-label="back"
                                >
                                    <ArrowBackIcon />
                                </IconButtonEx>
                                <Typography noWrap variant="h6"
                                    sx={{ marginLeft: 2, flex: 1 }}
                                >
                                    Tone3000 Direct Download
                                </Typography>
                            </Toolbar>
                        </AppBar>
                    </div>
            
                <iframe style={{flex: "1 1 1px", border: "none"}}
                        src={ selectUrl}
                        >
                </iframe>
            </div>
        </DialogEx>

    );
}

export default Tone3000Dialog;