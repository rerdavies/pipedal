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
    fullwidth?: boolean;
    onEnterKey: () => void;
}

interface DialogExState {
}

export interface IDialogStackable {
    getTag: ()=> string;
    isOpen: ()=> boolean;
    onDialogStackClose(): void;
}

class DialogStackEntry {
    constructor(dialog: IDialogStackable)
    {
        this.tag = dialog.getTag();
        this.dialog = dialog;
    }
    tag: string;
    dialog: IDialogStackable;
};

let dialogStack: DialogStackEntry[] = [];


export function popDialogStack(): void {
    if (dialogStack.length !== 0)
    {
        dialogStack.splice(dialogStack.length-1,1);
    }
}
export function pushDialogStack(dialog: IDialogStackable): void {
    dialogStack.push(new DialogStackEntry(dialog));
}

export interface DialogStackState{
    tag: String;
    previousState: DialogStackState | null;
};


// Close all dialogs higher in the dialog stack than "tag".
export function removeDialogStackEntriesAbove(tag: string)
{
    for (let i = dialogStack.length-1; i >= 0; --i)
    {
        let entry = dialogStack[i];
        if (entry.dialog.getTag() === tag)
        {
            return;
        }
        popDialogStack();
        if (entry.dialog.isOpen()) 
        {
            entry.dialog.onDialogStackClose();
        }

    }
}
class DialogEx extends React.Component<DialogExProps,DialogExState> implements IDialogStackable  {
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

    getTag() {
        return this.props.tag;
    }
    isOpen():  boolean {
        return this.props.open;
    }
    onDialogStackClose() {
        this.props.onClose?.({},"backdropClick");
    }

    handlePopState(ev: PopStateEvent): any 
    {
        let evTag: DialogStackState | null = ev.state as DialogStackState | null;
        if (evTag && evTag.tag === this.props.tag) {
            removeDialogStackEntriesAbove(this.props.tag);

            if (!this.stateWasPopped)
            {
                this.stateWasPopped = true;
                popDialogStack();
                if (this.props.open) {
                    this.props.onClose?.(ev,"backdropClick");
                }
            }
            window.history.replaceState(evTag.previousState,"",window.location.href);
            window.removeEventListener("popstate",this.handlePopState);

            if (window.location.hash === "#menu") // The menu popstate is next?
            {
                window.history.back(); // then clear off the menu popstate too.
            }
            ev.stopPropagation();
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
                
                pushDialogStack(this);
                let dialogStackState: DialogStackState = {tag: this.props.tag,previousState: window.history.state as DialogStackState | null};
                window.history.replaceState(dialogStackState,"",window.location.href);
                window.history.pushState(
                    null,
                    "",
                    "#" + this.props.tag
                );
            } else {
                if (!this.stateWasPopped)
                {
                    window.history.back();
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

    onEnterKey() {
        if (this.props.onEnterKey)
        {
            this.props.onEnterKey();
        }
    }
    onKeyDown(evt: React.KeyboardEvent<HTMLDivElement>)
    {
        if (evt.key === 'Enter')
        {
            this.onEnterKey();
        }
    }
    render() {
        let { tag,onClose,...extra} = this.props;
        return (
            <Dialog fullWidth={this.props.fullWidth??false} 
                maxWidth={this.props.fullWidth ? false: undefined} {...extra} 
                onClose={(event,reason)=>{ this.myOnClose(event,reason);}}
                onKeyDown={(evt)=>{ this.onKeyDown(evt); }}
                >
                {this.props.children} 
            </Dialog>
        );
    }

};

export default DialogEx;