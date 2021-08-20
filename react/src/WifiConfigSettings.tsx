

export default class WifiConfigSettings {
    deserialize(input: any) : WifiConfigSettings{
        this.valid = input.valid;
        this.wifiWarningGiven = input.wifiWarningGiven;
        this.rebootRequired = input.rebootRequired;
        this.enable = input.enable;
        this.hotspotName = input.hotspotName;
        this.hasPassword = input.hasPassword;
        this.password = input.password;
        this.countryCode = input.countryCode;
        this.channel = input.channel;
        return this;
    }
    clone() : WifiConfigSettings {
        return this.deserialize(this);
    }
    valid: boolean =  true;
    wifiWarningGiven: boolean = false;
    enable: boolean = true;
    hasPassword: boolean = false;
    rebootRequired = false;
    hotspotName: string = "pipedal";
    password: string = "";
    countryCode: string = "US";
    channel: string = "g6";

    getSummaryText() {
        let result: string;
        if (!this.valid) {
            result =  "Not available.";
        } else if (!this.enable) {
            result = "Disabled.";
        } else {
            result = this.hotspotName;
        }
        if (this.rebootRequired)
        {
            result += " (Restart required)";
        }
        return result;
    }

}