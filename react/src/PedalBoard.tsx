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

import { PiPedalArgumentError } from './PiPedalError';
import MidiBinding from './MidiBinding';


const SPLIT_PEDALBOARD_ITEM_URI = "uri://two-play/pipedal/pedalboard#Split";
const EMPTY_PEDALBOARD_ITEM_URI = "uri://two-play/pipedal/pedalboard#Empty";


interface Deserializable<T> {
    deserialize(input: any): T;
}

export class ControlValue implements Deserializable<ControlValue> {
    constructor(key?: string, value?: number)
    {
        this.key = key??"";
        this.value = value?? 0;
    }
    deserialize(input: any): ControlValue {
        this.key = input.key;
        this.value = input.value;
        return this;
    }
    static EmptyArray: ControlValue[] = [];

    static deserializeArray(input: any[]): ControlValue[] {
        let result: ControlValue[] = [];
        for (let i = 0; i < input.length; ++i) {
            result[i] = new ControlValue().deserialize(input[i]);
        }
        return result;
    }
    setValue(value: number) {
        this.value = value;
    }

    key: string;
    value: number;

}

export class PedalBoardItem implements Deserializable<PedalBoardItem> {
    deserializePedalBoardItem(input: any): PedalBoardItem {
        this.instanceId = input.instanceId ?? -1;
        this.uri = input.uri;
        this.pluginName = input.pluginName;
        this.isEnabled = input.isEnabled;
        this.midiBindings = MidiBinding.deserialize_array(input.midiBindings);

        this.controlValues = ControlValue.deserializeArray(input.controlValues);
        return this;
    }
    deserialize(input: any): PedalBoardItem {
        return this.deserializePedalBoardItem(input);
    }
    static deserializeArray(input: any): PedalBoardItem[] {
        let result: PedalBoardItem[] = [];
        for (let i = 0; i < input.length; ++i) {
            let inputItem: any = input[i];
            let uri: string = inputItem.uri as string;
            let outputItem: PedalBoardItem;

            if (uri === SPLIT_PEDALBOARD_ITEM_URI) {
                outputItem = new PedalBoardSplitItem().deserialize(inputItem);
            } else {
                outputItem = new PedalBoardItem().deserialize(inputItem);
            }
            result[i] = outputItem;

        }
        return result;
    }

    getInstanceId() : number {
        if (this.instanceId === undefined)
        {
            throw new PiPedalArgumentError("Item does not have an id.");
        }
        return this.instanceId;
    }
    isEmpty(): boolean {
        return this.uri === EMPTY_PEDALBOARD_ITEM_URI;
    }
    isSplit(): boolean {
        return this.uri === SPLIT_PEDALBOARD_ITEM_URI;
    }

    getControl(key: string): ControlValue {
        for (let i = 0; i < this.controlValues.length; ++i) {
            let v = this.controlValues[i];
            if (v.key === key) {
                return v;
            }
        }
        throw new PiPedalArgumentError("Invalid key.");

    }

    getControlValue(key: string): number {
        for (let i = 0; i < this.controlValues.length; ++i) {
            let v = this.controlValues[i];
            if (v.key === key) {
                return v.value;
            }
        }
        return 0;
    }
    setControlValue(key: string, value: number): boolean {
        for (let i = 0; i < this.controlValues.length; ++i) {
            let v = this.controlValues[i];
            if (v.key === key) {
                if (v.value === value) return false;
                v.setValue(value);
                return true;
            }
        }
        return false;
    }
    setMidiBinding(midiBinding: MidiBinding): boolean {
        if (this.midiBindings)
        {
            for (let i = 0; i < this.midiBindings.length; ++i)
            {
                let binding = this.midiBindings[i];
                if (midiBinding.symbol === binding.symbol)
                {
                    if (binding.equals(midiBinding))
                    {
                        return false;
                    }
                    this.midiBindings.splice(i,1,midiBinding);
                    return true;
                }
            }
            this.midiBindings.push(midiBinding);
            return true;
        }
        this.midiBindings = [ midiBinding];
        return true;
    }
    getMidiBinding(symbol: string): MidiBinding {
        if (this.midiBindings)
        {
            for (let i = 0; i < this.midiBindings.length; ++i)
            {
                let midiBinding = this.midiBindings[i];
                if (midiBinding.symbol === symbol)
                {
                    return midiBinding;
                }
            }
        }
        let result = new MidiBinding();
        result.symbol = symbol;
        return result;
    }


    static EmptyArray: PedalBoardItem[] = [];

    instanceId: number = -1;
    isEnabled: boolean = false;
    uri: string = "";
    pluginName?: string;
    controlValues: ControlValue[] = ControlValue.EmptyArray;
    midiBindings: MidiBinding[] = [];
};


export enum SplitType {
    Ab = 0,
    Mix = 1,
    Lr = 2
}



export class PedalBoardSplitItem extends PedalBoardItem {
    static PANL_KEY: string = "panL";
    static PANR_KEY: string = "panR";
    static VOLL_KEY: string = "volL";
    static VOLR_KEY: string = "volR";
    
    static MIX_KEY: string = "mix";
    static TYPE_KEY: string = "splitType";
    static SELECT_KEY: string = "select";


    deserialize(input: any): PedalBoardSplitItem {
        this.deserializePedalBoardItem(input);
        this.topChain = PedalBoardItem.deserializeArray(input.topChain);
        this.bottomChain = PedalBoardItem.deserializeArray(input.bottomChain);

        return this;
    }
    getSplitType(): SplitType {
        let rawValue = this.getControlValue(PedalBoardSplitItem.TYPE_KEY);

        if (rawValue <  1) return SplitType.Ab;
        if (rawValue < 2) return SplitType.Mix;
        return SplitType.Lr;
    }
    getToggleAbControlValue(): ControlValue {
        let cv = new ControlValue();
        cv.key = "select";
        cv.value = this.isASelected() ? 1 : 0;
        return cv;

    }
    getMixControl(): ControlValue {
        return this.getControl(PedalBoardSplitItem.MIX_KEY);
    }
    getMix(): number {
        return this.getControlValue(PedalBoardSplitItem.MIX_KEY);
    }
    isASelected(): boolean {
        return this.getSplitType() !== SplitType.Ab ||  this.getControlValue(PedalBoardSplitItem.SELECT_KEY) === 0;
    }
    isBSelected(): boolean {
        return this.getSplitType() !== SplitType.Ab || this.getControlValue(PedalBoardSplitItem.SELECT_KEY) !== 0;
    }

    topChain: PedalBoardItem[] = PedalBoardItem.EmptyArray;
    bottomChain: PedalBoardItem[] = PedalBoardItem.EmptyArray;
}





export class PedalBoard implements Deserializable<PedalBoard> {

    deserialize(input: any): PedalBoard {
        this.name = input.name;
        this.items = PedalBoardItem.deserializeArray(input.items);
        this.nextInstanceId = input.nextInstanceId ?? -1;
        return this;
    }

    clone(): PedalBoard {
        return new PedalBoard().deserialize(this);
    }
    name: string = "";
    items: PedalBoardItem[] = [];
    nextInstanceId: number = -1;

    *itemsGenerator(): Generator<PedalBoardItem, void, undefined> {
        let it = itemGenerator_(this.items);
        while (true)
        {
            let v = it.next();
            if (v.done) break;
            yield v.value;
        }
    }

    hasItem(instanceId: number): boolean
    {
        let it = this.itemsGenerator();
        while (true)
        {
            let v = it.next();
            if (v.done) break;
            if (v.value.instanceId === instanceId)
            {
                return true;
            }
        }
        return false;

    }

    getFirstSelectableItem(): number
    {
        if (this.items.length !== 0)
        {
            return this.items[0].instanceId;
        }
        return -1;

    }

    maybeGetItem(instanceId: number): PedalBoardItem | null{
        let it = this.itemsGenerator();
        while (true)
        {
            let v = it.next();
            if (v.done) break;
            if (v.value.instanceId === instanceId)
            {
                return v.value;
            }
        }
        return null;
    }
    getItem(instanceId: number): PedalBoardItem {
        let it = this.itemsGenerator();
        while (true)
        {
            let v = it.next();
            if (v.done) break;
            if (v.value.instanceId === instanceId)
            {
                return v.value;
            }
        }
        throw new PiPedalArgumentError("Item not found.");
    }
    deleteItem_(instanceId: number,items: PedalBoardItem[]): number | null
    {
        for (let i = 0; i < items.length; ++i)
        {
            let item = items[i];
            if (item.instanceId === instanceId)
            {
                if (items.length > 1) {
                    items.splice(i,1);
                    let nextSelectedItem = i;
                    if (i >= items.length) --nextSelectedItem;
                    return items[nextSelectedItem].instanceId;
                } else {
                    // replace with an empty item.
                    let newItem = this.createEmptyItem();
                    items[i] = newItem;
                    return newItem.instanceId;

                }
            } else {
                if (item.isSplit())
                {
                    let splitItem = item as PedalBoardSplitItem;
                    let t = this.deleteItem_(instanceId,splitItem.topChain);
                    if (t != null) return t;

                    t = this.deleteItem_(instanceId,splitItem.bottomChain);
                    if (t != null) return t;
                }
            }
        }
        return null;
    }

    canDeleteItem_(instanceId: number,items: PedalBoardItem[]): boolean
    {
        for (let i = 0; i < items.length; ++i)
        {
            let item = items[i];
            if (item.instanceId === instanceId)
            {
                if (items.length > 1) return true;
                return !item.isEmpty(); // can delete if there's one non-empty item.
            }
            if (item.isSplit())
            {
                let splitItem = item as PedalBoardSplitItem;
                if (this.canDeleteItem_(instanceId,splitItem.topChain))
                {
                    return true;
                }
                if (this.canDeleteItem_(instanceId,splitItem.bottomChain))
                {
                    return true;
                }
            }
        }
        return false;
    }

    canDeleteItem(instanceId: number): boolean 
    {
        return this.canDeleteItem_(instanceId,this.items);
    }
    // Returns the next selected instanceId, or null if no deletion occurred.
    deleteItem(instanceId: number): number | null {
        return this.deleteItem_(instanceId,this.items);
    }

    setMidiBinding(instanceId: number, midiBinding: MidiBinding): boolean
    {
        let item = this.getItem(instanceId);
        if (!item) return false;
        return item.setMidiBinding(midiBinding);
    }
    addToStart(item: PedalBoardItem)
    {
        this.items.splice(0,0,item);
    }
    addToEnd(item: PedalBoardItem)
    {
        this.items.splice(this.items.length,0,item);
    }
    static _addRelative(items: PedalBoardItem[],newItem: PedalBoardItem, instanceId: number, addBefore: boolean): boolean
    {
        for (let i = 0; i < items.length; ++i)
        {
            let item = items[i];
            if (item.instanceId === instanceId)
            {
                if (addBefore)
                {
                    items.splice(i,0,newItem);
                } else {
                    items.splice(i+1,0,newItem);
                }
                return true;
            }
            if (item.isSplit())
            {
                let split = item as PedalBoardSplitItem;
                if (this._addRelative(split.topChain,newItem,instanceId,addBefore))
                {
                    return true;
                }
                if (this._addRelative(split.bottomChain,newItem,instanceId,addBefore))
                {
                    return true;
                }
            }
        }
        return false;

    }

    addBefore(item: PedalBoardItem, instanceId: number)
    {
        if (item.instanceId === instanceId) return;
        let result = PedalBoard._addRelative(this.items,item, instanceId, true);
        if (!result) {
            throw new PiPedalArgumentError("instanceId not found.");
        }

    }
    addAfter(item: PedalBoardItem, instanceId: number)
    {
        if (item.instanceId === instanceId) return;
        let result = PedalBoard._addRelative(this.items,item, instanceId, false);
        if (!result) {
            throw new PiPedalArgumentError("instanceId not found.");
        }

    }
    
    ensurePedalboardIds() {
        if (this.nextInstanceId === -1) {
            let minId = 1;
            let it = this.itemsGenerator();
            while (true) {
                let v = it.next();
                if (v.done) break;
                if (v.value.instanceId) {
                    let t = v.value.instanceId;
                    if (t+1 > minId) minId = t+1;
                }
    
            }
            this.nextInstanceId = minId;
        }
        let it = this.itemsGenerator();
        while (true) {
            let v = it.next();
            if (v.done) break;
            if (v.value.instanceId === -1) {
                v.value.instanceId = ++this.nextInstanceId;
    
            }
        }
    }
    createEmptySplit(): PedalBoardSplitItem {
        let result: PedalBoardSplitItem = new PedalBoardSplitItem();
        result.uri = SPLIT_PEDALBOARD_ITEM_URI;
        result.instanceId = ++this.nextInstanceId;
        result.pluginName = "";
        result.isEnabled = true;
        result.topChain = [ this.createEmptyItem()];
        result.bottomChain = [ this.createEmptyItem()];
        result.controlValues = [
            new ControlValue(PedalBoardSplitItem.TYPE_KEY, 0),
            new ControlValue(PedalBoardSplitItem.SELECT_KEY,0),
            new ControlValue(PedalBoardSplitItem.MIX_KEY,0),
            new ControlValue(PedalBoardSplitItem.PANL_KEY,0),
            new ControlValue(PedalBoardSplitItem.VOLL_KEY,-3),
            new ControlValue(PedalBoardSplitItem.PANR_KEY,0),
            new ControlValue(PedalBoardSplitItem.VOLR_KEY,-3)
        ];


        return result;


    }
    createEmptyItem(): PedalBoardItem {
        let result: PedalBoardItem = new PedalBoardItem();
        result.uri = EMPTY_PEDALBOARD_ITEM_URI;
        result.instanceId = ++this.nextInstanceId;
        result.pluginName = "";
        result.isEnabled = true;

        return result;
    }
    setItemEmpty(item: PedalBoardItem)
    {
        item.uri = EMPTY_PEDALBOARD_ITEM_URI;
        item.instanceId = ++this.nextInstanceId;
        item.pluginName = "";
        item.isEnabled = true;
        item.controlValues = [];

    }

    _replaceItem(items: PedalBoardItem[], instanceId: number, newItem: PedalBoardItem): boolean {
        for (let i = 0; i < items.length; ++i)
        {
            let item = items[i];
            if (item.instanceId === instanceId)
            {
                items[i] = newItem;
                return true;
            }
            if (items[i].isSplit())
            {
                let  splitItem = item as PedalBoardSplitItem;
                if (this._replaceItem(splitItem.topChain,instanceId,newItem))
                    return true;
                if (this._replaceItem(splitItem.bottomChain,instanceId,newItem))
                {
                    return true;
                }
            }
        }
        return false;
    }
    
    replaceItem(instanceId: number, newItem: PedalBoardItem)
    {
        let result = this._replaceItem(this.items,instanceId,newItem);
        if (!result)
        {
            throw new PiPedalArgumentError("instanceId not found.");
        }
    }
    _addItem(items: PedalBoardItem[], newItem: PedalBoardItem, instanceId: number, append: boolean)
    {
        for (let i = 0; i < items.length; ++i)
        {
            let item = items[i];
            if (item.instanceId === instanceId)
            {
                if (append)
                {
                    items.splice(i+1,0,newItem);
                    return true;
                } else {
                    items.splice(i,0,newItem);
                    return true;
                }
            }
            if (item.isSplit())
            {
                let splitItem = item as PedalBoardSplitItem;
                if (this._addItem(splitItem.topChain,newItem,instanceId,append)) return true;
                if (this._addItem(splitItem.bottomChain,newItem,instanceId,append)) return true;
            }
        }
    }

    addItem(newItem: PedalBoardItem, instanceId: number, append: boolean): void
    {
        this._addItem(this.items,newItem,instanceId,append);
    }

}

function* itemGenerator_(items: PedalBoardItem[]): Generator<PedalBoardItem, void, undefined> {
    for (let i = 0; i < items.length; ++i) {
        let item = items[i];
        yield item;
        if (item.uri === SPLIT_PEDALBOARD_ITEM_URI) {

            let splitItem = item as PedalBoardSplitItem;

            let it = itemGenerator_(splitItem.topChain);
            while (true) {
                let v = it.next();
                if (v.done) break;
                yield v.value;
            }
            it = itemGenerator_(splitItem.bottomChain);
            while (true) {
                let v = it.next();
                if (v.done) break;
                yield v.value;
            }
        }
    }
}


