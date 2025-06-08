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

import { pathConcat, pathParentDirectory } from "./FileUtils";
import { PiPedalModel } from "./PiPedalModel";

export class ThumbnailType {
    static Unknown = 0;
    static Embedded = 1; // embedded in the file
    static Folder = 2;   // from a albumArt.jpg or similar
    static None = 3;      // Use default thumbnail.
};


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