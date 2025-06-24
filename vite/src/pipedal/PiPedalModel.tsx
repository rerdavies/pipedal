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

import { UiPlugin, UiControl, PluginType, UiFileProperty } from './Lv2Plugin';

import { PiPedalArgumentError, PiPedalStateError } from './PiPedalError';
import { UpdateStatus, UpdatePolicyT } from './Updater';
import ObservableEvent from './ObservableEvent';
import { ObservableProperty } from './ObservableProperty';
import { Pedalboard, PedalboardItem, ControlValue, Snapshot } from './Pedalboard'
import PluginClass from './PluginClass';
import PiPedalSocket, { PiPedalMessageHeader } from './PiPedalSocket';
import { nullCast } from './Utility'
import { JackConfiguration, JackChannelSelection } from './Jack';
import { BankIndex } from './Banks';
import JackHostStatus from './JackHostStatus';
import JackServerSettings from './JackServerSettings';
import MidiBinding from './MidiBinding';
import { PluginUiPresets } from './PluginPreset';
import WifiConfigSettings from './WifiConfigSettings';
import WifiDirectConfigSettings from './WifiDirectConfigSettings';
import GovernorSettings from './GovernorSettings';
import WifiChannel from './WifiChannel';
import AlsaDeviceInfo from './AlsaDeviceInfo';
import { AndroidHostInterface, FakeAndroidHost } from './AndroidHost';
import { ColorTheme, getColorScheme, setColorScheme } from './DarkMode';
import FilePropertyDirectoryTree from './FilePropertyDirectoryTree';
import AudioFileMetadata from './AudioFileMetadata';
import { pathFileName } from './FileUtils';


export enum State {
    Loading,
    Ready,
    Error,
    Background,
    Reconnecting,
    ApplyingChanges,
    ReloadingPlugins,
    DownloadingUpdate,
    InstallingUpdate,
    HotspotChanging,
};

function getErrorMessage(error: any) {
    if (error instanceof Error) {
        return (error as Error).message;
    }
    if (!error) {
        return "";
    }
    return error.toString();
}
class UpdatedError extends Error {
};
export function wantsReloadingScreen(state: State) {
    return state >= State.Reconnecting;
}

export enum ReconnectReason {
    Disconnected,
    LoadingSettings,
    ReloadingPlugins,
    Updating,
    HotspotChanging
};

export type ControlValueChangedHandler = (key: string, value: number) => void;

export interface FileEntry {
    pathname: string;
    displayName: string;
    isDirectory: boolean;
    isProtected: boolean;
    // optional metadata for audio
    metadata?: AudioFileMetadata;
};

export interface BreadcrumbEntry {
    pathname: string;
    displayName: string;
};
export class FileRequestResult {
    files: FileEntry[] = [];
    isProtected: boolean = false;
    breadcrumbs: BreadcrumbEntry[] = [];
    currentDirectory: string = "";
};

export type PluginPresetsChangedHandler = (pluginUri: string) => void;

export interface PluginPresetsChangedHandle {
    _id: number;
    _handler: PluginPresetsChangedHandler;

};

export type StateChangedHandler = (instanceId: number) => void;

interface StateChangedEntry {
    handle: number;
    instanceId: number;
    onStateChanged: StateChangedHandler;
};

export interface StateChangedHandle {
    _handle: number;
}



export interface ZoomedControlInfo {
    source: HTMLElement;
    name: string;
    instanceId: number;
    uiControl: UiControl;
}

export interface VuUpdateInfo {
    instanceId: number;

    sampleTime: number; // in samples.
    isStereoInput: boolean;
    isStereoOutput: boolean;
    inputMaxValueL: number;
    inputMaxValueR: number;
    outputMaxValueL: number;
    outputMaxValueR: number;
};

export interface MonitorPortHandle {

};
export interface ControlValueChangedHandle {
    _ControlValueChangedHandle: number;

};
interface ControlValueChangeItem {
    handle: number;
    instanceId: number;
    onValueChanged: ControlValueChangedHandler;

};


class MidiEventListener {
    constructor(handle: number, callback: (isNote: boolean, noteOrControl: number) => void) {
        this.handle = handle;
        this.callback = callback;
    }
    handle: number;
    callback: (isNote: boolean, noteOrControl: number) => void;
};

export type PatchPropertyListener = (instanceId: number, propertyUri: string, atomObject: any) => void;

class PatchPropertyListenerItem {
    constructor(handle: number, instanceId: number, callback: PatchPropertyListener) {
        this.handle = handle;
        this.instanceId = instanceId;
        this.callback = callback;
    }
    handle: number;
    instanceId: number;
    callback: PatchPropertyListener;
};



export interface ListenHandle {
    _handle: number;
}
class MonitorPortHandleImpl implements MonitorPortHandle {
    constructor(instanceId: number, key: string, onUpdated: (value: number) => void) {
        this.instanceId = instanceId;
        this.key = key;
        this.onUpdated = onUpdated;
    }
    valid: boolean = true;
    subscriptionHandle?: number;

    instanceId: number;
    key: string;
    onUpdated: (value: number) => void

};

export interface VuSubscriptionHandle {

};

class VuSubscriptionHandleImpl implements VuSubscriptionHandle {
    constructor(instanceId_: number, callback_: VuChangedHandler) {
        this.instanceId = instanceId_;
        this.callback = callback_;
    }
    instanceId: number;
    callback: VuChangedHandler;
};

export type VuChangedHandler = (vuInfo: VuUpdateInfo) => void;


class VuSubscriptionTarget {
    serverSubscriptionHandle?: number;
    subscribers: VuSubscriptionHandleImpl[] = [];
};

export type PiPedalVersion = {
    server: string;
    serverVersion: string;
    operatingSystem: string;
    osVersion: string;
    debug: boolean;
    webAddresses: string[];
};

export class PresetIndexEntry {
    deserialize(input: any): PresetIndexEntry {
        this.instanceId = input.instanceId;
        this.name = input.name;
        return this;
    }
    static deserialize_array(input: any): PresetIndexEntry[] {
        let result: PresetIndexEntry[] = [];
        for (let i = 0; i < input.length; ++i) {
            result[i] = new PresetIndexEntry().deserialize(input[i]);
        }
        return result;
    }
    instanceId: number = 0;
    name: string = "";

    areEqual(other: PresetIndexEntry): boolean {
        return (
            (this.instanceId === other.instanceId)
            && this.name === other.name
        );
    }
};

export class PresetIndex {
    deserialize(input: any): PresetIndex {
        this.selectedInstanceId = input.selectedInstanceId;
        this.presetChanged = input.presetChanged;
        this.presets = PresetIndexEntry.deserialize_array(input.presets);
        return this;
    }
    selectedInstanceId: number = 0;
    presetChanged: boolean = false;
    presets: PresetIndexEntry[] = [];

    clone(): PresetIndex {
        return new PresetIndex().deserialize(this);
    }
    areEqual(other: PresetIndex, includeSelectedInstance: boolean): boolean {
        if (includeSelectedInstance && this.selectedInstanceId !== other.selectedInstanceId) return false;
        if (this.presets.length !== other.presets.length) return false;
        for (let i = 0; i < this.presets.length; ++i) {
            if (!this.presets[i].areEqual(other.presets[i])) return false;
        }
        return true;
    }
    getItem(instanceId: number): PresetIndexEntry | null {
        for (let i = 0; i < this.presets.length; ++i) {
            if (this.presets[i].instanceId === instanceId) {
                return this.presets[i];
            }
        }
        return null;
    }
    getSelectedText(): string {
        let item = this.getItem(this.selectedInstanceId);
        if (item === null) return "";

        if (this.presetChanged) {
            return item.name + " *";
        } else {
            return item.name;
        }
    }
    addItem(instanceId: number, name: string): number {
        let newItem: PresetIndexEntry = new PresetIndexEntry();
        newItem.instanceId = instanceId;
        newItem.name = name;
        this.presets.push(newItem);
        return newItem.instanceId;
    }
    movePreset(from: number, to: number) {
        let newElements = this.presets;
        let t = newElements[from];
        newElements.splice(from, 1);
        newElements.splice(to, 0, t);

    }
    deleteItem(instanceId: number): number {
        for (let i = 0; i < this.presets.length; ++i) {
            let preset = this.presets[i];
            if (preset.instanceId === instanceId) {
                this.presets.splice(i, 1);
                if (i < this.presets.length) {
                    return this.presets[i].instanceId;
                } else {
                    if (i - 1 > 0) {
                        return this.presets[i - 1].instanceId;
                    } else {
                        return -1;
                    }
                }
            }
        }
        return -1;

    }


}

enum VisibilityState {
    Visible,
    Hidden
};
interface MonitorPortOutputBody {
    subscriptionHandle: number;
    value: number;
}


interface ChannelSelectionChangedBody {
    clientId: number;
    jackChannelSelection: JackChannelSelection;
}
interface RenamePresetBody {
    clientId: number;
    instanceId: number;
    name: string;
}

interface PresetsChangedBody {
    clientId: number,
    presets: PresetIndex
}
interface PedalboardItemEnableBody {
    clientId: number,
    instanceId: number,
    enabled: boolean
}
interface PedalboardChangedBody {
    clientId: number;
    pedalboard: Pedalboard;
}
interface ControlChangedBody {
    clientId: number;
    instanceId: number;
    symbol: string;
    value: number;
};
interface Vst3ControlChangedBody {
    clientId: number;
    instanceId: number;
    symbol: string;
    value: number;
    state: string;
};

export interface FavoritesList {
    [url: string]: boolean;
};

export interface SnapshotModifiedEvent {
    snapshotIndex: number;
    modified: boolean;
};


export class PiPedalModel //implements PiPedalModel 
{
    clientId: number = -1;

    serverVersion?: PiPedalVersion;
    countryCodes: { [Name: string]: string } = {};

    socketServerUrl: string = "";
    varServerUrl: string = "";
    lv2Path: string = "";
    webSocket?: PiPedalSocket;


    hasWifiDevice: ObservableProperty<boolean> = new ObservableProperty<boolean>(false);
    onSnapshotModified: ObservableEvent<SnapshotModifiedEvent> = new ObservableEvent<SnapshotModifiedEvent>();

    ui_plugins: ObservableProperty<UiPlugin[]>
        = new ObservableProperty<UiPlugin[]>([]);
    state: ObservableProperty<State> = new ObservableProperty<State>(State.Loading);
    visibilityState: ObservableProperty<VisibilityState> = new ObservableProperty<VisibilityState>(VisibilityState.Visible);

    promptForUpdate: ObservableProperty<boolean> = new ObservableProperty<boolean>(false);
    updateStatus: ObservableProperty<UpdateStatus> = new ObservableProperty<UpdateStatus>(new UpdateStatus());

    errorMessage: ObservableProperty<string> = new ObservableProperty<string>("");
    alertMessage: ObservableProperty<string> = new ObservableProperty<string>("");

    showStatusMonitor: ObservableProperty<boolean> = new ObservableProperty<boolean>(true);

    pedalboard: ObservableProperty<Pedalboard> = new ObservableProperty<Pedalboard>(new Pedalboard());
    presetChanged: ObservableProperty<boolean> = new ObservableProperty<boolean>(false);
    selectedSnapshot: ObservableProperty<number> = new ObservableProperty<number>(-1);
    plugin_classes: ObservableProperty<PluginClass> = new ObservableProperty<PluginClass>(new PluginClass());
    jackConfiguration: ObservableProperty<JackConfiguration> = new ObservableProperty<JackConfiguration>(new JackConfiguration());
    jackSettings: ObservableProperty<JackChannelSelection> = new ObservableProperty<JackChannelSelection>(new JackChannelSelection());
    banks: ObservableProperty<BankIndex>
        = new ObservableProperty<BankIndex>(new BankIndex());
    jackServerSettings: ObservableProperty<JackServerSettings>
        = new ObservableProperty<JackServerSettings>(new JackServerSettings());

    wifiConfigSettings: ObservableProperty<WifiConfigSettings> = new ObservableProperty<WifiConfigSettings>(new WifiConfigSettings());
    wifiDirectConfigSettings: ObservableProperty<WifiDirectConfigSettings> = new ObservableProperty<WifiDirectConfigSettings>(new WifiDirectConfigSettings());
    governorSettings: ObservableProperty<GovernorSettings> = new ObservableProperty<GovernorSettings>(new GovernorSettings());

    favorites: ObservableProperty<FavoritesList> = new ObservableProperty<FavoritesList>({});


    presets: ObservableProperty<PresetIndex> = new ObservableProperty<PresetIndex>
        (
            new PresetIndex()
        );
    systemMidiBindings: ObservableProperty<MidiBinding[]> = new ObservableProperty<MidiBinding[]>
        (
            [
                MidiBinding.systemBinding("prevProgram"),
                MidiBinding.systemBinding("nextProgram")
            ]
        );
    zoomedUiControl: ObservableProperty<ZoomedControlInfo | undefined> = new ObservableProperty<ZoomedControlInfo | undefined>(undefined);

    svgImgUrl(svgImage: string): string {
        //return this.varServerUrl + "img/" + svgImage;
        return "img/" + svgImage;
    }


    getUiPlugin(uri: string): UiPlugin | null {
        let plugins: UiPlugin[] = this.ui_plugins.get();

        for (let plugin of plugins) {
            if (plugin.uri === uri) return plugin;
        }
        return null;
    }

    androidHost?: AndroidHostInterface;

    constructor() {
        this.androidHost = (window as any).AndroidHost as AndroidHostInterface;

        this.onSocketError = this.onSocketError.bind(this);
        this.onSocketMessage = this.onSocketMessage.bind(this);
        this.onSocketReconnecting = this.onSocketReconnecting.bind(this);
        this.onSocketReconnected = this.onSocketReconnected.bind(this);
        this.onVisibilityChanged = this.onVisibilityChanged.bind(this);
        this.onSocketConnectionLost = this.onSocketConnectionLost.bind(this);
    }


    private expectDisconnectTimer?: number = undefined;

    cancelExpectDisconnectTimer() {
        if (this.expectDisconnectTimer) {
            clearTimeout(this.expectDisconnectTimer);
        }
    }
    startExpectDisconnectTimer() {
        this.cancelExpectDisconnectTimer();
        // poll for access to a running pipedal server
        this.expectDisconnectTimer = setTimeout(
            async () => {
                this.reconnectReason = ReconnectReason.Disconnected;
            },
            5 * 1000);

    }

    expectDisconnect(reason: ReconnectReason) {
        this.cancelExpectDisconnectTimer();
        this.reconnectReason = reason;
        if (this.reconnectReason !== ReconnectReason.Disconnected) {
            this.startExpectDisconnectTimer();
        }
    }

    onSocketReconnecting(retry: number, maxRetries: number): boolean {
        this.cancelExpectDisconnectTimer();
        if (this.isClosed) return false;
        //if (retry !== 0) {
        switch (this.reconnectReason) {
            case ReconnectReason.Disconnected:
            default:
                this.setState(State.Reconnecting);
                break;
            case ReconnectReason.LoadingSettings:
                this.setState(State.ApplyingChanges);
                break;
            case ReconnectReason.ReloadingPlugins:
                this.setState(State.ReloadingPlugins);
                break;
            case ReconnectReason.Updating:
                this.setState(State.InstallingUpdate);
                break;
            case ReconnectReason.HotspotChanging:
                this.setState(State.HotspotChanging);
                this.startHotspotReconnectTimer();
                break;

        }
        return true;
    }


    onSocketError(errorMessage: string, exception: any) {
        if (this.visibilityState.get() === VisibilityState.Hidden) return;
        this.onError(errorMessage);
    }

    compareFavorites(left: FavoritesList, right: FavoritesList): boolean {
        for (let key of Object.keys(left)) {
            if (!right[key]) {
                return false;
            }
        }

        for (let key of Object.keys(right)) {
            if (!left[key]) {
                return false;
            }
        }
        return true;

    }
    private setModelPedalboard(pedalboard: Pedalboard) {
        this.pedalboard.set(pedalboard);
        this.selectedSnapshot.set(pedalboard.selectedSnapshot);

    }
    onSocketMessage(header: PiPedalMessageHeader, body?: any) {

        let message = header.message;
        if (message === "onControlChanged") {
            let controlChangedBody = body as ControlChangedBody;
            this._setPedalboardControlValue(
                controlChangedBody.instanceId,
                controlChangedBody.symbol,
                controlChangedBody.value,
                false // do NOT notify the server of the change.
            );
        }
        else if (message === "onOutputVolumeChanged") {
            let value = body as number;
            this._setOutputVolume(value, false);
        }
        else if (message === "onInputVolumeChanged") {
            let value = body as number;
            this._setInputVolume(value, false);
        } else if (message === "onLv2StateChanged") {

            let instanceId = body.instanceId as number;
            let state = body.state as [boolean, any];

            this.onLv2StateChanged(instanceId, state);
        } else if (message === "onVst3ControlChanged") {
            let controlChangedBody = body as Vst3ControlChangedBody;
            this._setVst3PedalboardControlValue(
                controlChangedBody.instanceId,
                controlChangedBody.symbol,
                controlChangedBody.value,
                controlChangedBody.state,
                false // do NOT notify the server of the change.
            );

        } else if (message === "onMonitorPortOutput") {
            let monitorPortOutputBody = body as MonitorPortOutputBody;
            for (let i = 0; i < this.monitorPortSubscriptions.length; ++i) {
                let subscription = this.monitorPortSubscriptions[i];
                if (subscription.subscriptionHandle === monitorPortOutputBody.subscriptionHandle) {
                    subscription.onUpdated(body.value);
                    break;
                }
            }
            if (header.replyTo) {
                this.webSocket?.reply(header.replyTo, "onMonitorPortOutput", true);
            }
        } else if (message === "onFavoritesChanged") {
            let favorites = body as FavoritesList;
            if (!this.compareFavorites(favorites, this.favorites.get())) {
                this.favorites.set(favorites);
            }
        } else if (message === "onShowStatusMonitorChanged") {
            let value = body as boolean;
            this.showStatusMonitor.set(value);
        } else if (message === "onChannelSelectionChanged") {
            let channelSelectionBody = body as ChannelSelectionChangedBody;
            let channelSelection = new JackChannelSelection().deserialize(channelSelectionBody.jackChannelSelection);
            this.jackSettings.set(channelSelection);
        } else if (message === "onSnapshotModified") {
            let { snapshotIndex, modified } = (body as { snapshotIndex: number, modified: boolean });
            let snapshots = this.pedalboard.get().snapshots;
            if (snapshotIndex >= 0 && snapshotIndex < snapshots.length) {
                let snapshot = snapshots[snapshotIndex]
                if (snapshot) {
                    if (snapshot.isModified !== modified) {
                        snapshot.isModified = modified;
                        this.onSnapshotModified.fire({ snapshotIndex: snapshotIndex, modified: modified });
                    }
                }
            }
        } else if (message === "onSelectedSnapshotChanged") {
            let selectedSnapshot = body as number;
            this.pedalboard.get().selectedSnapshot = selectedSnapshot;
            this.selectedSnapshot.set(selectedSnapshot);
        } else if (message === "onPresetChanged") {
            let changed = body as boolean;

            if (this.presets.get().presetChanged !== changed) {
                let newPresets = this.presets.get().clone(); // deep clone.
                newPresets.presetChanged = changed;
                this.presets.set(newPresets);
            }
            this.presetChanged.set(changed);
        } else if (message === "onPresetsChanged") {
            let presetsChangedBody = body as PresetsChangedBody;
            let presets = new PresetIndex().deserialize(presetsChangedBody.presets);
            this.presets.set(presets);
            this.presetChanged.set(presets.presetChanged);
        } else if (message === "onPluginPresetsChanged") {
            let pluginUri = body as string;
            this.handlePluginPresetsChanged(pluginUri);
        } else if (message === "onJackConfigurationChanged") {
            this.jackConfiguration.set(new JackConfiguration().deserialize(body));
        } else if (message === "onLoadPluginPreset") {
            let instanceId = body.instanceId as number;
            let controlValues = ControlValue.deserializeArray(body.controlValues);
            this.handleOnLoadPluginPreset(instanceId, controlValues);
        } else if (message === "onItemEnabledChanged") {
            let itemEnabledBody = body as PedalboardItemEnableBody;
            this._setPedalboardItemEnabled(
                itemEnabledBody.instanceId,
                itemEnabledBody.enabled,
                false  // No server notification.
            );
        } else if (message === "onPedalboardChanged") {
            let pedalChangedBody = body as PedalboardChangedBody;
            this.setModelPedalboard(new Pedalboard().deserialize(pedalChangedBody.pedalboard));

        } else if (message === "onMidiValueChanged") {
            let controlChangedBody = body as ControlChangedBody;
            this._setPedalboardControlValue(
                controlChangedBody.instanceId,
                controlChangedBody.symbol,
                controlChangedBody.value,
                false // do NOT notify the server of the change.
            );
            if (header.replyTo) {
                this.webSocket?.reply(header.replyTo, "onMidiValueChanged", true);
            }

        } else if (message === "onNotifyMidiListener") {
            let clientHandle = body.clientHandle as number;
            let isNote = body.isNote as boolean;
            let noteOrControl = body.noteOrControl as number;
            this.handleNotifyMidiListener(clientHandle, isNote, noteOrControl);
        } else if (message === "onNotifyPathPatchPropertyChanged") {
            let instanceId = body.instanceId as number;
            let propertyUri = body.propertyUri as string;
            let atomJsonString = body.atomJson as string;
            this.handleNotifyPathPatchPropertyChanged(instanceId, propertyUri, atomJsonString);
            if (header.replyTo) {
                this.webSocket?.reply(header.replyTo, "onNotifyPathPatchPropertyChanged", true);
            }
        } else if (message === "onNotifyPatchProperty") {
            let clientHandle = body.clientHandle as number;
            let instanceId = body.instanceId as number;
            let propertyUri = body.propertyUri as string;
            let atomJson = body.atomJson as any;
            this.handleNotifyPatchProperty(clientHandle, instanceId, propertyUri, atomJson);
            if (header.replyTo) {
                this.webSocket?.reply(header.replyTo, "onNotifyPatchProperty", true);
            }
        } else if (message === "onJackServerSettingsChanged") {
            let jackServerSettings = new JackServerSettings().deserialize(body);
            this.jackServerSettings.set(jackServerSettings);
        } else if (message === "onWifiConfigSettingsChanged") {
            let wifiConfigSettings = new WifiConfigSettings().deserialize(body);
            this.wifiConfigSettings.set(wifiConfigSettings);
        } else if (message === "onWifiDirectConfigSettingsChanged") {
            let wifiDirectConfigSettings = new WifiDirectConfigSettings().deserialize(body);
            this.wifiDirectConfigSettings.set(wifiDirectConfigSettings);
        }
        else if (message === "onGovernorSettingsChanged") {
            let governor = body as string;
            let newSettings = this.governorSettings.get().clone();
            newSettings.governor = governor;
            this.governorSettings.set(newSettings);
        } else if (message === "onBanksChanged") {
            let banks = new BankIndex().deserialize(body);
            this.banks.set(banks);
        } else if (message === "onVuUpdate") {
            let vuUpdate = body as VuUpdateInfo;
            let item = this.vuSubscriptions[vuUpdate.instanceId];
            if (item) {
                for (let i = 0; i < item.subscribers.length; ++i) {
                    item.subscribers[i].callback(vuUpdate);
                }
            }
            if (header.replyTo) {
                this.webSocket?.reply(header.replyTo, "onVuUpdate", true);
            }
        } else if (message === "onSystemMidiBindingsChanged") {
            let bindings = MidiBinding.deserialize_array(body);
            this.systemMidiBindings.set(bindings);
        } else if (message === "onErrorMessage") {
            this.showAlert(body as string);

        }
        else if (message === "onLv2PluginsChanging") {
            this.onLv2PluginsChanging();
        } else if (message === "onUpdateStatusChanged") {
            let updateStatus = new UpdateStatus().deserialize(body);
            this.onUpdateStatusChanged(updateStatus);
        } else if (message === "onNetworkChanging") {
            this.onNetworkChanging(body as boolean);
        }
        else if (message === "onHasWifiChanged") {
            let hasWifi = body as boolean;
            this.hasWifiDevice.set(hasWifi);
        }
    }


    private updateLaterTimeout?: number = undefined;

    private clearPromptForUpdateTimer() {
        if (this.updateLaterTimeout) {
            clearTimeout(this.updateLaterTimeout);
            this.updateLaterTimeout = undefined;
        }
    }

    private setPromptForUpdateTimer(when: Date) {
        let ms = when.getTime() - Date.now();
        this.updateLaterTimeout = setTimeout(
            () => {
                this.updateLaterTimeout = undefined;
                // make the server do a fresh check
                this.getUpdateStatus()
                    .then(
                        () => {
                            this.updatePromptForUpdate();
                        })
                    .catch((e) => {

                    });
            },
            ms);
    }


    private lastCanUpdateNow: boolean = false;
    private updatePromptForUpdate() {
        this.clearPromptForUpdateTimer();
        if (!this.enableAutoUpdate) {
            return;
        }

        let stateEnabled = true; // must be present to accept alerts.  this.state.get() === State.Ready;
        let timeEnabled = false;

        let updateLaterTime = this.getUpdateTime();
        if (updateLaterTime == null) {
            timeEnabled = true;
        } else {
            let nDate = Date.now();

            let now: Date = new Date(nDate);
            let maxDate: Date = new Date(nDate + 86400000 * 2); // sanity check for systems with unstable system clock

            timeEnabled = (updateLaterTime < now || updateLaterTime >= maxDate)
        }
        let updateStatus = this.updateStatus.get();
        let statusEnabled = updateStatus.isOnline && updateStatus.isValid && updateStatus.getActiveRelease().updateAvailable;

        let canUpdateNow: boolean = (stateEnabled && timeEnabled && statusEnabled);
        if (updateStatus.updatePolicy === UpdatePolicyT.Disable) {
            canUpdateNow = false;
        }

        if (canUpdateNow && canUpdateNow !== this.lastCanUpdateNow) {
            this.showUpdateDialogValue = true; // make the dialog sticky so it can show OK button
        }
        this.lastCanUpdateNow = canUpdateNow;
        this.promptForUpdate.set(this.showUpdateDialogValue || canUpdateNow);

        if (stateEnabled && statusEnabled && !timeEnabled && updateLaterTime) {
            this.setPromptForUpdateTimer(updateLaterTime);
        }
    }
    private showUpdateDialogValue: boolean = false;

    showUpdateDialog(show: boolean = true) {
        if (this.showUpdateDialogValue !== show) {
            this.showUpdateDialogValue = show;
            if (show) {
                this.forceUpdateCheck();
            }
            this.updatePromptForUpdate();
        }
    }

    onUpdateStatusChanged(updateStatus: UpdateStatus): void {
        let current = this.updateStatus.get();
        if (!current.equals(updateStatus)) {
            this.updateStatus.set(updateStatus);
            if (current.currentVersion.length !== 0 && current.currentVersion !== updateStatus.currentVersion) {
                // !! Server has been updated!!!
                this.reloadPage();
                throw new UpdatedError("Server has been updated");
            }
            this.updatePromptForUpdate()
        }
    }
    onLv2PluginsChanging(): void {
        this.expectDisconnect(ReconnectReason.ReloadingPlugins);
        // this.webSocket?.reconnect(); // let the server do it for us.

    }
    setError(message: string): void {
        this.errorMessage.set(message);
        this.setState(State.Error);
    }
    varRequest(url: string): string {
        return "var/" + url;
    }

    setState(state: State) {
        if (this.state.get() !== state) {
            this.state.set(state);
        }
    }

    validatePluginClasses(plugin: PluginClass) {
        if (plugin.plugin_type === PluginType.None) {
            console.log("Error: No plugin type for uri '" + plugin.uri + "'");
        }
        for (let i = 0; i < plugin.children.length; ++i) {
            this.validatePluginClasses(plugin.children[i]);
        }
    }



    private reconnectReason: ReconnectReason = ReconnectReason.Disconnected;

    isReloading(): boolean {
        return this.state.get() !== State.Ready;
    }

    getWebSocket(): PiPedalSocket {
        if (this.webSocket === undefined) {
            throw new PiPedalStateError("Attempt to access web socket before it's connected.");
        }
        return this.webSocket;
    }

    androidReconnectTimeout?: number = undefined;

    cancelAndroidReconnectTimer() {
        if (this.androidReconnectTimeout) {
            clearTimeout(this.androidReconnectTimeout);
            this.androidReconnectTimeout = undefined;
        }
    }

    startAndroidReconnectTimer() {
        this.cancelAndroidReconnectTimer();
        this.androidReconnectTimeout = setTimeout(() => {
            this.androidReconnectTimeout = undefined;
            this.androidHost?.setDisconnected(true);
        }, 20 * 1000);
    }
    onSocketConnectionLost() {
        // remove all the events and subscriptions we have.
        if (this.isClosed) {
            return; // page unloading. do NOT change the UI.
        }
        this.vuSubscriptions = [];
        this.monitorPatchPropertyListeners = [];

        if (this.isAndroidHosted()) {
            // if unexpected, go back to the device browser immediately.
            if (this.reconnectReason === ReconnectReason.Disconnected) {
                this.androidHost?.setDisconnected(true);
            } else {
                this.startAndroidReconnectTimer();
            }
        }
    }



    async onSocketReconnected(): Promise<void> {
        this.cancelOnNetworkChanging();
        this.cancelAndroidReconnectTimer();

        if (this.isAndroidHosted()) {
            this.androidHost?.setDisconnected(false);
        }

        if (this.visibilityState.get() === VisibilityState.Hidden) return;

        // reload state, but not configuration.
        this.clientId = await this.getWebSocket().request<number>("hello");

        let newServerVersion = this.serverVersion = await this.getWebSocket().request<PiPedalVersion>("version");
        if (newServerVersion.serverVersion !== this.serverVersion.serverVersion) {
            this.reloadPage();
            return;
        }

        // anything could have changed while we were disconnected.
        await this.loadServerState();
    }
    makeSocketServerUrl(hostName: string, port: number): string {
        return "ws://" + hostName + ":" + port + "/pipedal";

    }
    makeVarServerUrl(protocol: string, hostName: string, port: number): string {
        return protocol + "://" + hostName + ":" + port + "/var/";

    }

    async getNextAudioFile(filePath: string): Promise<string> {
        try {
            let url =
                this.varServerUrl
                + "NextAudioFile?path=" + encodeURIComponent(filePath);

            let response = await fetch(url);
            let json = await response.json();
            return json as string;
        } catch (e) {
            return "";
        }
    }
    async getPreviousAudioFile(filePath: string): Promise<string> {
        try {
            let url =
                this.varServerUrl
                + "PreviousAudioFile?path=" + encodeURIComponent(filePath);

            let response = await fetch(url);
            let json = await response.json();
            return json as string;
        } catch (e) {
            return "";
        }
    }


    async getAudioFileMetadata(filePath: string): Promise<AudioFileMetadata> {
        try {
            let url =
                this.varServerUrl
                + "AudioMetadata?path=" + encodeURIComponent(filePath);

            let response = await fetch(url);
            let json = await response.json();
            return new AudioFileMetadata().deserialize(json);
        } catch (e) {
            return new AudioFileMetadata();
        }
    }


    maxFileUploadSize: number = 512 * 1024 * 1024;
    maxPresetUploadSize: number = 1024 * 1024;
    debug: boolean = false;
    enableAutoUpdate: boolean = false;


    async requestConfig(): Promise<boolean> {

        try {
            const myRequest = new Request(this.varRequest('config.json'));
            let response: Response = await fetch(myRequest);
            let data = await response.json();

            this.enableAutoUpdate = !!data.enable_auto_update;
            this.hasWifiDevice.set(!!data.has_wifi_device);
            if (data.max_upload_size) {
                this.maxPresetUploadSize = data.max_upload_size;
            }
            if (data.fakeAndroid) {
                this.androidHost = new FakeAndroidHost();
            }
            this.debug = !!data.debug;
            let { socket_server_port, socket_server_address, max_upload_size } = data;
            if ((!socket_server_address) || socket_server_address === "*") {
                socket_server_address = window.location.hostname;
            }
            if (!socket_server_port) socket_server_port = 8080;
            let socket_server = this.makeSocketServerUrl(socket_server_address, socket_server_port);
            let var_server_url = this.makeVarServerUrl("http", socket_server_address, socket_server_port);

            this.socketServerUrl = socket_server;
            this.varServerUrl = var_server_url;
            this.maxFileUploadSize = parseInt(max_upload_size);
        } catch (error: any) {
            this.setError("Can't connect to server. " + getErrorMessage(error));
            return false;
        }

        this.webSocket = new PiPedalSocket(
            this.socketServerUrl,
            {
                onMessageReceived: this.onSocketMessage,
                onError: this.onSocketError,
                onConnectionLost: this.onSocketConnectionLost,
                onReconnect: this.onSocketReconnected,
                onReconnecting: this.onSocketReconnecting
            }
        );

        try {
            await this.webSocket.connect();
        } catch (error) {
            this.setError("Failed to connect to server. " + getErrorMessage(error));
            return false;

        }
        try {
            this.countryCodes = await this.getWebSocket().request<{ [Name: string]: string }>("getWifiRegulatoryDomains");

            this.clientId = (await this.getWebSocket().request<number>("hello")) as number;

            this.preloadImages((await this.getWebSocket().request<string>("imageList")));
        } catch (error) {
            this.setError("Failed to establish connection. " + getErrorMessage(error));
            return false;
        }
        return await this.loadServerState();
    }
    async loadServerState(): Promise<boolean> {
        try {
            this.serverVersion = await this.getWebSocket().request<PiPedalVersion>("version");

            this.updateStatus.set(new UpdateStatus().deserialize(await this.getUpdateStatus()));

            this.hasWifiDevice.set(await this.getWebSocket().request<boolean>("getHasWifi"));

            this.ui_plugins.set(
                UiPlugin.deserialize_array(await this.getWebSocket().request<any>("plugins"))
            );
            this.setModelPedalboard(
                new Pedalboard().deserialize(
                    await this.getWebSocket().request<Pedalboard>("currentPedalboard")
                )
            );
            this.plugin_classes.set(new PluginClass().deserialize(
                await this.getWebSocket().request<any>("pluginClasses")
            ));
            this.validatePluginClasses(this.plugin_classes.get());

            this.presets.set(
                new PresetIndex().deserialize(
                    await this.getWebSocket().request<PresetIndex>("getPresets")
                )
            );
            this.wifiConfigSettings.set(
                new WifiConfigSettings().deserialize(
                    await this.getWebSocket().request<any>("getWifiConfigSettings")
                ));
            this.wifiDirectConfigSettings.set(
                new WifiDirectConfigSettings().deserialize(
                    await this.getWebSocket().request<any>("getWifiDirectConfigSettings")
                ));
            this.governorSettings.set(new GovernorSettings().deserialize(
                await this.getWebSocket().request<any>("getGovernorSettings")
            ));
            this.showStatusMonitor.set(
                await this.getWebSocket().request<boolean>("getShowStatusMonitor")
            );
            this.jackServerSettings.set(
                new JackServerSettings().deserialize(
                    await this.getWebSocket().request<any>("getJackServerSettings")
                )
            );
            this.jackConfiguration.set(new JackConfiguration().deserialize(
                await this.getWebSocket().request<JackConfiguration>("getJackConfiguration")
            ));
            this.jackSettings.set(new JackChannelSelection().deserialize(
                await this.getWebSocket().request<any>("getJackSettings")
            ));
            this.banks.set(new BankIndex().deserialize(await this.getWebSocket().request<any>("getBankIndex")));

            this.favorites.set(await this.getWebSocket().request<FavoritesList>("getFavorites"));

            this.systemMidiBindings.set(MidiBinding.deserialize_array(await this.getWebSocket().request<MidiBinding[]>("getSystemMidiBindings")));

            // load at lest once before we allow a reconnect.
            this.getWebSocket().canReconnect = true;

            this.setState(State.Ready);
            return true;
        }
        catch (error) {
            this.setError("Failed to fetch server state.\n\n" + getErrorMessage(error));
            return false;
        }
    }


    onError(message: string | Error): void {
        let m = message;
        if (message instanceof Error) {
            let e = message as Error;
            if (e.message) {
                m = e.message as string;
            } else {
                m = e.toString();
            }
        } else {
            m = message.toString();
        }
        this.errorMessage.set(m);
        this.state.set(State.Error);

    }

    backgroundStateTimeout?: number = undefined;

    exitBackgroundState() {
        if (this.backgroundStateTimeout) {
            clearTimeout(this.backgroundStateTimeout);
            this.backgroundStateTimeout = undefined;
        }
        if (this.state.get() === State.Background) {
            console.log("Exiting background state.");
            this.visibilityState.set(VisibilityState.Visible);
            this.webSocket?.exitBackgroundState();
        }

    }
    enterBackgroundState() {
        // on Android, delay entering background state by 180 seconds,
        // since background management is more complicated. e.g. screen flips, and system upload dialogs.

        // if (this.isAndroidHosted()) {
        //     yyyx;
        //     if (this.backgroundStateTimeout) {
        //         clearTimeout(this.backgroundStateTimeout);
        //     }
        //     this.backgroundStateTimeout = setTimeout(() => {
        //         this.backgroundStateTimeout = undefined;
        //         this.enterBackgroundState_();
        //     }, 180000);
        // } else {
        //     this.enterBackgroundState_();
        // }
        this.enterBackgroundState_();
    }
    enterBackgroundState_() {
        if (this.state.get() !== State.Background) {
            console.log("Entering background state.");
            this.visibilityState.set(VisibilityState.Hidden);
            this.setState(State.Background);
            this.webSocket?.enterBackgroundState();
        }
    }


    onVisibilityChanged(doc: Document, event: Event): any {
        if (document.visibilityState) {

            switch (document.visibilityState) {
                case "visible":
                    this.exitBackgroundState();
                    break;
                case "hidden":
                    this.enterBackgroundState();
                    break;
            }
        }
        return false;
    }

    initialize(): void {
        this.setError("");
        this.setState(State.Loading);

        this.requestConfig()
            .then((succeeded) => {
                if (succeeded) {
                    this.state.set(State.Ready);
                }
            })
            .catch((error) => {
                this.setError("Failed to get server state. \n\n" + error.toString());
            });
        let t = this.onVisibilityChanged;

        (document as any).addEventListener("visibilitychange", (doc: Document, event: Event) => {
            return t(doc, event);
        });
    }

    getControl(uiPlugin: UiPlugin, portSymbol: string): UiControl | null {
        for (let i = 0; i < uiPlugin.controls.length; ++i) {
            let control = uiPlugin.controls[i];
            if (control.symbol === portSymbol) {
                return control;
            }

        }
        return null;
    }
    getDefaultValues(uri: string): ControlValue[] {
        let uiPlugin = this.getUiPlugin(uri);
        if (uiPlugin === null) throw new PiPedalArgumentError("Pedal uri not found.");
        let result: ControlValue[] = [];

        let controls = uiPlugin.controls;
        for (let i = 0; i < controls.length; ++i) {
            let control = controls[i];

            let uiControl = this.getControl(uiPlugin, control.symbol);
            if (uiControl === null) {
                // this is default values, so a mis-match is in fact a problem.
                throw new PiPedalStateError("ui and pedealboard ports don't match.");
            }
            let cv = new ControlValue();
            cv.key = control.symbol;
            cv.value = control.default_value;


            result.push(cv);
        }
        return result;


    }



    handleOnLoadPluginPreset(instanceId: number, controlValues: ControlValue[]) {
        // note that plugins with state are dealt with server-side.
        // if we made it here, we can just load the controls.
        let pedalboard = this.pedalboard.get();
        if (pedalboard === undefined) throw new PiPedalStateError("Pedalboard not ready.");
        let newPedalboard = pedalboard.clone();

        let item = newPedalboard.getItem(instanceId);

        let changed = false;
        for (let i = 0; i < controlValues.length; ++i) {
            let controlValue = controlValues[i];
            changed = item.setControlValue(controlValue.key, controlValue.value) || changed;
        }
        if (changed) {
            this.setModelPedalboard(newPedalboard);
        }
    }


    private _controlValueChangeItems: ControlValueChangeItem[] = [];

    addControlValueChangeListener(instanceId: number, onValueChanged: ControlValueChangedHandler): ControlValueChangedHandle {
        let handle = ++this.nextListenHandle;
        this._controlValueChangeItems.push({ handle: handle, instanceId: instanceId, onValueChanged: onValueChanged });
        return { _ControlValueChangedHandle: handle };
    }
    removeControlValueChangeListener(handle: ControlValueChangedHandle) {
        for (let i = 0; i < this._controlValueChangeItems.length; ++i) {
            if (this._controlValueChangeItems[i].handle === handle._ControlValueChangedHandle) {
                this._controlValueChangeItems.splice(i, 1);
                return;
            }
        }
    }

    private stateChangedListeners: StateChangedEntry[] = [];

    addLv2StateChangedListener(instanceId: number, onStateChanged: StateChangedHandler): StateChangedHandle {
        let handle = ++this.nextListenHandle;

        let item: StateChangedEntry = {
            handle: handle,
            instanceId: instanceId,
            onStateChanged: onStateChanged
        };
        this.stateChangedListeners.push(item);
        return { _handle: handle };
    }
    removeLv2StateChangedListener(handle: StateChangedHandle): void {
        let h = handle._handle;

        for (let i = 0; i < this.stateChangedListeners.length; ++i) {
            if (this.stateChangedListeners[i].handle === h) {
                this.stateChangedListeners.splice(i, 1);
            }
        }
    }

    private onLv2StateChanged(instanceId: number, state: [boolean, any]): void {
        let item = this.pedalboard.get().getItem(instanceId);
        item.lv2State = state;
        for (let item of this.stateChangedListeners) {
            if (item.instanceId === instanceId) {
                item.onStateChanged(instanceId);
            }
        }
    }


    private _pluginPresetsChangedHandles: PluginPresetsChangedHandle[] = [];

    addPluginPresetsChangedListener(onPluginPresetsChanged: PluginPresetsChangedHandler): PluginPresetsChangedHandle {
        let handle = ++this.nextListenHandle;
        let t: PluginPresetsChangedHandle = {
            _id: handle,
            _handler: onPluginPresetsChanged
        };
        this._pluginPresetsChangedHandles.push(t);
        return t;
    }
    removePluginPresetsChangedListener(handle: PluginPresetsChangedHandle): void {
        let pos = -1;
        for (let i = 0; i < this._pluginPresetsChangedHandles.length; ++i) {
            if (this._pluginPresetsChangedHandles[i]._id === handle._id) {
                pos = i;
                break;
            }
        }
        if (pos !== -1) {
            this._pluginPresetsChangedHandles.splice(pos, 1);
        }
    }
    firePluginPresetsChanged(pluginUri: string): void {
        for (let i = 0; i < this._pluginPresetsChangedHandles.length; ++i) {
            this._pluginPresetsChangedHandles[i]._handler(pluginUri);
        }

    }

    nextBank() {
        this.webSocket?.send("nextBank");
    }
    previousBank() {
        this.webSocket?.send("previousBank");
    }
    nextPreset() {
        this.webSocket?.send("nextPreset");
    }
    previousPreset() {
        this.webSocket?.send("previousPreset");
    }


    selectSnapshot(index: number) {
        this.webSocket?.send("setSnapshot", index);
    }
    setSnapshots(snapshots: (Snapshot | null)[], selectedSnapshot: number) {
        let pedalboard = this.pedalboard.get().clone();
        pedalboard.snapshots = snapshots;
        if (selectedSnapshot !== -1) {
            pedalboard.selectedSnapshot = selectedSnapshot;
        }
        this.setModelPedalboard(pedalboard);

        this.webSocket?.send("setSnapshots", { snapshots: snapshots, selectedSnapshot: selectedSnapshot });
    }

    setInputVolume(volume_db: number): void {
        this._setInputVolume(volume_db, true);
    }
    previewInputVolume(volume_db: number): void {
        nullCast(this.webSocket).send("previewInputVolume", volume_db);

    }
    previewOutputVolume(volume_db: number): void {
        nullCast(this.webSocket).send("previewOutputVolume", volume_db);

    }

    private _setInputVolume(volume_db: number, notifyServer: boolean): void {
        let changed: boolean = false;

        let pedalboard = this.pedalboard.get();
        if (pedalboard.input_volume_db !== volume_db) {
            let newPedalboard = pedalboard.clone();
            newPedalboard.input_volume_db = volume_db;
            this.setModelPedalboard(newPedalboard);
            changed = true;
        }
        if (changed) {
            if (notifyServer) {
                nullCast(this.webSocket).send("setInputVolume", volume_db);
            }

            for (let i = 0; i < this._controlValueChangeItems.length; ++i) {
                let item = this._controlValueChangeItems[i];
                if (Pedalboard.START_CONTROL === item.instanceId) {
                    item.onValueChanged("volume_db", volume_db);
                }
            }
        }
    }

    setOutputVolume(volume_db: number, notifyServer: boolean): void {
        this._setOutputVolume(volume_db, true);
    }

    private _setOutputVolume(volume_db: number, notifyServer: boolean): void {
        let changed: boolean = false;

        let pedalboard = this.pedalboard.get();
        if (pedalboard.output_volume_db !== volume_db) {
            let newPedalboard = pedalboard.clone();
            newPedalboard.output_volume_db = volume_db;
            this.setModelPedalboard(newPedalboard);
            changed = true;
        }
        if (changed) {
            if (notifyServer) {
                nullCast(this.webSocket).send("setOutputVolume", volume_db);
            }
            for (let i = 0; i < this._controlValueChangeItems.length; ++i) {
                let item = this._controlValueChangeItems[i];
                if (Pedalboard.END_CONTROL === item.instanceId) {
                    item.onValueChanged("volume_db", volume_db);
                }
            }
        }
    }


    private _setPedalboardControlValue(instanceId: number, key: string, value: number, notifyServer: boolean): void {
        let pedalboard = this.pedalboard.get();
        if (pedalboard === undefined) throw new PiPedalStateError("Pedalboard not ready.");
        let newPedalboard = pedalboard.clone();

        if (instanceId === Pedalboard.START_CONTROL && key === "volume_db") {
            this._setInputVolume(value, notifyServer);
            return;
        } else if (instanceId === Pedalboard.END_CONTROL) {
            this._setOutputVolume(value, notifyServer);
            return;
        }
        let item = newPedalboard.getItem(instanceId);
        let changed = item.setControlValue(key, value);

        if (changed) {
            if (notifyServer) {
                this._setServerControl("setControl", instanceId, key, value);
            }
            this.setModelPedalboard(newPedalboard);
            for (let i = 0; i < this._controlValueChangeItems.length; ++i) {
                let item = this._controlValueChangeItems[i];
                if (instanceId === item.instanceId) {
                    item.onValueChanged(key, value);
                }
            }
        }

    }
    private _setVst3PedalboardControlValue(instanceId: number, key: string, value: number, state: string, notifyServer: boolean): void {
        let pedalboard = this.pedalboard.get();
        if (pedalboard === undefined) throw new PiPedalStateError("Pedalboard not ready.");
        let newPedalboard = pedalboard.clone();

        let item: PedalboardItem = newPedalboard.getItem(instanceId);
        let changed = item.setControlValue(key, value);
        item.vstState = state;

        if (changed) {
            this.setModelPedalboard(newPedalboard);
            if (notifyServer) {
                this._setServerControl("setControl", instanceId, key, value);
            }
            for (let i = 0; i < this._controlValueChangeItems.length; ++i) {
                let item = this._controlValueChangeItems[i];
                if (instanceId === item.instanceId) {
                    item.onValueChanged(key, value);
                }
            }
        }
    }


    sendPedalboardControlTrigger(instanceId: number, key: string, value: number): void {
        // no state change, no saving the value, just send it to the realtime thread/
        this._setServerControl("previewControl", instanceId, key, value);
    }


    setPedalboardControl(instanceId: number, key: string, value: number): void {
        this._setPedalboardControlValue(instanceId, key, value, true);
    }
    setPedalboardItemEnabled(instanceId: number, value: boolean): void {
        this._setPedalboardItemEnabled(instanceId, value, true);
    }
    private _setPedalboardItemEnabled(instanceId: number, value: boolean, notifyServer: boolean): void {
        let pedalboard = this.pedalboard.get();
        if (pedalboard === undefined) throw new PiPedalStateError("Pedalboard not ready.");
        let newPedalboard = pedalboard.clone();

        let item = newPedalboard.getItem(instanceId);

        let changed = value !== item.isEnabled;
        if (changed) {
            item.isEnabled = value;
            this.setModelPedalboard(newPedalboard);
            if (notifyServer) {
                let body: PedalboardItemEnableBody = {
                    clientId: this.clientId,
                    instanceId: instanceId,
                    enabled: value
                }
                this.webSocket?.send("setPedalboardItemEnable", body);

            }
        }

    }

    updateServerPedalboard(): void {
        let body: PedalboardChangedBody = {
            clientId: this.clientId,
            pedalboard: this.pedalboard.get()
        };
        this.webSocket?.send("updateCurrentPedalboard", body);

    }

    setShowStatusMonitor(show: boolean): void {
        this.webSocket?.send("setShowStatusMonitor", show);
    }

    loadPedalboardPlugin(itemId: number, selectedUri: string): number {
        let pedalboard = this.pedalboard.get();
        if (pedalboard === undefined) throw new PiPedalArgumentError("Can't clone an undefined object.");
        let newPedalboard = pedalboard.clone();

        let plugin = this.getUiPlugin(selectedUri);
        if (plugin === null) {
            throw new PiPedalArgumentError("Plugin not found.");
        }

        let it = newPedalboard.itemsGenerator();
        while (true) {
            let v = it.next();
            if (v.done) break;
            let item = v.value;
            if (item.instanceId === itemId) {
                item.deserialize(new PedalboardItem()); // skeezy way to re-initialize.
                item.instanceId = ++newPedalboard.nextInstanceId;
                item.uri = selectedUri;
                item.pluginName = plugin.name;
                item.controlValues = this.getDefaultValues(item.uri);
                item.isEnabled = true;
                item.lv2State = [false, {}];
                item.vstState = "";
                item.pathProperties = {};
                for (let fileProperty of plugin.fileProperties) {
                    // stringized json for an atom. see AtomConverter.hpp.
                    //  null -> we've never seen a value.
                    item.pathProperties[fileProperty.patchProperty] = "null";
                }
                this.setModelPedalboard(newPedalboard);
                this.updateServerPedalboard()
                return item.instanceId;
            }
        }
        throw new PiPedalArgumentError("Pedalboard item not found.");

    }
    private _setServerControl(message: string, instanceId: number, key: string, value: number) {
        let body: ControlChangedBody = {
            clientId: this.clientId,
            instanceId: instanceId,
            symbol: key,
            value: value
        };
        this.webSocket?.send(message, body);
    }
    previewPedalboardValue(instanceId: number, key: string, value: number): void {
        // mouse is down. Don't update EVERYBODY, but we must change 
        // the control on the running audio plugin.
        // TODO: respect "expensive" port attribute.
        if (instanceId === Pedalboard.START_CONTROL && key === "volume_db") {
            this.previewInputVolume(value);
            return;
        } else if (instanceId === Pedalboard.END_CONTROL) {
            this.previewOutputVolume(value);
            return;
        }

        this._setServerControl("previewControl", instanceId, key, value);
    }

    // returns the next selected instanceId, or null,if no item was deleted.
    deletePedalboardPedal(instanceId: number): number | null {
        let pedalboard = this.pedalboard.get();
        let newPedalboard = pedalboard.clone();
        this.updateVst3State(newPedalboard);

        let result = newPedalboard.deleteItem(instanceId);
        if (result !== null) {
            this.setModelPedalboard(newPedalboard);
            this.updateServerPedalboard();
        }
        return result;


    }
    movePedalboardItemBefore(fromInstanceId: number, toInstanceId: number): void {
        if (fromInstanceId === toInstanceId) return;
        let pedalboard = this.pedalboard.get();
        let newPedalboard = pedalboard.clone();

        this.updateVst3State(newPedalboard);

        let fromItem = newPedalboard.getItem(fromInstanceId);
        if (fromItem === null) {
            throw new PiPedalArgumentError("fromInstanceId not found.");
        }

        newPedalboard.deleteItem(fromInstanceId);

        newPedalboard.addBefore(fromItem, toInstanceId);
        this.setModelPedalboard(newPedalboard);
        this.updateServerPedalboard();

    }
    movePedalboardItemAfter(fromInstanceId: number, toInstanceId: number): void {
        if (fromInstanceId === toInstanceId) return;
        let pedalboard = this.pedalboard.get();
        let newPedalboard = pedalboard.clone();

        this.updateVst3State(newPedalboard);

        let fromItem = newPedalboard.getItem(fromInstanceId);
        if (fromItem === null) {
            throw new PiPedalArgumentError("fromInstanceId not found.");
        }

        newPedalboard.deleteItem(fromInstanceId);

        newPedalboard.addAfter(fromItem, toInstanceId);

        this.setModelPedalboard(newPedalboard);
        this.updateServerPedalboard();
    }

    movePedalboardItemToStart(instanceId: number): void {

        let pedalboard = this.pedalboard.get();
        let newPedalboard = pedalboard.clone();

        this.updateVst3State(newPedalboard);


        let fromItem = newPedalboard.getItem(instanceId);
        if (fromItem === null) {
            throw new PiPedalArgumentError("fromInstanceId not found.");
        }

        newPedalboard.deleteItem(instanceId);

        newPedalboard.addToStart(fromItem);

        this.setModelPedalboard(newPedalboard);
        this.updateServerPedalboard();
    }
    movePedalboardItemToEnd(instanceId: number): void {

        let pedalboard = this.pedalboard.get();
        let newPedalboard = pedalboard.clone();
        this.updateVst3State(newPedalboard);


        let fromItem = newPedalboard.getItem(instanceId);
        if (fromItem === null) {
            throw new PiPedalArgumentError("fromInstanceId not found.");
        }

        newPedalboard.deleteItem(instanceId);

        newPedalboard.addToEnd(fromItem);

        this.setModelPedalboard(newPedalboard);

        this.updateServerPedalboard();


    }


    movePedalboardItem(fromInstanceId: number, toInstanceId: number): void {
        if (fromInstanceId === toInstanceId) return;

        let pedalboard = this.pedalboard.get();
        let newPedalboard = pedalboard.clone();
        this.updateVst3State(newPedalboard);


        let fromItem = newPedalboard.getItem(fromInstanceId);
        let toItem = newPedalboard.getItem(toInstanceId);
        if (fromItem === null) {
            throw new PiPedalArgumentError("fromInstanceId not found.");
        }
        if (toItem === null) {
            throw new PiPedalArgumentError("toInstanceId not found.");
        }
        let emptyItem = newPedalboard.createEmptyItem();
        newPedalboard.replaceItem(fromInstanceId, emptyItem);
        newPedalboard.replaceItem(toInstanceId, fromItem);

        this.setModelPedalboard(newPedalboard);
        this.updateServerPedalboard();

    }
    addPedalboardItem(instanceId: number, append: boolean): number {
        let pedalboard = this.pedalboard.get();
        if (instanceId === Pedalboard.START_CONTROL && append) {
            instanceId = pedalboard.items[0].instanceId;
            append = false;
        } else if (instanceId === Pedalboard.END_CONTROL && !append) {
            instanceId = pedalboard.items[pedalboard.items.length - 1].instanceId;
            append = true;
        }
        let newPedalboard = pedalboard.clone();
        this.updateVst3State(newPedalboard);


        let item = newPedalboard.getItem(instanceId);
        if (item === null) {
            throw new PiPedalArgumentError("instanceId not found.");
        }
        let newItem = newPedalboard.createEmptyItem();
        newPedalboard.addItem(newItem, instanceId, append);
        this.setModelPedalboard(newPedalboard);
        this.updateServerPedalboard();
        return newItem.instanceId;
    }
    addPedalboardSplitItem(instanceId: number, append: boolean): number {
        let pedalboard = this.pedalboard.get();

        if (instanceId === Pedalboard.START_CONTROL && append) {
            instanceId = pedalboard.items[0].instanceId;
            append = false;
        } else if (instanceId === Pedalboard.END_CONTROL && !append) {
            instanceId = pedalboard.items[pedalboard.items.length - 1].instanceId;
            append = true;
        }


        let newPedalboard = pedalboard.clone();
        this.updateVst3State(newPedalboard);

        let item = newPedalboard.getItem(instanceId);
        if (item === null) {
            throw new PiPedalArgumentError("instanceId not found.");
        }
        let newItem = newPedalboard.createEmptySplit();
        newPedalboard.addItem(newItem, instanceId, append);
        this.setModelPedalboard(newPedalboard);
        this.updateServerPedalboard();
        return newItem.instanceId;

    }
    setPedalboardItemEmpty(instanceId: number): number {
        let pedalboard = this.pedalboard.get();
        let newPedalboard = pedalboard.clone();
        this.updateVst3State(newPedalboard);


        let item = newPedalboard.getItem(instanceId);
        if (item === null) {
            throw new PiPedalArgumentError("instanceId not found.");
        }
        newPedalboard.setItemEmpty(item);

        this.setModelPedalboard(newPedalboard);
        this.updateServerPedalboard();
        return item.instanceId;

    }


    saveCurrentPreset(): void {
        this.webSocket?.send("saveCurrentPreset", this.clientId);
    }

    isOnboarding(): boolean {
        var settings = this.jackServerSettings.get();
        return settings.isOnboarding;
    }

    setOnboarding(value: boolean): void {
        nullCast(this.webSocket)
            .request("setOnboarding", value);
    }

    moveAudioFile(
        path: string,
        from: number,
        to: number
    ): Promise<boolean> {
        return new Promise<boolean>(
            (accept, reject) => {
                this.webSocket?.request<boolean>("moveAudioFile", {
                    path: path,
                    from: from,
                    to: to
                }).then(() => {
                    accept(true);
                }).catch((e) => {
                    this.setError(getErrorMessage(e));
                    accept(false);
                });
            }
        );
    }


    saveCurrentPresetAs(newName: string, saveAfterInstanceId = -1): Promise<number> {
        // default behaviour is to save after the currently selected preset.
        if (saveAfterInstanceId === -1) {
            saveAfterInstanceId = this.presets.get().selectedInstanceId;
        }
        let request: any = {
            clientId: this.clientId,
            name: newName,
            saveAfterInstanceId: saveAfterInstanceId

        };

        return nullCast(this.webSocket)
            .request<number>("saveCurrentPresetAs", request)
            .then((newPresetId) => {
                this.loadPreset(newPresetId);
                return newPresetId;
            });
    }

    getUpdateStatus(): Promise<UpdateStatus> {
        return new Promise<UpdateStatus>(
            (accept, reject) => {
                nullCast(this.webSocket)
                    .request<UpdateStatus>('getUpdateStatus')
                    .then((result) => {
                        let updateStatus = new UpdateStatus().deserialize(result);
                        this.onUpdateStatusChanged(updateStatus);
                        accept(result);
                    }).catch(
                        (e) => {
                            reject(e);
                        }

                    );

            });

    }
    launchExternalUrl(url: string) {
        if (this.isAndroidHosted()) {
            try {
                this.androidHost?.launchExternalUrl(url);
            } catch (e) {
                // if they haven't updated their client yet, just don't do it.
            }


        } else {
            window.open(url, "_blank")?.focus();
        }
    }

    updateLater(delayMs: number) {
        let futureDate = new Date(Date.now() + delayMs);
        localStorage.setItem('nextUpdateTime', futureDate.toISOString());

        this.updatePromptForUpdate();
    }

    updateNow(): Promise<void> {
        return new Promise<void>(
            (accept, reject) => {
                let updateStatus = this.updateStatus.get();
                if (updateStatus.isOnline && updateStatus.isValid && updateStatus.getActiveRelease().updateAvailable) {
                    this.setState(State.DownloadingUpdate);
                    this.expectDisconnect(ReconnectReason.Updating);
                    let url = updateStatus.getActiveRelease().updateUrl;

                    nullCast(this.webSocket)
                        .request<void>('updateNow', url)
                        .then(() => {
                            this.setState(State.InstallingUpdate);
                            accept();
                        })
                        .catch(
                            (e: any) => {
                                this.expectDisconnect(ReconnectReason.Disconnected);

                                this.setState(State.Ready); // TODO: hopefully we haven't had an intermediate disconnect.
                                reject(e);
                            });
                } else {
                    reject(new Error("Invalid  update request."));
                }

            }
        );
    }

    getUpdateTime(): Date | null {
        let item = localStorage.getItem('nextUpdateTime');
        if (item) {
            return new Date(item);
        }
        return null;
    }
    // deprecated.
    requestFileList(piPedalFileProperty: UiFileProperty): Promise<string[]> {
        return nullCast(this.webSocket)
            .request<string[]>('requestFileList', piPedalFileProperty);
    }
    requestFileList2(relativeDirectoryPath: string, piPedalFileProperty: UiFileProperty): Promise<FileRequestResult> {
        return nullCast(this.webSocket)
            .request<FileRequestResult>('requestFileList2',
                { relativePath: relativeDirectoryPath, fileProperty: piPedalFileProperty }
            );
    }

    deleteUserFile(fileName: string): Promise<boolean> {
        return nullCast(this.webSocket).request<boolean>('deleteUserFile', fileName);
    }


    saveCurrentPluginPresetAs(pluginInstanceId: number, newName: string): Promise<number> {
        // default behaviour is to save after the currently selected preset.
        let request: any = {
            instanceId: pluginInstanceId,
            name: newName,
        };
        return nullCast(this.webSocket)
            .request<number>("savePluginPresetAs", request)
            .then((data) => {
                return data;
            });
    }


    loadPreset(instanceId: number): void {
        this.webSocket?.send("loadPreset", instanceId);

    }
    updatePresets(presets: PresetIndex): Promise<void> {
        return nullCast(this.webSocket).request<void>("updatePresets", presets);
    }
    moveBank(from: number, to: number): Promise<void> {
        return nullCast(this.webSocket).request<void>("moveBank", { from: from, to: to });
    }


    deletePresetItem(instanceId: number): Promise<number> {
        return nullCast(this.webSocket).request<number>("deletePresetItem", instanceId);

    }
    deleteBankItem(instanceId: number): Promise<number> {
        return nullCast(this.webSocket).request<number>("deleteBankItem", instanceId);

    }

    renamePresetItem(instanceId: number, name: string): Promise<void> {
        let body: RenamePresetBody = {
            clientId: this.clientId,
            instanceId: instanceId,
            name: name
        };

        return nullCast(this.webSocket).request<void>("renamePresetItem", body);
    }

    copyPreset(fromId: number, toId: number = -1): Promise<number> {
        let body: any = {
            clientId: this.clientId,
            fromId: fromId,
            toId: toId
        };
        return nullCast(this.webSocket).request<number>("copyPreset", body);

    }
    duplicatePreset(instanceId: number): Promise<number> {
        return this.copyPreset(instanceId, -1);
    }

    showAlert(message: any): void {
        let m: string;
        if (message instanceof Error) {
            let e = message as Error;
            if (e.message) {
                m = e.message as string;
            } else {
                m = e.toString();
            }
        } else {
            m = message.toString();
        }
        this.alertMessage.set(m);
    }


    setJackSettings(jackSettings: JackChannelSelection): void {
        this.expectDisconnect(ReconnectReason.LoadingSettings);
        this.webSocket?.send("setJackSettings", jackSettings);
    }


    monitorPortSubscriptions: MonitorPortHandleImpl[] = [];

    monitorPort(instanceId: number, key: string, updateRateSeconds: number, onUpdated: (value: number) => void): MonitorPortHandle {
        let result = new MonitorPortHandleImpl(instanceId, key, onUpdated);
        this.monitorPortSubscriptions.push(result);
        if (!this.webSocket) return result;


        this.webSocket.request<number>("monitorPort",
            {
                instanceId: instanceId,
                key: key,
                updateRate: updateRateSeconds
            })
            .then((handle) => {
                if (result.valid) {
                    result.subscriptionHandle = handle;
                } else {
                    try {
                        this.webSocket?.send("unmonitorPort", handle);
                    } catch (err) {

                    }
                }
            });
        return result;


    }
    unmonitorPort(handle: MonitorPortHandle): void {
        let t = handle as MonitorPortHandleImpl;

        for (let i = 0; i < this.monitorPortSubscriptions.length; ++i) {
            let item = this.monitorPortSubscriptions[i];
            if (item === t) {
                this.monitorPortSubscriptions.splice(i, 1);
                item.valid = false;
                if (item.subscriptionHandle) {
                    try {
                        this.webSocket?.send("unmonitorPort", item.subscriptionHandle);
                    } catch (err) {

                    }
                }
                break;
            }
        }
    }

    vuSubscriptions: (VuSubscriptionTarget | undefined)[] = [];


    addVuSubscription(instanceId: number, vuChangedHandler: VuChangedHandler): VuSubscriptionHandle {

        let result = new VuSubscriptionHandleImpl(instanceId, vuChangedHandler);

        if (!this.webSocket) return result; // racing to death. don't subscribe.

        let item = this.vuSubscriptions[instanceId];
        if (item) {
            item.subscribers.push(result);
        } else {
            let newTarget = new VuSubscriptionTarget();
            newTarget.subscribers.push(result);

            this.webSocket.request<number>("addVuSubscription", instanceId)
                .then((subscriptionHandle) => {
                    newTarget.serverSubscriptionHandle = subscriptionHandle;
                    if (newTarget.subscribers.length === 0) {
                        this.webSocket?.send("removeVuSubscription", subscriptionHandle);
                    } else {
                        newTarget.serverSubscriptionHandle = subscriptionHandle;
                    }
                });

            this.vuSubscriptions[instanceId] = newTarget;
        }
        return result;
    }

    removeVuSubscription(handle: VuSubscriptionHandle): void {
        let handleImpl = handle as VuSubscriptionHandleImpl;

        let item = this.vuSubscriptions[handleImpl.instanceId];
        if (item) {
            for (let i = 0; i < item.subscribers.length; ++i) {
                if (item.subscribers[i] === handleImpl) {
                    item.subscribers.splice(i, 1);
                    if (item.subscribers.length === 0) {
                        this.vuSubscriptions[handleImpl.instanceId] = undefined;
                        if (item.serverSubscriptionHandle) {
                            this.webSocket?.send("removeVuSubscription", item.serverSubscriptionHandle);
                            item.serverSubscriptionHandle = undefined;
                        }
                    }
                }
            }
        }

    }
    private isClosed = false;
    close() {
        if (!this.isClosed) {
            this.isClosed = true;
            this.webSocket?.close();
            this.webSocket = undefined;
        }
    }
    setPatchProperty(instanceId: number, uri: string, value: any): Promise<boolean> {
        let result = new Promise<boolean>((resolve, reject) => {
            if (this.webSocket) {
                this.webSocket.request<boolean>(
                    "setPatchProperty",
                    { instanceId: instanceId, propertyUri: uri, value: value }
                ).then((result) => {
                    resolve(true);
                }).catch((message) => {
                    reject(message);
                });
            }
            else {
                reject("Socket closed.");
            }
        });
        return result;
    }
    getPatchProperty<Type = any>(instanceId: number, uri: string): Promise<Type> {
        let result = new Promise<Type>((resolve, reject) => {
            if (!this.webSocket) {
                reject("Socket closed.");
            } else {
                this.webSocket.request<Type>("getPatchProperty", { instanceId: instanceId, propertyUri: uri })
                    .then((data) => {
                        resolve(data);
                    })
                    .catch((error) => {
                        reject(error + " (" + uri + ")");
                    });
            }
        });
        return result;
    }
    openBank(bankId: number): Promise<void> {
        let result = new Promise<void>((resolve, reject) => {
            if (this.webSocket) {
                this.webSocket.request<void>("openBank", bankId)
                    .then(() => {
                        resolve();

                    })
                    .catch((error) => {
                        reject(error);

                    });
            }
        });
        return result;
    }
    renameBank(bankId: number, newName: string): Promise<void> {
        let result = new Promise<void>((resolve, reject) => {
            if (!this.webSocket) {
                reject("No server connection.");
            } else {
                this.webSocket.request<void>("renameBank", { bankId: bankId, newName: newName })
                    .then(() => {
                        resolve();
                    })
                    .catch(error => reject(error));
            }
        });
        return result;
    }
    saveBankAs(bankId: number, newName: string): Promise<number> {
        let result = new Promise<number>((resolve, reject) => {
            if (!this.webSocket) {
                reject("No server connection.");
            } else {
                this.webSocket.request<number>("saveBankAs", { bankId: bankId, newName: newName })
                    .then((newInstanceId) => {
                        resolve(newInstanceId);
                    })
                    .catch(error => reject(error));
            }
        });
        return result;
    }

    getJackStatus(): Promise<JackHostStatus> {
        let result = new Promise<JackHostStatus>((resolve, reject) => {
            if (!this.webSocket) {
                reject("No connection to server.");
            } else {
                this.webSocket.request<any>("getJackStatus")
                    .then((data) => {
                        resolve(new JackHostStatus().deserialize(data));
                    })
                    .catch(error => reject(error));
            }
        });
        return result;
    }

    presetCache: { [uri: string]: PluginUiPresets } = {};


    uncachePluginPreset(pluginUri: string): void {
        delete this.presetCache[pluginUri];
    }
    getPluginPresets(uri: string): Promise<PluginUiPresets> {
        return new Promise<PluginUiPresets>((resolve, reject) => {
            if (this.presetCache[uri] !== undefined) {
                resolve(this.presetCache[uri]);
            } else {
                this.webSocket!.request<any>("getPluginPresets", uri)
                    .then((result) => {
                        let presets = new PluginUiPresets().deserialize(result);
                        this.presetCache[uri] = presets;
                        resolve(presets);
                    })
                    .catch((error) => {
                        reject(error);
                    })
            }
        });
    }

    loadPluginPreset(pluginInstanceId: number, presetInstanceId: number): void {
        this.webSocket?.send("loadPluginPreset", { pluginInstanceId: pluginInstanceId, presetInstanceId: presetInstanceId });
    }

    handlePluginPresetsChanged(pluginUri: string): void {
        this.uncachePluginPreset(pluginUri);
        this.firePluginPresetsChanged(pluginUri);
    }


    updatePluginPresets(pluginUri: string, presets: PluginUiPresets): Promise<void> {
        this.presetCache[pluginUri] = presets;
        this.firePluginPresetsChanged(pluginUri);
        return nullCast(this.webSocket).request<void>("updatePluginPresets", presets);
    }
    duplicatePluginPreset(pluginUri: string, instanceId: number): Promise<number> {
        return nullCast(this.webSocket).request<number>("copyPluginPreset", {
            pluginUri: pluginUri, instanceId: instanceId
        });

    }


    shutdown(): Promise<void> {
        return this.webSocket!.request<void>("shutdown");
    }
    restart(): Promise<void> {
        return this.webSocket!.request<void>("restart");

    }
    getJackServerSettings(): Promise<JackServerSettings> {
        return this.webSocket!.request<any>("getJackServerSettings")
            .then((data) => {
                return new JackServerSettings().deserialize(data);
            });
    }
    setJackServerSettings(jackServerSettings: JackServerSettings): void {
        this.webSocket?.request<void>("setJackServerSettings", jackServerSettings)
            .catch((error) => {
                this.showAlert(error);
            });
    }

    updateVst3State(pedalboard: Pedalboard) {
        // let it = pedalboard.itemsGenerator();
        // while (true) {
        //     let v = it.next();
        //     if (v.done) break;
        //     let item = v.value;
        // }
    }

    setMidiBinding(instanceId: number, midiBinding: MidiBinding): void {
        let pedalboard = this.pedalboard.get();
        if (!pedalboard) {
            throw new PiPedalStateError("Pedalboard not loaded.");
        }
        let newPedalboard = pedalboard.clone();
        this.updateVst3State(newPedalboard);
        if (newPedalboard.setMidiBinding(instanceId, midiBinding)) {
            this.setModelPedalboard(newPedalboard);


            // notify the server.
            // TODO: a more efficient notification.
            this.updateServerPedalboard()

        }
    }
    setSystemMidiBinding(instanceId: number, midiBinding: MidiBinding): void {
        let currentBindings = this.systemMidiBindings.get();

        let result: MidiBinding[] = [];

        for (var binding of currentBindings) {
            if (binding.symbol === midiBinding.symbol) {
                result.push(midiBinding);
            } else {
                result.push(binding);
            }
        }
        this.systemMidiBindings.set(result);
        this.webSocket?.send("setSystemMidiBindings", result);
    }
    private midiListeners: MidiEventListener[] = [];
    private monitorPatchPropertyListeners: PatchPropertyListenerItem[] = [];

    nextListenHandle = 1;
    listenForMidiEvent(listenForControl: boolean, onComplete: (isNote: boolean, noteOrControl: number) => void): ListenHandle {
        let handle = this.nextListenHandle++;

        this.midiListeners.push(new MidiEventListener(handle, onComplete));

        this.webSocket?.send("listenForMidiEvent", { listenForControls: listenForControl, handle: handle });
        return {
            _handle: handle
        };

    }

    monitorPatchProperty(
        instanceId: number,
        propertyUri: string,
        onReceived: PatchPropertyListener
    ): ListenHandle {
        let handle = this.nextListenHandle++;

        this.monitorPatchPropertyListeners.push(new PatchPropertyListenerItem(handle, instanceId, onReceived));

        this.webSocket?.send("monitorPatchProperty", { clientHandle: handle, instanceId: instanceId, propertyUri: propertyUri, });
        return {
            _handle: handle
        };
    }
    cancelMonitorPatchProperty(listenHandle: ListenHandle): void {
        for (let i = 0; i < this.monitorPatchPropertyListeners.length; ++i) {
            if (this.monitorPatchPropertyListeners[i].handle === listenHandle._handle) {
                this.monitorPatchPropertyListeners.splice(i, 1);
                this.webSocket?.send("cancelMonitorPatchProperty", listenHandle._handle);
                break;
            }
        }
    }

    private handleNotifyPathPatchPropertyChanged(
        instanceId: number, propertyUri: string, jsonObjectString: string
    ) {
        let pedalboard = this.pedalboard.get();
        let pedalboardItem = pedalboard.getItem(instanceId);
        if (pedalboardItem) {
            pedalboardItem.pathProperties[propertyUri] = jsonObjectString;
        }
        // No NOT notify monitorPatchProperty listeners, because they extpect objects not strings, 
        // AND they will a NotifyPatchPropertychanged message anyway.

    }
    private handleNotifyPatchProperty(clientHandle: number, instanceId: number, propertyUri: string, jsonObject: any) {
        let pedalboard = this.pedalboard.get();
        let pedalboardItem = pedalboard.getItem(instanceId);
        if (pedalboardItem) {
            pedalboardItem.pathProperties[propertyUri] = JSON.stringify(jsonObject);
        }
        for (let i = 0; i < this.monitorPatchPropertyListeners.length; ++i) {
            let listener = this.monitorPatchPropertyListeners[i];
            if (listener.handle === clientHandle && listener.instanceId === instanceId) {
                listener.callback(instanceId, propertyUri, jsonObject);
            }
        }
    }


    handleNotifyMidiListener(clientHandle: number, isNote: boolean, noteOrControl: number) {
        for (let i = 0; i < this.midiListeners.length; ++i) {
            let listener = this.midiListeners[i];
            if (listener.handle === clientHandle) {
                listener.callback(isNote, noteOrControl);

            }
        }
    }
    cancelListenForMidiEvent(listenHandle: ListenHandle): void {
        let found = false;
        for (let i = 0; i < this.midiListeners.length; ++i) {
            if (this.midiListeners[i].handle === listenHandle._handle) {
                this.midiListeners.splice(i, 1);
                found = true;
                break;
            }
        }
        if (!found) {
            console.log('cancelListenForMidiEvent: event not found.');
        }
        this.webSocket?.send("cancelListenForMidiEvent", listenHandle._handle);
    }

    downloadAudioFile(filePath: string) {
        let downloadUrl = this.varServerUrl + "downloadMediaFile?path=" + encodeURIComponent(filePath);

        // download with no flashing temporary tab.
        let link = window.document.createElement("A") as HTMLAnchorElement;
        link.href = downloadUrl;
        link.target = "_blank";
        link.setAttribute("download", pathFileName(filePath));
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    }

    download(targetType: string, instanceId: number | string): void {
        if (instanceId === -1) return;
        let url = this.varServerUrl + targetType + "?id=" + instanceId;

        // window.open(url, "_blank");
        // download with no flashing temporary tab.
        let link = window.document.createElement("A") as HTMLAnchorElement;
        link.href = url;
        link.target = "_blank";
        link.download = "download.piPreset";    
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    }

    uploadUserFile(uploadPage: string, file: File, contentType: string = "application/octet-stream", abortController?: AbortController): Promise<string> {
        let result = new Promise<string>((resolve, reject) => {
            try {
                if (file.size > this.maxFileUploadSize) {
                    reject("File is too large.");
                }
                let url = this.varServerUrl + uploadPage;
                let parsedUrl = new URL(url);
                let fileNameOnly = file.name.split('/').pop()?.split('\\')?.pop();
                let query = parsedUrl.search;
                if (query.length === 0) {
                    query += '?';
                } else {
                    query += '&';
                }
                if (!fileNameOnly) {
                    reject("Invalid filename.");
                }
                query += "filename=" + encodeURIComponent(fileNameOnly ?? "");
                parsedUrl.search = query;
                url = parsedUrl.toString();
                fetch(
                    url,
                    {
                        method: "POST",
                        body: file,
                        headers: {
                            'Content-Type': contentType,
                        },
                        signal: abortController?.signal
                    }
                )
                    .then((response: Response) => {
                        if (!response.ok) {
                            reject("Upload failed. " + response.statusText);
                            return;
                        } else {
                            return response.json();
                        }
                    })
                    .then((json) => {
                        let response = json as { errorMessage: string, path: string };
                        if (response.errorMessage !== "") {
                            throw new Error(response.errorMessage);
                        }
                        resolve(response.path);
                    })
                    .catch((error) => {
                        if (error instanceof Error) {
                            reject("Upload failed. " + (error as Error).message);
                        } else {
                            reject("Upload failed. " + error);
                        }
                    })
                    ;
            } catch (error) {
                reject("Upload failed. " + error);
            }
        });
        return result;
    }

    uploadPreset(uploadPage: string, file: File, uploadAfter: number): Promise<number> {
        let result = new Promise<number>((resolve, reject) => {
            try {
                if (file.size > this.maxPresetUploadSize) {
                    reject("File is too large.");
                }
                let url = this.varServerUrl + uploadPage;
                if (uploadAfter && uploadAfter !== -1) {
                    url += "?uploadAfter=" + uploadAfter;
                }
                fetch(
                    url,
                    {
                        method: "POST",
                        body: file,
                        headers: {
                            'Content-Type': 'application/json',
                        },
                    }
                )
                    .then((response: Response) => {
                        if (!response.ok) {
                            reject("Upload failed. " + response.statusText);
                            return;
                        } else {
                            return response.json(); // read the empty body to keep the connection alive.
                        }
                    })
                    .then((json) => {
                        resolve(json as number);
                    })
                    .catch((error) => {
                        reject("Upload failed. " + error.toString());
                    })
                    ;
            } catch (error) {
                reject("Upload failed. " + error);
            }
        });
        return result;
    }
    uploadBank(file: File, uploadAfter: number): Promise<number> {
        let result = new Promise<number>((resolve, reject) => {
            try {
                console.log("File: " + file.name + " Size: " + file.size);
                if (file.size > this.maxPresetUploadSize) {
                    reject("File is too large.");
                }
                let url = this.varServerUrl + "uploadBank";
                if (uploadAfter && uploadAfter !== -1) {
                    url += "?uploadAfter=" + uploadAfter;
                }
                fetch(
                    url,
                    {
                        method: "POST",
                        body: file,
                        headers: {
                            'Content-Type': 'application/json',
                        },
                    }
                )
                    .then((response: Response) => {
                        if (!response.ok) {
                            reject("Upload failed. " + response.statusText);
                            return;
                        } else {
                            return response.json(); // read the empty body to keep the connection alive.
                        }
                    })
                    .then((json) => {
                        resolve(json as number);
                    })
                    .catch((error) => {
                        reject("Upload failed. " + error);
                    })
                    ;
            } catch (error) {
                reject("Upload failed. " + error);
            }
        });
        return result;
    }
    setGovernorSettings(governor: string): Promise<void> {
        let newSettings = this.governorSettings.get().clone();
        newSettings.governor = governor;
        this.governorSettings.set(newSettings);

        return new Promise<void>((resolve, reject) => {

            let ws = this.webSocket;
            if (!ws) {
                resolve();
                return;
            }
            ws.request<void>(
                "setGovernorSettings",
                governor
            )
                .then(() => {
                    resolve();
                })
                .catch((err) => {
                    reject(err);
                });
        });
    }
    createNewSampleDirectory(relativePath: string, uiFileProperty: UiFileProperty): Promise<string> {
        return new Promise<string>((resolve, reject) => {

            let ws = this.webSocket;
            if (!ws) {
                resolve("");
                return;
            }
            ws.request<string>(
                "createNewSampleDirectory",
                {
                    relativePath: relativePath,
                    uiFileProperty: uiFileProperty
                }
            )
                .then((newPath) => {
                    resolve(newPath);
                })
                .catch((err) => {
                    reject(err);
                });
        });
    }

    getFilePropertyDirectoryTree(uiFileProperty: UiFileProperty, selectedPath: string): Promise<FilePropertyDirectoryTree> {
        return new Promise<FilePropertyDirectoryTree>((resolve, reject) => {
            let ws = this.webSocket;
            if (!ws) {
                resolve(new FilePropertyDirectoryTree());
                return;
            }
            ws.request<FilePropertyDirectoryTree>(
                "getFilePropertyDirectoryTree",
                { fileProperty: uiFileProperty, selectedPath: selectedPath }
            ).then((result) => {
                resolve(new FilePropertyDirectoryTree().deserialize(result));
            }).catch((e) => {
                reject(e);
            });
        });
    }
    renameFilePropertyFile(oldRelativePath: string, newRelativePath: string, uiFileProperty: UiFileProperty): Promise<string> {
        return new Promise<string>((resolve, reject) => {

            let ws = this.webSocket;
            if (!ws) {
                resolve("");
                return;
            }
            ws.request<string>(
                "renameFilePropertyFile",
                {
                    oldRelativePath: oldRelativePath,
                    newRelativePath: newRelativePath,
                    uiFileProperty: uiFileProperty
                }
            )
                .then((newPath) => {
                    resolve(newPath);
                })
                .catch((err) => {
                    reject(err);
                });
        });
    }
    copyFilePropertyFile(
        oldRelativePath: string, 
        newRelativePath: string, 
        uiFileProperty: UiFileProperty,
        overwrite: boolean
    ): Promise<string> {
        return new Promise<string>((resolve, reject) => {

            let ws = this.webSocket;
            if (!ws) {
                resolve("");
                return;
            }
            ws.request<string>(
                "copyFilePropertyFile",
                {
                    oldRelativePath: oldRelativePath,
                    newRelativePath: newRelativePath,
                    uiFileProperty: uiFileProperty,
                    overwrite: overwrite
                }
            )
                .then((newPath) => {
                    resolve(newPath);
                })
                .catch((err) => {
                    reject(err);
                });
        });
    }

    setWifiConfigSettings(wifiConfigSettings: WifiConfigSettings): Promise<void> {
        let result = new Promise<void>((resolve, reject) => {
            let oldSettings = this.wifiConfigSettings.get();
            wifiConfigSettings = wifiConfigSettings.clone();

            if ((!oldSettings.isEnabled()) && (!wifiConfigSettings.isEnabled())) {
                // no effective change.
                resolve();
                return;
            }
            if (!wifiConfigSettings.isEnabled()) {
                wifiConfigSettings.hasPassword = false;
                wifiConfigSettings.password = "";
            } else {
                if (wifiConfigSettings.hasPassword) {
                    wifiConfigSettings.hasSavedPassword = true;
                }
            }
            // save a  version for the server (potentially carrying a password)
            let serverConfigSettings: WifiConfigSettings;
            if (wifiConfigSettings.isEnabled()) {
                serverConfigSettings = wifiConfigSettings.clone();
            } else {
                // avoid leaking edits to the server
                serverConfigSettings = oldSettings.clone();
                serverConfigSettings.autoStartMode = wifiConfigSettings.autoStartMode;
                wifiConfigSettings = oldSettings.clone();
                wifiConfigSettings.autoStartMode = 0;
            }

            wifiConfigSettings.hasPassword = false;
            wifiConfigSettings.password = "";
            this.wifiConfigSettings.set(wifiConfigSettings);



            // notify the server.
            let ws = this.webSocket;
            if (!ws) {
                reject("Not connected.");
                return;
            }
            ws.request<void>(
                "setWifiConfigSettings",
                serverConfigSettings
            )
                .then(() => {
                    resolve();
                })
                .catch((err) => {
                    reject(err);
                });

        });
        return result;
    }
    setWifiDirectConfigSettings(wifiDirectConfigSettings: WifiDirectConfigSettings): Promise<void> {
        let result = new Promise<void>((resolve, reject) => {
            let oldSettings = this.wifiDirectConfigSettings.get();
            wifiDirectConfigSettings = wifiDirectConfigSettings.clone();

            if ((!oldSettings.enable) && (!wifiDirectConfigSettings.enable)
                && (oldSettings.hotspotName === wifiDirectConfigSettings.hotspotName)
            ) {
                // no effective change.
                resolve();
                return;
            }
            if (!wifiDirectConfigSettings.enable) {
                let t = wifiDirectConfigSettings.hotspotName; // hotspot name can be changed when disabled because it's also the mDNS service name.
                wifiDirectConfigSettings = oldSettings.clone();
                wifiDirectConfigSettings.enable = false;
                wifiDirectConfigSettings.hotspotName = t;
            } else {
                if (wifiDirectConfigSettings.countryCode === oldSettings.countryCode
                    && wifiDirectConfigSettings.channel === oldSettings.channel
                    && wifiDirectConfigSettings.hotspotName === oldSettings.hotspotName
                    && wifiDirectConfigSettings.enable === oldSettings.enable) {
                    if (wifiDirectConfigSettings.pin === oldSettings.pin) {
                        // no effective change.
                        resolve();
                        return;
                    }
                }
            }
            // save a  version for the server (potentially carrying a password)
            let serverConfigSettings = wifiDirectConfigSettings.clone();
            this.wifiDirectConfigSettings.set(wifiDirectConfigSettings);



            // notify the server.
            let ws = this.webSocket;
            if (!ws) {
                reject("Not connected.");
                return;
            }
            ws.request<void>(
                "setWifiDirectConfigSettings",
                serverConfigSettings
            )
                .then(() => {
                    //resolve();
                })
                .catch((err) => {
                    //resolve();
                });
            resolve();

        });
        this.expectDisconnect(ReconnectReason.LoadingSettings);
        // this.webSocket?.reconnect(); // avoid races by letting the server do it for us.
        return result;
    }


    getKnownWifiNetworks(): Promise<string[]> {
        let result = new Promise<string[]>((resolve, reject) => {
            if (!this.webSocket) {
                reject("Connection closed.");
                return;
            }
            this.webSocket.request<string[]>("getKnownWifiNetworks")
                .then((data) => {
                    resolve(data);
                })
                .catch((err) => reject(err));
        });
        return result;

    }


    getWifiChannels(countryIso3661: string): Promise<WifiChannel[]> {
        let result = new Promise<WifiChannel[]>((resolve, reject) => {
            if (!this.webSocket) {
                reject("Connection closed.");
                return;
            }
            this.webSocket.request<WifiChannel[]>("getWifiChannels", countryIso3661)
                .then((data) => {
                    resolve(WifiChannel.deserialize_array(data));
                })
                .catch((err) => reject(err));
        });
        return result;
    }

    getAlsaDevices(): Promise<AlsaDeviceInfo[]> {
        let result = new Promise<AlsaDeviceInfo[]>((resolve, reject) => {
            if (!this.webSocket) {
                reject("Connection closed.");
                return;
            }
            this.webSocket.request<any>("getAlsaDevices")
                .then((data) => {
                    resolve(AlsaDeviceInfo.deserialize_array(data));
                })
                .catch((err) => reject(err));
        });
        return result;

    }
    zoomUiControl(sourceElement: HTMLElement, instanceId: number, uiControl: UiControl): void {
        let name = uiControl.name;
        if (uiControl.port_group !== "") {
            let pedalboard = this.pedalboard.get();
            if (pedalboard) {
                let plugin = pedalboard.getItem(instanceId);
                let uiPlugin = this.getUiPlugin(plugin.uri);
                if (uiPlugin) {
                    for (let i = 0; i < uiPlugin.port_groups.length; ++i) {
                        if (uiPlugin.port_groups[i].symbol === uiControl.port_group) {
                            name = uiPlugin.port_groups[i].name + " / " + name;
                            break;
                        }
                    }
                }
            }
        }
        this.zoomedUiControl.set({ source: sourceElement, name: name, instanceId: instanceId, uiControl: uiControl });
    }
    onPreviousZoomedControl(): void {
        let currentUiControl = this.zoomedUiControl.get();
        if (!currentUiControl) return;

        let currentSymbol = currentUiControl.uiControl.symbol;

        let pedalboard = this.pedalboard.get();
        if (!pedalboard) return;

        let pedalboardItem = pedalboard.getItem(currentUiControl.instanceId);

        let uiPlugin = this.getUiPlugin(pedalboardItem.uri);
        if (!uiPlugin) return;

        let i = 0;
        let ix = -1;
        for (i = 0; i < uiPlugin.controls.length; ++i) {
            if (uiPlugin.controls[i].symbol === currentSymbol) {
                ix = i;
                break;
            }
        }
        if (ix === -1) return;

        ++ix;
        if (ix >= uiPlugin.controls.length) return;
        while (!uiPlugin.controls[ix].is_input) {
            ++ix;
            if (ix >= uiPlugin.controls.length) return;
        }

        this.zoomUiControl(currentUiControl.source, currentUiControl.instanceId, uiPlugin.controls[ix]);
    }
    onNextZoomedControl(): void {
        let currentUiControl = this.zoomedUiControl.get();
        if (!currentUiControl) return;

        let currentSymbol = currentUiControl.uiControl.symbol;

        let pedalboard = this.pedalboard.get();
        if (!pedalboard) return;

        let pedalboardItem = pedalboard.getItem(currentUiControl.instanceId);

        let uiPlugin = this.getUiPlugin(pedalboardItem.uri);
        if (!uiPlugin) return;

        let i = 0;
        let ix = -1;
        for (i = 0; i < uiPlugin.controls.length; ++i) {
            if (uiPlugin.controls[i].symbol === currentSymbol) {
                ix = i;
                break;
            }
        }
        if (ix === -1) return;

        --ix;
        if (ix < 0) return;
        while (!uiPlugin.controls[ix].is_input) {
            --ix;
            if (ix < 0) return;
        }

        this.zoomUiControl(currentUiControl.source, currentUiControl.instanceId, uiPlugin.controls[ix]);
    }

    clearZoomedControl(): void {
        this.zoomedUiControl.set(undefined);
    }

    setFavorite(pluginUrl: string, isFavorite: boolean): void {
        let favorites = this.favorites.get();
        let newFavorites: FavoritesList = {};
        Object.assign(newFavorites, favorites);
        if (isFavorite) {
            newFavorites[pluginUrl] = true;
        } else {
            if (newFavorites[pluginUrl]) {
                delete newFavorites[pluginUrl];
            }
        }
        this.favorites.set(newFavorites);
        if (this.webSocket) {
            this.webSocket.send("setFavorites", newFavorites);
        }
        // stub: update server.
    }


    setUpdatePolicy(updatePolicy: UpdatePolicyT): void {
        let iPolicy = updatePolicy as number;
        if (this.webSocket) {
            this.webSocket.send("setUpdatePolicy", iPolicy);
        }

    }
    forceUpdateCheck() {
        if (this.webSocket) {
            this.webSocket.send("forceUpdateCheck");
        }

    }

    preloadImages(imageList: string): void {
        let imageNames = imageList.split(';');
        for (let i = 0; i < imageNames.length; ++i) {
            let imageName = imageNames[i];
            let img = new Image();
            img.src = "/img/" + imageName;
        }
    }

    isAndroidHosted(): boolean { return this.androidHost !== undefined; }

    showAndroidDonationActivity(): void {
        if (this.androidHost) {
            this.androidHost.showSponsorship();
        }
    }
    getAndroidHostVersion(): string {
        if (this.androidHost) {
            return this.androidHost.getHostVersion();
        }
        return "";
    }
    chooseNewDevice(): void {
        if (this.androidHost) {
            return this.androidHost.chooseNewDevice();
        }
    }

    // returns the ID of the new preset.
    newPresetItem(createAfter: number): Promise<number> {
        return nullCast(this.webSocket).request<number>("newPreset");
    }

    getTheme(): ColorTheme {
        return getColorScheme();
    }

    setTheme(value: ColorTheme) {

        if (this.getTheme() !== value) {
            setColorScheme(value);
            setTimeout(() => {
                this.reloadPage();
            },
                200);
        }
    }


    reloadPage() {
        // eslint-disable-next-line no-restricted-globals
        let url = window.location.href.split('#')[0];
        window.location.href = url;
        //window.location.reload();
    }

    private networkChanging_expectHotspot = false;
    private expectNetworkChangeTimeout?: number = undefined;
    private hotspotReconnectTimer?: number = undefined;

    async detectServer(address: string) {
        let port = window.location.port;
        let newUrl = new URL("http://" + address + ":" + port + "/manifest.json");

        try {
            let response = await fetch(newUrl);
            if (response.ok) {
                return "http://" + address + ":" + port;
            }
            return "";
        } catch (error: any) {
            return "";
        }
    }
    async pollForLiveServer() {
        let wifiConfigSettings = this.wifiConfigSettings.get();
        if (wifiConfigSettings.mdnsName.length !== 0) {
            let newUrl = await this.detectServer(wifiConfigSettings.mdnsName);
            if (newUrl.length !== 0) {
                return newUrl;
            }
        }
        if (this.networkChanging_expectHotspot) {
            let newUrl = await this.detectServer("10.40.0.1");
            if (newUrl.length !== 0) {
                return newUrl;
            }
        }
        return "";
    }
    cancelHotspotReconnectTimer() {
        if (this.hotspotReconnectTimer) {
            clearTimeout(this.hotspotReconnectTimer);
        }

    }
    startHotspotReconnectTimer() {
        // poll for access to a running pipedal server
        this.hotspotReconnectTimer = setTimeout(
            async () => {
                let newUrl = await this.pollForLiveServer();
                if (newUrl.length === 0) {
                    this.startHotspotReconnectTimer();
                } else {
                    this.cancelOnNetworkChanging();
                    window.location.replace(newUrl);
                }
            },
            5 * 1000);

    }
    cancelOnNetworkChanging() {
        this.cancelHotspotReconnectTimer();
        if (this.expectNetworkChangeTimeout) {
            clearTimeout(this.expectNetworkChangeTimeout);
            this.expectNetworkChangeTimeout = undefined;
        }
        this.expectDisconnect(ReconnectReason.Disconnected);
    }
    onNetworkChanging(hotspotConnected: boolean) {
        this.cancelOnNetworkChanging();

        this.networkChanging_expectHotspot = hotspotConnected;
        this.expectDisconnect(ReconnectReason.HotspotChanging);

        this.expectNetworkChangeTimeout = setTimeout(
            () => {
                this.cancelOnNetworkChanging();
            },
            30 * 1000);

    }
    setPedalboardItemTitle(instanceId: number, title: string): void {
        let pedalboard = this.pedalboard.get();
        if (!pedalboard) {
            throw new PiPedalStateError("Pedalboard not loaded.");
        }
        let newPedalboard = pedalboard.clone();
        this.updateVst3State(newPedalboard);
        let item = newPedalboard.getItem(instanceId);
        if (item.title === title) {
            return;
        }
        item.title = title;
        this.pedalboard.set(newPedalboard);
        // notify the server.
        // xxx;
    }
};

let instance: PiPedalModel | undefined = undefined;



export class PiPedalModelFactory {
    static getInstance(): PiPedalModel {
        if (instance === undefined) {
            let impl: PiPedalModel = new PiPedalModel();
            instance = impl;

            impl.initialize();
        }
        return instance;

    }
};



