/*
 * MIT License
 * 
 * Copyright (c) 2023 Robin E. R. Davies
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

import React, { TouchEvent, PointerEvent, ReactNode, Component, SyntheticEvent } from 'react';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import { PiPedalFileProperty, PiPedalFileType } from './Lv2Plugin';
import Typography from '@mui/material/Typography';
import Input from '@mui/material/Input';
import Select from '@mui/material/Select';
import Switch from '@mui/material/Switch';
import Utility, { nullCast } from './Utility';
import MenuItem from '@mui/material/MenuItem';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import ButtonBase from '@mui/material/ButtonBase'
import MoreHorizIcon from '@mui/icons-material/MoreHoriz';

const MIN_ANGLE = -135;
const MAX_ANGLE = 135;
const FONT_SIZE = "0.8em";



const SELECTED_OPACITY = 0.8;
const DEFAULT_OPACITY = 0.6;
const RANGE_SCALE = 120; // 120 pixels to move from 0 to 1.
const FINE_RANGE_SCALE = RANGE_SCALE * 10; // 1200 pixels to move from 0 to 1.
const ULTRA_FINE_RANGE_SCALE = RANGE_SCALE * 50; // 12000 pixels to move from 0 to 1.


export const StandardItemSize = { width: 80, height: 140 }



const styles = (theme: Theme) => createStyles({
    frame: {
        position: "relative",
        margin: "12px"
    },
    switchTrack: {
        height: '100%',
        width: '100%',
        borderRadius: 14 / 2,
        zIndex: -1,
        transition: theme.transitions.create(['opacity', 'background-color'], {
            duration: theme.transitions.duration.shortest
        }),
        backgroundColor: theme.palette.secondary.main,
        opacity: theme.palette.mode === 'light' ? 0.38 : 0.3
    },
    displayValue: {
        position: "absolute",
        top: 0,
        left: 0,
        right: 0,
        bottom: 4,
        textAlign: "center",
        background: "white",
        color: "#666",
        // zIndex: -1,
    }
});


export interface FilePropertyControlProps extends WithStyles<typeof styles> {
    instanceId: number;
    fileProperty: PiPedalFileProperty;
    value: string;
    onFileClick: (fileProperty:  PiPedalFileProperty,value: string) => void;
    theme: Theme;
}
type FilePropertyControlState = {
    error: boolean;
    showDialog: boolean;
};

const FilePropertyControl =
    withStyles(styles, { withTheme: true })(
        class extends Component<FilePropertyControlProps, FilePropertyControlState>
        {

            model: PiPedalModel;


            constructor(props: FilePropertyControlProps) {
                super(props);

                this.state = {
                    error: false,
                    showDialog: false
                };
                this.model = PiPedalModelFactory.getInstance();
            }

            componentWillUnmount() {
            }
            inputChanged: boolean = false;

            onDrag(e: SyntheticEvent) {
                e.preventDefault();
            }

            onFileClick() {
                this.props.onFileClick(this.props.fileProperty,this.props.value);
            }
            private fileNameOnly(path: string): string {
                let slashPos = path.lastIndexOf('/');
                if (slashPos < 0)
                {
                    slashPos = 0;
                } else {
                    ++slashPos;
                }
                let extPos = path.lastIndexOf('.');
                if (extPos < 0 || extPos < slashPos)
                {
                    extPos = path.length;
                } 

                return path.substring(slashPos,extPos);

            }   
            render() {
                let classes = this.props.classes;
                let fileProperty = this.props.fileProperty;

                let value = this.props.value;
                if (!value || value.length === 0)
                {
                    value = "\u00A0";
                }
                value = this.fileNameOnly(value);

                let item_width = 264;

                return (
                    <div style={{ display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "flex-start", width: item_width, margin: 8, paddingLeft: 8}}>
                        {/* TITLE SECTION */}
                        <div style={{ flex: "0 0 auto", width: "100%", marginBottom: 8, marginLeft: 0, marginRight: 0 }}>
                            <Typography variant="caption" display="block" noWrap style={{
                                width: "100%",
                                textAlign: "start"
                            }}> {fileProperty.name}</Typography>
                        </div>
                        {/* CONTROL SECTION */}

                        <div style={{ flex: "0 0 auto", width: "100%" }}>

                            <ButtonBase style={{ width: "100%", borderRadius: "4px 4px 0px 0px", overflow: "hidden",marginTop: 8}} onClick={()=> {this.onFileClick()}} >
                                <div style={{width: "100%", background: "rgba(0,0,0,0.07)", borderRadius: "4px 4px 0px 0px"}}>
                                    <div style={{ display: "flex", alignItems: "center", flexFlow: "row nowrap" }}>
                                        <Typography noWrap={true} style={{ flex: "1 1 100%", textAlign: "start", verticalAlign: "center", paddingTop: 4,paddingBottom: 4,paddingLeft: 4}}
                                            variant='caption'
                                            >{value}</Typography>
                                        <MoreHorizIcon style={{ flex: "0 0 auto", width: "16px", height: "16px", verticalAlign: "center",opacity: 0.5, marginRight: 4 }} />
                                    </div>
                                    <div style={{ height: "1px", width: "100%", background: "black" }}>&nbsp;</div>
                                </div>
                            </ButtonBase>
                        </div>

                        {/* LABEL/EDIT SECTION*/}
                        <div style={{ flex: "0 0 auto", position: "relative", width: 60 }}>
                        </div>
                    </div >
                );
            }
        }
    );

export default FilePropertyControl;