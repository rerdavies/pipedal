// Copyright (c) 2023 Robin Davies
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

import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';


import Button from '@mui/material/Button';
import FileUploadIcon from '@mui/icons-material/FileUpload';
import AudioFileIcon from '@mui/icons-material/AudioFile';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';

import DialogActions from '@mui/material/DialogActions';
import DialogTitle from '@mui/material/DialogTitle';
import DeleteIcon from '@mui/icons-material/Delete';
import IconButton from '@mui/material/IconButton';

import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { PiPedalFileProperty } from './Lv2Plugin';
import ButtonBase from '@mui/material/ButtonBase';
import Typography from '@mui/material/Typography';
import DialogEx from './DialogEx';

export interface FilePropertyDialogProps {
    open: boolean,
    fileProperty: PiPedalFileProperty,
    selectedFile: string,
    onOk: (fileProperty: PiPedalFileProperty, selectedItem: string) => void,
    onCancel: () => void
};

export interface FilePropertyDialogState {
    fullScreen: boolean;
    selectedFile: string;
    hasSelection: boolean;
    files: string[];
};

export default class FilePropertyDialog extends ResizeResponsiveComponent<FilePropertyDialogProps, FilePropertyDialogState> {


    constructor(props: FilePropertyDialogProps) {
        super(props);


        this.model = PiPedalModelFactory.getInstance();

        this.state = {
            fullScreen: false,
            selectedFile: props.selectedFile,
            hasSelection: false,
            files: []
        };

        this.requestFiles();
    }
    private mounted: boolean = false;
    private model: PiPedalModel;

    private lastFileProperty: PiPedalFileProperty  = new PiPedalFileProperty();

    private requestFiles() {
        if (!this.props.open) 
        {
            return;
        }
        if (this.props.fileProperty.directory === "")
        {
            return;
        }
        if (this.lastFileProperty === this.props.fileProperty)
        {
            return;
        }
        this.lastFileProperty = this.props.fileProperty;

        this.model.requestFileList(this.props.fileProperty)
        .then((files) => {
            if (this.mounted)
            {
                this.setState({files: files,hasSelection: this.isFileInList(files,this.state.selectedFile)});
            }
        }).catch((error)=>{
            this.model.showAlert(error.toString())
        });
    }

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
    componentDidUpdate(prevProps: Readonly<FilePropertyDialogProps>, prevState: Readonly<FilePropertyDialogState>, snapshot?: any): void {
        super.componentDidUpdate?.(prevProps,prevState,snapshot);
        if (prevProps.fileProperty !== this.props.fileProperty)
        {
            this.requestFiles()
        }
    }

    private fileNameOnly(path: string): string {
        let slashPos = path.lastIndexOf('/');
        if (slashPos < 0) {
            slashPos = 0;
        } else {
            ++slashPos;
        }
        let extPos = path.lastIndexOf('.');
        if (extPos < 0 || extPos < slashPos) {
            extPos = path.length;
        }

        return path.substring(slashPos, extPos);

    }

    private isFileInList(files: string[],file: string) {

        let hasSelection = false;
        for (var listFile of files) {
            if (listFile === file) {
                hasSelection = true;
                break;
            }
        }
        return hasSelection;
    }

    onSelect(selectedFile: string) {
        this.setState({ selectedFile: selectedFile, hasSelection: this.isFileInList(this.state.files,selectedFile) })
    }

    mapKey: number = 0;
    render() {
        this.mapKey = 0;
        return this.props.open &&
            (
            <DialogEx onClose={() => this.props.onCancel()} open={this.props.open} tag="FilePropertyDialog" fullWidth maxWidth="xl" style={{ height: "90%" }}
                PaperProps={{ style: { minHeight: "90%", maxHeight: "90%", overflowY: "visible" } }}
            >
                <DialogTitle >
                    <div>
                        <IconButton edge="start" color="inherit" onClick={()=> {this.props.onCancel();}} aria-label="back"
                        >
                            <ArrowBackIcon fontSize="small" style={{opacity: "0.6"}} />
                        </IconButton>
                    <Typography display="inline" >{this.props.fileProperty.name}</Typography>
                    </div>
                </DialogTitle>
                <div style={{ flex: "0 0 auto", height: "1px", background: "rgba(0,0,0,0.2" }}>&nbsp;</div>
                <div style={{ flex: "1 1 auto", height: "300", display: "flex", flexFlow: "row nowrap", overflowX: "auto", overflowY: "visible" }}>
                    <div style={{ flex: "1 1 100%", display: "flex", flexFlow: "column wrap", justifyContent: "start", alignItems: "flex-start",paddingLeft: 16,paddingTop:16 }}>
                        {
                            this.state.files.map(
                                (value: string, index: number) => {
                                    let selected = value === this.state.selectedFile;
                                    let selectBg = selected ? "rgba(0,0,0,0.15)" : "rgba(0,0,0,0.0)";
                                    return (
                                        <ButtonBase key={ this.mapKey++ }
                                            style={{ width: "320px", flex: "0 0 48px", position: "relative"}}
                                            onClick={() => this.onSelect(value)}
                                        >
                                            <div style={{ position: "absolute", background: selectBg, width: "100%", height: "100%", borderRadius: 4 }} />
                                            <div style={{ display: "flex", flexFlow: "row nowrap", justifyContent: "start", alignItems: "center", width: "100%", height: "100%" }}>
                                                <AudioFileIcon style={{ flex: "0 0 auto", opacity: 0.7, marginRight: 8, marginLeft: 8 }} />
                                                <Typography noWrap style={{ flex: "1 1 auto", textAlign: "left" }}>{this.fileNameOnly(value)}</Typography>
                                            </div>
                                        </ButtonBase>
                                    );
                                }
                            )
                        }
                        </div>
                </div>
                <div style={{ flex: "0 0 auto", height: "1px", background: "rgba(0,0,0,0.2" }}>&nbsp;</div>
                <DialogActions style={{ justifyContent: "stretch" }}>
                    <div style={{ display: "flex", width: "100%", flexFlow: "row nowrap" }}>
                        <Button style={{ flex: "0 0 auto" }} startIcon={<FileUploadIcon />}>
                            <input hidden accept="audio/x-wav" name="Upload Impuse File" type="file" multiple />
                            Upload
                        </Button>
                        <IconButton style={{visibility: (this.state.hasSelection? "visible": "hidden")}} aria-label="delete selected file" component="label" color="primary" >
                            <DeleteIcon fontSize='small' />
                        </IconButton>
                        <div style={{ flex: "1 1 auto" }}>&nbsp;</div>
                        <Button onClick={() => {this.props.onCancel();}} aria-label="cancel">
                            Cancel
                        </Button>
                        <Button style={{ flex: "0 0 auto" }} onClick={() => {this.props.onOk(this.props.fileProperty,this.state.selectedFile);}} color="secondary" disabled={!this.state.hasSelection} aria-label="select">  
                            Select
                        </Button>
                    </div>
                </DialogActions>
            </DialogEx>
        );
    }
}