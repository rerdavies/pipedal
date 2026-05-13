import { PiPedalModel, getErrorMessage, Tone3000PkceParams } from "./PiPedalModel";
import Tone3000DownloadType from "./Tone3000DownloadType";
import { PkceParams, buildPkceParams, handleOAuthCallback } from "./t3k/tone3000-client.ts";

import { Model, PaginatedResponse, Platform, Tone } from "./t3k/types.ts";
import { PUBLISHABLE_KEY } from './t3k/config.ts';


import { startSelectFlowPopup, T3KClient } from "./t3k/tone3000-client.ts";
import Tone3000DownloadProgress from './Tone3000DownloadProgress';
import { safeFilenameEncode } from "./SafeFilename";


// const  T3K_API = "https://www.tone3000.com";
// used to check for online status.
let TONE3000_PING_URL = "https://www.tone3000.com/robots.txt";


async function asyncSleep(ms: number): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, ms));
}


const RUN_THROTTLER_TEST = true;
const ALLOWED_DOWNLOADS_PER_MINUTE = 25;
const THROTTLING_TRIGGGER_LEVEL = Math.floor(ALLOWED_DOWNLOADS_PER_MINUTE/2);


class DownloadThrottler {

    private fifo: number[] = [];
    private downloadCount: number = 0;

    
    resetThrottling(now: number = Date.now()): void {
        // clean out entries that are no longer counted by the Ton3000 throttler.
        while (this.fifo.length > 0 && this.fifo[0] + 60000 <= now) {
            this.fifo.shift();
        }

        this.downloadCount = this.fifo.length;
    }
    getThrottleDelay(now : number = Date.now()): number {
        while (this.fifo.length > 0 && this.fifo[0] + 60000 <= now) {
            this.fifo.shift();
        }


        let downloadCount = this.downloadCount;
        let delay: number;
        if (downloadCount < THROTTLING_TRIGGGER_LEVEL) { 
            delay = 0;
        } else if (downloadCount < ALLOWED_DOWNLOADS_PER_MINUTE) {
            delay =  Math.ceil((60000 / (ALLOWED_DOWNLOADS_PER_MINUTE-THROTTLING_TRIGGGER_LEVEL)));
        } else {
            delay = Math.ceil(60000 / (ALLOWED_DOWNLOADS_PER_MINUTE-1));
        }
        this.fifo.push(now + delay);
        console.debug(`       ${now}: Download: ${downloadCount}  Delay: ${delay} ms Throttle count: ${this.fifo.length}`);
        ++this.downloadCount;
        return delay;
    }
}


function runThrottlerTest() {
    // just dump the delay times for a few downloads to the debugger console, to verify that the throttler is working as expected;
    let throttler = new DownloadThrottler();

    let download = 0;
    let now = 0;
    console.debug("Tone3000 Throttler Test");

    while (download < ALLOWED_DOWNLOADS_PER_MINUTE *4) {
        let delay = throttler.getThrottleDelay(now);
        now += delay;
        ++download;
    } 
}

export class Tone3000DownloadHandler {

    // private async handleTone3000Download(
    //     code: string, toneId: String, downloadType: Tone3000DownloadType): Promise<void> 
    // {
    //     const {access_token} = await exchangeCode(code);
    //     try {
    //         const xxx;
    //         await this.model.downloadModelsFromTone3000(tone3000DownloadUrl, this.downloadPath, downloadType);
    //         return;
    //     } catch (e) {
    //         this.handleTone3000DownloadError(getErrorMessage(e));
    //         return;
    //     }
    // }

    private messageEventListener: ((event: MessageEvent) => void) | null = null;
    private t3kClient: T3KClient;
    private throttler: DownloadThrottler = new DownloadThrottler();

    private model: PiPedalModel;
    pkceParams: PkceParams | null = null;


    public constructor(model: PiPedalModel) {
        if (RUN_THROTTLER_TEST) {
            runThrottlerTest();
        }
        this.model = model;
        this.t3kClient = new T3KClient(
            PUBLISHABLE_KEY,
            () => {
                model.showAlert("TONE3000 authentication failed. Please try again.");
            }
        );
        let this_ = this;
        this.messageEventListener = (event: MessageEvent) => {
            if (event.data?.type === "t3k_response") {
                let uri = event.data.uri;
                this.handleTone3000DownloadComplete();
                if (uri) {
                    this_.handleT3kSelectResponse(uri, event.data.storedState, event.data.codeVerifier)
                        .then(() => { })
                        .catch((error) => {
                            model.showAlert(getErrorMessage(error));
                        });
                }

            }

        };

        window.addEventListener("message", this.messageEventListener);
    }

    public dispose(): void {
        if (this.messageEventListener) {
            window.removeEventListener("message", this.messageEventListener);
            this.messageEventListener = null;
        }
    }

    private async handleT3kSelectResponse(
        uri: string,
        popupStoredState: string | null,
        popupCodeVerifier: string | null
    ): Promise<void> {
        if (this.pkceParams === null) {
            this.model.showAlert("Invalid PKCE parameters. Please try again.");
            return;

        }
        let tone3000PckceParams: Tone3000PkceParams = {
            publishableKey: PUBLISHABLE_KEY,
            redirectUrl: this.redirectUrl(),
            codeChallenge: this.pkceParams.codeChallenge,
            codeVerifier: this.pkceParams.codeVerifier,
            state: this.pkceParams.state

        };

        this.DownloadModelsFromTone3000(uri, tone3000PckceParams, this.downloadPath, this.downloadType);
    }

    private handle = 0;

    private nextHandle(): number {
        return ++this.handle;
    }


    private progress: Tone3000DownloadProgress = new Tone3000DownloadProgress();

    private async throttleRequests(): Promise<void> {
        let now = Date.now();
        let delay = this.throttler.getThrottleDelay(now);
        while (delay > 0) {
            await asyncSleep(250);
            if (this.checkForCancel())
            {
                return;
            }
            delay -= 250;
        }
    }

    private onTone3000DownloadStarted() {
        this.throttler.resetThrottling(); // allow full-rate burst of downloads at the start of each tone download.

        this.progress.handle = this.nextHandle();
        this.progress.title = "";
        this.progress.progress = 0;
        this.progress.total = 0;
        this.progress.cancelled = false;
        this.progress.transferring = true;
        this.model.onTone3000DownloadStarted(this.progress.handle, this.progress.title);
    }
    private onTone3000DownloadProgress(progress: Tone3000DownloadProgress) {
        let newProgress = progress.clone();
        this.model.onTone3000DownloadProgress(newProgress);
    }
    private onTone3000DownloadError(errorMessage: string) {
        this.progress.transferring = false;
        this.model.onTone3000DownloadError(this.progress.handle, errorMessage);
    }
    private onTone3000DownloadComplete(resultPath: string) {
        this.progress.transferring = false;
        this.model.onTone3000DownloadComplete(resultPath);
    }
    private async DownloadModelsFromTone3000(
        responseUri: string,
        tone3000PckceParams: Tone3000PkceParams,
        downloadPath: string,
        downloadType: Tone3000DownloadType
    ): Promise<void> {
        try {
            this.onTone3000DownloadStarted();
            this.progress.title = "Authenticating..."
            this.onTone3000DownloadProgress(this.progress);
            if (this.checkForCancel()) {
                return;
            }

            await this.throttleRequests();
            let tokenResponse = await handleOAuthCallback(
                PUBLISHABLE_KEY,
                this.redirectUrl(),
                responseUri);
            if (!tokenResponse.ok) {
                ;
                throw new Error(tokenResponse.error);
            }
            if (this.checkForCancel()) {
                return;
            }

            this.t3kClient.setTokens(tokenResponse.tokens);

            if (tokenResponse.toneId == undefined) {
                throw new Error("No tone selected. Please try again.");
            }
            let toneId = tokenResponse.toneId;

            this.progress.title = "Fetching tone information...";
            this.onTone3000DownloadProgress(this.progress);


            await this.throttleRequests();
            let tone: Tone = await this.t3kClient.getTone(tokenResponse.toneId);

            if (this.checkForCancel()) {
                return;
            }

            this.progress.title = tone.title;
            this.onTone3000DownloadProgress(this.progress);

            let page = 1;
            let models: Model[] = [];
            let architectureFilter: number | undefined = undefined;
            // Take A2 models only for NAM downloads, and only if the tone actually has A2 models.
            if (downloadType === Tone3000DownloadType.Nam && !!tone.a2_models_count && this.model.tone3000_A2_models) {
                architectureFilter = 2; // NAM models only
            }

            while (true) {
                await this.throttleRequests();
                if (this.checkForCancel()) {
                    return;
                }

                let modelsThisTime: PaginatedResponse<Model> = await this.t3kClient.listModels(toneId, page, 800, architectureFilter);
                models.push(...modelsThisTime.data);
                if (modelsThisTime.total_pages <= page) {
                    break;
                }
                ++page;
            }
            let uploadPath = this.downloadPath;
            let extension: string;

            switch (tone.platform) {
                case Platform.Nam:
                    extension = ".nam";
                    break;
                case Platform.Ir:
                    extension = ".wav";
                    break;
                case Platform.Proteus:
                    extension = ".json";
                    break;
                default:
                    throw new Error("Unsupported platform: " + tone.platform);
                    break;
            }
            let toneUploadPath = uploadPath + "/" + safeFilenameEncode(tone.title) + "/";
            this.progress.total = models.length;
            this.onTone3000DownloadProgress(this.progress);

            let lastUpdateTime = Date.now();



            for (const model of models) {
                this.progress.title = model.name;;
                if (Date.now() - lastUpdateTime > 250) {
                    this.onTone3000DownloadProgress(this.progress);
                    lastUpdateTime = Date.now();
                }

                await this.throttleRequests();

                if (!model.model_url) {
                    throw new Error("Model " + model.name + " does not have a model URL.");
                }
                let modelUploadPath = toneUploadPath + safeFilenameEncode(model.name) + extension;

                let accessToken = await this.t3kClient.getAccessToken();
                let modelResult = await fetch(model.model_url,
                    {
                        headers: {
                            Authorization: `Bearer ${accessToken}`,
                        },
                        duplex: 'half',
                    } as RequestInit & { duplex: 'half' }

                );
                if (!modelResult.ok) {
                    throw new Error(`Model download failed: ${modelResult.status} ${modelResult.statusText}`);
                }
                if (this.checkForCancel()) {
                    return;
                }

                let serverUrl = this.model.varServerUrl + "t3k_uploadAsset?path="
                    + encodeURIComponent(modelUploadPath);

                // get the content length of modelResult from modelResult headers (modelresult is the return value from fetch())
                const strContentLength = modelResult.headers.get('Content-Length');
                let contentLength: number = 0;
                if (strContentLength) {
                    contentLength = parseInt(strContentLength);
                    if (isNaN(contentLength)) {
                        throw new Error("File download from Tone3000 server failed: Content-Length header is invalid.");
                    }

                }
                // POST binary modelResult to serverUrl.
                // Prefer streaming request body when available, else fall back to blob().
                const uploadBody: Blob =
                    await modelResult.blob();
                const uploadResponse = await fetch(serverUrl, {
                    method: 'POST',
                    body: uploadBody,
                    headers: {
                        'Content-Type': modelResult.headers.get('Content-Type') ?? 'application/octet-stream',
                        "Content-Length": contentLength.toString(),
                        "Transfer-Encoding": "chunked"
                    },
                });

                if (!uploadResponse.ok) {
                    throw new Error(`Upload failed: ${uploadResponse.statusText}`);
                }
                // discard the response body, if any, to free up memory
                let uploadResult: any = await uploadResponse.json();

                if (!uploadResult.ok === true) {
                    throw new Error(`Upload failed: ${uploadResult.error ?? "Unknown error."}`);
                }
                this.progress.progress++;

            }
            // yyy: get the image file.
            // yyy: write the reaadme file.

            this.onTone3000DownloadComplete(uploadPath);
        } catch (error) {
            this.onTone3000DownloadError(getErrorMessage(error)
                + " " + this.progress.progress.toString() + "/" + this.progress.total.toString() + " models downloaded.");
            return;
        }
    }

    private popupWindow: Window | null = null;

    private redirectUrl(): string {
        let hostname = window.location.hostname;
        let port = window.location.port;
        let protocol = window.location.protocol;
        let serverUrl: string;
        if (protocol === "http:" && (port === "80" || port === "")) {
            serverUrl = `${protocol}//${hostname}`;
        } else if (protocol === "https:" && (port === "443" || port === "")) {
            serverUrl = `${protocol}//${hostname}`;
        } else {
            serverUrl = `${protocol}//${hostname}:${port}`;
        }
        return `${serverUrl}/html/t3k_response.html`;
        // return `${serverUrl}/t3k_response.html`; // for a debuggable react version of the page
    }



    private handleTone3000DownloadComplete() {
        if (this.popupWindow) {
            if (!this.popupWindow.closed) {
                this.popupWindow.close();
            }
            this.popupWindow = null;
        }
        this.progress.transferring = false;
        this.model.hideTone3000DownloadStatus();

    }
    closeTone3000Popup() {
        this.handleTone3000DownloadComplete();
    }
    private handleTone3000DownloadError(errorMessage: string) {
        this.progress.transferring = false;
        this.handleTone3000DownloadComplete();
        this.model.showAlert(errorMessage);
    }

    private downloadPath: string = "";
    private downloadType: Tone3000DownloadType = Tone3000DownloadType.Nam;

    private checkForCancel(): boolean {
        if (this.progress.cancelled) {
            this.handleTone3000DownloadComplete();
            return true;
        }
        return false;
    }

    private launchInstance = 0;
    public async launchTone3000Popup(
        downloadType: Tone3000DownloadType,
        downloadPath: string,

    ): Promise<void> {
        if (this.progress.transferring) {
            return;
        }
        this.closeTone3000Dialog();
        // online

        this.downloadPath = downloadPath;
        this.downloadType = downloadType;

        let popupWidth = Math.floor(window.innerWidth * 0.8);
        let popupHeight = Math.floor(window.innerHeight * 0.8);
        if (window.innerWidth < 410) {
            popupWidth = 400;
        }

        this.model.showTone3000DownloadStatus();

        let launchInstance = ++this.launchInstance;
        let cancelled = () => {
            return (launchInstance !== this.launchInstance) || this.popupWindow == null || this.popupWindow.closed;
        }

        try {

            let pkceParams = await buildPkceParams();
            this.pkceParams = pkceParams;
            this.popupWindow = await startSelectFlowPopup(
                pkceParams,
                PUBLISHABLE_KEY,
                this.redirectUrl(),
                {
                    architecture: downloadType === Tone3000DownloadType.CabIr ? undefined : 2,
                    platform: downloadType === Tone3000DownloadType.CabIr ? Platform.Ir : Platform.Nam,
                    menubar: true,
                    width: popupWidth,
                    height: popupHeight
                },

            );

            if (!this.popupWindow) {
                console.error("Failed to open TONE3000 dialog popup window.");
                throw new Error("Cannot open popup window.");
            }
            // No procedure for detecting 404, so let's ping the server to see if it's reachable, and cancel if it's not.
            try {
                let response = await fetch(TONE3000_PING_URL, { method: "HEAD", cache: "no-cache" });
                if (cancelled()) return;
                if (!response.ok) {
                    // offline
                    throw new Error("Unable to connect to the TONE3000 server.");
                }
            } catch (error) {
                throw new Error("Not online. Unable to connect to the TONE3000 server. Your client device must have access to the internet to use this feature..");
            };

            let pipedalServerCanReachTone3000 = false;

            // check to see that the PiPedal server can reach TONE3000.
            try {
                pipedalServerCanReachTone3000 = await this.model.pingTone3000Server();
                if (cancelled()) return;
            } catch (error) {
            }
            if (!pipedalServerCanReachTone3000) {
                throw new Error("The PiPedal server cannot reach a TONE3000 server. The PiPedal server must have access to the internet to use this feature.");
            }
            while (true) {
                await new Promise(resolve => setTimeout(resolve, 500));
                if (launchInstance != this.launchInstance) {
                    // A new dialog launch has been initiated. Stop monitoring this one.
                    return;
                }
                if (this.popupWindow === null || this.popupWindow.closed) {
                    this.closeTone3000Dialog();
                    return;
                }
            }
        } catch (error) {
            this.handleTone3000DownloadError(getErrorMessage(error));
        }
    }

    cancelDownload(handle: number): void {
        this.closeTone3000Dialog();
        if (this.progress.handle == handle) {
            this.progress.cancelled = true;
        }
    }
    public closeTone3000Dialog(): void {
        if (this.popupWindow !== null) {
            this.popupWindow.close();
            this.popupWindow = null;
            if (!this.progress.transferring) {
                this.handleTone3000DownloadComplete();
            }
        }

    }
}
