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

import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";
import {createStyles} from './WithStyles';

import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';

const styles = (theme: Theme) => createStyles({
    pgraph: {
        paddingBottom: 16
    }
});


export interface ScratchClassProps extends WithStyles<typeof styles> {
}

export interface ScratchClassState {
    screenWidth: number, 
    screenHeight: number
}



const ScratchClass = withStyles(
    class extends ResizeResponsiveComponent<ScratchClassProps, ScratchClassState> {
        private model: PiPedalModel;
        constructor(props: ScratchClassProps) {
            super(props);
            
            this.model = PiPedalModelFactory.getInstance();
            void this.model;  // suppress unused variable warning

            this.state = {
                screenWidth: this.windowSize.width,
                screenHeight: this.windowSize.height
            };
        }
        mounted: boolean = false;



        onWindowSizeChanged(width: number, height: number): void {
            this.setState({ screenWidth: width, screenHeight: height });
        }


        componentDidMount() {
            super.componentDidMount();
            this.mounted = true;
        }
        componentWillUnmount() {
            this.mounted = false;
            super.componentWillUnmount();
        }

        
        componentDidUpdate(prevProps: ScratchClassProps) {
        }



        render() {
            const classes = withStyles.getClasses(this.props);
            void classes; // suppress unused variable warning
            return (<div/>);
        }
    },
    styles);

export default ScratchClass;