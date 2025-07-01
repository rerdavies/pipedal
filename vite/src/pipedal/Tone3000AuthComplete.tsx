
import React from 'react';
import Typography from '@mui/material/Typography';

import { styled } from '@mui/material/styles';


const Frame = styled('div', {
    name: 'MuiStat', // The component name
    slot: 'root', // The slot name
})(({ theme }) => ({
    flex: "1 1 auto",
    gap: theme.spacing(0.5),
    padding: theme.spacing(3, 4),
    backgroundColor: theme.palette.background.paper,
    width: "100%",
    height: "100%"
}));


interface Tone3000AuthCompleteProps {

}

function Tone3000AuthComplete(props: Tone3000AuthCompleteProps) {

    function postApiKey() {
        let url = new URL(window.location.href);
        let apiKey = url.searchParams.get("api_key");
        if (apiKey) {
            let myUrl = new URL(window.location.href);
            let port = myUrl.port;
            if (port === "5173") { // when running debug server.
                port = "8080";
            }
            let postUrl = "http://" + myUrl.hostname + ":" + port + "/var/Tone3000Auth?api_key=" + encodeURIComponent(apiKey);
            fetch(postUrl, {
                method: "POST",
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ api_key: apiKey })
            }).then((response) => {
                if (!response.ok) {
                    alert("Failed to post API key to Pipedal: " + response.statusText);
                }
                return response.json();
            })
                .catch((error) => {
                    alert("Error posting API key to Pipedal: " + error.message);
                })
        } else {
            alert("No API key found in URL.");
        }
    }
    React.useEffect(() => {
        postApiKey();
        return () => { };
    })

    return (
        <Frame style={{
            position: "absolute", top: 0, left: 0, right: 0, bottom: 0,
        }}>
            <div style={{ display: "flex", flexFlow: "row nowrap", alignItems: "start" }}>
                <div style={{ flex: "1 1 1px" }} />
                <div style={{
                    display: "flex", flexFlow: "column nowrap", alignItems: "start",
                    maxWidth: 600, marginLeft: "auto", marginRight: "auto", gap: 16, padding: 32
                }}>
                    <Typography variant="h5">TONE3000 Authorization Complete</Typography>
                    <Typography variant="body1">
                        You have successfully obtained an access token from TONE3000, and can now download Toob Neural Amp models from TONE3000
                        directly.</Typography>
                    <Typography variant="body1">
                        Close this page and return to Pipedal in order to continue.</Typography>
                </div>
                <div style={{ flex: "2 2 1px" }} />
            </div>
        </Frame>
    );

}

export default Tone3000AuthComplete;