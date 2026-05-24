import { JSX } from "@emotion/react/jsx-dev-runtime";
import { PiPedalModel, State  } from "./PiPedalModel";
import { Button, Typography } from "@mui/material";
import { useEffect, useState } from "react";
import LinearProgress from "@mui/material/LinearProgress";
import Tone3000DownloadProgress from "./Tone3000DownloadProgress";
import Dialog from "@mui/material/Dialog";
import DialogContent from "@mui/material/DialogContent";
import DialogActions from "@mui/material/DialogActions";

export type OnTone3000DownloadHandler = (tone3000DownloadUrl: string) => void;



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
    if (progress && progress.total !== 0) {
        filename = `(${progress.progress}/${progress.total}) ${filename}`;
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
