/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

import React from 'react';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButtonEx from './IconButtonEx';
import Button from '@mui/material/Button';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';
import DialogEx from './DialogEx';
import DialogTitle from '@mui/material/DialogTitle';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';

import ReportGmailerrorredIcon from '@mui/icons-material/ReportGmailerrorred';

export interface OkDialogProps {
    open: boolean,
    title?: string,
    text: string,
    okButtonText?: string,
    onClose: () => void
};

export interface OkDialogState {
};

export default class OkDialog extends React.Component<OkDialogProps, OkDialogState> {


    constructor(props: OkDialogProps) {
        super(props);
        this.state = {
        };
    }

    render() {
        let props = this.props;
        let { open, okButtonText, text, onClose } = props;

        const handleClose = () => {
            onClose();
        };

        const handleOk = () => {
            onClose();
        }
        return (
            <DialogEx tag="alert" open={open} onClose={handleClose}
                onEnterKey={handleOk}
                maxWidth="sm"
                style={{ userSelect: "none" }}
            >
                {props.title && (
                    <DialogTitle id="alert-dialog-title" style={{ userSelect: "none", padding: 0, margin: 0 }}>
                        <Toolbar style={{paddingLeft: 24, paddingRight: 24,margin: 0}} >
                            <IconButtonEx 
                            tooltip="Back" edge="start" color="inherit" onClick={()=>{this.props.onClose();}} aria-label="back"
                            >
                                <ArrowBackIcon />
                            </IconButtonEx>

                            <Typography noWrap variant="body1" style={{ flex: "1 1 auto" }}>{props.title}</Typography>
                        </Toolbar>
                    </DialogTitle>
                )}

                <DialogContent  >
                    <div style={{ display: "flex", flexFlow: "row nowrap", gap: 16, marginLeft: 32, marginRight: 32, alignItems: "start" }}>
                        <ReportGmailerrorredIcon style={{ color: "var(--color-error)", fontSize: 48, flexShrink: 0, opacity: 0.8 }} />
                        <Typography variant="body2" style={{ marginTop:16}}
                        >{text}</Typography>
                    </div>
                </DialogContent>
                <DialogActions>

                    <Button variant="dialogPrimary" onClick={handleOk} style={{marginRight: 24}} >
                        {okButtonText ? okButtonText : "OK"}
                    </Button>
                </DialogActions>
            </DialogEx>
        );
    }
}