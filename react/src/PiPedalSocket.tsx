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

import { PiPedalStateError } from './PiPedalError';
import Utility from './Utility';

export type MessageHandler = (header: PiPedalMessageHeader, body: any | null) => void;
export type ErrorHandler = (message: string, exception?: Error) => void;
export type ReconnectHandler = () => void;
export type ReconnectingHandler = (retry: number, maxRetries: number) => void;

const MAX_RETRIES = 6;
const MAX_RETRY_TIME = 90*1000;

export type PiPedalMessageHeader = {
    replyTo?: number;
    reply?: number;
    message: string;
}
type ReplyHandler = (header: PiPedalMessageHeader, body: any | null) => void;


export interface PiPedalSocketListener {
    onMessageReceived : (header: PiPedalMessageHeader, body: any | null) => void;
    onError: (message: string, exception?: Error) => void;
    onConnectionLost: () => void;
    onReconnect:  () => void;
    onReconnecting: (retry: number, maxRetries: number) => boolean;
};

class PiPedalSocket {

    listener: PiPedalSocketListener;


    socket?: WebSocket;
    nextResponseCode: number = 0;
    url: string;
    retrying: boolean = false;
    retryCount: number = 0;
    retryDelay: number = 0;
    totalRetryDelay: number = 0;

    constructor(
        url: string,
        listener: PiPedalSocketListener
        ) {
        this.url = url;
        this.listener = listener;
    }

    handleOpen(event: Event): any {

    }

    sendInternal_(json: string) {
        if (!this.retrying) {
            this.socket?.send(json);
        }
    }
    send(message: string, jsonObject?: any) {
        let msg: any;
        if (jsonObject === undefined) {
            msg = [{ message: message }];
        } else {
            msg = [{ message: message }, jsonObject];
        }
        let json = JSON.stringify(msg);
        this.sendInternal_(json);
    }
    reply(replyTo: number, message: string, jsonObject?: any) {
        if (replyTo !== -1) {
            let msg: any;
            if (jsonObject === undefined) {
                msg = [{ reply: replyTo, message: message }];
            } else {
                msg = [{ reply: replyTo, message: message }, jsonObject];
            }
            let json = JSON.stringify(msg);
            this.sendInternal_(json);
        }
    }

    _nextResponseCode(): number {
        return ++this.nextResponseCode;
    }

    _replyMap: Map<number, ReplyHandler> = new Map<number, ReplyHandler>();

    _discardReplyReservations()
    {
        // it's ok. All pending reservations disappear into the GC.
        this._replyMap = new Map<number, ReplyHandler>();
    }


    _addReservation(replyCode: number, handler: ReplyHandler): void {
        this._replyMap.set(replyCode, handler);
    }

    request<Type = any>(message_: string, requestArgs?: any): Promise<Type> {
        let responseCode = this._nextResponseCode();

        return new Promise<Type>((resolve, reject) => {
            try {
                this._addReservation(responseCode, (header: PiPedalMessageHeader, jsonObject?: any) => {
                    if (header.message === "error") {
                        reject(jsonObject + "");
                    } else {
                        resolve(jsonObject as Type);
                    }
                });
                let msg: any;
                if (requestArgs !== undefined) {
                    msg = [{ message: message_, replyTo: responseCode }, requestArgs];
                } else {
                    msg = [{ message: message_, replyTo: responseCode }];
                }
                let jsonMessage = JSON.stringify(msg);
                this.sendInternal_(jsonMessage);
            } catch (err) {
                reject(err);
            }
        });
    }
    handleMessage(event: MessageEvent<string>): any {
        try {
            let message: any = JSON.parse(event.data);
            if (!Array.isArray(message)) {
                throw new PiPedalStateError("Invalid message received from server.");
            }
            let header = message[0] as PiPedalMessageHeader;
            let body = undefined;
            if (message.length === 2) {
                body = message[1];
            }
            if (header.reply !== undefined) {
                let handler = this._replyMap.get(header.reply);
                if (handler) {
                    handler(header, body);
                }
                return;
            } else {
                if (header.message === "error") {
                    throw new PiPedalStateError("Server error: " + body);
                }
                this.listener.onMessageReceived(header, body);
            }
        } catch (error) {
            if (this.listener) {
                this.listener.onError("Invalid server response. " + error, error as Error);
            } else {
                throw new PiPedalStateError("Invalid server response.");
            }
        }
    }

    handleError(_event: Event): any {
        if (this.listener) {
            this.listener.onError("Server connection lost.");
        } else {
            throw new PiPedalStateError("Server connection lost.");
        }
    }

    canReconnect: boolean = false;

    handleClose(_event: any): any {
        if (this.retrying) {
            this.close();
            // treat this as a fatal error.
            if (this.listener) {
                this.listener.onError("Server connection lost.");
            } else {
                throw new PiPedalStateError("Server connection closed.");
            }
            return;

        }
        if (this.canReconnect)
        {
            this.listener.onConnectionLost();
            this._reconnect();
        }
    }
    _reconnect() {
        this._discardReplyReservations();
        this.retrying = true;
        this.retryCount = 0;
        this.socket = undefined;
        this.retryDelay = 10000;
        this.totalRetryDelay = 0;

        this.reconnect();
    }

    isBackground: boolean = false;

    enterBackgroundState()
    {
        this.isBackground = true;
        this.close();

    }
    exitBackgroundState()
    {
        this.isBackground = false;
        this._reconnect();
    }

    reconnect() {

        if (this.socket)
        {
            this.close();
        }

        if (!this.listener.onReconnecting(this.retryCount,MAX_RETRIES)) {
            return;
        }
        ++this.retryCount;

        this.connectInternal_()
            .then((socket) => {
                this.socket = socket;
                this.retrying = false;
                this.listener.onReconnect();
            })
            .catch(error => {
                if (this.totalRetryDelay === MAX_RETRY_TIME)
                {
                    this.listener.onError("Server connection lost.");
                    return;
                } else {
                    this.totalRetryDelay += this.retryDelay;
                    Utility.delay(this.retryDelay).then(() => this.reconnect());
                    this.retryDelay *= 2;
                    if (this.retryDelay > 3000) this.retryDelay = 3000;
                }
            });
    }

    close(): void {
        try {
            if (this.socket)
            {
                this.socket.onclose = null;
                this.socket.onerror = null;
                this.socket.onmessage = null;
                this.socket.onopen = null;
                this.socket.close();
                this.socket = undefined;
            }
        } catch  (ignored)
        {

        }
        this.socket = undefined;
    }
    connectInternal_(): Promise<WebSocket> {
        return new Promise<WebSocket>((resolve, reject) => {
            let ws = new WebSocket(this.url);

            let self = this;

            ws.onmessage = this.handleMessage.bind(this);
            ws.onclose = (event: Event) => {
                ws.onclose = null;
                ws.onerror = null;
                reject("Connection closed unexpectedly.");
            };
            ws.onerror = (event: Event) => {
                ws.onclose = null;
                ws.onerror = null;
                reject("Failed to connect.");
            };
            ws.onopen = (event: Event) => {
                ws.onerror = self.handleError.bind(self);
                ws.onclose = self.handleClose.bind(self);
                ws.onopen = null;
                resolve(ws);
            };
        });
    }
    connect(): Promise<void> {
        return new Promise<void>((resolve, reject) => {

            this.connectInternal_()
                .then((socket) => {
                    this.socket = socket;
                    resolve();
                })
                .catch((reason) => {
                    reject(reason);
                });
        });
    }

}

export default PiPedalSocket;
