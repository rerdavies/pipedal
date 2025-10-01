// Copyright (c) 2024 Robin Davies
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

import WithStyles from './WithStyles';
import { css } from '@emotion/react';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import CloseIcon from '@mui/icons-material/Close';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import CssBaseline from '@mui/material/CssBaseline';
import { createStyles } from './WithStyles';

import { withStyles } from "tss-react/mui";
import IconButtonEx from './IconButtonEx';
import FullscreenIcon from '@mui/icons-material/Fullscreen';
import FullscreenExitIcon from '@mui/icons-material/FullscreenExit';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { PiPedalModelFactory, PiPedalModel, State, ZoomedControlInfo } from './PiPedalModel';
import ZoomedUiControl from './ZoomedUiControl'
import MainPage from './MainPage';
import JackStatusView from './JackStatusView';
import { Theme } from '@mui/material/styles';
import { isDarkMode } from './DarkMode';
import Typography from '@mui/material/Typography';
import TextField from '@mui/material/TextField';
import { Snapshot } from './Pedalboard';
import ColorDropdownButton, { DropdownAlignment } from './ColorDropdownButton';
import { getBackgroundColor } from './MaterialColors';


const selectColor = isDarkMode() ? "#888" : "#FFFFFF";


const appStyles = (theme: Theme) => createStyles({
    "&": css({ // :root
        colorScheme: (isDarkMode() ? "dark" : "light")
    }),


    mainFrame: css({
        overflow: "hidden",
        display: "flex",
        flexFlow: "column",
        marginTop: 8,
        flex: "1 1 100%"
    }),
    heroContent: css({
        backgroundColor: theme.mainBackground,
        position: "relative",
        height: "100%",
        width: "100%"
    }),
    shadowCatcher: css({
        backgroundColor: theme.mainBackground
    }),

    select: css({ // fu fu fu.Overrides for white selector on dark background.
        '&:before': {
            borderColor: selectColor,
        },
        '&:after': {
            borderColor: selectColor,
        },
        '&:hover:not(.Mui-disabled):before': {
            borderColor: selectColor,
        }
    }),
    select_icon: css({
        fill: selectColor,
    })


});


function supportsFullScreen(): boolean {
    let doc: any = window.document;
    let docEl: any = doc.documentElement;
    var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullScreen || docEl.msRequestFullscreen;
    return (!!requestFullScreen);

}

function setFullScreen(value: boolean) {
    let doc: any = window.document;
    let docEl: any = doc.documentElement;

    var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullScreen || docEl.msRequestFullscreen;
    var cancelFullScreen = doc.exitFullscreen || doc.mozCancelFullScreen || doc.webkitExitFullscreen || doc.msExitFullscreen;

    if (!doc.fullscreenElement && !doc.mozFullScreenElement && !doc.webkitFullscreenElement && !doc.msFullscreenElement) {
        requestFullScreen.call(docEl);
    }
    else {
        cancelFullScreen.call(doc);
    }
}

type SnapshotEditorState = {
    zoomedControlInfo: ZoomedControlInfo | undefined;
    zoomedControlOpen: boolean;

    canFullScreen: boolean;
    isFullScreen: boolean;
    isDebug: boolean;
    collapseLabel: boolean;

    showStatusMonitor: boolean;
    name: string,
    color: string

};
interface SnapshotEditorProps extends WithStyles<typeof appStyles> {
    onClose: () => void;
    onOk: (snapshotIndex: number, name: string, color: string, newSnapshots: (Snapshot | null)[]) => void;
    snapshotIndex: number;
}

const SnapshotEditor = withStyles(
    class extends ResizeResponsiveComponent<SnapshotEditorProps, SnapshotEditorState> {
        // Before the component mounts, we initialise our state

        model_: PiPedalModel;

        getCollapseLabel() {
            return this.windowSize.width < 500;
        }

        constructor(props: SnapshotEditorProps) {
            super(props);
            this.model_ = PiPedalModelFactory.getInstance();

            this.model_.zoomedUiControl.addOnChangedHandler(
                () => {
                    if (this.mounted93) {
                        this.setState({
                            zoomedControlOpen: this.model_.zoomedUiControl.get() !== undefined,
                            zoomedControlInfo: this.model_.zoomedUiControl.get()
                        });
                    }
                }
            );

            let snapshot = this.model_.pedalboard.get().snapshots[this.props.snapshotIndex] ?? new Snapshot();
            this.state = {
                zoomedControlOpen: false,
                zoomedControlInfo: this.model_.zoomedUiControl.get(),
                canFullScreen: supportsFullScreen() && !this.model_.isAndroidHosted(),
                isFullScreen: !!document.fullscreenElement,
                showStatusMonitor: this.model_.showStatusMonitor.get(),
                isDebug: true,
                name: snapshot.name,
                color: snapshot.color,
                collapseLabel: this.getCollapseLabel()

            };

            this.showStatusMonitorHandler = this.showStatusMonitorHandler.bind(this);
        }

        showStatusMonitorHandler() {
            this.setState({
                showStatusMonitor: this.model_.showStatusMonitor.get()
            });
        }
        toggleFullScreen(): void {
            setFullScreen(this.state.isFullScreen);
            this.setState({ isFullScreen: !this.state.isFullScreen });
        }

        private mounted93: boolean = false;
        componentDidMount() {

            super.componentDidMount();
            this.mounted93 = true;
            this.model_.showStatusMonitor.addOnChangedHandler(this.showStatusMonitorHandler);

        }


        updateOverscroll(): void {
            if (this.model_.serverVersion) {
                // no pull-down refresh on android devices once we're ready (unless we're debug)
                let preventOverscroll =
                    this.model_.state.get() === State.Ready
                    && !this.model_.debug;

                let overscrollBehavior = preventOverscroll ? "none" : "auto";
                document.body.style.overscrollBehavior = overscrollBehavior;
            }
        }

        componentDidUpdate() {
        }

        componentWillUnmount() {
            this.mounted93 = false;
            super.componentWillUnmount();
            this.model_.banks.removeOnChangedHandler(this.showStatusMonitorHandler);
        }

        onWindowSizeChanged(width: number, height: number): void {
            super.onWindowSizeChanged(width, height);
            this.setState({
                collapseLabel: this.getCollapseLabel()
            });

        }

        handleOk() {
            let currentPedalboard = this.model_.pedalboard.get();
            let selectedSnapshot = this.props.snapshotIndex;
            let currentSnapshot: Snapshot | null = currentPedalboard.snapshots[selectedSnapshot];
            if (!currentSnapshot) {
                this.props.onClose();
                return;
            }

            let changed = this.state.name !== currentSnapshot.name
                || this.state.color !== currentSnapshot.color
                || currentSnapshot.isModified;
            if (!changed) {
                this.props.onClose();
                return;
            }
            let newSnapshots = Snapshot.cloneSnapshots(currentPedalboard.snapshots);
            let newSnapshot = this.model_.pedalboard.get().makeSnapshot();
            newSnapshot.name = this.state.name;
            newSnapshot.color = this.state.color;
            newSnapshots[selectedSnapshot] = newSnapshot;
            this.model_.setSnapshots(newSnapshots, selectedSnapshot);

            this.props.onOk(selectedSnapshot, this.state.name, this.state.color, newSnapshots);


        }

        render() {

            const classes = withStyles.getClasses(this.props);

            return (

                <div
                    className={classes.shadowCatcher}
                    style={{
                        position: "absolute", top: 0, left: 0, right: 0, bottom: 0,
                        colorScheme: isDarkMode() ? "dark" : "light", // affects scrollbar color
                        minHeight: 345, minWidth: 390,
                        userSelect: "none",
                        display: "flex", flexDirection: "column", flexWrap: "nowrap",
                        overscrollBehavior: this.state.isDebug ? "auto" : "none"
                    }}
                    onContextMenu={(e) => {
                        if (!this.model_.debug) {
                            e.preventDefault(); e.stopPropagation();
                        }
                    }}
                >
                    <CssBaseline />
                    <AppBar position="static" sx={{ bgcolor: isDarkMode() ? getBackgroundColor("purple") : "#200040" }} >
                        <Toolbar variant="dense"  >
                            <IconButtonEx
                                tooltip="Back"
                                edge="start"
                                aria-label="menu"
                                color="inherit"
                                onClick={() => { this.handleOk(); }}
                                size="large">
                                <ArrowBackIcon />
                            </IconButtonEx>
                            {!this.state.collapseLabel && (
                                <Typography style={{ flex: "0 1 auto", opacity: 0.66 }}
                                    variant="body1" noWrap
                                >
                                    {'Snapshot ' + (this.props.snapshotIndex + 1)}
                                </Typography>
                            )}

                            <div style={{ flex: "1 1 auto", display: "flex", flexFlow: "column nowrap", marginRight: 8 }}>
                                <TextField
                                    sx={{
                                        input: { color: 'white' },
                                        '& .MuiInput-underline:before': { borderBottomColor: '#FFFFFFC0' },
                                        '& .MuiInput-underline:hover:before': { borderBottomColor: '#FFFFFFE0' },
                                        '& .MuiInput-underline:after': { borderBottomColor: '#FFFFFF' }
                                    }}
                                    variant="standard"
                                    style={{ flex: "1 1 auto", marginLeft: 16, maxWidth: 500, color: "white" }}
                                    value={this.state.name}
                                    onChange={(ev) => {
                                        this.setState({ name: ev.target.value });
                                    }}
                                />
                            </div>
                            <ColorDropdownButton
                                currentColor={this.state.color}
                                dropdownAlignment={DropdownAlignment.SW}
                                onColorChange={(newColor) => {
                                    this.setState({ color: newColor });
                                }} />
                            {this.state.canFullScreen &&
                                <IconButtonEx
                                    tooltip={this.state.isFullScreen ? "Exit Full Screen" : "Full Screen"}
                                    aria-label="full-screen"
                                    onClick={() => { this.toggleFullScreen(); }}
                                    color="inherit"
                                    size="medium">
                                    {this.state.isFullScreen ? (
                                        <FullscreenExitIcon style={{ opacity: 0.75 }} />
                                    ) : (
                                        <FullscreenIcon style={{ opacity: 0.75 }} />

                                    )}

                                </IconButtonEx>
                            }
                            <IconButtonEx
                                tooltip="Cancel"
                                aria-label="cancel"
                                onClick={() => { this.props.onClose(); }}
                                color="inherit"
                                size="medium">
                                <CloseIcon />
                            </IconButtonEx>

                        </Toolbar>
                    </AppBar>
                    <main className={classes.mainFrame} >
                        <div style={{
                            overflow: "hidden",
                            flex: "1 1 auto"
                        }} >
                            <div className={classes.heroContent}>
                                <MainPage hasTinyToolBar={false} enableStructureEditing={false} />
                            </div>

                        </div>
                    </main>

                    <ZoomedUiControl
                        dialogOpen={this.state.zoomedControlOpen}
                        controlInfo={this.state.zoomedControlInfo}
                        onDialogClose={() => { this.setState({ zoomedControlOpen: false }); }}
                        onDialogClosed={() => { this.model_.zoomedUiControl.set(undefined); }
                        }
                    />
                    {this.state.showStatusMonitor && (<JackStatusView />)}
                </div >
            );
        }
    },
    appStyles
);

export default SnapshotEditor;
