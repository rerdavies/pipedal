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
import { IDialogStackable, pushDialogStack, popDialogStack } from './DialogStack';
import Dialog, { DialogProps } from '@mui/material/Dialog';


interface DialogExState {
}


interface DialogExProps extends DialogProps {
    tag: string;
    fullwidth?: boolean;
    onEnterKey: () => void;
}

class DialogEx extends React.Component<DialogExProps, DialogExState> implements IDialogStackable {
    constructor(props: DialogExProps) {
        super(props);
        this.state = {

        };
    }

    mounted: boolean = false;

    hasHooks: boolean = false;
    stateWasPopped: boolean = false;

    getTag() {
        return this.props.tag;
    }
    isOpen(): boolean {
        return this.props.open;
    }
    onDialogStackClose() {
        this.props.onClose?.({}, "backdropClick");
    }
    updateHooks(): void {
        let wantHooks = this.mounted && this.props.open;
        if (wantHooks !== this.hasHooks) {
            this.hasHooks = wantHooks;

            if (this.hasHooks) {
                this.stateWasPopped = false;
                pushDialogStack(this);
            } else {
                popDialogStack(this);
            }
        }
    }

    componentDidMount() {
        super.componentDidMount?.();

        this.mounted = true;
        this.updateHooks();
    }
    componentWillUnmount() {
        super.componentWillUnmount?.();
        this.mounted = false;
        this.updateHooks();
    }

    componentDidUpdate() {
        this.updateHooks();
    }
    myOnClose(event: {}, reason: "backdropClick" | "escapeKeyDown") {
        if (this.props.onClose) this.props.onClose(event, reason);
    }

    handleEnterKey() {
        if (this.props.onEnterKey) {
            this.props.onEnterKey();
        }
    }
    onKeyDown(evt: React.KeyboardEvent<HTMLDivElement>) {
        if (evt.key === 'Enter') {
            this.handleEnterKey();
        }
        evt.stopPropagation();
    }
    render() {
        let { tag, onClose, fullWidth, onEnterKey, ...extra } = this.props;
        return (
            <Dialog fullWidth={fullWidth ?? false}
                maxWidth={fullWidth ? false : undefined} {...extra}
                onClose={(event, reason) => { this.myOnClose(event, reason); }}
                onKeyDown={(evt) => { this.onKeyDown(evt); }}
            >
                {this.props.children}
            </Dialog>
        );
    }

};

export default DialogEx;