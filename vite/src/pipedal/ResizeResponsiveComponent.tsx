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

import React from 'react';


class ResizeResponsiveComponent<PROPS={},STATE={},S=any> extends React.Component<PROPS,STATE,S> {
    constructor(props: PROPS) {
        super(props);
        this.handleWindowResize = this.handleWindowResize.bind(this);
        this.windowSize.width = document.documentElement.clientWidth;
        this.windowSize.height = document.documentElement.clientHeight;
    }

    windowSize: {width: number,height:number} = {width: 1280, height: 1024};

    onWindowSizeChanged(width: number, height: number): void {

    }
    handleWindowResize() {
        let width_ = document.documentElement.clientWidth;
        let height_ = document.documentElement.clientHeight;
        if (width_ !== this.windowSize.width || height_ !== this.windowSize.height)
        {
            this.windowSize = {width: width_, height: height_};
            this.onWindowSizeChanged(this.windowSize.width,this.windowSize.height);


        } 
    }


    componentDidMount() {
        window.addEventListener('resize', this.handleWindowResize);

        let width_ = document.documentElement.clientWidth;
        let height_ = document.documentElement.clientHeight;
        this.windowSize = {width: width_, height: height_};
        this.onWindowSizeChanged(this.windowSize.width,this.windowSize.height);
    }
    componentWillUnmount() {
        window.removeEventListener('resize', this.handleWindowResize);
    }

}

export default ResizeResponsiveComponent;