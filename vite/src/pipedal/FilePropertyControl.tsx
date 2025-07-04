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

import { Component, SyntheticEvent } from 'react';
import { Theme } from '@mui/material/styles';
import { css } from '@emotion/react';

import { UiFileProperty } from './Lv2Plugin';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, ListenHandle, State } from './PiPedalModel';
import ButtonBase from '@mui/material/ButtonBase'
import MoreHorizIcon from '@mui/icons-material/MoreHoriz';
import { PedalboardItem } from './Pedalboard';
import { isDarkMode } from './DarkMode';


export const StandardItemSize = { width: 80, height: 140 }


import { withStyles } from "tss-react/mui";
import WithStyles from './WithStyles';
import { withTheme } from './WithStyles';

const styles = (theme: Theme) => {
    return {
        frame: css({
            position: "relative",
            margin: "12px"
        }),
        switchTrack: css({
            height: '100%',
            width: '100%',
            borderRadius: 14 / 2,
            zIndex: -1,
            transition: theme.transitions.create(['opacity', 'background-color'], {
                duration: theme.transitions.duration.shortest
            }),
            backgroundColor: theme.palette.secondary.main,
            opacity: theme.palette.mode === 'light' ? 0.38 : 0.3
        }),
        controlFrame: css({
            display: "flex", flexDirection: "column", alignItems: "start", justifyContent: "space-between", height: 116
        }),
        displayValue: css({
            position: "absolute",
            top: 0,
            left: 0,
            right: 0,
            bottom: 4,
            textAlign: "center",
            background: theme.mainBackground,
            color: theme.palette.text.secondary,
            // zIndex: -1,
        }),
        titleSection: css({
            flex: "0 0 auto", alignSelf: "stretch", marginBottom: 8, marginLeft: 8, marginRight: 0
        }),
        midSection: css({
            flex: "1 1 1", display: "flex", flexFlow: "column nowrap", alignContent: "center", justifyContent: "center"
        }),
        editSection: css({
            flex: "0 0 0", position: "relative", width: 60, height: 28, minHeight: 28
        })
    }
}
    ;


export interface FilePropertyControlProps extends WithStyles<typeof styles> {
    fileProperty: UiFileProperty;
    pedalboardItem: PedalboardItem;
    onFileClick: (fileProperty: UiFileProperty, value: string) => void;
    theme: Theme;
}
type FilePropertyControlState = {
    error: boolean;
    hasValue: boolean;
    value: string;
};

const FilePropertyControl =
    withTheme(withStyles(
    class extends Component<FilePropertyControlProps, FilePropertyControlState> {

        model: PiPedalModel;


        constructor(props: FilePropertyControlProps) {
            super(props);

            this.state = {
                error: false,
                hasValue: false,
                value: "",
            };
            this.model = PiPedalModelFactory.getInstance();
            this.onStateChanged = this.onStateChanged.bind(this);
        }


        onStateChanged(state: State) {
            if (this.mounted) {
                if (state === State.Ready) {
                    this.subscribeToPatchProperty();
                }
            }
        }
        // private propertyGetHandle?: ListenHandle;

        monitorPropertyHandle?: ListenHandle;

        subscribeToPatchProperty() {
            // this.propertyGetHandle = this.model.listenForAtomOutput(this.props.instanceId,(instanceId,atomOutput)=>{
            //     // PARSE THE OBJECT!
            //     // this.setState({value: atomOutput?.value as string});
            // });
            this.unsubscribeToPatchProperty();
            this.monitorPropertyHandle = this.model.monitorPatchProperty(this.props.pedalboardItem.instanceId,
                this.props.fileProperty.patchProperty,
                (instanceId, property, propertyValue) => {
                    if (propertyValue.otype_ === "Path") {
                        this.setState({ value: propertyValue.value as string, hasValue: true });
                    }
                }
            );

        }
        unsubscribeToPatchProperty() {
            if (this.monitorPropertyHandle !== undefined) {
                this.model.cancelMonitorPatchProperty(this.monitorPropertyHandle);
                this.monitorPropertyHandle = undefined;
            }

        }
        private mounted: boolean = false;
        componentDidMount() {
            this.model.state.addOnChangedHandler(this.onStateChanged);
            this.mounted = true;
            this.subscribeToPatchProperty();
        }
        componentWillUnmount() {
            this.unsubscribeToPatchProperty();
            this.mounted = false;
            this.model.state.removeOnChangedHandler(this.onStateChanged);
        }
        componentDidUpdate(prevProps: Readonly<FilePropertyControlProps>, prevState: Readonly<FilePropertyControlState>, snapshot?: any): void {
            if (prevProps.fileProperty.patchProperty !== this.props.fileProperty.patchProperty
                || prevProps.pedalboardItem.instanceId !== this.props.pedalboardItem.instanceId
                || prevProps.pedalboardItem.stateUpdateCount !== this.props.pedalboardItem.stateUpdateCount

            ) {
                this.unsubscribeToPatchProperty();
                this.subscribeToPatchProperty();
            }

        }


        inputChanged: boolean = false;

        onDrag(e: SyntheticEvent) {
            e.preventDefault();
        }

        onFileClick() {
            this.props.onFileClick(this.props.fileProperty, this.state.value);
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
        render() {
            //const classes = withStyles.getClasses(this.props);

            const classes = withStyles.getClasses(this.props);

            let fileProperty = this.props.fileProperty;

            let value = "\u00A0";
            if (this.state.hasValue) {
                if (this.state.value.length === 0) {
                    value = "<none>";
                } else {
                    value = this.fileNameOnly(this.state.value);
                }
            }

            let item_width = 264;

            return (
                <div className={classes.controlFrame}
                    style={{ width: item_width }}>
                    {/* TITLE SECTION */}
                    <div className={classes.titleSection}
                    >
                        <Typography variant="caption" display="block" noWrap style={{
                            width: "100%",
                            textAlign: "start"
                        }}> {fileProperty.label}</Typography>
                    </div>
                    {/* CONTROL SECTION */}

                    <div className={classes.midSection} style={{ width: "100%", paddingLeft: 8 }}>

                        <ButtonBase style={{ width: "100%", borderRadius: "4px 4px 0px 0px", overflow: "hidden" }} onClick={() => { this.onFileClick() }} >
                            <div style={{
                                width: "100%", background:
                                    isDarkMode() ? "rgba(255,255,255,0.03)" : "rgba(0,0,0,0.07)",
                                borderRadius: "4px 4px 0px 0px"
                            }}>
                                <div style={{ display: "flex", alignItems: "center", flexFlow: "row nowrap" }}>
                                    <Typography noWrap={true} style={{ flex: "1 1 100%", textAlign: "start", verticalAlign: "center", paddingTop: 4, paddingBottom: 4, paddingLeft: 4 }}
                                        variant='caption'
                                    >{value}</Typography>
                                    <MoreHorizIcon style={{ flex: "0 0 auto", width: "16px", height: "16px", verticalAlign: "center", opacity: 0.5, marginRight: 4 }} />
                                </div>
                                <div style={{ height: "1px", width: "100%", background: this.props.theme.palette.text.secondary }}>&nbsp;</div>
                            </div>
                        </ButtonBase>
                    </div>

                    {/* LABEL/EDIT SECTION*/}
                    <div className={classes.editSection} >
                    </div>
                </div >
            );
        }
    }
    ,styles)
)
    ;
export default FilePropertyControl;