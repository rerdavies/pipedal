// Copyright (c) 2024 Robin Davies
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

export enum UpdatePolicyT {
    // must match declaration in Updater.hpp
    ReleaseOnly = 0,
    ReleaseOrBeta = 1,
    Development = 2,
    Disable = 3,

};
export function intToUpdatePolicyT(intValue: number): UpdatePolicyT {
    return (Object.values(UpdatePolicyT).find(enumValue => enumValue === intValue)) as UpdatePolicyT;
}
export class UpdateRelease {
    deserialize(input: any): UpdateRelease {
        this.updateAvailable = input.updateAvailable;
        this.upgradeVersion = input.upgradeVersion;
        this.upgradeVersionDisplayName = input.upgradeVersionDisplayName;
        this.assetName = input.assetName;
        this.updateUrl = input.updateUrl;
        return this;
    }


    updateAvailable: boolean = false;
    upgradeVersion: string = ""; //just the version.
    upgradeVersionDisplayName: string = ""; // display name for the version.
    assetName: string = ""; // filename only 
    updateUrl: string = ""; // url from which to download the .deb file.
    gpgSignatureUrl: string = "";

    equals(other: UpdateRelease): boolean {
        return (this.updateAvailable === other.updateAvailable) &&
            (this.upgradeVersion === other.upgradeVersion) &&
            (this.upgradeVersionDisplayName === other.upgradeVersionDisplayName) &&
            (this.assetName === other.assetName) &&
            (this.updateUrl === other.updateUrl) &&
            (this.gpgSignatureUrl === other.gpgSignatureUrl)
            ;
    }
};

export class UpdateStatus {
    lastUpdateTime: number = 0;
    isValid: boolean = false;
    errorMessage: string = "";
    updatePolicy: UpdatePolicyT = UpdatePolicyT.ReleaseOrBeta;
    isOnline: boolean = false;
    currentVersion: string = "";
    currentVersionDisplayName: string = "";

    releaseOnlyRelease: UpdateRelease = new UpdateRelease();
    releaseOrBetaRelease: UpdateRelease = new UpdateRelease();
    devRelease: UpdateRelease = new UpdateRelease();

    deserialize(input: any): UpdateStatus {
        this.lastUpdateTime = input.lastUpdateTime;
        this.isValid = input.isValid;
        this.errorMessage = input.errorMessage;
        this.isOnline = input.isOnline;
        this.updatePolicy = intToUpdatePolicyT(input.updatePolicy);
        this.currentVersion = input.currentVersion;
        this.currentVersionDisplayName = input.currentVersionDisplayName;
        this.releaseOnlyRelease = new UpdateRelease().deserialize(input.releaseOnlyRelease);
        this.releaseOrBetaRelease = new UpdateRelease().deserialize(input.releaseOrBetaRelease);
        this.devRelease = new UpdateRelease().deserialize(input.devRelease);
        return this;
    }

    static deserialize_array(input: any): UpdateStatus[] {
        let result: UpdateStatus[] = [];
        for (let i = 0; i < input.length; ++i) {
            result[i] = new UpdateStatus().deserialize(input[i]);
        }
        return result;
    }
    getActiveRelease(): UpdateRelease {
        switch (this.updatePolicy) {
            case UpdatePolicyT.Disable: // show available updates in the settings dialog.
            case UpdatePolicyT.ReleaseOnly:
                return this.releaseOnlyRelease;
            case UpdatePolicyT.ReleaseOrBeta:
                return this.releaseOrBetaRelease;
            case UpdatePolicyT.Development:
                return this.devRelease;
            default:
                throw new Error("Invalid argument.");
        }
    }
    equals(other: UpdateStatus): boolean {
        return (this.isValid === other.isValid)
            && (this.lastUpdateTime === other.lastUpdateTime)
            && (this.errorMessage === other.errorMessage)
            && (this.isOnline === other.isOnline)
            && (this.currentVersion === other.currentVersion)
            && (this.currentVersionDisplayName === other.currentVersionDisplayName)
            && (this.releaseOnlyRelease.equals(other.releaseOnlyRelease))
            && (this.releaseOrBetaRelease.equals(other.releaseOrBetaRelease))
            && (this.devRelease.equals(other.devRelease))
            ;
        // other properties are contractually dealt with.

    }


}