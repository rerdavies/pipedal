

export default class WifiChannel {
    deserialize(input: any): WifiChannel {
        this.channelId = input.channelId;
        this.channelName = input.channelName;
        return this;
    }
    static deserialize_array(input: any): WifiChannel[]
    {
        let result: WifiChannel[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result.push(new WifiChannel().deserialize(input[i]));
        }
        return result;
    }
    channelId: string = "";
    channelName: string = "";
};