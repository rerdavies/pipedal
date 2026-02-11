import { JSX } from "@emotion/react/jsx-dev-runtime";
import { PiPedalModel, State, getErrorMessage } from "./PiPedalModel";
import { Button, Typography } from "@mui/material";
import { useEffect, useState } from "react";
import LinearProgress from "@mui/material/LinearProgress";
import Tone3000DownloadProgress from "./Tone3000DownloadProgress";
import Dialog from "@mui/material/Dialog";
import DialogContent from "@mui/material/DialogContent";
import DialogActions from "@mui/material/DialogActions";
import Tone3000DownloadType from "./Tone3000DownloadType";




export type OnTone3000DownloadHandler = (tone3000DownloadUrl: string) => void;

// used to check for online status.
let TONE3000_PING_URL = "https://www.tone3000.com/robots.txt";

export class Tone3000DownloadHandler {

    private async handleTone3000Download(tone3000DownloadUrl: string, downloadType: Tone3000DownloadType): Promise<void> {
        try {
            await this.model.downloadModelsFromTone3000(tone3000DownloadUrl, this.downloadPath, downloadType);
            return;
        } catch (e) {
            this.handleTone3000DownloadError(getErrorMessage(e));
            return;
        }
    }

    private messageEventListener: ((event: MessageEvent) => void) | null = null;

    private model: PiPedalModel;
    private downloadType: Tone3000DownloadType = Tone3000DownloadType.Nam;

    public constructor(model: PiPedalModel) {
        this.model = model;
        this.messageEventListener = (event: MessageEvent) => {
            if (event.data.type !== "tone3000Download") {
                return;
            }
            this.handleTone3000Download(event.data.toneUrl as string, this.downloadType);
        };

        window.addEventListener("message", this.messageEventListener);
    }

    public dispose(): void {
        if (this.messageEventListener) {
            window.removeEventListener("message", this.messageEventListener);
            this.messageEventListener = null;
        }
    }

    private popupWindow: Window | null = null;

    private appId: string = "pipedal_app4";
    private redirectUrl(): string {
        let hostname = window.location.hostname;
        let port = window.location.port;
        let protocol = window.location.protocol;
        let serverUrl: string;
        if (protocol === "http:" && (port === "80" || port === "")) {
            serverUrl = `${protocol}//${hostname}`;
        } else if (protocol === "https:" && (port === "443" || port === "")) {
            serverUrl = `${protocol}//${hostname}`;
        } else {
            serverUrl = `${protocol}//${hostname}:${port}`;
        }
        return `${serverUrl}/public/handleTone3000download.html`;
    }

    private tone3000SelectUrl(downloadType: Tone3000DownloadType): string {
        let redirectUrl_ = this.redirectUrl();
        let result = `https://www.tone3000.com/api/v1/select?app_id=${this.appId}&redirect_url=${redirectUrl_}`;
        if (downloadType === Tone3000DownloadType.CabIr) {
            result += "&gear=ir";
        }
        return result;
    }


    private handleTone3000DownloadComplete() {
        if (this.popupWindow) {
            if (!this.popupWindow.closed) {
                this.popupWindow.close();
            }
            this.popupWindow = null;
        }
        this.model.hideTone3000DownloadStatus();

    }
    closeTone3000Popup() {
        this.handleTone3000DownloadComplete();
    }
    private handleTone3000DownloadError(errorMessage: string) {
        if (this.open) 
        {
            this.handleTone3000DownloadComplete();
            this.model.showAlert(errorMessage);
        }
    }

    private downloadPath: string = "";
    private open = false;

    private launchInstance = 0;
    public async launchTone3000Popup(
        downloadType: Tone3000DownloadType,
        downloadPath: string,

    ): Promise<void> {
        this.closeTone3000Dialog();
        // online

        this.downloadPath = downloadPath;
        this.downloadType = downloadType;

        let popupWidth = Math.floor(window.innerWidth * 0.8);
        let popupHeight = Math.floor(window.innerHeight * 0.8);
        if (window.innerWidth < 410) {
            popupWidth = 400;
        }

        this.open = true;

        this.model.showTone3000DownloadStatus();

        let launchInstance = ++this.launchInstance;
        let cancelled = () => {
            return (launchInstance !== this.launchInstance) || this.popupWindow == null || this.popupWindow.closed;
        }

        try {

            this.popupWindow =
                window.open(
                    this.tone3000SelectUrl(downloadType),
                    "popup",
                    `innerWidth=${popupWidth},innerHeight=${popupHeight},scrollbars=yes`
                );

            if (!this.popupWindow) {
                console.error("Failed to open TONE3000 dialog popup window.");
                throw new Error("Cannot open popup window.");
            }
            // No procedure for detecting 404, so let's ping the server to see if it's reachable, and cancel if it's not.
            try {
                let response = await fetch(TONE3000_PING_URL, { method: "HEAD", cache: "no-cache" });
                if (cancelled()) return;
                if (!response.ok) {
                    // offline
                    throw new Error("Unable to connect to the TONE3000 server.");
                }
            } catch (error) {
                throw new Error("Not online. Unable to connect to the TONE3000 server. Your client device must have access to the internet to use this feature..");
            };

            let pipedalServerCanReachTone3000 = false;

            // check to see that the PiPedal server can reach TONE3000.
            try {
                pipedalServerCanReachTone3000 = await this.model.pingTone3000Server();
                if (cancelled()) return;
            } catch (error) {
            }
            if (!pipedalServerCanReachTone3000) {
                throw new Error("The PiPedal server cannot reach a TONE3000 server. The PiPedal server must have access to the internet to use this feature.");
            }
            while (true) {
                await new Promise(resolve => setTimeout(resolve, 500));
                if (launchInstance != this.launchInstance) {
                    // A new dialog launch has been initiated. Stop monitoring this one.
                    return;
                }
                if (this.popupWindow == null || this.popupWindow.closed) {
                    this.closeTone3000Dialog();
                    return;
                }
            }
        } catch (error) {
            this.handleTone3000DownloadError(getErrorMessage(error));
        }
    }

    public closeTone3000Dialog(): void {
        this.handleTone3000DownloadComplete();
    }
}


export function Tone3000DownloadStaus(
    props: {
        zindex: number;
    }): JSX.Element {
    const model = PiPedalModel.getInstance();
    const [downloading, setDownloading] = useState<boolean>(model.tone3000Downloading.get());
    const [progress, setProgress] = useState<Tone3000DownloadProgress | null>(model.tone3000DownloadProgress.get());

    useEffect(() => {
        let onDownloadingChanged = (value: boolean) => {

            setDownloading(value);
        }
        model.tone3000Downloading.addOnChangedHandler(onDownloadingChanged);

        let onProgressChanged = (value: Tone3000DownloadProgress | null) => {
            setProgress(value);
        }
        model.tone3000DownloadProgress.addOnChangedHandler(onProgressChanged);

        return () => {
            model.tone3000Downloading.removeOnChangedHandler(onDownloadingChanged);
            model.tone3000DownloadProgress.removeOnChangedHandler(onProgressChanged);
        }
    })
    let open = downloading;
    if (model.state.get() !== State.Ready) {
        open = false;
    }
    let filename = progress?.title ?? "\u00A0";
    if (filename === "") {
        filename = "\u00A0";
    }
    return (
        <Dialog
            open={open}
            onClose={() => { /* Do nothing */ }}
        >
            <DialogContent style={{ marginBottom: 0, paddingBottom: 0 }}>
                <div style={{ display: "flex", flexFlow: "column nowrap", alignItems: "stretch" }}>
                    <Typography noWrap variant="body2">Downloading from TONE3000...</Typography>
                    <LinearProgress
                        style={{ visibility: progress !== null ? "visible" : "hidden", marginTop: 16 }}
                        variant="determinate"
                        value={(progress && progress.total !== 0) ? progress.progress / progress.total * 100 : 0}
                    />
                    <Typography noWrap variant="caption"
                        style={{
                            width: 300, maxWidth: 300, paddingTop: 8, paddingLeft: 16, paddingRight: 16,
                            paddingBottom: 0, marginBottom: 0
                        }}
                    >{filename}</Typography>
                </div>
            </DialogContent>
            <DialogActions>
                <Button variant="dialogSecondary"
                    onClick={() => {
                        model.closeTone3000DownloadPopup();
                    }}
                >Cancel</Button>
            </DialogActions>

        </Dialog>

    );
}
