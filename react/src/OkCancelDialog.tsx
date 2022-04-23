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

import React  from 'react';
import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';
import DialogEx from './DialogEx';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';


export interface OkCancelDialogProps {
    open: boolean,
    text: string,
    okButtonText: string,
    onOk: () => void,
    onClose: () => void
};

export interface OkCancelDialogState {
};

export default class OkCancelDialog extends React.Component<OkCancelDialogProps, OkCancelDialogState> {


    constructor(props: OkCancelDialogProps) {
        super(props);
        this.state = {
        };
    }

    render() {
        let props = this.props;
        let { open,okButtonText,text, onClose, onOk } = props;

        const handleClose = () => {
            onClose();
        };

        const handleOk = () => {
            onOk();
        }
        return (
            <DialogEx tag="OkCancelDialog" open={open} onClose={handleClose} 
                style={{userSelect: "none"}}
                >
                <DialogContent>
                    <Typography
                    >{text}</Typography>
                </DialogContent>
                <DialogActions>
                    <Button onClick={handleClose} color="primary">
                        Cancel
                    </Button>
                    <Button onClick={handleOk} color="secondary" >
                        {okButtonText}
                    </Button>
                </DialogActions>
            </DialogEx>
        );
    }
}