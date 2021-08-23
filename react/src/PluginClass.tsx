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

import {PluginType} from './Lv2Plugin';

class PluginClass {
    deserialize(input: any) : PluginClass {
        this.uri = input.uri;
        this.display_name = input.display_name;
        this.parent_uri = input.parent_uri;
        this.plugin_type = input.plugin_type as PluginType;
        this.children = PluginClass.deserialize_array(input.children);
        return this;
    }
    static deserialize_array(input: any): PluginClass[] {
        let result: PluginClass[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result[i] = new PluginClass().deserialize(input[i]);
        }

        return result;

    }
    findChild(classUri: string): PluginClass | null {
        if (this.uri === classUri) return this;
        for (let i = 0; i < this.children.length; ++i)
        {
            let t = this.children[i].findChild(classUri);
            if (t != null) return t;
        }
        return null;
    }
    findChildType(pluginType: PluginType): PluginClass | null {
        if (this.plugin_type === pluginType) return this;
        for (let i = 0; i < this.children.length; ++i)
        {
            let t = this.children[i].findChildType(pluginType);
            if (t != null) return t;
        }
        return null;
    }



    is_a(parentClassUri: string,childClassUri: string): boolean {
        let parent = this.findChild(parentClassUri);
        if (parent === null) return false;
        let child = parent.findChild(childClassUri);
        return child != null;
    }
    is_type_of(parentType: PluginType,childType: PluginType): boolean {
        let parent = this.findChildType(parentType);
        if (parent === null) return false;
        let child = parent.findChildType(childType);
        return child != null;
    }

    uri: string = "";
    display_name: string = "";
    parent_uri: string = "";
    plugin_type: PluginType = PluginType.Plugin;
    children: PluginClass[] = [];
}

export default PluginClass;