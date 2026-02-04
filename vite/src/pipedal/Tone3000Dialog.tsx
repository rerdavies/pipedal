import { JSX } from "@emotion/react/jsx-dev-runtime";
import { PiPedalModel, getErrorMessage } from "./PiPedalModel";
import DialogEx from "./DialogEx";
import DialogContent from "@mui/material/DialogContent";
import DialogAction from "@mui/material/DialogActions";
import { Button, Typography } from "@mui/material";
import { useEffect, useState } from "react";
import LinearProgress from "@mui/material/LinearProgress";


export type OnTone3000DownloadHandler = (tone3000DownloadUrl: string) => void;


// used to check for online status.
let TONE3000_PING_URL = "https://www.tone3000.com/robots.txt";

export class Tone3000DownloadHandler {

    private async handleTone3000Download(tone3000DownloadUrl: string): Promise<void> {
        try {
            // Dialog can close. We'll use the following await to manage
            // lifecycle from here onward.
            this.stopTone3000DialogMonitor();

            this.handleTone3000DownloadComplete();

            await this.model.downloadModelsFromTone3000(tone3000DownloadUrl,this.downloadPath);
            return;
        } catch (e) {
            this.handleTone3000DownloadError(getErrorMessage(e));
            return;
        }
    }

    private messageEventListener: ((event: MessageEvent) => void) | null = null;

    private model: PiPedalModel;

    public constructor(model: PiPedalModel) {
        this.model = model;
        this.messageEventListener = (event: MessageEvent) => {
            if (event.data.type !== "tone3000Download") {
                return;
            }
            this.handleTone3000Download(event.data.toneUrl as string);
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

    private appId: string = "pipedal_app";
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

    private tone3000SelectUrl(): string {
        return `https://www.tone3000.com/api/v1/select?app_id=${this.appId}&redirect_url=${this.redirectUrl()}`;
    }


    private handleTone3000DialogClosed() {
        let t = this.onClosedCallback;
        this.onClosedCallback = undefined;
        this.onErrorCallback = undefined;
        this.onDownloadCompleteCallback = undefined;
        if (t) {
            t();
        }
        this.stopTone3000DialogMonitor();
    }
    private handleTone3000Downloadstarted() {
        if (this.onDownloadStartedCallback) {
            this.onDownloadStartedCallback();
        }
    }
    private handleTone3000DownloadComplete() {
        let t = this.onDownloadCompleteCallback;
        this.onClosedCallback = undefined;
        this.onErrorCallback = undefined;
        this.onDownloadCompleteCallback = undefined;
        if (t) {
            t();
        }
        this.stopTone3000DialogMonitor();
    }
    private handleTone3000DownloadError(errorMessage: string) {
        let t = this.onErrorCallback;
        this.onClosedCallback = undefined;
        this.onErrorCallback = undefined;
        this.onDownloadCompleteCallback = undefined;
        if (t) {
            t(errorMessage);
        }
        this.stopTone3000DialogMonitor();
    }

    private tone3000DialogInterval: number | undefined = undefined;

    private stopTone3000DialogMonitor() {
        if (this.tone3000DialogInterval !== undefined) {
            clearInterval(this.tone3000DialogInterval);
            this.tone3000DialogInterval = undefined;
        }
    }
    private startTone3000DialogMonitor() {
        this.tone3000DialogInterval = setInterval(() => {
            if (this.popupWindow == null || this.popupWindow.closed) {
                this.stopTone3000DialogMonitor();
                this.popupWindow = null;
                this.handleTone3000DialogClosed();
            }
        }, 500);
    }
    private downloadPath: string = "";
    private onClosedCallback: (() => void) | undefined = undefined;
    private onErrorCallback: ((errorMessage: string) => void) | undefined = undefined;
    private onDownloadStartedCallback: (() => void) | undefined = undefined;
    private onDownloadCompleteCallback: (() => void) | undefined = undefined;

    public launchTone3000Dialog(
        downloadPath: string,
        onClosed: () => void,
        onError: (errorMessage: string) => void,
        onDownloadStarted: () => void,
        onDownloadComplete: () => void
    ) {
        this.closeTone3000Dialog();
        this.downloadPath = downloadPath;
        this.onClosedCallback = onClosed;
        this.onDownloadCompleteCallback = onDownloadComplete;
        this.onDownloadStartedCallback = onDownloadStarted;
        this.onErrorCallback = onError;

        fetch(TONE3000_PING_URL, { cache: "no-cache" })
            .then((response) => {
                if (!response.ok) {
                    this.handleTone3000DownloadError(`Unable to connect to Tone3000: ${response.status}${response.statusText}`);
                    return;
                }

                // online
                let popupWidth = Math.floor(window.innerWidth * 0.8);
                let popupHeight = Math.floor(window.innerHeight * 0.8);
                if (window.innerWidth < 410) {
                    popupWidth = 400;
                }

                this.popupWindow =
                    window.open(
                        this.tone3000SelectUrl(),
                        "popup",
                        `innerWidth=${popupWidth},innerHeight=${popupHeight},scrollbars=yes`
                    );
                this.startTone3000DialogMonitor();

                if (!this.popupWindow) {
                    console.error("Failed to open Tone3000 dialog popup window.");
                    this.handleTone3000DownloadError("Cannot open popup window.");
                    return;
                }
            })
            .catch((error) => {
                // offline
                this.handleTone3000DownloadError("Not online. Unable to connect to Tone3000 servers.");
                return;
            });

    }
    public closeTone3000Dialog(): void {
        this.handleTone3000DialogClosed();
        this.stopTone3000DialogMonitor();
        if (this.popupWindow) {
            try {

                let t = this.popupWindow;
                this.popupWindow = null;
                t.close();
            } catch (e) {
                console.error("Error closing Tone3000 dialog popup:", e);
            }
        }
    }
}

export interface Tone3000DownloadDialogProps {

    onClose: () => void,
    onDownloadComplete: () => void,
    downloadPath: string
}
export function Tone3000DownloadDialog(props: Tone3000DownloadDialogProps): JSX.Element {
    const model = PiPedalModel.getInstance();
    let { onClose, onDownloadComplete, downloadPath } = props;
    let [downloading, setDownloading] = useState(false);

    useEffect(() => {
        model.showTone3000DownloadDialog(
            downloadPath,
            () => {
                onClose();
            },
            () => {
                setDownloading(true);
            },
            () => {
                onDownloadComplete();
            }
        );
        return () => {
            model.closeTone3000DownloadDialog();
        };
    });
    return (
        <DialogEx tag="tone3000-download"
            open={true}
            onClose={() => {
                onClose();
            }}
            onEnterKey={() => { }}
        >
            <DialogContent>
                <Typography variant="body2">Downloading from Tone3000...</Typography>
                {downloading &&
                    <div style={{ display: "flex", justifyContent: "center", alignItems: "center" }}>
                        <LinearProgress style={{ marginTop: 16, flexGrow: 1 }} />
                    </div>
                }
            </DialogContent>
            <DialogAction>
                {!downloading && (
                    <Button
                        onClick={() => {
                            onClose();
                        }}>
                        Cancel
                    </Button>
                )}
            </DialogAction>
        </DialogEx>
    );
}
