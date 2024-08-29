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

#pragma once

/*
    PiPedal uses whitelisting and GPG signatures to verify that updates are valid.
    You must modify these defines appropriately if you want to host a fork.

    If you don't, PiPedal will prompt to update your installs with the latest
    installs published on github/rerdavies/pipedal.
*/

#ifndef GITHUB_PROJECT
#define GITHUB_PROJECT "rerdavies/pipedal"
#endif


// public key info for the private key you used to sign .deb files.

#ifndef UPDATE_GPG_ADDRESS
#define UPDATE_GPG_ADDRESS "Robin Davies <rerdavies@gmail.com>"
#endif 

#ifndef UPDATE_GPG_FINGERPRINT
#define UPDATE_GPG_FINGERPRINT "381124E2BB4478D225D2313B2AEF3F7BD53EAA59"
#endif

// Configuration for downloading of updates.

#define GITHUB_DOWNLOAD_PREFIX   "https://github.com/" GITHUB_PROJECT "/releases/"

#define GITHUB_RELEASES_URL "https://api.github.com/repos/" GITHUB_PROJECT "/releases"

#define PGP_UPDATE_KEYRING_PATH "/var/pipedal/config/gpg"

inline bool WhitelistDownloadUrl(const std::string &downloadUrl)
{
    return downloadUrl.starts_with(GITHUB_DOWNLOAD_PREFIX);
}