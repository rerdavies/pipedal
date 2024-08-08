// Copyright (c) 2022 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import React from 'react';
import Button from '@mui/material/Button';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';

import DialogEx from './DialogEx';
import DialogTitle from '@mui/material/DialogTitle';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import Typography from '@mui/material/Typography';
import { UiFileProperty,UiFileType } from './Lv2Plugin';
import SvgIcon from '@mui/material/SvgIcon';
import ErrorIcon from '@mui/icons-material/Error';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButton from '@mui/material/IconButton';
import CheckCircleOutlineIcon from '@mui/icons-material/CheckCircleOutline';
import CircularProgress from '@mui/material/CircularProgress';


// const PRESET_EXTENSION = ".piPreset";
// const BANK_EXTENSION = ".piBank";
// const PLUGIN_PRESETS_EXTENSION = ".piPluginPresets";

export interface UploadFileDialogProps {
    open: boolean,
    onClose: () => void,
    onUploaded: (fileName: string) => void,
    uploadPage: string,
    fileProperty: UiFileProperty

};

enum FileUploadStatus {
    Ready,
    Uploading,
    Uploaded,
    Error,
    Cancelled
}

let nextId = 0;

class FileUploadEntry {
    constructor(file: File) {
        this.id = nextId++;
        this.file = file;
        this.abortController = undefined;
        this.status = FileUploadStatus.Ready;
        this.statusMessage = "";
    }
    status: FileUploadStatus
    statusMessage: string;
    file: File;
    abortController?: AbortController;
    id: number;
}

class FileUploadDisplayEntry {
    constructor(fileUploadEntry: FileUploadEntry) {
        this.name = fileUploadEntry.file.name;
        this.status = fileUploadEntry.status;
        this.statusMessage = fileUploadEntry.statusMessage;
        this.id = fileUploadEntry.id;
    }
    name: string;
    status: FileUploadStatus;
    statusMessage: string;
    id: number;
};

export interface UploadFileDialogState {
    fullScreen: boolean;
    okEnabled: boolean;
    files: FileUploadDisplayEntry[];
};

export default class UploadFileDialog extends ResizeResponsiveComponent<UploadFileDialogProps, UploadFileDialogState> {

    model: PiPedalModel;

    constructor(props: UploadFileDialogProps) {
        super(props);
        this.state = {
            fullScreen: false,
            okEnabled: true,
            files: []
        };
        this.model = PiPedalModelFactory.getInstance();

    }
    mounted: boolean = false;



    onWindowSizeChanged(width: number, height: number): void {
        this.setState({ fullScreen: height < 200 })
    }


    componentDidMount() {
        super.componentDidMount();
        this.mounted = true;

    }
    componentWillUnmount() {
        super.componentWillUnmount();
        this.mounted = false;

    }

    componentDidUpdate(prevProps: Readonly<UploadFileDialogProps>, prevState: Readonly<UploadFileDialogState>, snapshot?: any): void {
        if (prevProps.open !== this.props.open) {
            if (this.props.open) {
                this.uploadList = [];
                this.setState({ files: [], okEnabled: true });
            }
        }
    }

    handleClose(): void {
        this.uploadList = [];
        this.setState({ files: [] });
        this.props.onClose();
    }

    handleCancel() {
        for (let upload of this.uploadList) {
            if (upload.status === FileUploadStatus.Ready || upload.status === FileUploadStatus.Uploading) {
                upload.status = FileUploadStatus.Cancelled;
                upload.statusMessage = "Cancelled.";
            }
            if (upload.abortController) {
                upload.abortController.abort();
                upload.abortController = undefined;
            }

        }
        this.handleClose();
    }
    handleOk(): void {
        this.props.onClose();
    }

    updateUploadDisplayList() {
        let result: FileUploadDisplayEntry[] = [];
        let uploadsPending = false;
        for (let upload of this.uploadList) {
            result.push(new FileUploadDisplayEntry(upload));
            if (upload.status === FileUploadStatus.Ready || upload.status === FileUploadStatus.Uploading) {
                uploadsPending = true;
            }

        }
        this.setState({ files: result, okEnabled: !uploadsPending });
    }

    private uploading = false;
    private wantsScrollTo = false;

    private uploadList: FileUploadEntry[] = [];

    async uploadFiles(files: FileList) {
        for (let i = 0; i < files.length; ++i) {
            let file = files.item(i);
            if (file) {
                this.uploadList.push(new FileUploadEntry(file));
            }
        }
        this.wantsScrollTo = true;
        if (this.uploading) {
            this.updateUploadDisplayList();
            return;
        }
        this.uploading = true;

        try {
            for (let i = 0; i < this.uploadList.length; ++i) {
                let upload = this.uploadList[i];
                if (upload.status !== FileUploadStatus.Ready) {
                    continue;
                }
                upload.status = FileUploadStatus.Uploading;
                upload.statusMessage = "Uploading...";
                this.wantsScrollTo = true;
                this.updateUploadDisplayList();
                try {
                    upload.abortController = new AbortController();
                    if (!this.wantsFile(this.uploadList[i].file))
                    {
                        throw new Error("Invalid file extension.");
                    }
                    let filename = await this.model.uploadFile(this.props.uploadPage, this.uploadList[i].file, "application/octet-stream", upload.abortController);
                    this.props.onUploaded(filename);
                    upload.status = FileUploadStatus.Uploaded;
                    upload.statusMessage = "Uploaded.";
                    upload.abortController = undefined;
                } catch (error: any) {
                    // @ts-ignore: TS2367   // No overlap between FileUploadStatus.Updating and FileUploadStatus.Cancelled
                    if (upload.status !== FileUploadStatus.Cancelled) {
                        upload.status = FileUploadStatus.Error;
                        if (error instanceof Error) {
                            upload.statusMessage = (error as Error).message;
                        } else if (typeof error === "string") {
                            upload.statusMessage = error as string;
                        } else {
                            throw new Error("Unrecgnized exception type: " + error.toString());
                        }
                    }
                }
            }
        } catch (error) {
            this.model.showAlert(error + "");
        };
        this.updateUploadDisplayList();
        this.uploading = false;
    }
    handleDrop(e: React.DragEvent<HTMLDivElement>) {
        e.preventDefault();
        e.stopPropagation();

        e.currentTarget.style.background = "";

        if (this.wantsTransfer(e.dataTransfer)) {
            this.uploadFiles(e.dataTransfer.files);
        }
    }

    private wantsFile(file: File | null) {
        if (file === null) return false;
        return this.props.fileProperty.wantsFile(file.name);

    }
    private wantsTransfer(fileList: DataTransfer): boolean {
        if (fileList.files.length === 0) return false;

        console.log("File count: " + fileList.files.length);
        for (let i = 0; i < fileList.files.length; ++i) {
            let file = fileList.files[i];
            if (this.wantsFile(file)) return true;
        }
        return false;
    }
    handleDragOver(e: React.DragEvent<HTMLDivElement>) {
        e.preventDefault();
        e.stopPropagation();

        e.currentTarget.style.background = "#D8D8D8";
    }

    private static IsAndroid() : boolean
    {
        return /Android/i.test(navigator.userAgent);
    }

    handleButtonSelect(e: any) {
        if (e.currentTarget.files) {
            this.uploadFiles(e.currentTarget.files);
        }
    }
    hasFileList() {
        return this.state.files.length !== 0;
    }

    getIcon(status: FileUploadStatus) {
        let size = 14;

        let style = { width: size, height: size, opacity: 0.75 };
        let top = 0;

        switch (status) {
            case FileUploadStatus.Error:
                return (<div style={{ position: "relative", top: top }}><ErrorIcon color="error" style={style} /></div>);
            case FileUploadStatus.Uploaded:
                return (<div style={{ position: "relative", top: top }}><CheckCircleOutlineIcon color="success" style={style}/></div >);
            case FileUploadStatus.Uploading:
                return (<CircularProgress size={size } style={{ margin: 2  }} />);

            default:
                return (<SvgIcon style={{ width: size, height: size, }} />);
        }
    }
    makeBrowserAcceptList() {
        let result = UiFileType.MergeMimeTypes(this.props.fileProperty.fileTypes);
        return result;
    }

    render() {

        let isAndroid = UploadFileDialog.IsAndroid();
        return (
            <DialogEx tag="uploadFile" open={this.props.open} fullWidth onClose={() => this.handleClose()}
                fullScreen={this.state.fullScreen}
                style={{ userSelect: "none" }}
            >
                <DialogTitle >
                    <div>
                        <IconButton edge="start" color="inherit" onClick={() => { this.handleCancel(); }} aria-label="back"
                        >
                            <ArrowBackIcon fontSize="small" style={{ opacity: "0.6" }} />
                        </IconButton>
                        <Typography display="inline" >Upload</Typography>
                    </div>
                </DialogTitle>
                <DialogContent style={{ paddingBottom: 0 }}>
                    <div style={{
                        width: "100%", height: 140, marginBottom: 0,
                        border: (isAndroid? "2px solid #CCC" :"2px dashed #CCC"),
                        borderRadius: "10px",
                        fontFamily: "Roboto",
                        padding: 0
                    }}
                        onDragEnter={(e) => { this.handleDragOver(e); }}
                        onDragLeave={(e) => { e.currentTarget.style.background = ""; e.preventDefault(); }}
                        onDragOver={(e) => { this.handleDragOver(e); }}
                        onDrop={(e) => {
                            this.handleDrop(e);
                        }}
                    >
                        {!this.hasFileList() && (
                            <Typography noWrap color="textSecondary" align="center" display="block" variant="caption" style={{ width: "100%", marginTop: 20, verticalAlign: "middle" }} >
                                { isAndroid? "Select files": "Drop files here" }</Typography>
                        )}
                        {this.hasFileList() && (
                            <div style={{ width: "100%", height: "100%", overflow: "auto" }}>
                                <table style={{ margin: 8, }}>
                                    {
                                        this.state.files.map(
                                            (upload) => {
                                                let myRef: ((arg: HTMLDivElement) => void) | undefined = undefined;
                                                if (this.wantsScrollTo && upload.status === FileUploadStatus.Uploading) {
                                                    myRef = (refElement: HTMLDivElement | null) => {
                                                        if (refElement && this.wantsScrollTo) {
                                                            let options: ScrollIntoViewOptions = {
                                                                block: "center"
                                                            };
                                                            refElement.scrollIntoView(options);
                                                            this.wantsScrollTo = false;
                                                        }
                                                    }
                                                }
                                                return (
                                                    <tr>
                                                        <td style={{ verticalAlign: "middle" }}>
                                                            <div ref={myRef} style={{ display: "flex", flexFlow: "row nowrap", alignItems: "center" }}>
                                                                {this.getIcon(upload.status)}
                                                                <Typography variant="caption" noWrap display="block" style={{ marginLeft: 4 }}> {upload.name} </Typography>
                                                            </div>
                                                        </td>
                                                        <td style={{ verticalAlign: "middle" }}>
                                                            <Typography variant="caption" noWrap display="block" style={{ marginLeft: 8 }}> {upload.statusMessage} </Typography>
                                                        </td>
                                                    </tr>
                                                );

                                            }
                                        )
                                    }
                                </table>
                            </div>
                        )}


                    </div>
                </DialogContent >

                <DialogActions >
                    <Button
                        component="label" color="primary" style={{ whiteSpace: "nowrap", textOverflow: "ellipsis", marginLeft: 16, width: 120, flex: "0 0 auto" }}
                    >
                        Select&nbsp;Files
                        <input
                            type="file"
                            hidden
                            accept={this.makeBrowserAcceptList()}
                            multiple
                            onInput={(e) => this.handleButtonSelect(e)}
                        />
                    </Button>
                    <div style={{ flex: "1 1 20px" }} />
                    {
                        this.state.okEnabled ?
                            (
                                <Button onClick={() => this.handleClose()} color="primary">
                                    OK
                                </Button>

                            ) : (
                                <Button onClick={() => this.handleCancel()} color="primary">
                                    Cancel
                                </Button>

                            )
                    }
                </DialogActions>
            </DialogEx>
        );
    }
}