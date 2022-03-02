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

import Dialog, {DialogProps} from '@mui/material/Dialog';


interface DialogExProps extends DialogProps {
    tag: string;
}

interface DialogExState {

}

class DialogStackEntry {
    constructor(tag: string)
    {
        this.tag = tag;
    }
    tag: string;
};

let dialogStack: DialogStackEntry[] = [];

function peekDialogStack(): string {
    if (dialogStack.length !== 0) {
        return dialogStack[dialogStack.length-1].tag;
    }
    return "";
}

let ignoreNextPop = false;

function popDialogStack(ignoreNextPop_: boolean): void {
    if (dialogStack.length !== 0)
    {
        ignoreNextPop = ignoreNextPop_;
        dialogStack.splice(dialogStack.length-1,1);
    }
}
function pushDialogStack(tag: string): void {
    dialogStack.push(new DialogStackEntry(tag));
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
        let shouldClose = (peekDialogStack() === this.props.tag);
        if (shouldClose)
        {
            if (ignoreNextPop)
            {
                ignoreNextPop = false;
            } else {
                if (!this.stateWasPopped)
                {
                    this.stateWasPopped = true;

                    if (this.props.onClose)
                    {
                        popDialogStack(false);
                        this.props.onClose(e,"backdropClick");
                    }
                }
            }
            e.stopPropagation();
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

                pushDialogStack(this.props.tag);
                // eslint-disable-next-line no-restricted-globals
                history.pushState(
                    state,
                    "",
                    "" //"#" + this.props.tag
                );
            } else {
                if (!this.stateWasPopped)
                {
                    this.stateWasPopped = true;
                    popDialogStack(true);
                    // eslint-disable-next-line no-restricted-globals
                    history.back();
                }
                window.removeEventListener("popstate",this.handlePopState);
            }
        }
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