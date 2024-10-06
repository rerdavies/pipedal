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



// converts android handling of the virtual keyboard to use pan instead of zoom.

import { isAndroidHosted } from './AndroidHost';
import Rectangle from './Rectangle';


export default class VirtualKeyboardHandler
{

    enabled: boolean = false;
    originalPosition: Rectangle


    constructor() 
    {

        let enabled = this.isChromeMobile();

        if ("virtualKeyboard" in navigator) {
            this.virtualKeyboard = navigator["virtualKeyboard"];
            if (this.virtualKeyboard) 
            {
                alert("Has Virtual keyboard!");
                this.virtualKeyboard.addEventListener("geometryChange",this.handleGeometryChange);
            }
        }   

        if ('visualViewport' in window && isAndroidHosted()) {
            window.visualViewport?.addEventListener('resize', this.handleVisualViewportResize.bind(this));
        } else {
            enabled = false;
        }
        this.enabled = enabled;
        this.originalPosition = new Rectangle(0,0, window.innerWidth,window.innerHeight)
    }
    private handleVisualViewportResize(event: Event): void {
        const viewport = event.target as VisualViewport;
        if (viewport.width !== this.originalPosition.width)
        {
            // a flip, not a virtual keyboard event.
            this.originalPosition = new Rectangle(0,0,viewport.width,viewport.height);
            this.resetContentPosition();
        }
        const heightDifference = this.originalPosition.height - viewport.height;
        if (Math.abs(heightDifference) < 150)
        {
            // fullscren !fullscreen
            this.originalPosition = new Rectangle(0,0,viewport.width,viewport.height);
            this.resetContentPosition();

        }

        let viewportRect = new Rectangle(viewport.offsetLeft,viewport.offsetTop,viewport.width,viewport.height);
        if (heightDifference > 0) {
          // Keyboard is likely visible
          this.panContent(viewportRect);
        } else {
          // Keyboard is likely hidden
          this.resetContentPosition();
        }
    }

    panContent(viewportRect: Rectangle)
    {
        let focusObject = this.getFocusedInputElement();
        if (focusObject) {
            let clientRect_ = focusObject.getBoundingClientRect();
            let clientRect = new Rectangle(clientRect_.left,clientRect_.top,clientRect_.width,clientRect_.height);

            // center the client in the available height.
            let yOffset = viewportRect.top + viewportRect.height/2 - (clientRect.top + clientRect.height/2);

            if (yOffset < 0) {
                yOffset = 0;
            }
            let transform = `translateY(-${yOffset}px)`;
            document.body.style.transform = transform;
            document.body.style.height = `${this.originalPosition.height}px`;
            document.body.style.overflow = 'hidden';            
        }
    }
    resetContentPosition() {
        document.body.style.transform = "";
        document.body.style.height = "";
        document.body.style.overflow = "hidden";
    }

    getFocusedInputElement(): HTMLInputElement | null {
        const element = document.activeElement;
        return element instanceof HTMLInputElement ? element : null;
    }


    isChromeMobile(): boolean {
        const userAgent = navigator.userAgent.toLowerCase();
        const vendor = navigator.vendor.toLowerCase();
    
        const isAndroid = /android/.test(userAgent);
        const isChrome = /chrome/.test(userAgent) && /google inc/.test(vendor);
        const isMobile = /mobile/.test(userAgent);
    
        // Additional check for Chrome's navigator.userAgentData (if available)
        const userAgentData = (navigator as any).userAgentData;
        const isMobileDevice = userAgentData?.mobile ?? false;
    
        // Check if it's Chrome
        if (isChrome) {
            // Check if it's on Android or explicitly marked as mobile
            if (isAndroid || isMobile || isMobileDevice) {
                return true;
            }
        }
    
        return false;
    }

    private handleGeometryChange(event: any) {
    }

    private virtualKeyboard: any;
    componentDidMount(): void {
    }
    componentWillUnmount(): void {
        if (this.virtualKeyboard)
        {
            this.virtualKeyboard.removeEventListener("geometryChange",this.handleGeometryChange);
        }
    }
}

