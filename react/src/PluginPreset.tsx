

export default class PluginPreset {
    deserialize(input: any): PluginPreset {
        this.presetUri = input.presetUri;
        this.name = input.name;
        return this;
    }
    static deserialize_array(input: any) : PluginPreset[] {
        let result: PluginPreset[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result.push(new PluginPreset().deserialize(input[i]));
        }
        return result;
    }

    presetUri: string = "";
    name: string = "";
}