// Copyright (c) 2023 Robin Davies
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


#include "MimeTypes.hpp"
#include "ss.hpp"
#include <mutex>
#include <memory>

using namespace pipedal;

static std::string empty;

const std::string& MimeTypes::MimeTypeFromExtension(const std::string &extension) const
{
    auto iter = extensionToMimeType.find(extension);
    if (iter == extensionToMimeType.end()) return empty;
    return iter->second;
}

const std::string& MimeTypes::ExtensionFromMimeType(const std::string &mimeType) const
{
    auto iter = mimeTypeToExtension.find(mimeType);
    if (iter == mimeTypeToExtension.end()) return empty;
    return iter->second;
}


static std::string toLower(const std::string&value)
{
    std::string result;
    result.resize(value.length());
    for (size_t i = 0; i < value.length(); ++i)
    {
        char c = value[i];
        if (c >= 'A' && c <= 'Z')
        {
            c += 'a'-'A';
        }
        result[i] = c;
    }
    return result;
}

void MimeTypes::AddMimeType(const std::string&extension_, const std::string&mimeType) 
{
    std::string extension = SS("." << toLower(extension_));
    mimeTypeToExtensions[mimeType].insert(extension);
    mimeTypeToExtension[mimeType] = extension;
    extensionToMimeType[extension] = mimeType;
    if (mimeType.starts_with("audio/midi"))
    {
        midiExtensions.insert(extension);
    } else if (mimeType.starts_with("audio/"))
    {
        mimeTypeToExtensions["audio/*"].insert(extension);
        audioExtensions.insert(extension);
    }
    if (mimeType.starts_with("video/"))
    {
        mimeTypeToExtensions["video/*"].insert(extension);
        videoExtensions.insert(extension);
    }

}

const std::set<std::string> &MimeTypes::AudioExtensions() const
{
    return audioExtensions;

}
const std::set<std::string> &MimeTypes::VideoExtensions() const
{
    return videoExtensions;
}
const std::set<std::string> &MimeTypes::MidiExtensions() const
{
    return midiExtensions;
}

static std::mutex m_instanceMutex;
static std::unique_ptr<MimeTypes> m_instance;


MimeTypes::MimeTypes()
{
    AddMimeType("MP3", "audio/mpeg");
    AddMimeType("MPGA", "audio/mpeg");
    AddMimeType("M4A", "audio/mp4");
    AddMimeType("WAV", "audio/x-wav");
    AddMimeType("WAV", "audio/wav");
    AddMimeType("AMR", "audio/amr");
    AddMimeType("AWB", "audio/amr-wb");
    AddMimeType("WMA", "audio/x-ms-wma");
    AddMimeType("OGG", "audio/ogg");
    AddMimeType("OGG", "application/ogg");
    AddMimeType("OGA", "application/ogg");
    AddMimeType("AAC", "audio/aac");
    AddMimeType("AAC", "audio/aac-adts");
    AddMimeType("MKA", "audio/x-matroska");
    AddMimeType("MID", "audio/midi");
    AddMimeType("MIDI", "audio/midi");
    AddMimeType("XMF", "audio/midi");
    AddMimeType("RTTTL", "audio/midi");
    AddMimeType("SMF", "audio/sp-midi");
    AddMimeType("IMY", "audio/imelody");
    AddMimeType("RTX", "audio/midi");
    AddMimeType("OTA", "audio/midi");
    AddMimeType("MXMF", "audio/midi");
    
    AddMimeType("MPEG", "video/mpeg");
    AddMimeType("MPG", "video/mpeg");
    AddMimeType("MP4", "video/mp4");
    AddMimeType("M4V", "video/mp4");
    AddMimeType("3GP", "video/3gpp");
    AddMimeType("3GPP", "video/3gpp");
    AddMimeType("3G2", "video/3gpp2");
    AddMimeType("3GPP2", "video/3gpp2");
    AddMimeType("MKV", "video/x-matroska");
    AddMimeType("WEBM", "video/webm");
    AddMimeType("TS", "video/mp2ts");
    AddMimeType("AVI", "video/avi");
    AddMimeType("WMV", "video/x-ms-wmv");
    AddMimeType("ASF", "video/x-ms-asf");
    AddMimeType("JPG", "image/jpeg");
    AddMimeType("JPEG", "image/jpeg");
    AddMimeType("GIF", "image/gif");
    AddMimeType("PNG", "image/png");
    AddMimeType("BMP", "image/x-ms-bmp");
    AddMimeType("WBMP", "image/vnd.wap.wbmp");
    AddMimeType("WEBP", "image/webp");

    AddMimeType("M3U", "audio/x-mpegurl");
    AddMimeType("M3U", "application/x-mpegurl");
    AddMimeType("PLS", "audio/x-scpls");
    AddMimeType("WPL", "application/vnd.ms-wpl");
    AddMimeType("M3U8", "application/vnd.apple.mpegurl");
    AddMimeType("M3U8", "audio/mpegurl");
    AddMimeType("M3U8", "audio/x-mpegurl");
    AddMimeType("FL", "application/x-android-drm-fl");
    AddMimeType("TXT", "text/plain");
    AddMimeType("HTM", "text/html");
    AddMimeType("HTML", "text/html");
    AddMimeType("PDF", "application/pdf");
    AddMimeType("DOC", "application/msword");
    AddMimeType("XLS", "application/vnd.ms-excel");
    AddMimeType("PPT", "application/mspowerpoint");
    AddMimeType("FLAC", "audio/x-flac");
    AddMimeType("FLAC", "audio/flac");
    AddMimeType("ZIP", "application/zip");
    AddMimeType("MPG", "video/mp2p");
    AddMimeType("MPEG", "video/mp2p");

}

const MimeTypes&MimeTypes::instance()
{
    std::lock_guard lock { m_instanceMutex};
    if (!m_instance) {
        m_instance = std::unique_ptr<MimeTypes>(new MimeTypes());
    }
    return (*m_instance);
}

const bool MimeTypes::IsValidExtension(const std::string&mimeType, const std::string&extension) const
{
    auto iter = mimeTypeToExtensions.find(mimeType);
    if (iter == mimeTypeToExtensions.end()) return false;
    return iter->second.contains(extension);
}



