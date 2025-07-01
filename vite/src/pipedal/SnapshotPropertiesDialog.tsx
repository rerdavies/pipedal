/*
 * MIT License
 * 
 * Copyright (c) 2024 Robin E. R. Davies
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


import Button from '@mui/material/Button';
import TextField from '@mui/material/TextField';
import Typography from '@mui/material/Typography';
import DialogEx from './DialogEx';
import DialogTitle from '@mui/material/DialogTitle';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButtonEx from './IconButtonEx';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import { colorKeys } from "./MaterialColors";
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import ColorDropdownButton, { DropdownAlignment } from './ColorDropdownButton';

const snapshotColors = colorKeys.slice(0,100);
const NBSP = "\u00A0";

export interface SnapshotPropertiesDialogProps {
    open: boolean,
    name: string,
    editing: boolean,
    snapshotIndex: number,
    color: string,
    onOk: (snapshotId: number, name: string, color: string, editing: boolean) => void,
    onClose: () => void
};

export interface SnapshotPropertiesDialogState {
    name: string,
    color: string,
    nameErrorMessage: string,
    compactVertical: boolean,
};

export default class SnapshotPropertiesDialog extends ResizeResponsiveComponent<SnapshotPropertiesDialogProps, SnapshotPropertiesDialogState> {


    constructor(props: SnapshotPropertiesDialogProps) {
        super(props);
        this.state = this.stateFromProps();
    }
    getCompactVertical() {
        return window.innerHeight < 450;
    }
    stateFromProps() {
        let color = this.props.color;
        if (color.length === 0) {
            color = snapshotColors[0];
        }
        let name = this.props.name;
        if (name.length === 0 && !this.props.editing)
        {
            name = "Snapshot " + (this.props.snapshotIndex+1);
        }
        return {
            name: name,
            color: color,
            nameErrorMessage: "",
            compactVertical: this.getCompactVertical()
        };
    }

    onWindowSizeChanged(width: number, height: number): void {
        super.onWindowSizeChanged(width,height);
        this.setState({compactVertical: this.getCompactVertical() })
    }

    handleOk() {
        if (this.state.name === "") {
            this.setState({ nameErrorMessage: "* Required" });
        } else {
            this.props.onOk(
                this.props.snapshotIndex,
                this.state.name,
                this.state.color,
                this.props.editing
            );
        }
    }

    render() {
        let props = this.props;

        const handleClose = () => {
            props.onClose();
        };

        let okButtonText = this.props.editing ? "OK" : "Save";
        return (
            <DialogEx maxWidth="sm" fullWidth={true} tag="snapshotProps" open={this.props.open} onClose={handleClose} 
                style={{ userSelect: "none" }} fullScreen={this.state.compactVertical}
                onEnterKey={()=> { this.handleOk(); }}
            >
                <DialogTitle>
                    <div>
                        <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={() => { this.props.onClose(); }} aria-label="back"
                        >
                            <ArrowBackIcon fontSize="small" style={{ opacity: "0.6" }} />
                        </IconButtonEx>
                        <Typography display="inline" variant="body1" color="textSecondary" >
                            {this.props.editing ? "Edit snapshot" : "Save snapshot"}
                        </Typography>
                    </div>

                </DialogTitle>
                <DialogContent style={{paddingBottom: 0}}>

                    <TextField variant="standard" style={{ flexGrow: 1, flexBasis: 1 }}
                        autoComplete="off"
                        spellCheck="false"
                        error={this.state.nameErrorMessage.length !== 0}
                        id="name"
                        label="Name"
                        type="text"
                        fullWidth
                        helperText={this.state.nameErrorMessage.length === 0 ? NBSP : this.state.nameErrorMessage}
                        value={this.state.name}
                        onChange={(e) => this.setState({ name: e.target.value, nameErrorMessage: "" })}
                        InputLabelProps={{
                            shrink: true
                        }}
                    />
                    <Typography variant="caption" color="textSecondary">Color</Typography>
                    <ColorDropdownButton aria-label="color" currentColor={this.state.color} dropdownAlignment={DropdownAlignment.SE}
                          onColorChange={(newcolor) => {
                            this.setState({color: newcolor});
                          }} />
                          
                </DialogContent>
                <DialogActions>
                    <Button variant="dialogSecondary" onClick={handleClose} >
                        Cancel
                    </Button>
                    <Button variant="dialogPrimary" onClick={()=> {this.handleOk();}}  >
                        {okButtonText}
                    </Button>
                </DialogActions>
            </DialogEx>
        );
    }
}