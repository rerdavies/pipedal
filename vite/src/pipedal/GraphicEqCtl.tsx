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

import SvgPathBuilder from "./SvgPathBuilder";

export interface GraphicEqCtlProps {
    position: number;
    imgRef: React.Ref<SVGSVGElement> | undefined;
    onTouchStart: React.TouchEventHandler<SVGSVGElement> | undefined;
    onTouchMove: React.TouchEventHandler<SVGSVGElement> | undefined;
    onPointerDown: React.PointerEventHandler<SVGSVGElement> | undefined;
    onPointerUp: React.PointerEventHandler<SVGSVGElement> | undefined;
    onPointerMoveCapture: React.PointerEventHandler<SVGSVGElement> | undefined;
    onDrag: React.DragEventHandler<SVGSVGElement> | undefined;
    dialColor: string;
    opacity: number;
};

const STROKE_HEIGHT=8.0;

function MakeGraphicEqPath(value: number)
{

    let x0 = 6;
    let xN = 64-x0;
    let xC0 = (64-STROKE_HEIGHT)/2;
    let xC1 = xC0+STROKE_HEIGHT;

    let h = 64-STROKE_HEIGHT*2.5;

    let y0 = 0;
    let y1 = h-h*value;;
    let y2 = y1 + STROKE_HEIGHT;
    let y3 = y2 + STROKE_HEIGHT/2;
    let y4 = y3 + STROKE_HEIGHT;
    let y5 = 64;


    let s = new SvgPathBuilder();
    s.moveTo(x0,y1)
        .lineTo(xC0,y1)
        .lineTo(xC0,y0)
        .lineTo(xC1,y0)
        .lineTo(xC1,y1)
        .lineTo(xN,y1)
        .lineTo(xN,y2)
        .lineTo(x0,y2)
        .closePath()
        
        .moveTo(x0,y3)
        .lineTo(x0,y4)
        .lineTo(xC0,y4)
        .lineTo(xC0,y5)
        .lineTo(xC1,y5)
        .lineTo(xC1,y4)
        .lineTo(xN,y4)
        .lineTo(xN,y3)
        .lineTo(x0,y3)
        .closePath();
    return s.toString();

}
export function UpdateGraphicEqPath(el: SVGSVGElement, value: number) {
    if (!el) return;
    let p =  MakeGraphicEqPath(value);
    let pathEl = el.getElementById("myPath");
    pathEl?.setAttribute("d",p);
}

export default function GraphicEqCtl(props: GraphicEqCtlProps) {
    return (
        <svg width="64px" height="64px"
            ref={props.imgRef}
            viewBox="0 0 64 64"
            style={{
                overscrollBehavior: "none",
                touchAction: "none",
                width: 36, height: 42, opacity: props.opacity
            }}
            onTouchStart={props.onTouchStart}
            onTouchMove={props.onTouchMove}
            onPointerDown={props.onPointerDown}
            onPointerUp={props.onPointerUp}
            onPointerMoveCapture={props.onPointerMoveCapture}
            onDrag={props.onDrag}
            fill={props.dialColor}
        >
            <path id="myPath" d={MakeGraphicEqPath(props.position)} />
            </svg>
    );
}