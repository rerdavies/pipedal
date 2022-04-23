/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


export default class WifiDirectConfigSettings {
    deserialize(input: any) : WifiDirectConfigSettings{
        this.valid = input.valid;
        this.rebootRequired = input.rebootRequired;
        this.enable = input.enable;
        this.pinChanged = input.pinChanged;
        this.hotspotName = input.hotspotName;
        this.countryCode = input.countryCode;
        this.pin = input.pin;
        this.channel = input.channel;

        return this;
    }
    clone() : WifiDirectConfigSettings {
        return new WifiDirectConfigSettings().deserialize(this);
    }
    valid: boolean =  true;
    enable: boolean = true;
    rebootRequired = false;
    hotspotName: string = "pipedal";
    pinChanged: boolean = false;
    pin: string = "";
    countryCode: string = "US";
    channel: string = "6";

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