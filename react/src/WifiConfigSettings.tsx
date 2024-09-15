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



export default class WifiConfigSettings {
    deserialize(input: any) : WifiConfigSettings{
        this.autoStartMode = input.autoStartMode;
        this.hasSavedPassword = input.hasSavedPassword;
        this.homeNetwork = input.homeNetwork;
        this.valid = input.valid;

        this.wifiWarningGiven = input.wifiWarningGiven;
        this.hotspotName = input.hotspotName;
        this.mdnsName = input.mdnsName;
        this.hasPassword = input.hasPassword;
        this.password = input.password;
        this.countryCode = input.countryCode;
        this.channel = input.channel;
        return this;
    }
    clone() : WifiConfigSettings {
        return new WifiConfigSettings().deserialize(this);
    }
    autoStartMode: number = 0;
    hasSavedPassword: boolean = false;
    homeNetwork: string = "";
    mdnsName: string = "";
    wifiWarningGiven: boolean = false;
    hasPassword: boolean = false;
    valid: boolean = false;
    hotspotName: string = "pipedal";
    password: string = "";
    countryCode: string = "US";
    channel: string = "0";

    isEnabled() { return this.autoStartMode !== 0;}
    getSummaryText() {
        let result: string;
        if (!this.valid) {
            result =  "Not available.";
        } else if (this.autoStartMode === 0) {
            result = "Disabled.";
        } else {
            result = this.hotspotName;
        }
        return result;
    }

}