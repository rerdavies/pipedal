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



export class PluginUiPreset {
    deserialize(input: any): PluginUiPreset {
        this.instanceId = input.instanceId;
        this.label = input.label;
        return this;
    }
    static deserialize_array(input: any) : PluginUiPreset[] {
        let result: PluginUiPreset[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result.push(new PluginUiPreset().deserialize(input[i]));
        }
        return result;
    }

    instanceId: number = -1;
    label: string = "";

    static equals(left: PluginUiPreset, right: PluginUiPreset): boolean
    {
        return left.instanceId === right.instanceId && left.label === right.label;
    }
}

export class PluginUiPresets {
    deserialize(input: any): PluginUiPresets {
        this.pluginUri = input.pluginUri;
        this.presets = PluginUiPreset.deserialize_array(input.presets);
        return this;
    }

    clone(): PluginUiPresets {
        return new PluginUiPresets().deserialize(this);
    }
    movePreset(from: number, to: number): void
    {
        let t = this.presets[from];
        this.presets.splice(from,1);
        this.presets.splice(to,0,t);
    }
    getItem(instanceId: number): PluginUiPreset | undefined {
        for (let i = 0; i < this.presets.length; ++i)
        {
            if (this.presets[i].instanceId === instanceId)
            {
                return this.presets[i];
            }
        }
        return undefined;
    }
    static equals(left: PluginUiPresets,right: PluginUiPresets): boolean
    {
        if (left.pluginUri !== right.pluginUri) return false;
        if (left.presets.length !== right.presets.length) return false;
        for (let i = 0; i < left.presets.length; ++i)
        {
            if (!PluginUiPreset.equals(left.presets[i],right.presets[i]))
            {
                return false;
            }
        }
        return true;
    }
    pluginUri: string = "";
    presets: PluginUiPreset[] = [];
}