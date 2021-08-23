// Copyright (c) 2021 Robin Davies
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

import Dialog, {DialogProps} from '@material-ui/core/Dialog';


interface DialogExProps extends DialogProps {
    tag: string;
}

interface DialogExState {

}

class DialogEx extends React.Component<DialogExProps,DialogExState>  {
    constructor(props: DialogExProps)
    {
        super(props);
        this.state = {

        };
        this.handlePopState = this.handlePopState.bind(this);

    }

    mounted: boolean = false;

    hasHooks: boolean = false;

    stateWasPopped: boolean = false;
    handlePopState(e: any): any {
        this.stateWasPopped = true;
        let shouldClose = (!e.state || !e.state[this.props.tag]);
        if (shouldClose)
        {
            if (this.props.onClose)
            {
                this.props.onClose(e,"backdropClick");
            }
        }
    }

    updateHooks() : void {
        let wantHooks = this.mounted && this.props.open;
        if (wantHooks !== this.hasHooks)
        {
            this.hasHooks = wantHooks;

            if (this.hasHooks)
            {
                this.stateWasPopped = false;
                window.addEventListener("popstate",this.handlePopState);
                // eslint-disable-next-line no-restricted-globals
                let state = history.state;
                if (!state)
                {
                    state = {};
                }
                state[this.props.tag] = true;

                // eslint-disable-next-line no-restricted-globals
                history.pushState(
                    state,
                    "",
                    "#" + this.props.tag
                );
            } else {
                window.removeEventListener("popstate",this.handlePopState);
                if (!this.stateWasPopped)
                {
                    // eslint-disable-next-line no-restricted-globals
                    history.back();
                }
            }

        }
    }
    onWindowSizeChanged(width: number, height: number): void 
    {
        this.setState({fullScreen: height < 200})
    }


    componentDidMount()
    {
        if (super.componentDidMount)
        {
            super.componentDidMount();
        }
        this.mounted = true;
        this.updateHooks();
    }
    componentWillUnmount()
    {
        if (super.componentWillUnmount)
        {
            super.componentWillUnmount();
        }
        this.mounted = false;
        this.updateHooks();
    }

    componentDidUpdate()
    {
        this.updateHooks();
    }


    render() {
        let { tag, ...extra} = this.props;
        return (
            <Dialog {...extra}>
                {this.props.children}
            </Dialog>
        );
    }

};

export default DialogEx;