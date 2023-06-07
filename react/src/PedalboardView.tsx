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

import React, { ReactNode, Component, SyntheticEvent } from 'react';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { PluginType } from './Lv2Plugin';
import ButtonBase from '@mui/material/ButtonBase';
import Typography from '@mui/material/Typography';
import { SelectIcon } from './PluginIcon';
import { SelectHoverBackground } from './SelectHoverBackground';
import SvgPathBuilder from './SvgPathBuilder';
import Draggable from './Draggable'
import Rect from './Rect';
import {PiPedalStateError} from './PiPedalError';
import Utility from './Utility'
import {
    Pedalboard, PedalboardItem, PedalboardSplitItem, SplitType
} from './Pedalboard';

const START_CONTROL = Pedalboard.START_CONTROL;
const END_CONTROL = Pedalboard.END_CONTROL;


const START_PEDALBOARD_ITEM_URI = Pedalboard.START_PEDALBOARD_ITEM_URI;
const END_PEDALBOARD_ITEM_URI = Pedalboard.END_PEDALBOARD_ITEM_URI;

const ENABLED_CONNECTOR_COLOR = "#666";
const DISABLED_CONNECTOR_COLOR = "#CCC";


const CELL_WIDTH: number = 96;
const CELL_HEIGHT: number = 64;
const FRAME_SIZE: number = 36;

const STROKE_WIDTH = 3;
const STEREO_STROKE_WIDTH = 6;

const SVG_STROKE_WIDTH = "3";
const SVG_STEREO_STROKE_WIDTH = "6";



const EMPTY_ICON_URL = "img/fx_empty.svg";
const ERROR_ICON_URL = "img/fx_error.svg";
const TERMINAL_ICON_URL = "img/fx_terminal.svg";

function CalculateConnection(numberOfInputs: number, numberOfOutputs: number)
{

    let result = Math.min(numberOfInputs, numberOfOutputs);
    if (result > 2) result = 2;
    return result;
}

const pedalboardStyles = (theme: Theme) => createStyles({
    scrollContainer: {
    },

    container: {
        position: "relative",
        overflow: "visible",

    },
    splitItem: {
        position: "absolute",
        display: "flex",
        alignItems: "center",
        justifyContent: "center",


        width: CELL_WIDTH,
        height: CELL_HEIGHT

    },
    splitStart: {
        position: "absolute",
        display: "flex",
        width: CELL_WIDTH,
        height: CELL_HEIGHT,
        left: 0,
        top: 0
    },
    splitEnd: {
        position: "absolute",
        display: "flex",
        width: CELL_WIDTH,
        height: CELL_HEIGHT,
        right: 0,
        top: 0
    },
    buttonDraggable: {
        display: "flex",
        alignItem: "center",
        justifyContent: "center",
        width: "100%",
        height: "100%"
    },
    pedalItem: {
        position: "absolute",
        width: CELL_WIDTH,
        height: CELL_HEIGHT,
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
    },
    iconFrame: {

        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        
        background: theme.palette.background.paper,
        marginLeft: (CELL_WIDTH - FRAME_SIZE) / 2,
        marginRight: (CELL_WIDTH - FRAME_SIZE) / 2,
        marginTop: (CELL_HEIGHT - FRAME_SIZE) / 2,
        marginBottom: (CELL_HEIGHT - FRAME_SIZE) / 2,
        width: FRAME_SIZE,
        height: FRAME_SIZE,
        border: "1pt #666 solid",
        borderRadius: 6
    },
    borderlessIconFrame: {

        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        
        background: "transparent",
        marginLeft: (CELL_WIDTH - FRAME_SIZE) / 2,
        marginRight: (CELL_WIDTH - FRAME_SIZE) / 2,
        marginTop: (CELL_HEIGHT - FRAME_SIZE) / 2,
        marginBottom: (CELL_HEIGHT - FRAME_SIZE) / 2,
        width: FRAME_SIZE,
        height: FRAME_SIZE,
        border: "0pt #666 solid",
        borderRadius: 6
    },

    pedalIcon: {
        width: 24,
        height: 24,
        opacity: 0.8
    },
    connector: {
        position: "absolute",
        width: CELL_WIDTH,
        height: CELL_HEIGHT
    },
    stroke: {
        position: "absolute",
        background: "#888"
    },
    stereoStrokeOuter: {
        position: "absolute",
        background: "#888"
    },
    stereoStrokeInner: {
        position: "absolute",
        background: "#888"
    },

});

export type OnSelectHandler = (selectedPedal: number) => void;

interface PedalboardProps extends WithStyles<typeof pedalboardStyles> {
    theme: Theme;
    selectedId?: number;
    onSelectionChanged?: OnSelectHandler;
    onDoubleClick?: OnSelectHandler;
    hasTinyToolBar: boolean;

}
interface LayoutSize {
    width: number;
    height: number;
}

type PedalboardState = {
    pedalboard?: Pedalboard;
};

const EMPTY_PEDALS: PedalLayout[] = [];


function makeChain(model: PiPedalModel, uiItems?: PedalboardItem[]): PedalLayout[] {
    let result: PedalLayout[] = [];
    if (uiItems) {
        for (let i = 0; i < uiItems.length; ++i) {
            let item = uiItems[i];
            result.push(new PedalLayout(model, item));
        }
    }
    return result;
}

class PedalLayout {
    uri: string = "";
    name: string = "";
    pluginType: PluginType = PluginType.Plugin;
    iconUrl: string = "";

    bounds: Rect = new Rect();

    numberOfInputs: number = 2;
    numberOfOutputs: number = 2;

    pedalItem?: PedalboardItem;

    // Split Layout only.
    topChildren: PedalLayout[] = EMPTY_PEDALS;
    bottomChildren: PedalLayout[] = EMPTY_PEDALS;
    topConnectorY: number = 0;
    bottomConnectorY: number = 0;


    static Start(): PedalLayout {
        let t: PedalLayout = new PedalLayout();
        t.uri = START_PEDALBOARD_ITEM_URI;
        t.iconUrl = TERMINAL_ICON_URL;
        t.numberOfInputs = 0;
        t.numberOfOutputs = 2;
        return t;
    }
    static End(): PedalLayout {
        let t: PedalLayout = new PedalLayout();
        t.pluginType = PluginType.UtilityPlugin;
        t.uri = END_PEDALBOARD_ITEM_URI;
        t.iconUrl = TERMINAL_ICON_URL;
        t.numberOfInputs = 2;
        t.numberOfOutputs = 0;
        return t;
    }
    constructor(model?: PiPedalModel, pedalItem?: PedalboardItem) {
        if (model === undefined && pedalItem === undefined) {
            return;
        }
        if (model === undefined || pedalItem === undefined) {
            throw new Error("Invalid arguments.");
        }
        this.pedalItem = pedalItem;
        this.uri = pedalItem.uri;
        if (pedalItem.isSplit()) {
            let splitter = pedalItem as PedalboardSplitItem;

            this.pluginType = PluginType.UtilityPlugin;
            this.topChildren = makeChain(model, splitter.topChain);
            this.bottomChildren = makeChain(model, splitter.bottomChain);

            this.numberOfInputs = 2;
            this.numberOfOutputs = 2;

        } else if (pedalItem.isEmpty()) {

            this.pluginType = PluginType.UtilityPlugin;
            this.iconUrl = EMPTY_ICON_URL;
            this.numberOfInputs = 2;
            this.numberOfOutputs = 2;

        } else if (pedalItem.uri === START_PEDALBOARD_ITEM_URI) {
            this.pluginType = PluginType.UtilityPlugin;
            this.iconUrl = TERMINAL_ICON_URL;
            this.numberOfInputs = 0;
            this.numberOfOutputs = PiPedalModelFactory.getInstance().jackSettings.get().inputAudioPorts.length;

        } else if (pedalItem.uri === END_PEDALBOARD_ITEM_URI) {
            this.pluginType = PluginType.UtilityPlugin;
            this.iconUrl = TERMINAL_ICON_URL;
            this.numberOfInputs = PiPedalModelFactory.getInstance().jackSettings.get().outputAudioPorts.length;
            this.numberOfOutputs = 0;
        }
        else {
            let uiPlugin = model.getUiPlugin(pedalItem.uri);
            if (uiPlugin != null) {
                this.pluginType = uiPlugin.plugin_type;
                this.iconUrl = SelectIcon(uiPlugin.plugin_type,uiPlugin.uri);
                this.name = uiPlugin.name;
                this.numberOfInputs = Math.max(uiPlugin.audio_inputs,2);
                this.numberOfOutputs = Math.max(uiPlugin.audio_outputs,2);
            } else {
                // default to empty plugin.
                this.pluginType = PluginType.UtilityPlugin;
                this.name = pedalItem.pluginName??"#error";
                this.iconUrl = ERROR_ICON_URL;
                this.numberOfInputs = 2;
                this.numberOfOutputs = 2;
            }
        }
    }
    isEmpty(): boolean {
        return (!this.pedalItem) || this.pedalItem.isEmpty();
    }
    isSplitter(): boolean {
        return this.pedalItem !== undefined && this.pedalItem.isSplit();
    }
    isStart() {
        return this.uri === START_PEDALBOARD_ITEM_URI;
    }
    isEnd() {
        return this.uri === END_PEDALBOARD_ITEM_URI;
    }
}

function* chainIterator(layoutChain: PedalLayout[]): Generator<PedalLayout, void, undefined> {
    for (let i = 0; i < layoutChain.length; ++i) {
        let item = layoutChain[i];
        yield item;
        if (item.isSplitter()) {
            let g = chainIterator(item.topChildren);
            while (true) {
                let v = g.next();
                if (v.done) {
                    break;
                }
                yield v.value;
            }
            g = chainIterator(item.bottomChildren);
            while (true) {
                let v = g.next();
                if (v.done) {
                    break;
                }
                yield v.value;
            }
        }
    }
    return;
}



class LayoutParams {
    nextId: number = 1;
    cx: number = 0;
    cy: number = 0;
}


const PedalboardView =
    withStyles(pedalboardStyles, { withTheme: true })(
        class extends Component<PedalboardProps, PedalboardState>
        {
            model: PiPedalModel;

            frameRef: React.RefObject<HTMLDivElement>;
            scrollRef: React.RefObject<HTMLDivElement>;


            constructor(props: PedalboardProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();

                if (!props.selectedId) props.selectedId = -1;
                this.state = {
                    pedalboard: this.model.pedalboard.get(),
                };
                this.onPedalboardChanged = this.onPedalboardChanged.bind(this);
                this.frameRef = React.createRef();
                this.scrollRef = React.createRef();
                this.handleTouchStart = this.handleTouchStart.bind(this);
            }

            handleTouchStart(e: any)
            {
                // just has to exist to allow Draggable to receive 
                // touchyMove. :-/
            }

            onDragEnd(instanceId: number, clientX: number, clientY: number) {
                if (!this.currentLayout) return;

                if (!this.frameRef.current) return;

                let currentLayout: PedalLayout[] = this.currentLayout;
                let frameElement = this.frameRef.current;

                let rc = frameElement.getBoundingClientRect();
                clientX -= rc.left;
                clientY -= rc.top;

                let it = chainIterator(currentLayout);

                while (true) {
                    let v = it.next();
                    if (v.done) break;
                    let item = v.value;

                    if (item.isSplitter() && item.pedalItem) {
                        if (item.bounds.contains(clientX, clientY)) {
                            if (clientX < item.bounds.x + CELL_WIDTH / 2) {
                                this.model.movePedalboardItemBefore(instanceId, item.pedalItem.instanceId);
                                this.setSelection(instanceId);
                                return;
                            } else if (clientX > item.bounds.right - CELL_WIDTH / 2) {
                                this.model.movePedalboardItemAfter(instanceId, item.pedalItem.instanceId);
                                this.setSelection(instanceId);
                                return;

                            }
                        }
                        let yMid = (item.bounds.y + item.bounds.bottom) / 2;
                        if (clientX >= item.bounds.x
                            && clientY < yMid && clientY >= item.topChildren[0].bounds.y

                        ) {
                            if (clientX < item.topChildren[0].bounds.x) {
                                let topPedalItem = item.topChildren[0].pedalItem;
                                if (topPedalItem) {
                                    this.model.movePedalboardItemBefore(instanceId, topPedalItem.instanceId);
                                    this.setSelection(instanceId);
                                    return;
                                }
                            }
                            let lastTop = item.topChildren[item.topChildren.length - 1];
                            if (clientX >= lastTop.bounds.right && clientX < item.bounds.right - CELL_WIDTH / 2) {
                                if (lastTop.pedalItem) {
                                    this.model.movePedalboardItemAfter(instanceId, lastTop.pedalItem.instanceId);
                                    this.setSelection(instanceId);
                                    return;
                                }
                            }

                        }
                        if (clientX >= item.bounds.x
                            && clientY > yMid && clientY < item.bottomChildren[0].bounds.bottom
                        ) {
                            if (clientX < item.bottomChildren[0].bounds.x) {
                                let bottomPedalItem = item.bottomChildren[0].pedalItem;
                                if (bottomPedalItem) {
                                    this.model.movePedalboardItemBefore(instanceId, bottomPedalItem.instanceId);
                                    this.setSelection(instanceId);
                                    return;
                                }
                            }
                            let lastBottom = item.bottomChildren[item.bottomChildren.length-1];
                            if (clientX >= lastBottom.bounds.right && clientX < item.bounds.right-CELL_WIDTH/2)
                            {
                                if (lastBottom.pedalItem) {
                                    this.model.movePedalboardItemAfter(instanceId, lastBottom.pedalItem.instanceId);
                                    this.setSelection(instanceId);
                                    return;
                                }

                            }

                        }

                    } else if (item.bounds.contains(clientX, clientY)) {
                        if (item.isStart()) {
                            this.model.movePedalboardItemToStart(instanceId);
                            this.setSelection(instanceId);
                            return;
                        } else if (item.isEnd()) {
                            this.model.movePedalboardItemToEnd(instanceId);
                            this.setSelection(instanceId);
                            return;
                        } else {
                            if (item.pedalItem)
                            {
                                let margin = (CELL_WIDTH - FRAME_SIZE) / 2;
                                if (clientX < item.bounds.x + margin) {
                                    this.model.movePedalboardItemBefore(instanceId, item.pedalItem.instanceId);
                                } else if (clientX > item.bounds.right - margin) {
                                    this.model.movePedalboardItemAfter(instanceId, item.pedalItem.instanceId);
                                } else {
                                    this.model.movePedalboardItem(instanceId, item.pedalItem.instanceId);
                                }
                                this.setSelection(instanceId);
                                return;
                            }
                        }

                    }
                }
                // delete the plugin.
                let newId = this.model.setPedalboardItemEmpty(instanceId);
                this.setSelection(newId);

            }

            onPedalboardChanged(value?: Pedalboard) {
                this.setState({ pedalboard: value });
            }

            componentDidMount() {
                this.scrollRef.current!.addEventListener("touchstart",this.handleTouchStart, {passive: false});
                this.model.pedalboard.addOnChangedHandler(this.onPedalboardChanged);

            }
            componentWillUnmount() {
                this.scrollRef.current!.removeEventListener("touchstart",this.handleTouchStart);
                this.model.pedalboard.removeOnChangedHandler(this.onPedalboardChanged);
            }

            offsetLayout_(layoutItems: PedalLayout[], offset: number): void {
                for (let i = 0; i < layoutItems.length; ++i) {
                    let layoutItem = layoutItems[i];
                    layoutItem.bounds.y += offset;
                    if (layoutItem.isSplitter()) {
                        layoutItem.topConnectorY += offset;
                        layoutItem.bottomConnectorY += offset;
                        this.offsetLayout_(layoutItem.topChildren, offset);
                        this.offsetLayout_(layoutItem.bottomChildren, offset);
                    }
                }
            }

            getSplitterIcon(layoutItem: PedalLayout) {
                if (layoutItem.pedalItem === undefined) {
                    throw new Error("Invalid splitter");
                }
                let split = layoutItem.pedalItem as PedalboardSplitItem;
                if (split.getSplitType() === SplitType.Ab) {
                    if (split.isASelected()) {
                        return "img/fx_split_a.svg";
                    } else {
                        return "img/fx_split_b.svg";

                    }
                } else if (split.getSplitType() === SplitType.Mix) {
                    return "img/fx_dial.svg";
                } else {
                    return "img/fx_lr.svg";
                }
            }

            doLayout2_(lp: LayoutParams, layoutItems: PedalLayout[]): Rect {
                let bounds = new Rect();
                for (let i = 0; i < layoutItems.length; ++i) {
                    let layoutItem = layoutItems[i];
                    if (layoutItem.isSplitter()) {
                        let x0 = lp.cx;
                        let y0 = lp.cy;

                        layoutItem.bounds.x = x0;
                        layoutItem.bounds.y = y0;
                        layoutItem.bounds.height = CELL_HEIGHT;
                        layoutItem.bounds.width = CELL_WIDTH;

                        lp.cx += CELL_WIDTH;


                        let topBounds = this.doLayout2_(lp, layoutItem.topChildren);
                        if (topBounds.isEmpty()) {
                            topBounds.x = lp.cx;
                            topBounds.width = 0;
                            topBounds.y = y0 - CELL_HEIGHT / 2;
                            topBounds.height = CELL_HEIGHT;

                        }
                        

                        let dyTop = (lp.cy + CELL_HEIGHT / 2)- (topBounds.y + topBounds.height) ;

                        

                        this.offsetLayout_(layoutItem.topChildren, dyTop);
                        topBounds.offset(0, dyTop);
                        bounds.accumulate(topBounds);

                        let topCx = lp.cx;
                        lp.cx = x0;
                        lp.cx += CELL_WIDTH;

                        let bottomBounds = this.doLayout2_(lp, layoutItem.bottomChildren);
                        if (bottomBounds.isEmpty()) {
                            bottomBounds.x = lp.cx; bottomBounds.width = 0;
                            bottomBounds.y = lp.cy; bottomBounds.height = CELL_HEIGHT;
                        }
                        
                        let dyBottom = (lp.cy+CELL_HEIGHT/2)-bottomBounds.y ;
                        this.offsetLayout_(layoutItem.bottomChildren, dyBottom)
                        bottomBounds.offset(0, dyBottom);
                        bounds.accumulate(bottomBounds);

                        lp.cx = Math.max(lp.cx, topCx) + CELL_WIDTH;
                        lp.cy = y0;

                        layoutItem.bounds.width = lp.cx - layoutItem.bounds.x;
                        bounds.accumulate(layoutItem.bounds);

                        if (layoutItem.topChildren.length === 0) {
                            layoutItem.topConnectorY = bounds.y + CELL_HEIGHT / 2;
                        } else {
                            layoutItem.topConnectorY = layoutItem.topChildren[0].bounds.y + CELL_HEIGHT / 2;
                        }
                        if (layoutItem.bottomChildren.length === 0) {
                            layoutItem.bottomConnectorY = bounds.y + bounds.height - CELL_HEIGHT / 2;
                        } else {
                            layoutItem.bottomConnectorY = layoutItem.bottomChildren[0].bounds.y + CELL_HEIGHT / 2;
                        }
                    } else {
                        layoutItem.bounds.x = lp.cx;
                        layoutItem.bounds.y = lp.cy;
                        lp.cx += CELL_WIDTH;
                        layoutItem.bounds.width = CELL_WIDTH;
                        layoutItem.bounds.height = CELL_HEIGHT;
                        bounds.accumulate(layoutItem.bounds);

                    }
                }
                return bounds;
            }
            doLayout(layoutItems: PedalLayout[]): LayoutSize {
                const TWO_ROW_HEIGHT = 142 - 14;
                
                if (layoutItems.length === 0)
                {
                    // if the current pedalboard is empty, reserve display space anyway.
                    return {width: 1, height: TWO_ROW_HEIGHT};
                }

                let lp = new LayoutParams();

                let bounds = this.doLayout2_(lp, layoutItems);
                // shift everything down so there are no negative y coordinates.

                if (bounds.height < TWO_ROW_HEIGHT) {
                    
                    let extra = Math.floor((TWO_ROW_HEIGHT - Math.ceil(bounds.height))/2);
                    this.offsetLayout_(layoutItems, Math.floor(-bounds.y + extra/2 ));
                    bounds.height += extra;

                } else {
                    this.offsetLayout_(layoutItems, -bounds.y);
                }

                bounds.height += 14; // for labels that aren't accounted for.
                return { width: bounds.width, height: bounds.height };

            }

            onItemClick(e: SyntheticEvent, instanceId?: number): void {
                if (instanceId) {
                    this.setSelection(instanceId);
                }
            }
            setSelection(instanceId: number) {
                if (this.props.onSelectionChanged) {
                    this.props.onSelectionChanged(instanceId);
                }
            }

            onItemDoubleClick(event: SyntheticEvent, instanceId?: number): void {
                event.preventDefault();
                event.stopPropagation();

                if (this.props.onDoubleClick && instanceId) {
                    this.props.onDoubleClick(instanceId);
                }

            }

            onItemLongClick(event: SyntheticEvent, instanceId?: number): void {
                event.preventDefault();
                event.stopPropagation();

                if (!Utility.isTouchDevice()) {
                    if (this.props.onDoubleClick && instanceId) {
                        this.props.onDoubleClick(instanceId);
                    }
                }

            }

            renderConnector(output: ReactNode[], item: PedalLayout, enabled: boolean): void {
                // let classes = this.props.classes;
                let x_ = item.bounds.x + CELL_WIDTH / 2;
                let y_ = item.bounds.y + CELL_HEIGHT / 2;
                let numberOfOutputs = item.numberOfOutputs;
                let color = enabled ? ENABLED_CONNECTOR_COLOR : DISABLED_CONNECTOR_COLOR;
                let svgPath = new SvgPathBuilder().moveTo(x_, y_).lineTo(x_ + CELL_WIDTH, y_).toString();

                if (numberOfOutputs === 2) {
                    output.push((
                        <path key={this.renderKey++}  d={svgPath} stroke={color} strokeWidth={SVG_STEREO_STROKE_WIDTH} />
                    ));
                    output.push((
                        <path key={this.renderKey++}  d={svgPath} stroke="white" strokeWidth={SVG_STROKE_WIDTH} />
                    ));
                } else if (numberOfOutputs === 1) {
                    output.push((
                        <path key={this.renderKey++}  d={svgPath} stroke={color} strokeWidth={SVG_STROKE_WIDTH} />
                    ));
                }
            }
            renderSplitConnectors(output: ReactNode[], item: PedalLayout, enabled: boolean, shortSplitOutput: boolean,): void {
                //let classes = this.props.classes;
                let x_ = item.bounds.x + CELL_WIDTH / 2;
                let y_ = item.bounds.y + CELL_HEIGHT / 2;
                let yTop = item.topConnectorY;
                let yBottom = item.bottomConnectorY;
                //let isStereo = item.stereoOutput;
                let split = item.pedalItem as PedalboardSplitItem;

                let topEnabled = enabled && split.isASelected();
                let bottomEnabled = enabled && split.isBSelected();
                let topColor = topEnabled ? ENABLED_CONNECTOR_COLOR : DISABLED_CONNECTOR_COLOR;
                let bottomColor = bottomEnabled ? ENABLED_CONNECTOR_COLOR : DISABLED_CONNECTOR_COLOR;


                let topStartPath = new SvgPathBuilder().moveTo(x_, y_).lineTo(x_, yTop).lineTo(x_ + CELL_WIDTH, yTop).toString();
                let bottomStartPath = new SvgPathBuilder().moveTo(x_, y_).lineTo(x_, yBottom).lineTo(x_ + CELL_WIDTH, yBottom).toString();

                if (item.numberOfInputs === 2 && item.topChildren[0].numberOfInputs === 2) {
                    output.push((<path key={this.renderKey++}  d={topStartPath} stroke={topColor} strokeWidth={SVG_STEREO_STROKE_WIDTH} />));
                    output.push((<path key={this.renderKey++}  d={topStartPath} stroke="white" strokeWidth={SVG_STROKE_WIDTH} />));
                } else if (item.numberOfInputs !== 0 && item.topChildren[0].numberOfInputs !== 0) {
                    output.push((<path key={this.renderKey++}  d={topStartPath} stroke={topColor} strokeWidth={SVG_STROKE_WIDTH} />));
                }

                if (item.numberOfInputs === 2 && item.bottomChildren[0].numberOfInputs === 2) {
                    output.push((<path key={this.renderKey++}  d={bottomStartPath} stroke={bottomColor} strokeWidth={SVG_STEREO_STROKE_WIDTH} />));
                    output.push((<path key={this.renderKey++}  d={bottomStartPath} stroke="white" strokeWidth={SVG_STROKE_WIDTH} />));

                } else if (item.numberOfInputs !== 0 && item.bottomChildren[0].numberOfInputs !== 0) {
                    output.push((<path key={this.renderKey++}  d={bottomStartPath} stroke={bottomColor} strokeWidth={SVG_STROKE_WIDTH} />));
                }

                let lastTop = item.topChildren[item.topChildren.length - 1];
                let lastBottom = item.bottomChildren[item.bottomChildren.length - 1];

                let xTop = lastTop.bounds.right - CELL_WIDTH / 2;
                let xBottom = lastBottom.bounds.right - CELL_WIDTH / 2;

                let xEnd = shortSplitOutput ? item.bounds.right : item.bounds.right + CELL_WIDTH / 2;
                let xTee0 = item.bounds.right - CELL_WIDTH / 2;

                let firstPath: string;  // top or bottom depending on draw order.
                let secondPath: string;  // top or bottom depending on draw order.

                let firstPathStereo: boolean;
                let secondPathStereo: boolean;
                let firstPathAbsent: boolean;
                let secondPathAbsent: boolean;
                let firstPathEnabled: boolean;
                let secondPathEnabled: boolean;
                let xTee: number;

                let monoAdjustment = (STEREO_STROKE_WIDTH - STROKE_WIDTH) / 2;

                let bottomPathFirst = topEnabled && !bottomEnabled;
                let topPathFirst = bottomEnabled && !topEnabled;


                // Third case: L/R stereo output, when both outputs are mono, requires a third stroke.
                let thirdPath: string | null = null; // for L/R stereo output (which can be stereo even if both outputs are mono)
                let hasThirdPath = item.numberOfOutputs === 2 && (lastTop.numberOfOutputs !== 2) && (lastBottom.numberOfOutputs !== 2);

                if (hasThirdPath) {
                    firstPathStereo = false;
                    secondPathStereo = false;
                    firstPathAbsent = lastTop.numberOfOutputs === 0 || item.numberOfOutputs === 0;
                    secondPathAbsent = lastBottom.numberOfOutputs === 0 || item.numberOfOutputs === 0;
                    xTee = xTee0 - monoAdjustment;
                    firstPath = new SvgPathBuilder().moveTo(xBottom, yBottom).lineTo(xTee, yBottom).lineTo(xTee, y_).toString();

                    secondPath = new SvgPathBuilder().moveTo(xTop, yTop).lineTo(xTee, yTop).lineTo(xTee, y_).toString();

                    hasThirdPath = true;
                    thirdPath= new SvgPathBuilder().moveTo(xTee0,y_).lineTo(xEnd,y_).toString();
                    firstPathEnabled = bottomEnabled;
                    secondPathEnabled = topEnabled;
                } else if (bottomPathFirst || (topEnabled && lastTop.numberOfOutputs === 2)) {
                    // draw the bottom path first.
                    firstPathStereo = item.numberOfOutputs === 2 && lastBottom.numberOfOutputs === 2;
                    secondPathStereo = item.numberOfOutputs === 2 && lastTop.numberOfOutputs === 2;
                    firstPathAbsent = item.numberOfOutputs === 0 || lastBottom.numberOfOutputs === 0;
                    secondPathAbsent = item.numberOfOutputs === 0 || lastTop.numberOfOutputs === 0;

                    xTee = firstPathStereo ? xTee0 : xTee0 - monoAdjustment;
                    firstPath = new SvgPathBuilder().moveTo(xBottom, yBottom).lineTo(xTee, yBottom).lineTo(xTee, y_).toString();
                    xTee = secondPathStereo ? xTee0 : xTee0 - monoAdjustment;

                    secondPath = new SvgPathBuilder().moveTo(xTop, yTop).lineTo(xTee, yTop).lineTo(xTee, y_).lineTo(xEnd, y_).toString();
                    firstPathEnabled = bottomEnabled;
                    secondPathEnabled = topEnabled;
                } else {
                    // draw the top path first.
                    firstPathStereo = item.numberOfOutputs === 2 && lastTop.numberOfOutputs === 2;
                    secondPathStereo = item.numberOfOutputs === 2 && lastBottom.numberOfOutputs === 2;
                    firstPathAbsent = item.numberOfOutputs === 0 || lastTop.numberOfOutputs === 0;
                    secondPathAbsent = item.numberOfOutputs === 0 || lastBottom.numberOfOutputs === 0;

                    xTee = firstPathStereo ? xTee0 : xTee0 - monoAdjustment;
                    firstPath = new SvgPathBuilder().moveTo(xTop, yTop).lineTo(xTee, yTop).lineTo(xTee, y_).toString();

                    xTee = secondPathStereo ? xTee0 : xTee0 - monoAdjustment;
                    secondPath = new SvgPathBuilder().moveTo(xBottom, yBottom).lineTo(xTee, yBottom).lineTo(xTee, y_).lineTo(xEnd, y_).toString();

                    firstPathEnabled = topEnabled;
                    secondPathEnabled = bottomEnabled;
                }
                let firstPathColor = firstPathEnabled ? ENABLED_CONNECTOR_COLOR : DISABLED_CONNECTOR_COLOR;
                let secondPathColor = secondPathEnabled ? ENABLED_CONNECTOR_COLOR : DISABLED_CONNECTOR_COLOR;

                if (bottomPathFirst || topPathFirst) {
                    // display stereo strokes with cutoff line.
                    if (firstPathStereo) {
                        output.push((
                            <path key={this.renderKey++}  d={firstPath} stroke={firstPathColor} strokeWidth={SVG_STEREO_STROKE_WIDTH} />
                        ));
                        output.push((
                            <path key={this.renderKey++}  d={firstPath} stroke="white" strokeWidth={SVG_STROKE_WIDTH} />
                        ));
                    } else if (!firstPathAbsent) {
                        output.push((
                            <path key={this.renderKey++}  d={firstPath} stroke={firstPathColor} strokeWidth={SVG_STROKE_WIDTH} />
                        ));
                    }
                    if (secondPathStereo) {
                        output.push((
                            <path key={this.renderKey++}  d={secondPath} stroke={secondPathColor} strokeWidth={SVG_STEREO_STROKE_WIDTH} />
                        ));
                        output.push((
                            <path key={this.renderKey++}  d={secondPath} stroke="white" strokeWidth={SVG_STROKE_WIDTH} />
                        ));
                    } else if (!secondPathAbsent) {
                        output.push((
                            <path key={this.renderKey++}  d={secondPath} stroke={secondPathColor} strokeWidth={SVG_STROKE_WIDTH} />
                        ));
                    }

                } else {
                    // stereo strokes merge.
                    if (firstPathStereo) {
                        output.push((
                            <path key={this.renderKey++}  d={firstPath} stroke={firstPathColor} strokeWidth={SVG_STEREO_STROKE_WIDTH} />
                        ));
                    } else if (!firstPathAbsent) {
                        output.push((
                            <path key={this.renderKey++}  d={firstPath} stroke={firstPathColor} strokeWidth={SVG_STROKE_WIDTH} />
                        ));
                    }
                    if (secondPathStereo) {
                        output.push((
                            <path key={this.renderKey++}  d={secondPath} stroke={secondPathColor} strokeWidth={SVG_STEREO_STROKE_WIDTH} />
                        ));
                    } else if (!secondPathAbsent) {
                        output.push((
                            <path key={this.renderKey++}  d={secondPath} stroke={secondPathColor} strokeWidth={SVG_STROKE_WIDTH} />
                        ));
                    }

                    // draw stereo inner lines.
                    if (firstPathStereo) {
                        output.push((
                            <path key={this.renderKey++}  d={firstPath} stroke="white" strokeWidth={SVG_STROKE_WIDTH} />
                        ));
                    }
                    if (secondPathStereo) {
                        output.push((
                            <path key={this.renderKey++}  d={secondPath} stroke="white" strokeWidth={SVG_STROKE_WIDTH} />
                        ));

                    }
                }
                if (thirdPath != null)
                {
                    // stereo output of L/R splitter
                    output.push((
                        <path key={this.renderKey++}  d={thirdPath} stroke={secondPathColor} strokeWidth={SVG_STEREO_STROKE_WIDTH} />
                    ));
                    output.push((
                        <path key={this.renderKey++}  d={thirdPath} stroke="white" strokeWidth={SVG_STROKE_WIDTH} />
                    ));


                }
            }
            getScrollContainer()
            {
                let el: HTMLElement | undefined | null= this.scrollRef.current;
                // actually not here anymore. :-/ It has a reactive definition in MainPage.tsx now.
                while (el)
                {
                    if (el.id === "pedalboardScroll")
                    {
                        return el as HTMLDivElement;
                    }
                    el = el.parentElement;
                }
                throw new PiPedalStateError("scroll container not found.");
            }

            pedalButton(instanceId: number, iconUrl: string, draggable: boolean, enabled: boolean,hasBorder: boolean = true): ReactNode {
                let classes = this.props.classes;
                return (
                    <div className={hasBorder? classes.iconFrame : classes.borderlessIconFrame} onContextMenu={(e) => { e.preventDefault(); }}>

                        <ButtonBase style={{ width: "100%", height: "100%" }}
                            onClick={(e) => { this.onItemClick(e, instanceId); }}
                            onDoubleClick={(e: SyntheticEvent) => { this.onItemDoubleClick(e, instanceId); }}
                            onContextMenu={(e: SyntheticEvent) => { this.onItemLongClick(e, instanceId); }}
                        >
                            <SelectHoverBackground selected={instanceId === this.props.selectedId} showHover={true} borderRadius={6} >
                                <Draggable  draggable={draggable} getScrollContainer={() => this.getScrollContainer()}
                                    onDragEnd={(x, y) => { this.onDragEnd(instanceId, x, y) }}
                                >
                                    <img src={iconUrl} className={classes.pedalIcon} alt="Pedal" draggable={false} 
                                    style={{opacity: enabled? 0.99: 0.6}} />
                                </Draggable>
                            </SelectHoverBackground>
                        </ButtonBase>
                    </div>
                );

            }
            renderConnectors(output: ReactNode[], layoutChain: PedalLayout[], enabled: boolean, shortSplitOutput: boolean): void {
                let length = layoutChain.length - 1;
                if (layoutChain.length > 0 && layoutChain[layoutChain.length - 1].isSplitter()) {
                    ++length;
                }
                for (let i = 0; i < length; ++i) {
                    let item = layoutChain[i];
                    if (item.isSplitter()) {
                        let splitter = item.pedalItem as PedalboardSplitItem;
                        this.renderSplitConnectors(output, item, enabled, i === length - 1 && shortSplitOutput,);
                        this.renderConnectors(output, item.topChildren, enabled && splitter.isASelected(), false);
                        this.renderConnectors(output, item.bottomChildren, enabled && splitter.isBSelected(), false);

                    } else if (item.uri !== END_PEDALBOARD_ITEM_URI) {
                        this.renderConnector(output, item, enabled);

                    }
                }
            }
            renderConnectorFrame(layoutChain: PedalLayout[], layoutSize: LayoutSize): ReactNode {
                let outputs: ReactNode[] = [];
                this.renderConnectors(outputs, layoutChain, true, false);
                return (
                    <div style={{width: layoutSize.width, height: layoutSize.height, overflow: "hidden"}}>
                    <svg width={layoutSize.width} height={layoutSize.height}
                        xmlns="http://www.w3.org/2000/svg" viewBox={"0 0 " + layoutSize.width + " " + layoutSize.height}>
                        <g fill="none">
                            {
                                outputs
                            }
                        </g>

                    </svg>
                    </div>
                );

            }

            renderChain(layoutChain: PedalLayout[], layoutSize: LayoutSize): ReactNode {

                let classes = this.props.classes;

                let result: ReactNode[] = [];

                result.push(this.renderConnectorFrame(layoutChain, layoutSize));

                let it = chainIterator(layoutChain);
                while (true) {
                    let v = it.next();
                    if (v.done) break;
                    let item = v.value;
                    switch (item.uri) {
                        case START_PEDALBOARD_ITEM_URI:
                            result.push(<div key={this.renderKey++} className={classes.splitItem} style={{ left: item.bounds.x, top: item.bounds.y, width: item.bounds.width }} >
                                <div className={classes.splitStart} >

                                    {this.pedalButton(START_CONTROL, item.iconUrl, false,true,false)}
                                </div>
                            </div>);
                            break;
                        case END_PEDALBOARD_ITEM_URI:
                            result.push(<div key={this.renderKey++} className={classes.splitItem} style={{ left: item.bounds.x, top: item.bounds.y, width: item.bounds.width }} >
                                <div className={classes.splitStart} >

                                    {this.pedalButton(END_CONTROL, item.iconUrl, false,true,false)}
                                </div>
                            </div>);
                            break;
                        default:
                            if (item.isSplitter()) {

                                result.push(<div key={this.renderKey++} className={classes.splitItem} style={{ left: item.bounds.x, top: item.bounds.y, width: item.bounds.width }} >
                                    <div className={classes.splitStart} >
                                        {this.pedalButton(item.pedalItem?.instanceId ?? -1, this.getSplitterIcon(item), false,true)}
                                    </div>
                                </div>);

                            } else {
                                result.push(
                                    <div key={this.renderKey++} style={{
                                        display: "flex", justifyContent: "flex-start", alignItems: "flex-start",
                                        position: "absolute", left: item.bounds.x, width: CELL_WIDTH, top: item.bounds.bottom - 12, paddingLeft: 2, paddingRight: 2
                                    }}>
                                        <Typography variant="caption" display="block" noWrap={true}
                                            style={{ width: CELL_WIDTH - 4, textAlign: "center", flex: "0 1 auto" }}
                                        >{item.pedalItem?.pluginName}</Typography>
                                    </div>
                                )
                                result.push(<div key={this.renderKey++} className={classes.pedalItem} style={{ left: item.bounds.x, top: item.bounds.y }} >
                                    {this.pedalButton(item.pedalItem?.instanceId ?? -1, item.iconUrl, !item.isEmpty(), item.pedalItem?.isEnabled ?? false)}

                                </div>);

                            }
                            break;
                    }

                }
                return result;
            }

            canInputStero(item: PedalLayout): boolean {
                if (item.pedalItem) {
                    let plugin = this.model.getUiPlugin(item.pedalItem.uri);
                    if (plugin) {
                        return plugin.audio_inputs === 2;
                    }
                }
                return true;
            }
            getNumberOfInputs(item: PedalLayout): number {
                if (item.pedalItem) {
                    let plugin = this.model.getUiPlugin(item.pedalItem.uri);
                    if (plugin) {
                        return plugin.audio_inputs;
                    }
                }
                return 1;
            }
            getNumberOfOutputs(item: PedalLayout): number {
                if (item.pedalItem) {
                    let plugin = this.model.getUiPlugin(item.pedalItem.uri);
                    if (plugin) {
                        return plugin.audio_outputs;
                    }
                }
                return 1;
            }

            markStereoOutputs(layoutChain: PedalLayout[], numberOfInputs: number, numberOfOutputs: number) {
                // analyze forward flow.
                this.markStereoForward(layoutChain, numberOfInputs);
                // mark items that feed a mono effect as mono.
                this.markStereoBackward(layoutChain, numberOfOutputs);
            }

            markStereoBackward(layoutChain: PedalLayout[], numberOfOutputs: number) : number
            {
                for (let i = layoutChain.length-1; i >= 0; --i)
                {
                    let item = layoutChain[i];
                    if (item.isSplitter())
                    {
                        item.numberOfOutputs = CalculateConnection(item.numberOfOutputs, numberOfOutputs)

                        this.markStereoBackward(item.topChildren,numberOfOutputs);
                        this.markStereoBackward(item.bottomChildren,numberOfOutputs);
                        let topInputs = item.topChildren[0].numberOfInputs;
                        let bottomInputs = item.bottomChildren[0].numberOfInputs;

                        let splitItem = item.pedalItem as PedalboardSplitItem;
                        if (splitItem.getSplitType() !== SplitType.Lr)
                        {
                            item.numberOfInputs = CalculateConnection(item.numberOfInputs, Math.max(topInputs,bottomInputs));
                        }
                    } else if (item.isEnd())
                    {

                    } else if (item.isStart())
                    {
                        item.numberOfOutputs = CalculateConnection(item.numberOfOutputs,numberOfOutputs);
                        return item.numberOfOutputs;

                    } else if (item.isEmpty()) {
                        if (numberOfOutputs === 0)
                        {
                            item.numberOfOutputs = 0;
                            item.numberOfInputs = CalculateConnection(item.numberOfInputs,2);
                        } else {
                            item.numberOfOutputs = CalculateConnection(item.numberOfOutputs,numberOfOutputs);
                            item.numberOfInputs = CalculateConnection(item.numberOfInputs,numberOfOutputs);
                        }
                    } else {
                        item.numberOfOutputs = CalculateConnection(item.numberOfOutputs,numberOfOutputs);
                    }
                    numberOfOutputs = item.numberOfInputs;
                }
                return numberOfOutputs;
            }
            markStereoForward(layoutChain: PedalLayout[],numberOfInputs: number): number
            {
                if (layoutChain.length === 0) {
                    return numberOfInputs;
                }
                for (let i = 0; i < layoutChain.length; ++i) {
                    let item = layoutChain[i];
                    if (item.isSplitter()) {
                        let splitter = item.pedalItem as PedalboardSplitItem;
                        item.numberOfInputs = numberOfInputs;

                        let chainInputs = numberOfInputs;
                        if (splitter.getSplitType() === SplitType.Lr)
                        {
                            chainInputs = CalculateConnection(numberOfInputs,1);
                        }
                        let topOutputs = this.markStereoForward(item.topChildren, chainInputs);
                        let bottomOutputs = this.markStereoForward(item.bottomChildren, chainInputs);


                        if (splitter.getSplitType() === SplitType.Ab) {
                            if (splitter.isASelected()) {
                                item.numberOfOutputs = topOutputs;
                            } else {
                                item.numberOfOutputs = bottomOutputs;
                            }
                        } else {
                            item.numberOfOutputs = (topOutputs >= 1 || bottomOutputs >= 1) ? 2: 1;
                        } 
                    } else if (item.isStart())
                    {
                        item.numberOfOutputs = Math.min(PiPedalModelFactory.getInstance().jackSettings.get().inputAudioPorts.length,2);
                    } else if (item.isEnd()) {
                        item.numberOfInputs = 
                            CalculateConnection(
                                Math.min(PiPedalModelFactory.getInstance().jackSettings.get().outputAudioPorts.length,2),
                                numberOfInputs);
                        return item.numberOfInputs;
                    } else if (item.isEmpty()) {
                        item.numberOfInputs = numberOfInputs;
                        if (numberOfInputs === 0)
                        {
                            item.numberOfOutputs = 2;
                        } else {
                            item.numberOfOutputs = item.numberOfInputs;
                        }
                    } else  {
                        item.numberOfInputs = CalculateConnection(numberOfInputs,this.getNumberOfInputs(item));
                        item.numberOfOutputs = this.getNumberOfOutputs(item);
                    }
                    numberOfInputs = item.numberOfOutputs;
                }
                return numberOfInputs;
            }

            currentLayout?: PedalLayout[];
            private renderKey: number = 0;
            render() {
                const { classes } = this.props;
                this.renderKey = 0;
                let layoutChain = makeChain(this.model, this.state.pedalboard?.items);
                let start = PedalLayout.Start();
                let end = PedalLayout.End();
                if (layoutChain.length !== 0)
                {
                    layoutChain.splice(0, 0, start);
                    layoutChain.splice(layoutChain.length, 0, end);
                    this.markStereoOutputs(layoutChain, 2,2);
                }

                let layoutSize = this.doLayout(layoutChain);
                

                this.currentLayout = layoutChain; // save for mouse processing &c.

                return (
                    <div className={classes.scrollContainer} ref={this.scrollRef}
                     >
                        <div className={classes.container} ref={this.frameRef}
                            style={{
                                width: layoutSize.width, height: layoutSize.height,
                            }} >
                            {this.renderChain(layoutChain, layoutSize)}
                        </div>
                    </div>
                );
            }

        }
    );

export default PedalboardView