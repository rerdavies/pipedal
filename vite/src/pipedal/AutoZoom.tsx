// Copyright (c) 2025 Robin E. R. Davies
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

import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";
import { createStyles } from './WithStyles';
import FullscreenIcon from '@mui/icons-material/Fullscreen';
import IconButtonEx from './IconButtonEx';
import { isDarkMode } from './DarkMode';
import Backdrop from '@mui/material/Backdrop';
import CloseIcon from '@mui/icons-material/Close';
import Rect from './Rect';

import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';

const styles = (theme: Theme) => createStyles({
    pgraph: {
        paddingBottom: 16
    }
});


export interface AutoZoomProps extends WithStyles<typeof styles> {
    leftPad: number,
    rightPad: number,
    children?: React.ReactNode,
    contentReady: boolean,
    showZoomed: boolean,
    setShowZoomed: (value: boolean) => void;
}

export interface AutoZoomState {
    screenWidth: number,
    screenHeight: number,
}



const AutoZoom = withStyles(
    class extends ResizeResponsiveComponent<AutoZoomProps, AutoZoomState> {
        private model: PiPedalModel;
        constructor(props: AutoZoomProps) {
            super(props);

            this.model = PiPedalModelFactory.getInstance();
            void this.model;  // suppress unused variable warning

            this.state = {
                screenWidth: this.windowSize.width,
                screenHeight: this.windowSize.height,
            };
        }
        mounted: boolean = false;



        onWindowSizeChanged(width: number, height: number): void {
            this.setState({ screenWidth: width, screenHeight: height });
        }


        componentDidMount() {
            super.componentDidMount();
            this.mounted = true;
            if (this.normalRef) {
                this.updateZoom(this.normalRef);
            }
        }
        componentWillUnmount() {
            this.mounted = false;
            this.cancelZoomUpdate();
            this.cancelMaximizedZoomUpdate();
            super.componentWillUnmount();
        }


        componentDidUpdate(prevProps: AutoZoomProps) {
            if (prevProps.contentReady !== this.props.contentReady) {
                if (this.normalRef) {
                    this.updateZoom(this.normalRef);
                }
                if (this.maximizedRef) {
                    this.updateMaximizedZoom();
                }
            }
        }

        resizeObserver: ResizeObserver | null = null;


        calculateZoomedClientRect(
            clientRect: Rect,
            contentWidth: number,
            contentHeight: number,
            buttonPadding: number
        ): Rect {

            let rightButtonZoom = Math.min((clientRect.width - buttonPadding) / contentWidth, clientRect.height / contentHeight);
            let topButtonZoom = Math.min((clientRect.width - buttonPadding) / contentWidth, (clientRect.height - buttonPadding) / contentHeight);

            let effectiveClientRect = clientRect.copy();
            if (rightButtonZoom > topButtonZoom) {
                effectiveClientRect.width -= buttonPadding;
            } else {
                effectiveClientRect.y += buttonPadding;
                effectiveClientRect.height -= buttonPadding;
            }

            let zoom = Math.min(effectiveClientRect.width / contentWidth, effectiveClientRect.height / contentHeight);
            if (zoom > 1) {
                zoom = 1;
            }
            let leftPad = (effectiveClientRect.width - contentWidth * zoom) / 2;
            if (leftPad < 0) {
                leftPad = 0;
            }
            let topPad = (effectiveClientRect.height - contentHeight * zoom) / 4;
            if (topPad < 0) {
                topPad = 0;
            }
            return new Rect(effectiveClientRect.x + leftPad, effectiveClientRect.y + topPad, contentWidth * zoom, contentHeight * zoom);
        }

        updateZoom(ref: HTMLDivElement) {
            if (!this.mounted || !ref) {
                return;
            }
            let content = ref.firstElementChild as HTMLElement | null;

            if (content) {
                const { leftPad, rightPad } = this.props;
                let bounds = new Rect(leftPad, 0, ref.clientWidth - leftPad - rightPad, ref.clientHeight - 8);
                let zoomedClientRect = this.calculateZoomedClientRect(
                    bounds, content.clientWidth, content.clientHeight, 48);
                content.style.left = "0px";
                content.style.top = "0px";
                content.style.transformOrigin = "0 0"; // top-left corner
                content.style.transform =
                    `translate(${zoomedClientRect.x}px, ${zoomedClientRect.y}px) scale(${zoomedClientRect.width / content.clientWidth}, ${zoomedClientRect.height / content.clientHeight})`;

                if (this.zoomButton) {
                    this.zoomButton.style.display = zoomedClientRect.width < content.clientWidth ? "block" : "none";
                    let rightSizeSpace = ref.clientWidth - zoomedClientRect.right;
                    if (rightSizeSpace - rightPad >= 48) {
                        this.zoomButton.style.top = `${zoomedClientRect.y + 24}px`;
                        this.zoomButton.style.left = `${zoomedClientRect.right + 4}px`;
                    } else {
                        this.zoomButton.style.top = `${zoomedClientRect.y - 48}px`;
                        this.zoomButton.style.left = `${zoomedClientRect.right - 64 - 12}px`;
                    }
                }

                // save the position of the Mod UI in page coordinates for later zooming.
                this.zoomStartRect = zoomedClientRect.copy();
                let refBounds = ref.getBoundingClientRect();
                this.zoomStartRect.x += refBounds.left;
                this.zoomStartRect.y += refBounds.top;
            }
        }

        updateMaximizedZoom() {
            let ref = this.maximizedRef;
            if (!this.mounted || !ref) {
                return;
            }
            let content = ref.firstElementChild as HTMLElement | null;

            if (content) {
                let leftPad = 16; let rightPad = 16;
                let topPad = 16, bottomPad = 16;
                let bounds: Rect;
                bounds = new Rect(leftPad, topPad, ref.clientWidth - leftPad - rightPad, ref.clientHeight - topPad - bottomPad);

                let zoomedClientRect = this.calculateZoomedClientRect(
                    bounds, content.clientWidth, content.clientHeight, 48);
                content.style.left = "0px";
                content.style.top = "0px";
                content.style.transformOrigin = "0 0"; // top-left corner
                content.style.transform = `translate(${zoomedClientRect.x}px, ${zoomedClientRect.y}px) scale(${zoomedClientRect.width / content.clientWidth}, ${zoomedClientRect.height / content.clientHeight})`;
                //console.log("Zoomed to: ", zoomedClientRect.toString());

            }
        }


        private animateFrameHandle: number | null = null;
        cancelZoomUpdate() {
            if (this.animateFrameHandle) {
                window.cancelAnimationFrame(this.animateFrameHandle);
                this.animateFrameHandle = null;
            }
        }
        requestZoomUpdate() {
            if (this.animateFrameHandle) {
                return; // already scheduled
            }
            this.animateFrameHandle = window.requestAnimationFrame(() => {
                this.animateFrameHandle = null;
                if (this.normalRef) {
                    this.updateZoom(this.normalRef);
                }
            });
        }
        private maximizedAnimateFrameHandle: number | null = null;

        cancelMaximizedZoomUpdate() {
            if (this.maximizedAnimateFrameHandle) {
                window.cancelAnimationFrame(this.maximizedAnimateFrameHandle);
                this.maximizedAnimateFrameHandle = null;
            }
        }
        requestMaximizedZoomUpdate() {
            if (this.maximizedAnimateFrameHandle) {
                return; // already scheduled
            }
            this.maximizedAnimateFrameHandle = window.requestAnimationFrame(() => {
                this.maximizedAnimateFrameHandle = null;
                if (this.maximizedRef) {
                    this.updateMaximizedZoom();
                }
            });
        }


        private zoomButton: HTMLElement | null = null;
        private normalRef: HTMLDivElement | null = null;

        setNormalRef(ref: HTMLDivElement | null) {
            this.normalRef = ref;
            if (ref) {
                this.zoomButton = ref.querySelector("#maximize-button") as HTMLDivElement | null;
                this.resizeObserver = new ResizeObserver(() => {
                    this.requestZoomUpdate();
                });
                this.resizeObserver.observe(ref);
                let child = ref.firstElementChild as HTMLElement | null;
                if (child) {
                    this.resizeObserver.observe(child);
                }
                this.requestZoomUpdate();
            } else {
                if (this.resizeObserver) {
                    this.resizeObserver.disconnect();
                    this.resizeObserver = null;
                    this.zoomButton = null;
                }
                this.zoomStartRect = null;
            }
        }


        private maximizedRef: HTMLDivElement | null = null;
        private maximedResizeObserver: ResizeObserver | null = null;

        setMaximizedRef(ref: HTMLDivElement | null) {
            this.maximizedRef = ref;
            if (ref) {
                this.maximedResizeObserver = new ResizeObserver(() => {
                    this.requestMaximizedZoomUpdate();
                });
                this.maximedResizeObserver.observe(ref);
                let child = ref.firstElementChild as HTMLElement | null;
                if (child) {
                    this.maximedResizeObserver.observe(child);
                }
                this.requestMaximizedZoomUpdate();
            } else {
                if (this.maximedResizeObserver) {
                    this.maximedResizeObserver.disconnect();
                    this.maximedResizeObserver = null;
                }
            }
        }

        private zoomStartRect: Rect | null = null;

        startZoom() {
            if (!this.zoomStartRect) {
                return;
            }
            this.props.setShowZoomed(true);
        }

        render() {
            const classes = withStyles.getClasses(this.props);
            void classes; // suppress unused variable warning
            return (<div style={{ overflow: "hidden", height: "100%", width: "100%" }} ref={(ref) => { this.setNormalRef(ref); }}>
                <div style={{
                    display: "inline-block",
                    visibility: this.props.contentReady ? "visible" : "hidden",

                    position: "relative",
                }}>
                    {!this.props.showZoomed && this.props.children}
                </div>
                <div id="maximize-button" style={{
                    display: "none", position: "absolute", left: 0, top: 0, width: 52, height: 64, zIndex: 1101,
                    borderRadius: "0px 26px 26px 0px",

                }}
                >
                    <IconButtonEx tooltip="Fullscreen view"
                        style={{ margin: 2, background: isDarkMode() ? "#444" : "#eee" }}
                        onClick={() => {
                            this.startZoom();
                        }}
                    >
                        <FullscreenIcon style={{ color: "white" }} />
                    </IconButtonEx>
                </div>
                {this.props.showZoomed && (
                    <Backdrop id="modGuiZoomBackdrop"
                        sx={(theme) => ({
                            zIndex: this.props.showZoomed ? 1101 : -1,
                            overflow: "clip",
                            background: theme.palette.mode === 'dark' ? 'rgba(0, 0, 0, 0.9)' : 'rgba(255, 255, 255, 0.9)'
                        })}
                        open={this.props.showZoomed}
                    >
                        <div ref={(ref) => { this.setMaximizedRef(ref); }}
                            style={{
                                position: "absolute", left: 0, top: 0, right: 0, bottom: 0, overflow: "hidden",
                                visibility: this.props.contentReady ? "visible" : "hidden"
                            }}>

                            <div style={{ display: "inline-block", position: "relative" }}>
                                {
                                    this.props.showZoomed && this.props.children
                                }
                            </div>

                            <IconButtonEx tooltip="Exit fullscreen"
                                style={{
                                    position: "absolute", right: 16, top: 16, zIndex: 1104,

                                }}
                                onClick={(e) => {
                                    e.stopPropagation();
                                    this.props.setShowZoomed(false);
                                }}
                            >
                                <CloseIcon />
                            </IconButtonEx>
                        </div>
                    </Backdrop>
                )}
            </div>);
        }
    },
    styles);

export default AutoZoom;