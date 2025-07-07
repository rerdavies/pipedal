/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

import { UiPlugin, UiControl, ControlType } from './Lv2Plugin';
import React from 'react';
import CloseIcon from '@mui/icons-material/Close';
import IconButtonEx from './IconButtonEx';
import Typography from '@mui/material/Typography/Typography';
import ModGuiErrorBoundary from './ModGuiErrorBoundary';
import OkDialog from './OkDialog';
import {pathParentDirectory} from './FileUtils';

import { PiPedalModel, PiPedalModelFactory, MonitorPortHandle, State,
    ListenHandle, PatchPropertyListener } from './PiPedalModel';

const RANGE_SCALE = 120; // 120 pixels to move from 0 to 1.
const FINE_RANGE_SCALE = RANGE_SCALE * 10; // 1200 pixels to move from 0 to 1.
const ULTRA_FINE_RANGE_SCALE = RANGE_SCALE * 50; // 12000 pixels to move from 0 to 1.

const WHEEL_RANGE_SCALE = 120 * 20;
const FINE_WHEEL_RANGE_SCALE = WHEEL_RANGE_SCALE * 10;
const ULTRA_FINE_WHEEL_RANGE_SCALE = WHEEL_RANGE_SCALE * 50;

export interface IModGuiHostSite {
    monitorPort: (
        instanceId: number,
        symbol: string,
        interval: number,
        callback: (value: number) => void) => MonitorPortHandle;
    unmonitorPort: (handle: MonitorPortHandle) => void;
    setPedalboardControl: (instanceId: number, symbol: string, value: number) => void;

    monitorPatchProperty(
        instanceId: number,
        propertyUri: string,
        onReceived: PatchPropertyListener
    ): ListenHandle;    

    cancelMonitorPatchProperty(listenHandle: ListenHandle): void;

    getPatchProperty(instanceId: number, uri: string): Promise<any>;
    setPatchProperty(instanceId: number, uri: string, value: any): Promise<boolean>
};

export interface ModGuiHostProps {
    instanceId: number,
    plugin: UiPlugin;
    onClose: () => void;
    width?: number;
    height?: number;
    test?: boolean;
    hostSite: IModGuiHostSite;
}
export interface ModGuiJs {
    set_port_value: (symbol: string, value: number) => void;
    patch_get: (uri: string) => any;
    patch_set: (uri: string, valuetype: string, value: any) => void;
    get_custom_resource_filename: (filename: string) => string;
    get_port_index_for_symbol: (symbol: string) => number;
    get_port_symbol_for_index: (index: number) => string | null;
};

export interface ModGuiControl {

    //setControlValue: (value: number) => void;
    onMounted: () => void;
    onUnmount: () => void;
    render(): HTMLElement | null;
};

interface CustomSelectPathControlProps {
    instanceId: number;
    plugin: UiPlugin;
    propertyUri: string,
    hostSite: IModGuiHostSite;
};


class CustomSelectPathControl implements ModGuiControl {

    private props: CustomSelectPathControlProps;
    private frameElement?: HTMLElement;

    private animationFrameId: number | null = null;

    constructor(props: CustomSelectPathControlProps) {
        this.props = props;
    }


    requestUpdate() {
        if (!this.mounted) return;
        if (!this.frameElement) {
            return;
        }


        if (this.animationFrameId === null) {
            this.animationFrameId = window.requestAnimationFrame(() => {
                this.animationFrameId = null;
                this.updateImage();
            });
        }
    }
    cancelRequestUpdate() {
        let animationFrameId = this.animationFrameId;
        if (animationFrameId !== null) {
            window.cancelAnimationFrame(animationFrameId);
            this.animationFrameId = null;
        }
    }

    requestFileUpdate() {

    }
    private pathValue: string | null = null;
    private browsePath: string | null = null;
    setPathValue(value: string) {
        if (this.pathValue !== value) {
            this.pathValue = value;
            if (this.pathValue !== "") {
                let browsePath = pathParentDirectory(value);
                if (this.browsePath !== browsePath) {
                    this.browsePath = browsePath;
                    this.requestFileUpdate();
                }
            }
            this.requestUpdate();
        }
    }

    private updateImage() {
        if (!this.frameElement || !this.mounted) {
            return;
        }

        // this.frameElement.querySelectorAll('[.mod_enumerated_list]')
        //     .forEach((inputControl: Element) => {

        //         (inputControl as HTMLElement).textContent = text;
        //     });
        // this.frameElement.querySelectorAll('[mod-role=enumeration-option]')
        //     .forEach((inputControl: Element) => {
        //         let strValue = inputControl.getAttribute("mod-port-value");
        //         let value = parseFloat(strValue || ""); 
        //         inputControl.classList.remove("selected");
        //         if (value === this.value) {
        //             inputControl.classList.add("selected");
        //         }

        //         (inputControl as HTMLElement).textContent = text;
        //     });
        //this.frameElement.textContent = text;

    }

    handlePropertyValueChange(value: any) {

    }
    private mounted: boolean = false;

    private listenHandle: ListenHandle | null = null;
    onMounted() {
        this.mounted = true;
        this.props.hostSite.monitorPatchProperty(this.props.instanceId, this.props.propertyUri,
            (value: any) => {
                this.handlePropertyValueChange(value);
            }
        );
        this.props.hostSite.getPatchProperty(this.props.instanceId, this.props.propertyUri)
        .then((value: any) => {
            this.handlePropertyValueChange(value);
        }).
        catch((error: any) => {
            if (error instanceof Error) {
                console.error((error as Error).message);
            } else {
                console.error(error.toString());
            }
            console.error(error);
        });
        this.requestUpdate();
    }

    onUnmount() {
        if (this.listenHandle) {
            this.props.hostSite.cancelMonitorPatchProperty(this.listenHandle);
            this.listenHandle = null;   
        }
        this.mounted = false;
        this.cancelRequestUpdate();
    }


    render(): HTMLElement | null {
        return null;
    }

    toggleVisibility(element: HTMLElement) {
        element.classList.toggle("hidden");
        let display = element.style.display;
        if (display === "none" || display === "") {
            element.style.display = "block";
        } else {
            element.style.display = "none";
        }
    }

    handleSelected(value: string,strFileType: string)
    {
        this.setPathValue(value);
        this.props.hostSite.setPatchProperty(this.props.instanceId, this.props.propertyUri, value);
    }

    handleClick(event: MouseEvent) {
        if (!this.frameElement) {
            return;
        }
        event.preventDefault();
        event.stopPropagation();
        let enumeratedList = this.frameElement.querySelector(".mod-enumerated-list");
        if (enumeratedList) {
            let element = enumeratedList as HTMLElement;
            this.toggleVisibility(element)
        }
        if (event.target) {
            let target = event.target as HTMLElement;
            let role = target.getAttribute("mod-role");
            if (role === "enumeration-option")  // a click in the drop-down list?
            {
                let strValue = target.getAttribute("mod-parameter-value");
                let strFileType = target.getAttribute("mod-filetype");
                if (strValue && strFileType) {
                    this.handleSelected(strValue,strFileType);
                }

            }
        }
    }

    private enumeratedListContainerElement: HTMLElement | null = null;
    private enumeratedListItemTemplate: string = "";

    attach(frameElement: HTMLElement) {

        this.frameElement = frameElement;
        this.requestUpdate();
        this.frameElement.onclick = (event: MouseEvent) => { this.handleClick(event); }

        // swipe the enumeraation-option generated by the template. 
        // We'll use it as a template to create new file entries.
        let enumeratedList = this.frameElement.querySelector(".mod-enumerated-list");
        if (enumeratedList) {
            this.enumeratedListContainerElement = enumeratedList as HTMLElement;
            if (this.enumeratedListContainerElement.children.length === 1) {
                let firstChild = this.enumeratedListContainerElement.children[0];
                if (firstChild) {
                    this.enumeratedListItemTemplate = firstChild.outerHTML;
                    this.enumeratedListContainerElement.removeChild(firstChild);
                }
            }
        }
        void this.enumeratedListItemTemplate; // suppress unused variable warning


    }
};


interface CustomSelectControlProps {
    instanceId: number;
    pluginControl: UiControl;
    onValueChanged: (instanceId: number, symbol: string, value: number) => void;
    monitorPort: (
        instanceId: number,
        symbol: string,
        interval: number,
        callback: (value: number) => void) => MonitorPortHandle;
    unmonitorPort: (handle: MonitorPortHandle) => void;
};

class CustomSelectControl implements ModGuiControl {

    private props: CustomSelectControlProps;
    private frameElement?: HTMLElement;

    private value: number = -1.038327E-15
    private animationFrameId: number | null = null;

    constructor(props: CustomSelectControlProps) {
        this.props = props;
    }


    requestUpdate() {
        if (!this.mounted) return;
        if (!this.frameElement) {
            return;
        }


        if (this.animationFrameId === null) {
            this.animationFrameId = window.requestAnimationFrame(() => {
                this.animationFrameId = null;
                this.updateImage();
            });
        }
    }
    cancelRequestUpdate() {
        let animationFrameId = this.animationFrameId;
        if (animationFrameId !== null) {
            window.cancelAnimationFrame(animationFrameId);
            this.animationFrameId = null;
        }
    }
    setControlValue(value: number) {
        if (this.value !== value) {
            this.value = value;
            this.requestUpdate();
        }
    }

    private updateImage() {
        if (!this.frameElement || !this.mounted) {
            return;
        }
        let text: string = "";
        let bestError = 1E308;
        for (let scalePoint of this.props.pluginControl.scale_points) {
            let error = Math.abs(scalePoint.value - this.value);
            if (error < bestError) {
                bestError = error;
                text = scalePoint.label;
            }
        }

        this.frameElement.querySelectorAll('[mod-role=input-control-value]')
            .forEach((inputControl: Element) => {

                (inputControl as HTMLElement).textContent = text;
            });
        this.frameElement.querySelectorAll('[mod-role=enumeration-option]')
            .forEach((inputControl: Element) => {
                let strValue = inputControl.getAttribute("mod-port-value");
                let value = parseFloat(strValue || ""); 
                inputControl.classList.remove("selected");
                if (value === this.value) {
                    inputControl.classList.add("selected");
                }

                (inputControl as HTMLElement).textContent = text;
            });
        //this.frameElement.textContent = text;

    }
    private mounted: boolean = false;

    private monitorHandle: MonitorPortHandle | null = null;
    onMounted() {
        this.mounted = true;
        this.monitorHandle = this.props.monitorPort(
            this.props.instanceId,
            this.props.pluginControl.symbol,
            1.0 / 15,
            (value: number) => {
                if (value != this.value) {
                    this.setControlValue(value);
                }
            }
        );
        this.requestUpdate();
    }

    onUnmount() {
        if (this.monitorHandle) {
            this.props.unmonitorPort(this.monitorHandle);
            this.monitorHandle = null;
        }
        this.mounted = false;
        this.cancelRequestUpdate();
    }


    render(): HTMLElement | null {
        return null;
    }

    toggleVisibility(element: HTMLElement) {
        element.classList.toggle("hidden");
        let display = element.style.display;
        if (display === "none" || display === "") {
            element.style.display = "block";
        } else {
            element.style.display = "none";
        }
    }
    handleClick(event: MouseEvent) {
        if (!this.frameElement) {
            return;
        }
        event.preventDefault();
        event.stopPropagation();
        let enumeratedList = this.frameElement.querySelector(".mod-enumerated-list");
        if (enumeratedList) {
            let element = enumeratedList as HTMLElement;
            this.toggleVisibility(element)
        }
        if (event.target) {
            let target = event.target as HTMLElement;
            let role = target.getAttribute("mod-role");
            if (role === "enumeration-option")  // a click in the drop-down list?
            {
                let strValue = target.getAttribute("mod-port-value");
                if (strValue && strValue !== "")
                {
                    let value = parseFloat(strValue);
                    if (!isNaN(value)) {
                        this.setControlValue(value);
                        this.props.onValueChanged(this.props.instanceId, this.props.pluginControl.symbol, value);
                    }
                }

            }
        }
    }

    attach(frameElement: HTMLElement) {

        this.frameElement = frameElement;
        this.requestUpdate();
        this.frameElement.onclick = (event: MouseEvent) => { this.handleClick(event); }
    }
};


interface BypassLightControlProps {
    instanceId: number;
    pluginControl: UiControl;
    onValueChanged: (instanceId: number, symbol: string, value: number) => void;
    monitorPort: (
        instanceId: number,
        symbol: string,
        interval: number,
        callback: (value: number) => void) => MonitorPortHandle;
    unmonitorPort: (handle: MonitorPortHandle) => void;
};

function ModMessage(message: string) {
    return "Failed to generate UI. (" + message + ")";
}

class BypassLightControl implements ModGuiControl {

    private props: BypassLightControlProps;
    private frameElement?: HTMLElement;

    private value: number = -1.038327E-15
    private animationFrameId: number | null = null;

    constructor(props: BypassLightControlProps) {
        this.props = props;
    }


    requestUpdate() {
        if (!this.mounted) return;
        if (!this.frameElement) {
            return;
        }


        if (this.animationFrameId === null) {
            this.animationFrameId = window.requestAnimationFrame(() => {
                this.animationFrameId = null;
                this.updateImage();
            });
        }
    }
    cancelRequestUpdate() {
        let animationFrameId = this.animationFrameId;
        if (animationFrameId !== null) {
            window.cancelAnimationFrame(animationFrameId);
            this.animationFrameId = null;
        }
    }
    setControlValue(value: number) {
        if (this.value !== value) {
            this.value = value;
            this.requestUpdate();
        }
    }

    private updateImage() {
        if (!this.frameElement || !this.mounted) {
            return;
        }

        this.frameElement.classList.remove('on', 'off');
        this.frameElement.classList.add(
            this.value === this.props.pluginControl.min_value ?
                'on' : 'off'
        );

    }
    private mounted: boolean = false;

    private monitorHandle: MonitorPortHandle | null = null;
    onMounted() {
        this.mounted = true;
        this.monitorHandle = this.props.monitorPort(
            this.props.instanceId,
            this.props.pluginControl.symbol,
            1.0 / 15,
            (value: number) => {
                if (value != this.value) {
                    this.setControlValue(value);
                }
            }
        );
        this.requestUpdate();
    }

    onUnmount() {
        if (this.monitorHandle) {
            this.props.unmonitorPort(this.monitorHandle);
            this.monitorHandle = null;
        }
        this.mounted = false;
        this.cancelRequestUpdate();
    }


    render(): HTMLElement | null {
        return null;
    }
    attach(frameElement: HTMLElement) {

        this.frameElement = frameElement;
        this.requestUpdate();
    }
};


interface FilmstripControlProps {
    instanceId: number;
    pluginControl: UiControl;
    filmStrip: string;
    verticalStrip: boolean;

    onValueChanged: (instanceId: number, symbol: string, value: number) => void;
    monitorPort: (
        instanceId: number,
        symbol: string,
        interval: number,
        callback: (value: number) => void) => MonitorPortHandle;
    unmonitorPort: (handle: MonitorPortHandle) => void;
};

class FilmstripControl implements ModGuiControl {

    private props: FilmstripControlProps;
    private frameElement?: HTMLElement;

    private value: number = -1.038327E-15
    private pointerDownValue: number = -1.038327E-15;
    private isPointerDown: boolean = false;
    private animationFrameId: number | null = null;

    constructor(props: FilmstripControlProps) {
        this.props = props;
    }


    requestUpdate() {
        if (!this.mounted) return;
        if (!this.frameElement) {
            return;
        }


        if (this.animationFrameId === null) {
            this.animationFrameId = window.requestAnimationFrame(() => {
                this.animationFrameId = null;
                this.updateImagePosition();
            });
        }
    }
    cancelRequestUpdate() {
        let animationFrameId = this.animationFrameId;
        if (animationFrameId !== null) {
            window.cancelAnimationFrame(animationFrameId);
            this.animationFrameId = null;
        }
    }
    setControlValue(value: number) {
        if (this.value !== value) {
            this.value = value;
            this.requestUpdate();
        }
    }
    static urlRegex = /url\(["']?([^"')]+)["']?\)/i;
    getImageUrl() {
        if (!this.frameElement) {
            throw new Error("Logic  error: frameElement is not set.");
        }

        let bgImage = getComputedStyle(this.frameElement).backgroundImage;
        let urlMatch = bgImage.match(FilmstripControl.urlRegex);
        if (urlMatch && urlMatch.length > 1) {
            return urlMatch[1];
        }
        return null;
    }

    private currentImageUrl: string = "";

    private updateImagePosition() {
        if (!this.frameElement || !this.mounted) {
            return;
        }

        let rotationAttr = this.frameElement.getAttribute('mod-widget-rotation');
        if (rotationAttr && rotationAttr !== "") {
            let rotation = parseFloat(rotationAttr);
            if (isNaN(rotation)) {
                console.error("pipedal: Invalid mod-widget-rotation value: " + rotationAttr);
                return;
            }
            let range = this.valueToRange(this.value);
            this.frameElement.style.transform = `rotate(${rotation * range-rotation/2}deg)`;
            return;

        }
        let imageUrl = this.getImageUrl();
        if (!imageUrl) {
            return;
        }
        if (imageUrl != this.currentImageUrl) {
            this.filmstripInfo = null;

            this.currentImageUrl = imageUrl;
            this.imgElement = new Image();
            let img = this.imgElement;
            img.onload = () => {
                if (!this.frameElement) {
                    return;
                }
                let computedStyle = window.getComputedStyle(this.frameElement);
                let strWidth = computedStyle.getPropertyValue('width') || "0";
                let frameWidth = parseFloat(strWidth);
                if (isNaN(frameWidth) || frameWidth <= 0) { frameWidth = 0; }

                let strHeight = computedStyle.getPropertyValue("height") || "0";
                let frameHeight = parseFloat(strHeight);
                if (isNaN(frameHeight) || frameHeight <= 0) { frameHeight = 0; }

                let iHeight = 0;
                let bgSize = computedStyle.getPropertyValue('background-size');
                if (bgSize) {
                    let bgSizeParts = bgSize.split(' ');
                    if (bgSizeParts.length === 2) {
                        iHeight = parseFloat(bgSizeParts[1]);
                    }
                }
                if (iHeight === 0) {
                    iHeight = frameHeight;
                }
                let imgFrameWidth = Math.round(frameWidth / iHeight * img.naturalHeight);
                let nFrames = Math.round(img.naturalWidth / imgFrameWidth);

                this.filmstripInfo = {
                    width: img.naturalWidth,
                    height: img.naturalHeight,
                    frameWidth: frameWidth,
                    frameHeight: frameHeight,
                    nFrames: nFrames
                };
                this.requestUpdate();
            };
            img.src = imageUrl;
            return;
        }
        if (!this.filmstripInfo) {
            return; // Not loaded yet
        }


        let xOffset = 0;
        let yOffset = 0;
        let pluginControl = this.props.pluginControl;

        let value = this.isPointerDown ? this.pointerDownValue : this.value;
        value = this.clampValue(value);

        let isHorizontalStrip = 
            this.filmstripInfo.frameWidth/this.filmstripInfo.frameHeight 
            < this.filmstripInfo.width/this.filmstripInfo.height;
        if (isHorizontalStrip) {
            let range = this.valueToRange(value);

            if (range < 0) {
                range = 0;
            }
            if (range > 1) {
                range = 1;
            }
            let nFrame = Math.round(range * (this.filmstripInfo.nFrames - 1));
            xOffset = -nFrame * this.filmstripInfo.frameWidth;
            yOffset = 0;

        } else {
            xOffset = 0;
            if (this.value <= (pluginControl.min_value + pluginControl.max_value) / 2) {
                yOffset = 0; // Off position
            } else {
                yOffset = -this.frameElement.clientHeight; // On position
            }
        }
        this.frameElement.style.backgroundPosition = `${xOffset}px ${yOffset}px`;
    }
    private mounted: boolean = false;

    private monitorHandle: MonitorPortHandle | null = null;
    onMounted() {
        this.mounted = true;
        this.monitorHandle = this.props.monitorPort(
            this.props.instanceId,
            this.props.pluginControl.symbol,
            1.0 / 15,
            (value: number) => {
                if (value != this.value) {
                    this.setControlValue(value);
                }
            }
        );
        this.requestUpdate();
    }

    onUnmount() {
        if (this.monitorHandle) {
            this.props.unmonitorPort(this.monitorHandle);
            this.monitorHandle = null;
        }
        this.mounted = false;
        this.cancelRequestUpdate();
    }

    private filmstripInfo: {
        width: number,
        height: number,
        frameWidth: number,
        frameHeight: number,
        nFrames: number
    } | null = null;

    private pointerIds: number[] = [];
    private firstPointerId: number | null = null;
    lastX: number = 0;
    lastY: number = 0;

    handleToggleClick(event: PointerEvent) {
        event.preventDefault();
        event.stopPropagation();
        this.firstPointerId = null;
        this.isPointerDown = false;
        if (!this.frameElement) {
            return; // Not mounted yet
        }
        if (event.pointerType === "mouse" && event.button !== 0) {
            return; // Only left mouse button
        }
        let newValue: number;
        if (this.value === this.props.pluginControl.min_value) {
            newValue = this.props.pluginControl.max_value;
        } else {
            newValue = this.props.pluginControl.min_value;
        }
        this.setControlValue(newValue);
        this.props.onValueChanged(this.props.instanceId, this.props.pluginControl.symbol, newValue);
    }

    handlePointerDown(event: PointerEvent) {
        if (!this.frameElement) {
            return; // Not mounted yet
        }
        if (event.pointerType === "mouse" && event.button !== 0) {
            return; // Only left mouse button
        }
        if (this.props.pluginControl.controlType === ControlType.BypassLight) {
            return;
        }
        this.frameElement.setPointerCapture(event.pointerId);
        if (this.pointerIds.length === 0) {
            this.firstPointerId = event.pointerId;
            this.lastX = event.pageX;
            this.lastY = event.pageY;
        }
        this.isPointerDown = true;
        this.pointerIds.push(event.pointerId);

        if (this.props.pluginControl.controlType === ControlType.OnOffSwitch ||
            this.props.pluginControl.controlType === ControlType.ABSwitch) {
            this.handleToggleClick(event);
            return;
        }

        event.preventDefault();
        event.stopPropagation();
        this.pointerDownValue = this.value;
    }


    valueToRange(value: number): number {
        let uiControl = this.props.pluginControl;
        if (uiControl) {
            let range: number;
            if (uiControl.is_logarithmic) {
                let minValue = uiControl.min_value;
                if (minValue === 0) // LSP plugins do this.
                {
                    minValue = 0.0001;
                }
                range = Math.log(value / minValue) / Math.log(uiControl.max_value / minValue);
                if (!isFinite(range)) {
                    if (range < 0) {
                        range = 0;
                    } else {
                        range = 1.0;
                    }
                } else if (isNaN(range)) {
                    range = 0;
                }
            } else {
                range = (value - uiControl.min_value) / (uiControl.max_value - uiControl.min_value);
                if (!isFinite(range)) {
                    if (range < 0) {
                        range = 0;
                    } else {
                        range = 1.0;
                    }
                } else if (isNaN(range)) {
                    range = 0;
                }
            }

            if (range > 1) range = 1;
            if (range < 0) range = 0;
            return range;
        }
        return 0;
    }
    rangeToValue(range: number): number {
        if (range < 0) range = 0;
        if (range > 1) range = 1;
        let uiControl = this.props.pluginControl;
        if (uiControl) {
            let value: number;
            if (uiControl.min_value === uiControl.max_value) {
                value = uiControl.min_value;
            } else {
                if (uiControl.is_logarithmic) {
                    let minValue = uiControl.min_value;
                    if (minValue === 0) // LSP controls.    
                    {
                        minValue = 0.0001;
                    }
                    value = minValue * Math.pow(uiControl.max_value / minValue, range);
                    if (!isFinite(value)) {
                        value = uiControl.max_value;
                    } else if (isNaN(value)) {
                        value = uiControl.min_value;
                    }
                } else {
                    value = range * (uiControl.max_value - uiControl.min_value) + uiControl.min_value;
                }
            }
            return value;
        }
        return 0;
    }

    clampValue(value: number): number {
        let control = this.props.pluginControl;
        if (control.enumeration_property) {
            value = control.clampSelectValue(value);
        } else if (control.toggled_property) {
            if (value < (control.min_value + control.max_value) / 2) {
                value = control.min_value;
            } else {
                value = control.max_value;
            }
        } else if (control.integer_property) {
            value = Math.round(value);
        } else if (control.range_steps != 0) {
            let step = (control.max_value - control.min_value) / control.range_steps;
            value = Math.round((value - control.min_value) / step) * step + control.min_value;
        }
        if (value < control.min_value) {
            value = control.min_value;
        }
        if (value > control.max_value) {
            value = control.max_value;
        }
        return value;
    }

    handlePointerMove(event: PointerEvent) {
        if (!this.frameElement) {
            return; // Not mounted yet
        }

        if (this.firstPointerId === null || !this.frameElement.hasPointerCapture(event.pointerId)) {
            return; // Not the first pointer or not captured
        }
        event.preventDefault();
        event.stopPropagation();
        let dy = event.pageY - this.lastY;
        this.lastY = event.pageY;
        this.lastX = event.pageX;

        let rate = RANGE_SCALE; // pixels for full range.
        if (this.pointerIds.length == 2) {
            rate = FINE_RANGE_SCALE;
        } else if (this.pointerIds.length > 2) {
            rate = ULTRA_FINE_RANGE_SCALE;
        } else if (event.ctrlKey) {
            rate = ULTRA_FINE_RANGE_SCALE;
        } else if (event.shiftKey) {
            rate = FINE_RANGE_SCALE;
        }

        let dRange = -dy / rate;

        let range = this.valueToRange(this.pointerDownValue);
        range += dRange;
        if (range < 0) range = 0;
        if (range > 1) range = 1;
        let newValue = this.rangeToValue(range);
        this.pointerDownValue = newValue;
        this.setControlValue(newValue);
        this.props.onValueChanged(this.props.instanceId, this.props.pluginControl.symbol, this.clampValue(newValue));
    }
    handlePointerUp(event: PointerEvent) {
        let index = this.pointerIds.indexOf(event.pointerId);
        if (index >= 0) {
            this.pointerIds.splice(index, 1);
        } else {
            return;
        }
        this.isPointerDown = false;
        this.requestUpdate();
        if (event.pointerId === this.firstPointerId) {
            this.firstPointerId = null;
        }
        if (!this.frameElement) {
            return; // Not mounted yet
        }

        this.frameElement.releasePointerCapture(event.pointerId);
        event.preventDefault();
        event.stopPropagation();
    }
    handlePointerCancel(event: PointerEvent) {
        if (!this.frameElement) {
            return; // Not mounted yet
        }
        let index = this.pointerIds.indexOf(event.pointerId);
        if (index >= 0) {
            this.pointerIds.splice(index, 1);
        }
        if (event.pointerId === this.firstPointerId) {
            this.firstPointerId = null;
        }
        if (this.pointerIds.length === 0) {
            this.firstPointerId = null;
            this.lastY = 0;
            this.isPointerDown = false;
            this.requestUpdate();

        }
    }
    handleMouseWheel(event: WheelEvent) {
        if (!this.frameElement) {
            return; // Not mounted yet
        }
        event.preventDefault();
        event.stopPropagation();
        let rate = WHEEL_RANGE_SCALE; // pixels for full range.
        if (event.ctrlKey) {
            rate = ULTRA_FINE_WHEEL_RANGE_SCALE;
        } else if (event.shiftKey) {
            rate = FINE_WHEEL_RANGE_SCALE;
        }
        let dRange = event.deltaY / rate;

        let range = this.valueToRange(this.value);
        range += dRange;
        if (range < 0) range = 0;
        if (range > 1) range = 1;
        let newValue = this.rangeToValue(range);
        this.setControlValue(newValue);
        this.props.onValueChanged(this.props.instanceId, this.props.pluginControl.symbol, newValue);
    }


    private imgElement: HTMLImageElement | null = null;
    render(): HTMLElement | null {
        return null;
    }
    attach(frameElement: HTMLElement) {

        this.frameElement = frameElement;
        this.frameElement.onpointerdown = (event: PointerEvent) => {
            this.handlePointerDown(event);
        };
        this.frameElement.onpointerup = (event: PointerEvent) => {
            if (!this.frameElement) {
                return; // Not mounted yet
            }
            this.handlePointerUp(event);
        };
        this.frameElement.onpointermove = (event: PointerEvent) => {

            if (!this.frameElement) {
                return; // Not mounted yet
            }
            if (this.frameElement.hasPointerCapture(event.pointerId)) {
                this.handlePointerMove(event);
                return;
            }
        }
        this.frameElement.onpointercancel = (event: PointerEvent) => {
            this.handlePointerCancel(event);
        }
        this.frameElement.onwheel = (event: WheelEvent) => {
            this.handleMouseWheel(event);
        };
        this.requestUpdate();
    }
};

function ModGuiHost(props: ModGuiHostProps) {
    let xmodel: PiPedalModel = PiPedalModelFactory.getInstance();
    const { plugin, onClose } = props;
    const [hostDivRef, setHostDivRef] = React.useState<HTMLDivElement | null>(null);
    const [errorMessage, setErrorMessage] = React.useState<string | null>(null);
    const [modGuiControls] = React.useState<ModGuiControl[]>([]);
    const [ready, setReady] = React.useState<boolean>(xmodel.state.get() === State.Ready);


    if (!plugin.modGui) {
        return (
            <div>
                <Typography variant="h6">No Mod GUI</Typography>
            </div>
        );
    }

    function addPortClass(element: Element, selector: string, className: string) {
        let children = element.querySelectorAll(selector);
        // call addClass to each element that matches the selector
        children.forEach((child) => {
            child.classList.add(className);
        });
    }

    function createCustomSelectControl(control: Element, symbol: string, pluginControl: UiControl) {
        let customSelectControl = new CustomSelectControl(
            {
                instanceId: props.instanceId,
                pluginControl: pluginControl,
                onValueChanged: (instanceId: number, symbol: string, value: number) => {
                    try {
                        props.hostSite.setPedalboardControl(instanceId, symbol, value);
                    } catch (error) {

                    }
                },
                monitorPort: props.hostSite.monitorPort,
                unmonitorPort: props.hostSite.unmonitorPort
            }
        );
        customSelectControl.attach(control as HTMLElement);
        modGuiControls.push(customSelectControl);
        let modGuiElement = customSelectControl.render();
        if (modGuiElement !== null) {
            control.appendChild(modGuiElement);
        }
        customSelectControl.onMounted();
    }
    function createFootswitchControl(control: Element, symbol: string, controlType: ControlType) {
        let uiControl = new UiControl();
        uiControl.symbol = symbol;
        uiControl.controlType = controlType;
        uiControl.min_value = 0;
        uiControl.max_value = 1;
        uiControl.is_logarithmic = false;
        uiControl.integer_property = true;

        let filmstripControl = new FilmstripControl(
            {
                instanceId: props.instanceId,
                pluginControl: uiControl,
                filmStrip: "/img/footswitch_strip.png",
                verticalStrip: true,
                onValueChanged: (instanceId: number, symbol: string, value: number) => {
                    try {
                        props.hostSite.setPedalboardControl(instanceId, symbol, value);
                    } catch (error) {

                    }
                },
                monitorPort: props.hostSite.monitorPort,
                unmonitorPort: props.hostSite.unmonitorPort
            }
        );
        filmstripControl.attach(control as HTMLElement);
        modGuiControls.push(filmstripControl);
        let modGuiElement = filmstripControl.render();
        if (modGuiElement !== null) {
            control.appendChild(modGuiElement);
        }
        filmstripControl.onMounted();
    }
    function createCustomSelectPathControl(control: Element) {
        let pathUri = control.getAttribute("mod-parameter-uri");
        if (!pathUri) {
            setModError("No mod-parameter-uri attribute found on control element.");
            return;
        }
        let selectPathControl = new CustomSelectPathControl({
            instanceId: props.instanceId,
            propertyUri: pathUri, 
            plugin: props.plugin,
            hostSite: props.hostSite
        });

        selectPathControl.attach(control as HTMLElement);
        modGuiControls.push(selectPathControl);
        selectPathControl.onMounted();
    }

    
    function createBypassLightControl(control: Element, symbol: string) {
        let uiControl = new UiControl();
        uiControl.symbol = symbol;
        uiControl.controlType = ControlType.BypassLight;
        uiControl.min_value = 0;
        uiControl.max_value = 1;
        uiControl.is_logarithmic = false;
        uiControl.integer_property = true;

        let bypassLightControl = new BypassLightControl(
            {
                instanceId: props.instanceId,
                pluginControl: uiControl,
                onValueChanged: (instanceId: number, symbol: string, value: number) => {
                    try {
                        props.hostSite.setPedalboardControl(instanceId, symbol, value);
                    } catch (error) {

                    }
                },
                monitorPort: props.hostSite.monitorPort,
                unmonitorPort: props.hostSite.unmonitorPort
            }
        );
        bypassLightControl.attach(control as HTMLElement);
        modGuiControls.push(bypassLightControl);
        let modGuiElement = bypassLightControl.render();
        if (modGuiElement !== null) {
            control.appendChild(modGuiElement);
        }
        bypassLightControl.onMounted();
    }
    function createDialControl(control: Element, pluginControl: UiControl) {
        let modGui = plugin.modGui;
        if (!modGui) {
            alert("Logic error");
            return;
        }
        let knobUrl: string = "/img/BlackKnob.png";
        if (modGui.knob !== "") {
            knobUrl = modGui.knob;
        }
        let modGuiControl = new FilmstripControl(
            {
                instanceId: props.instanceId,
                pluginControl: pluginControl,
                filmStrip: knobUrl,
                verticalStrip: false,
                onValueChanged: (instanceId: number, symbol: string, value: number) => {
                    try {
                        props.hostSite.setPedalboardControl(instanceId, symbol, value);
                    } catch (error) {

                    }
                },
                monitorPort: props.hostSite.monitorPort,
                unmonitorPort: props.hostSite.unmonitorPort

            }
        );
        modGuiControls.push(modGuiControl);
        modGuiControl.attach(control as HTMLElement);

        let modGuiElement = modGuiControl.render();
        if (modGuiElement !== null) {
            control.appendChild(modGuiElement);
        }
        modGuiControl.onMounted();

    }
    function prepareElement(element: Element) {
        addPortClass(element, '[mod-role="input-audio-port"]', "mod-audio-input");
        addPortClass(element, '[mod-role="output-audio-port"]', "mod-audio-output");
        // addPortClass(element,'[mod-role="input-midi-port"]', "mod-midi-input");
        // addPortClass(element,'[mod-role="output-midi-port"]', "mod-midi-output");
        // addPortClass(element,'[mod-role="input-cv-port"]', "mod-cv-input");
        // addPortClass(element,'[mod-role="output-cv-port"]', "mod-cv-output");


        element.querySelectorAll('[mod-role=input-control-port]')
            .forEach((control) => {
                let symbol = control.getAttribute("mod-port-symbol");

                if (symbol) {
                    let pluginControl: UiControl | undefined = plugin.getControl(symbol);
                    if (!pluginControl) {
                        setModError(`No plugin info found for symbol ${symbol}`);
                        return;
                    }
                    let widgetType = control.getAttribute("mod-widget") || "";
                    // 'film': 'film',
                    // 'switch': 'switchWidget',
                    // 'bypass': 'bypassWidget',
                    // 'select': 'selectWidget',
                    // 'string': 'stringWidget',
                    // 'custom-select': 'customSelect',
                    // 'custom-select-path': 'customSelectPath',

                    switch (widgetType) {
                        case "":
                        case "film":
                            createDialControl(control, pluginControl);
                            break;
                        case "switch":
                            createFootswitchControl(control, symbol, ControlType.OnOffSwitch);
                            break;
                        case "custom-select":
                            createCustomSelectControl(control, symbol, pluginControl);
                            break;
                        case "custom-select-path":
                        case "bypass":
                        case "string":
                            setModError("Unsupported widget type: " + widgetType);
                            break;
                    }
                } else {
                    setModError("No mod-symbol attribute found on control element.");
                }
            });
        element.querySelectorAll('[mod-role=bypass]')
            .forEach((control) => {
                let symbol = "_bypass";
                let controlType = ControlType.OnOffSwitch;

                createFootswitchControl(control, symbol, controlType);
            });
        element.querySelectorAll('[mod-role=bypass-light]')
            .forEach((control) => {
                let symbol = "_bypass";

                createBypassLightControl(control, symbol);
            });
        element.querySelectorAll('[mod-role=input-parameter]')
            .forEach((control) => {
                // stub: mod-widget="custom-select-path".  Are there other widgets for this?
                createCustomSelectPathControl(control);
            });
    }
    function setModError(message: string) {
        setErrorMessage(ModMessage(message));
    }
    async function requestContent() {
        try {
            let modGui = plugin.modGui;
            if (!modGui) {
                setModError("No MOD GUI declared for this plugin.");
                return;
            }
            if (!modGui.iconTemplate) {
                setModError("Template file missing.");
                return;
            }
            if (!modGui.stylesheet) {
                setModError("Stylesheet file missing.");
                return;
            }

            let encodedUri = encodeURIComponent(props.plugin.uri);
            let version = props.plugin.minorVersion * 1000 + props.plugin.microVersion;

            let resourceUrl = xmodel.modResourcesUrl;
            let queryParams = "?ns=" + encodedUri + "&v=" + version.toString();
            let templateUri = resourceUrl + "_/iconTemplate" + queryParams;
            let cssUri = resourceUrl + "_/stylesheet" + queryParams;

            let fetchResult = await fetch(templateUri);
            if (!fetchResult.ok) {
                setModError("Failed to load template: " + fetchResult.statusText);
                return;
            }
            let parser = new DOMParser();
            let docText = await fetchResult.text();
            let doc = parser.parseFromString(docText, "text/html");
            let element = doc.body.firstElementChild;
            if (!element) {
                setModError("No root element found in template.");
                return;
            }
            let cssResult = await fetch(cssUri);
            if (!cssResult.ok) {
                setModError("Failed to load stylesheet: " + cssResult.statusText);
                return;
            }
            if (hostDivRef === null) {
                return; // cancelled.
            }

            let t = await cssResult.text();
            let cssText = "<style id='mod-gui-style'>" + t + "</style>";
            let cssDoc = parser.parseFromString(cssText, "text/html");
            let cssElement = cssDoc.getElementById('mod-gui-style');
            if (!cssElement) {
                setModError("No style element found.");
                return;
            }
            if (hostDivRef === null) {
                return; // cancelled.
            }
            prepareElement(element);
            // add element to the hostDivRef
            hostDivRef.appendChild(cssElement);
            hostDivRef.appendChild(element);
            let width = element.clientWidth;
            let height = element.clientHeight;
            hostDivRef.style.width = width + "px";
            hostDivRef.style.height = height + "px";
        } catch (error) {
            setModError("Error loading UI: " + (error instanceof Error ? error.message : String(error)));
        }

    }

    React.useEffect(() => {
        let tHostDivRef = hostDivRef;
        if (ready) {
            if (hostDivRef !== null) {
                requestContent();
            }
        }
        let stateHandler = (state: State) => {
            setReady(state === State.Ready);
        }
        xmodel.state.addOnChangedHandler(stateHandler)
        return () => {
            xmodel.state.removeOnChangedHandler(stateHandler);
            if (tHostDivRef !== null) {
                // unmount the custom content.
                let children = tHostDivRef.children;
                for (let i = children.length - 1; i >= 0; i--) {
                    let child = children[i];
                    if (child instanceof HTMLElement) {
                        child.remove();
                    }
                }
            }
            let mc = modGuiControls;
            for (let i = 0; i < mc.length; i++) {
                let modGuiControl = mc[i];
                modGuiControl.onUnmount();
            }
            mc.length = 0; // Clear the controls array
        };
    }, [hostDivRef, plugin, ready]);

    return (
        <ModGuiErrorBoundary plugin={props.plugin} onClose={() => { props.onClose(); setErrorMessage(null); }}>
            <div style={{
                position: "absolute", top: 0, left: 0, right: 0, bottom: 0,
                backgroundColor: "rgba(0.8,0.8,0.8, 0.8)",
                display: "flex", flexFlow: "column nowrap", alignItems: "center", justifyContent: "stretch",
                overflow: "hidden",
            }}
                onClick={(event) => {
                    // Prevent click events from propagating to the parent element.
                    event.stopPropagation();
                }}
            >
                <IconButtonEx tooltip="Close" onClick={() => onClose()} style={{
                    position: "absolute", top: 16, right: 16,
                    background: "#FFF5", zIndex: 600
                }}>
                    <CloseIcon />
                </IconButtonEx>

                <div style={{ flex: "1 1 1px" }} />

                <div style={{ display: "block", position: "relative" }}>
                    <div ref={setHostDivRef} style={{ display: "block", position: "relative" }}
                        onClick={(event) => {
                            // Prevent click events from propagating to the parent element.
                            event.stopPropagation();
                        }
                        }
                    />
                </div>

                <div style={{ flex: "2 2 1px" }} />

            </div>
            <OkDialog title="Error" open={errorMessage !== null}
                onClose={() => {
                    setErrorMessage(null);
                    onClose();
                }}
                text={errorMessage ?? ""}
            />
        </ModGuiErrorBoundary>
    );
}
export default ModGuiHost;