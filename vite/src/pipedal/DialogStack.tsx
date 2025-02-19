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


export interface IDialogStackable {
    getTag: () => string;
    isOpen: () => boolean;
    onDialogStackClose(): void;
}

interface DialogStackEntry {
    dialog: IDialogStackable | null;
    historyState: DialogStackHistoryState
};

let dialogStack: DialogStackEntry[] = [];


const DLG_STATE_MAGIC = "DlgState438327"
export interface DialogStackHistoryState {
    magic: string;
    topmost: boolean;
    stackDepth: number;
    tag: string;
    id: string;
    previousState: DialogStackHistoryState | null;
};

export function pushDialogStack(dialog: IDialogStackable): void {
    let stackId = Math.random().toString(36).substring(7);
    let dialogStackState: DialogStackHistoryState = {
        magic: DLG_STATE_MAGIC,
        topmost: false,
        stackDepth: dialogStack.length + 1,
        tag: dialog.getTag(),
        id: stackId, previousState:
            window.history.state as DialogStackHistoryState | null
    };
    window.history.replaceState(
        dialogStackState,
        "",
        "#" + dialog.getTag());

    // refresh reloads the page.
    let topState: DialogStackHistoryState = {
        topmost: true,
        magic: DLG_STATE_MAGIC,
        previousState: dialogStackState,
        tag: dialog.getTag(),
        id: stackId,
        stackDepth: dialogStack.length + 1
    };

    dialogStack.push(
        {
            dialog: dialog,
            historyState: dialogStackState
        });

    console.log("pushDialogStack " + stackId + " #" + dialog.getTag());
    window.history.pushState(
        topState,
        "",
        "#" + dialog.getTag());

    if (window.history.state.id != stackId)
    {
        console.log("pushDialogStack: nav deferred.");
    }
    if (!gWatchingHistory) {
        gWatchingHistory = true;
        window.addEventListener("popstate", handlePopState);
    }
}

let gWatchingHistory = false;
let gPopFromBack = true;

let handlePopState = (ev: PopStateEvent) => {
    let evTag: DialogStackHistoryState | null = ev.state as DialogStackHistoryState | null;
    if (!evTag) return;
    if (evTag.magic !== DLG_STATE_MAGIC) return;

    ev.stopPropagation();

    console.log("handlePopState: " + evTag?.tag + " " + evTag?.id + " " + gPopFromBack);
    let id = evTag.id;
    let ix = dialogStack.length-1;
    while (ix >= 0) {
        let entry = dialogStack[ix];
        if (entry.historyState.id === id) {
            break;
        }
        ix--;
    }
    if (ix < 0) {
        // user must have navigated directly.
        // so just reload the page.
        dialogStack = [];
        window.location.reload();
        ev.stopPropagation();
        return;
    }

    while (dialogStack.length > ix) {
        let topEntry = dialogStack[dialogStack.length - 1];
        if (topEntry.dialog) {
            try {

                if (topEntry.dialog.isOpen()) {
                    topEntry.dialog.onDialogStackClose();
                }
            } catch (_e) {

            }
            topEntry.dialog = null;
        }
        dialogStack.pop();
    }
    let previousState  = evTag.previousState;
    if (previousState) 
    {
        let newState = {...previousState, topmost: true};
        window.history.replaceState(newState,"","#"+previousState.tag);
    } else {
        window.history.replaceState(null,"",window.location.pathname);
    }

    if (dialogStack.length != 0 && gPopFromBack) {
        if (dialogStack[dialogStack.length -1].dialog === null)
        {
            if (window.history.state != null)
            {
                console.log("handlePopState: nav back");
                window.setTimeout(() => {   window.history.back(); }, 0);
                console.log("handlePopState: id= "+ (window.history?.state?.id??"null"));
            } else {
                dialogStack.pop();
            }
        }
    }
    ev.stopPropagation();
}


export function popDialogStack(dialog: IDialogStackable): void {
    let ix = dialogStack.length-1;

    while (ix >= 0) {
        let entry = dialogStack[ix];
        if (entry.dialog === dialog) {
            console.log("popDialogStack " + entry.historyState.id + " #" + dialog.getTag());
            break;
        }
        ix--;
    }
    if (ix < 0) 
    {
        return;
    }
    dialogStack[ix].dialog = null;
    if (dialogStack.length != 0 && dialogStack[dialogStack.length - 1].dialog === null)
    {
        console.log("popDialogStack nav back: " + dialogStack[ix].historyState.id );
        window.history.back();
    }
}









// Close all dialogs higher in the dialog stack than "tag".
export function removeDialogStackEntriesAbove(tag: string) {
    while (dialogStack.length > 0) {
        let entry = dialogStack[dialogStack.length - 1];
        if (entry.dialog && entry.dialog.getTag() === tag) {
            return;
        }

        dialogStack.pop();

        if (entry.dialog && entry.dialog.isOpen()) {
            entry.dialog.onDialogStackClose();
        }
    }
}

