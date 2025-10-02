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
import Backdrop from '@mui/material/Backdrop';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';

import Rect from './Rect';
import { css } from '@emotion/react';


import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';

const styles = (theme: Theme) => createStyles({
    pgraph: css({
        paddingBottom: 16
    }),
    backdrop: css({
        zIndex: 1101,
        overflow: "clip",
        background: theme.palette.background.default,

    }),
    noToolbar: css({
        display: "none"
    }),
    topToolBar: css({
        position: "absolute",
        zIndex: 1101,
        top: 0,
        display: "flex", flexFlow: "row nowrap", justifyContent: "start", columnGap: 8,
    }),
    rightToolbar: css({
        position: "absolute",
        zIndex: 1101,
        top: 24,
        display: "flex", flexFlow: "column nowrap", rowGap: 8
    })
});


export interface AutoZoomProps extends WithStyles<typeof styles> {
    leftPad: number,
    rightPad: number,
    children?: React.ReactNode,
    toolbarChildren?: React.ReactNode[],
    contentReady: boolean,
    showZoomed: boolean,
    setShowZoomed: (value: boolean) => void;
}

export interface AutoZoomState {
    screenWidth: number,
    screenHeight: number,
}


// non-react zoom state.
class UnzoomedLayout {
    horizontalToolbar = false;
    effectiveClientRect: Rect = new Rect();
    showToolbar = false;
    zoom: number = 1;
    showZoomButton = false;
    contentRect: Rect = new Rect();
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
            }
        }

        resizeObserver: ResizeObserver | null = null;


        calculateUnzoomedLayout(
            clientRect: Rect,
            contentWidth: number,
            contentHeight: number,
            buttonPadding: number
        ): UnzoomedLayout {

            let result = new UnzoomedLayout();

            let rightButtonZoom = Math.min((clientRect.width - buttonPadding) / contentWidth, clientRect.height / contentHeight);
            let topButtonZoom = Math.min((clientRect.width - buttonPadding) / contentWidth, (clientRect.height - buttonPadding) / contentHeight);

            let effectiveClientRect = clientRect.copy();
            if (rightButtonZoom > topButtonZoom) {
                result.horizontalToolbar = false;
                effectiveClientRect.width -= buttonPadding;
            } else {
                result.horizontalToolbar = true;
                effectiveClientRect.y += buttonPadding;
                effectiveClientRect.height -= buttonPadding;
            }
            result.effectiveClientRect = effectiveClientRect;

            let zoom = Math.min(effectiveClientRect.width / contentWidth, effectiveClientRect.height / contentHeight);
            if (zoom > 1) {
                zoom = 1;
            }
            result.zoom = zoom;
            result.showZoomButton = zoom !== 1;
            result.showToolbar = result.showZoomButton || !!this.props.toolbarChildren;

            let leftPad = (effectiveClientRect.width - contentWidth * zoom) / 2;
            if (leftPad < 0) {
                leftPad = 0;
            }
            let topPad = (effectiveClientRect.height - contentHeight * zoom) / 4;
            if (topPad < 0) {
                topPad = 0;
            }
            result.contentRect = new Rect(effectiveClientRect.x + leftPad, effectiveClientRect.y + topPad, contentWidth * zoom, contentHeight * zoom);
            return result;
        }

        updateZoom(ref: HTMLDivElement) {
            if (!this.mounted || !ref) {
                return;
            }
            let content = ref.firstElementChild as HTMLElement | null;
            const classes = withStyles.getClasses(this.props);

            if (content) {
                const { leftPad, rightPad } = this.props;
                let bounds = new Rect(leftPad, 0, ref.clientWidth - leftPad - rightPad, ref.clientHeight - 8);
                let layout = this.calculateUnzoomedLayout(
                    bounds, content.clientWidth, content.clientHeight, 48);

                let contentRect = layout.contentRect;

                content.style.left = "0px";
                content.style.top = "0px";
                content.style.transformOrigin = "0 0"; // top-left corner
                content.style.transform =
                    `translate(${contentRect.x}px, ${contentRect.y}px) scale(${layout.zoom}, ${layout.zoom})`;

                if (this.zoomToolbar) {
                    if (!layout.showToolbar) {

                        this.zoomToolbar.className = classes.noToolbar;
                    } else if (layout.horizontalToolbar) {
                        this.zoomToolbar.className = classes.topToolBar;
                        this.zoomToolbar.style.left = `${contentRect.x +16}px`
                        this.zoomToolbar.style.top = `${contentRect.y - 48}px`;
                        this.zoomToolbar.style.width = `${contentRect.width}px`;
                    } else {
                        this.zoomToolbar.className = classes.rightToolbar;
                        this.zoomToolbar.style.left = "";
                        this.zoomToolbar.style.top = `${contentRect.y}px`;
                        this.zoomToolbar.style.left = `${contentRect.right + 4}px`;
                        this.zoomToolbar.style.width = "";
                    }
                }
                if (this.zoomButton) {
                    this.zoomButton.style.display = layout.showZoomButton ? "block" : "none";
                }

                // save the position of the Mod UI in page coordinates for later zooming.
                this.zoomStartRect = layout.contentRect.copy();
                let refBounds = ref.getBoundingClientRect();
                this.zoomStartRect.x += refBounds.left;
                this.zoomStartRect.y += refBounds.top;
            } else {
                if (this.zoomToolbar) {
                    this.zoomToolbar.className = classes.noToolbar;
                }
            }
        }

        updateMaximizedZoom() {
            if (this.normalRef) {
                this.updateZoom(this.normalRef);
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
                this.updateMaximizedZoom();
            });
        }


        private zoomToolbar: HTMLElement | null = null;
        private zoomButton: HTMLElement | null = null;
        private normalRef: HTMLDivElement | null = null;

        setContentRef(ref: HTMLDivElement | null) {
            this.normalRef = ref;
            if (ref) {
                this.zoomToolbar = ref.querySelector("#zoom-toolbar") as HTMLDivElement | null;
                this.zoomButton = ref.querySelector("#zoom-button") as HTMLDivElement | null;
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
                    this.zoomToolbar = null;
                }
                this.zoomStartRect = null;
                this.zoomToolbar = null;
                this.zoomButton = null;
            }
        }


        private zoomStartRect: Rect | null = null;

        startZoom() {
            if (!this.zoomStartRect) {
                return;
            }
            this.props.setShowZoomed(true);
        }

        content() {
            const classes = withStyles.getClasses(this.props);
            void classes; // suppress unused variable warning
            return (
                <div ref={(ref) => { this.setContentRef(ref); }}
                    style={{ width: "100%", height: "100%" }}
                >
                    <div style={{
                        display: "inline-block",
                        position: "absolute",
                        visibility: this.props.contentReady ? "visible" : "hidden"

                    }}>
                        {this.props.children}
                    </div>
                    <div id="zoom-toolbar" className={classes.noToolbar}
                        style={{
                            visibility: this.props.contentReady ? "visible" : "hidden"
                        }}
                    >
                        {!this.props.showZoomed ? (
                            <IconButtonEx id="zoom-button" tooltip="Fullscreen view"
                                onClick={() => {
                                    this.startZoom();
                                }}
                            >
                                <FullscreenIcon style={{ opacity: 0.6 }}
                                />
                            </IconButtonEx>

                        ) : (
                            <IconButtonEx tooltip="Exit fullscreen"
                                style={{}}
                                onClick={(e) => {
                                    e.stopPropagation();
                                    this.props.setShowZoomed(false);
                                }}
                            >
                                <ArrowBackIcon style={{
                                    opacity: 0.6,
                                }} />
                            </IconButtonEx>
                        )}


                        {this.props.toolbarChildren && this.props.toolbarChildren.map((child, index) => (
                            <div key={index} style={{ display: "inline-block" }}>
                                {child}
                            </div>
                        ))}
                    </div>

                </div>
            );

        }

        render() {
            const classes = withStyles.getClasses(this.props);
            void classes; // suppress unused variable warning

            return (<div style={{ overflow: "hidden", height: "100%", width: "100%" }} >
                {!this.props.showZoomed && this.content()}

                {this.props.showZoomed && (
                    <Backdrop id="modGuiZoomBackdrop"
                        className={classes.backdrop}
                        open={this.props.showZoomed}
                    >
                        {this.content()}

                    </Backdrop>
                )}
            </div>);
        }
    },
    styles);

export default AutoZoom;