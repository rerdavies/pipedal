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
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

import React from 'react';
import Divider from '@mui/material/Divider';
import { PiPedalModel, PiPedalModelFactory, MonitorPortHandle, State, ListenHandle, PatchPropertyListener } from './PiPedalModel';
import { useTheme } from '@mui/material/styles';
import CircularProgress from '@mui/material/CircularProgress';
import LoadPluginDialog from './LoadPluginDialog';
import ButtonEx from './ButtonEx';
import Typography from '@mui/material/Typography/Typography';
import ModGuiHost, { IModGuiHostSite } from './ModGuiHost';
import { UiPlugin } from './Lv2Plugin';
import ModGuiErrorBoundary from './ModGuiErrorBoundary';

class MyPortHandle implements MonitorPortHandle {
    private static nextHandle: number = 1;
    constructor() {
        this.handle = MyPortHandle.nextHandle++;
    }
    handle: number;
};

type MonitorCallback = (value: number) => void;

class ValueEntry {
    constructor(defaultValue: number) {
        this.value = defaultValue;
    }
    private listeners: { [handle: number]: MonitorCallback } = {};

    monitor(interval: number, callback: MonitorCallback): MyPortHandle {
        let handle = new MyPortHandle();
        this.listeners[handle.handle] = callback;
        return handle;
    }
    unmonitor(handle: MonitorPortHandle) {
        let h = (handle as MyPortHandle).handle;
        // remove entry h from listeners
        if (!(h in this.listeners)) {
            throw new Error("Invalid handle: " + h);
        }
        // remove the callback from the listeners
        delete this.listeners[h];
    }
    setValue(value: number) {
        if (value !== this.value) {
            this.value = value;
            // notify all listeners
            for (let handle in this.listeners) {
                this.listeners[handle](value);
            }
        }
    }
    value: number;
};

class PatchListenerEntry  {
    constructor(instanceId: number, propertyUri: string) {
        this.instanceId = instanceId;
        this.propertyUri = propertyUri;
    }   
    instanceId: number;
    propertyUri: string;
    value: any = "";
    listeners: { handle: number, listener: PatchPropertyListener}[] = [];;
};


class ValueHandler {

    private valueDictionary: { [symbol: string]: ValueEntry } = {};
    private handleToValueEnry: { [handle: number]: ValueEntry } = {};

    constructor(instanceId: number, plugin: UiPlugin) {
        for (let control of plugin.controls) {
            this.valueDictionary[control.symbol] = new ValueEntry(control.default_value);
        }
        this.valueDictionary["_bypass"] = new ValueEntry(0);
    }
    monitorPort(instanceId: number, symbol: string, interval: number, callback: MonitorCallback): MonitorPortHandle {
        let valueEntry = this.valueDictionary[symbol];

        let portHandle = valueEntry.monitor(interval, callback);
        this.handleToValueEnry[(portHandle as MyPortHandle).handle] = valueEntry;
        callback(valueEntry.value);
        return portHandle;
    }
    unmonitorPort(handle: MonitorPortHandle): void {
        let h = (handle as MyPortHandle).handle;
        if (!(h in this.handleToValueEnry)) {
            throw new Error("Invalid handle: " + h);
        }
        let valueEntry = this.handleToValueEnry[h];
        valueEntry.unmonitor(handle);
        delete this.handleToValueEnry[h];
    }
    setValue(instanceId: number, symbol: string, value: number) {
        // add a delay just for funsies.
        window.setTimeout(() => {
            if (!(symbol in this.valueDictionary)) {
                throw new Error("Invalid symbol: " + symbol);
            }
            let valueEntry = this.valueDictionary[symbol];
            valueEntry.setValue(value);
        }, 100);
    }
    getValue(instanceId: number, symbol: string): number {
        if (!(symbol in this.valueDictionary)) {
            throw new Error("Invalid symbol: " + symbol);
        }
        let valueEntry = this.valueDictionary[symbol];
        return valueEntry.value;
    }
    private nextListenHandle: number = 1;
    

    private patchListeners: PatchListenerEntry[] = [];

    monitorPatchProperty(instanceId: number, propertyUri: string, onReceived: PatchPropertyListener): ListenHandle {
        let h = this.nextListenHandle++;
        for (let entry of this.patchListeners) {
            if (entry.instanceId === instanceId && entry.propertyUri === propertyUri) {
                // already listening to this property
                entry.listeners.push({ handle: h, listener: onReceived });
                onReceived(instanceId,propertyUri,entry.value);
                return { _handle: h };
            }
        }
        let entry = new PatchListenerEntry(instanceId, propertyUri);
        this.patchListeners.push(entry);
        entry.listeners.push({ handle: h, listener: onReceived });
        onReceived(instanceId, propertyUri, entry.value);
        return {_handle: h};
    }
    cancelMonitorPatchProperty(listenHandle: ListenHandle): void {
        for (let i = 0; i < this.patchListeners.length; ++i) {
            let entry = this.patchListeners[i];
            for (let j = 0; j < entry.listeners.length; ++j) {
                if (entry.listeners[j].handle === listenHandle._handle) {
                    // found the listener, remove it
                    entry.listeners.splice(j, 1);
                    if (entry.listeners.length === 0) {
                        // no more listeners, remove the entry
                        this.patchListeners.splice(i, 1);
                    }
                    return;
                }
            }
        }
    }
    getPatchProperty(instanceId: number, uri: string): Promise<any> {
        let promise = new Promise<any>((resolve, reject) => {
            window.setTimeout(() => {
                for (let entry of this.patchListeners) {
                    if (entry.instanceId === instanceId && entry.propertyUri === uri) {
                        window.setTimeout(() => {
                            // simulate a delay for getting the property
                            entry.listeners.forEach(l => l.listener(instanceId, uri, entry.value));
                        }, 100);
                        return resolve(entry.value);
                    }
                }
                reject(new Error(`Patch property not found: ${uri} for instance ${instanceId}`));
            },50);
        });
        return promise;
    }
    setPatchProperty(instanceId: number, uri: string, value: any): Promise<boolean> {
        // Implementation here
        for (let entry of this.patchListeners) {
            if (entry.instanceId === instanceId && entry.propertyUri === uri) {
                entry.value = value;
                // notify all listeners
                entry.listeners.forEach(l => l.listener(instanceId, uri, value));
                return Promise.resolve(true);
            }
        }
        // if we reach here, the property was not found
        return Promise.reject("Not found.");
    }
};

function ModGuiTest() {
    const [pluginUrl, setPluginUrl] = React.useState("");
    const [uiPlugin, setUiPlugin] = React.useState<UiPlugin | null>(null);
    const [pluginDialogOpen, setPluginDialogOpen] = React.useState(false);
    const [loading, setLoading] = React.useState(true);
    const [valueHandler, setValueHandler] = React.useState<ValueHandler | null>(null);

    const [modGuiHostSite] = React.useState<IModGuiHostSite>({
        monitorPort: (instanceId: number, symbol: string, interval: number, callback: (value: number) => void) => {
            if (!valueHandler) {
                throw new Error("ValueHandler is not set");
            }
            return valueHandler.monitorPort(instanceId, symbol, interval, callback);
        },
        unmonitorPort: (handle) => {
            if (!valueHandler) {
                throw new Error("ValueHandler is not set");
            }
            valueHandler.unmonitorPort(handle);
        },
        setPedalboardControl: (instanceId, symbol, value) => {
            if (!valueHandler) {
                throw new Error("ValueHandler is not set");
            }
            valueHandler.setValue(instanceId, symbol, value);
        },
        monitorPatchProperty(instanceId: number, propertyUri: string, onReceived: PatchPropertyListener): ListenHandle {
            if (!valueHandler) {
                throw new Error("ValueHandler is not set");
            }

            return valueHandler.monitorPatchProperty(instanceId, propertyUri, onReceived);
        },
        cancelMonitorPatchProperty: (listenHandle) => {
            if (!valueHandler) {
                throw new Error("ValueHandler is not set");
            }
            valueHandler.cancelMonitorPatchProperty(listenHandle);
        },
        getPatchProperty: async (instanceId, uri) => {
            if (!valueHandler) {
                throw new Error("ValueHandler is not set");
            }
            return valueHandler.getPatchProperty(instanceId, uri) as Promise<any>;
        },
        setPatchProperty: async (instanceId, uri, value) => {
            if (!valueHandler) {
                throw new Error("ValueHandler is not set");
            }
            return valueHandler.setPatchProperty(instanceId, uri, value) as Promise<boolean>;
        }
    });

    let model: PiPedalModel = PiPedalModelFactory.getInstance();

    React.useEffect(() => {

        const onStateChange = (state: State) => {
            if (state == State.Ready) {
                setLoading(false);
            }
        };

        model.state.addOnChangedHandler(onStateChange);

        return () => {
            model.state.removeOnChangedHandler(onStateChange);
        };
    }, [])

    const theme = useTheme();

    return (
        <div style={{
            display: "flex", flexFlow: "column nowrap", alignItems: "stretch", justifyContent: "stretch",
            position: "absolute", top: 0, left: 0, width: "100%", height: "100%", backgroundColor: (theme.palette as any).mainBackground
        }}>

            <div style={{
                position: "absolute", top: 0, left: 0, right: 0,
                bottom: 0, zIndex: 1000,
                display: loading ? "flex" : "none",
                alignItems: "center", justifyContent: "center",
                background: "rgba(0.5,0.5,0.5, 0.9)"
            }}>
                <CircularProgress style={{ color: theme.palette.text.secondary }} />
            </div>


            <div style={{
                flex: "0 0 auto", display: "flex", flexFlow: "row nowrap",
                alignItems: "center", gap: 16, paddingLeft: 32, paddingRight: 32, paddingTop: 16, paddingBottom: 16,
            }}>
                <Typography noWrap variant="h6" style={{ flex: "1 0 auto" }}>ModGUI Test</Typography>
                <Typography noWrap variant="body2" style={{ flex: "0 1 auto", }}>
                    {pluginUrl ? pluginUrl : "No plugin loaded"}
                </Typography>
                <ButtonEx style={{ flex: "0 0 auto" }} variant="contained" tooltip="Select plugin" onClick={() => setPluginDialogOpen(true)} >
                    Load
                </ButtonEx>
            </div>

            <Divider />

            {uiPlugin && (
                <div style={{ flex: "1 1 auto", position: "relative" }}>
                    <ModGuiErrorBoundary plugin={uiPlugin} onClose={() => {
                        setPluginUrl("");
                        setUiPlugin(null);
                        setValueHandler(null);
                    }}  >

                        <ModGuiHost instanceId={-1} plugin={uiPlugin}
                            onClose={() => {
                                setPluginUrl("");
                                setUiPlugin(null);
                            }}
                            hostSite={modGuiHostSite}
                        />
                    </ModGuiErrorBoundary>
                </div>
            )}
            {pluginDialogOpen &&
                <LoadPluginDialog
                    open={pluginDialogOpen}
                    onOk={(url) => {
                        setPluginUrl(url as string);
                        let uiPlugin = model.getUiPlugin(url);
                        setUiPlugin(uiPlugin);
                        if (uiPlugin) {
                            setValueHandler(new ValueHandler(-1, uiPlugin));
                        }
                        setPluginDialogOpen(false);
                    }}
                    onCancel={() => setPluginDialogOpen(false)}
                    uri={pluginUrl}
                    modGuiOnly={true}
                />
            }
        </div>
    );
}

export default ModGuiTest;