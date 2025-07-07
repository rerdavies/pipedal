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

import React from 'react';
import Divider from '@mui/material/Divider';

function TypeSample(props: { fontFamily: string, cssRef: string, fontWeights: number[] }) {
    let { fontFamily, cssRef, fontWeights } = props;
    React.useEffect(() => {
        let link = document.createElement('link');
        link.rel = 'stylesheet';
        link.href = cssRef;
        document.head.appendChild(link);
        return () => {
            document.head.removeChild(link);
        };
    }, []);

    return (
        <div>
            {fontWeights.map((weight) => (
                <div key={`${fontFamily}-${weight}`} style={{ marginBottom: 24 }}>
                    <Divider />
                    <div style={{ marginLeft: 24, marginRight: 24 }}>
                        <h4 style={{marginBottom: 0}}>{fontFamily} {weight}</h4>
                        <div style={{ fontFamily: fontFamily, fontSize: 20, fontWeight: weight, marginLeft: 24 }}>
                            <div style={{ fontFamily: fontFamily }}>The quick brown fox jumped over the lazy dog.</div>
                        </div>
                    </div>
                </div>
            ))}
        </div>
    );
}

function FontTest() {
    return (
        <div>
            <h4 style={{ marginLeft: 24, marginRight: 24 }}>Font Test</h4>
            {TypeSample({ fontFamily: "EnglandHand", cssRef: "/fonts/england-hand/stylesheet.css", fontWeights: [400] })}
            {TypeSample({ fontFamily: "epflul", cssRef: "/fonts/epf/stylesheet.css", fontWeights: [400] })}
            {TypeSample({ fontFamily: "Nexa", cssRef: "/fonts/nexa/stylesheet.css", fontWeights: [400] })}
            {TypeSample({ fontFamily: "Pirulen", cssRef: "/fonts/pirulen/stylesheet.css", fontWeights: [400] })}
            {TypeSample({ fontFamily: "Questrial", cssRef: "/fonts/questrial/stylesheet.css", fontWeights: [400] })}
            
            {TypeSample({ fontFamily: "Pirulen", cssRef: "/fonts/pirulen/stylesheet.css", fontWeights: [400] })}
            {TypeSample({ fontFamily: "Pirulen", cssRef: "/fonts/pirulen/stylesheet.css", fontWeights: [400] })}
            {TypeSample({ fontFamily: "Pirulen", cssRef: "/fonts/pirulen/stylesheet.css", fontWeights: [400] })}


    </div>
    );
}

export default FontTest;