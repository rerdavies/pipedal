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

function popDialogStack(): void {
    if (dialogStack.length !== 0)
    {
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

    handlePopState(e: any): any 
    {
        let shouldClose = (peekDialogStack() === this.props.tag);
        if (shouldClose)
        {

            if (!this.stateWasPopped)
            {
                this.stateWasPopped = true;
                popDialogStack();
                if (this.props.open) {
                    this.props.onClose?.(e,"backdropClick");
                }
            }
            window.removeEventListener("popstate",this.handlePopState);
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
                
                pushDialogStack(this.props.tag);
                let state: {tag: string} = {tag: this.props.tag};
                // eslint-disable-next-line no-restricted-globals
                history.pushState(
                    state,
                    "",
                    "#" + this.props.tag
                );
            } else {
                if (!this.stateWasPopped)
                {
                    // eslint-disable-next-line no-restricted-globals
                    history.back();
                }
            }
        }
    }

    componentDidMount()
    {
        super.componentDidMount?.();

        this.mounted = true;
        this.updateHooks();
    }
    componentWillUnmount()
    {
        super.componentWillUnmount?.();
        this.mounted = false;
        this.updateHooks();
    }

    componentDidUpdate()
    {
        this.updateHooks();
    }
    myOnClose(event:{}, reason: "backdropClick" | "escapeKeyDown")
    {
        if (this.props.onClose) this.props.onClose(event,reason);
    }

    render() {
        let { tag,onClose, ...extra} = this.props;
        return (
            <Dialog {...extra} onClose={(event,reason)=>{ this.myOnClose(event,reason);}}>
                {this.props.children}
            </Dialog>
        );
    }

};

export default DialogEx;