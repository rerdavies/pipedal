/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

import { pathConcat, pathParentDirectory, pathFileName } from "./FileUtils";
import { PiPedalModel } from "./PiPedalModel";

export class ThumbnailType {
    static Unknown = 0;
    static Embedded = 1; // embedded in the file
    static Folder = 2;   // from a albumArt.jpg or similar
    static None = 3;      // Use default thumbnail.
};


const  fileVersionRegex = /\((\d+)\)\.[0-9a-zA-Z]*$/;
function getVersionSuffix(filePath: string): string | null {
    const match = filePath.match(fileVersionRegex);
    return match ? match[1] : null;
}

export function safeFilenameDecode(filename: string): string {
    let originalFilename = filename;
    let pos = filename.indexOf('%');
    if (pos < 0) {
        return filename;
    }
    let result = "";
    while (true) {
        if (pos < 0) {
            result += filename;
            return result;
        }
        result += filename.substring(0, pos);
        if (pos+3 >= filename.length) {
            // invalid encoding. just use the original
            return originalFilename; 
        }
        let hex = filename.charAt(pos+1) + filename.charAt(pos+2);
        let byte = parseInt(hex, 16);

        // Handle UTF-8 to UTF-16 conversion
        if (byte < 0x80) {
            // Single-byte character (ASCII)
            result += String.fromCharCode(byte);
            filename = filename.substring(pos+3);
        } else if ((byte & 0xE0) === 0xC0) {
            // 2-byte sequence
            if (pos+6 > filename.length || filename.charAt(pos+3) !== '%') {
                return originalFilename; // invalid encoding
            }
            let byte2 = parseInt(filename.charAt(pos+4) + filename.charAt(pos+5), 16);
            let codePoint = ((byte & 0x1F) << 6) | (byte2 & 0x3F);
            result += String.fromCharCode(codePoint);
            filename = filename.substring(pos+6);
        } else if ((byte & 0xF0) === 0xE0) {
            // 3-byte sequence
            if (pos+9 > filename.length || filename.charAt(pos+3) !== '%' || filename.charAt(pos+6) !== '%') {
                return originalFilename; // invalid encoding
            }
            let byte2 = parseInt(filename.charAt(pos+4) + filename.charAt(pos+5), 16);
            let byte3 = parseInt(filename.charAt(pos+7) + filename.charAt(pos+8), 16);
            let codePoint = ((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
            result += String.fromCharCode(codePoint);
            filename = filename.substring(pos+9);
        } else if ((byte & 0xF8) === 0xF0) {
            // 4-byte sequence (surrogate pair needed)
            if (pos+12 > filename.length || filename.charAt(pos+3) !== '%' || filename.charAt(pos+6) !== '%' || filename.charAt(pos+9) !== '%') {
                return originalFilename; // invalid encoding
            }
            let byte2 = parseInt(filename.charAt(pos+4) + filename.charAt(pos+5), 16);
            let byte3 = parseInt(filename.charAt(pos+7) + filename.charAt(pos+8), 16);
            let byte4 = parseInt(filename.charAt(pos+10) + filename.charAt(pos+11), 16);
            let codePoint = ((byte & 0x07) << 18) | ((byte2 & 0x3F) << 12) | ((byte3 & 0x3F) << 6) | (byte4 & 0x3F);
            // Convert to UTF-16 surrogate pair
            codePoint -= 0x10000;
            result += String.fromCharCode(0xD800 + (codePoint >> 10));
            result += String.fromCharCode(0xDC00 + (codePoint & 0x3FF));
            filename = filename.substring(pos+12);
        } else {
            // Invalid UTF-8 start byte
            return originalFilename;
        }
        pos = filename.indexOf('%');
    }
}

export function getTrackTitle(pathname: string, metadata: AudioFileMetadata | null | undefined): string {
    if (!metadata) {
        return safeFilenameDecode(pathFileName(pathname));
    }
    if (metadata.title === "") {
        return safeFilenameDecode(pathFileName(pathname));
    }
    let trackDisplay = "";
    if (metadata.track > 0) {
        if (metadata.track >= 1000) {
            trackDisplay = (metadata.track % 1000).toString() + "/" + Math.floor(metadata.track / 1000) + ". ";
        } else {
            trackDisplay = metadata.track.toString() + ".  ";
        }
    }
    let result = trackDisplay + metadata.title;
    if (result === "") {
        result = safeFilenameDecode(pathFileName(pathname));
    }
    let versionSuffix = getVersionSuffix(pathname);
    if (versionSuffix) {
        result += " (" + versionSuffix + ")";
    }
    return result;
}


export default class AudioFileMetadata {
    //AudioFileMetadata&operator=(const AudioFileMetadata&) = default;
    deserialize(o: any) {
        this.fileName = o.fileName;
        this.lastModified = o.lastModified;
        this.title = o.title;
        this.track = o.track;
        this.album = o.album;
        this.artist = o.artist;
        this.albumArtist = o.albumArtist;
        this.duration = o.duration;
        this.thumbnailType = o.thumbnailType;
        this.thumbnailFile = o.thumbnailFile;
        this.thumbnailLastModified = o.thumbnailLastModified;

        return this;
    }
    fileName: string = "";
    lastModified: number = 0; 
    title: string = "";
    track: number = 0;
    album: string = "";
    artist: string = "";
    albumArtist: string = "";
    duration: number = 0;
    thumbnailType: ThumbnailType = new ThumbnailType();
    thumbnailFile = "";
    thumbnailLastModified = 0;
}



export function getAlbumArtUri(model: PiPedalModel, metadata: AudioFileMetadata | undefined, path: string): string {
    let coverArtUri: string;
    if (!metadata) {
        return "/img/missing_thumbnail.jpg";
    }
    if (metadata.thumbnailType === ThumbnailType.None) {
        return "/img/missing_thumbnail.jpg";
    
    } else if (metadata.thumbnailType ===  ThumbnailType.Folder) {
        // Use the thumbnailFile to get the cover art.
        let thumbnailPath = pathConcat(pathParentDirectory(path) ,metadata.thumbnailFile);

        coverArtUri = model.varServerUrl + "Thumbnail"
            + "?ffile=" + encodeURIComponent(thumbnailPath)
            + "&t=" + metadata.thumbnailLastModified
            + "&w=240&h=240"
            ;
        return coverArtUri;
    } else {
        // for embeed and unknown
        coverArtUri = model.varServerUrl + "Thumbnail"
            + "?path=" + encodeURIComponent(path)
            + "&t=" + metadata.lastModified
            + "&w=240&h=240"
            ;
        return coverArtUri;
    }

}