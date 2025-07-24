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

export interface  AndroidHostInterface {
    showSponsorship() : void;
    isAndroidHosted(): boolean;
    getHostVersion() : string;
    chooseNewDevice() : void;
    setDisconnected(isDisconnected: boolean): void;
    launchExternalUrl(url:string): boolean;
    setThemePreference(theme: number): void;
    getThemePreference(): number;
    isDarkTheme?: ()=> boolean;
    setServerVersion(serverVersion: string) : void;
    setKeepScreenOn(keepScreenOn: boolean): void;
    getKeepScreenOn(): boolean;
    setScreenOrientation(orientation: number): void;
    getScreenOrientation(): number;
};

export class FakeAndroidHost implements AndroidHostInterface
{
    isAndroidHosted(): boolean {
        return true;
    }
    getHostVersion(): string {
        return "Fake Host: v1.1.26";
    }
    chooseNewDevice(): void {
        
    }
    isDarkTheme(): boolean {
        return true;
    }
    setDisconnected(_isDisconnected: boolean): void {
        
    }
    showSponsorship() : void { }

    launchExternalUrl(_url:string): boolean
    {
		
        return false;
    }
    private theme = 1;
    setThemePreference(theme: number): void{
        this.theme = theme;
    }
    getThemePreference(): number {
        return this.theme;
    }
    setServerVersion(_serverVersion: string): void {
        // No-op for fake host
	
    }

    private keepScreenOn = false;
    setKeepScreenOn(keepScreenOn: boolean): void {this.keepScreenOn = keepScreenOn; };
    getKeepScreenOn(): boolean { return this.keepScreenOn; };

    private screenOrientation = 0;
    setScreenOrientation(_orientation: number): void {
        this.screenOrientation = _orientation;
    }
    getScreenOrientation(): number {
        return this.screenOrientation;
    }
}
export function isAndroidHosted(): boolean {
    return ((window as any).AndroidHost as AndroidHostInterface) !== undefined;
}

export function getAndroidHost(): AndroidHostInterface | undefined {
    return ((window as any).AndroidHost as AndroidHostInterface);
}