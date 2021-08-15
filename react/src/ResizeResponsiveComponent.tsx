import React from 'react';


class ResizeResponsiveComponent<PROPS={},STATE={},S=any> extends React.Component<PROPS,STATE,S> {
    constructor(props: PROPS) {
        super(props);
        this.handleWindowResize = this.handleWindowResize.bind(this);
        this.windowSize.width = window.innerWidth;
        this.windowSize.height = window.innerHeight;
    }

    windowSize: {width: number,height:number} = {width: 1280, height: 1024};

    onWindowSizeChanged(width: number, height: number): void {

    }
    handleWindowResize() {
        let width_ = window.innerWidth;
        let height_ = window.innerHeight;
        if (width_ !== this.windowSize.width || height_ !== this.windowSize.height)
        {
            this.windowSize = {width: width_, height: height_};
            this.onWindowSizeChanged(this.windowSize.width,this.windowSize.height);


        } 
    }


    componentDidMount() {
        window.addEventListener('resize', this.handleWindowResize);

        let width_ = window.innerWidth;
        let height_ = window.innerHeight;
        this.windowSize = {width: width_, height: height_};
        this.onWindowSizeChanged(this.windowSize.width,this.windowSize.height);
    }
    componentWillUnmount() {
        window.removeEventListener('resize', this.handleWindowResize);
    }

}

export default ResizeResponsiveComponent;