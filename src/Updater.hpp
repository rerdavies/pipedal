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

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include "json.hpp"
#include <chrono>

class GithubRelease;

namespace pipedal
{

    class Updater;

    enum class UpdatePolicyT
    {
        // ordinal values must not change, as files depend on them.
        // must match declaration in Updater.tsx

        ReleaseOnly = 0,
        ReleaseOrBeta = 1,
        Development = 2,
    };
    class UpdateRelease
    {
    private:
        friend class Updater;
        bool updateAvailable_ = false;
        std::string upgradeVersion_;            // just the version.
        std::string upgradeVersionDisplayName_; // display name for the version.
        std::string assetName_;                 // filename only
        std::string updateUrl_;                 // url from which to download the .deb file.
        std::string gpgSignatureUrl_;                 // url from which to download the .deb.asc file.
    public:
        UpdateRelease();

        bool UpdateAvailable() const { return updateAvailable_; }
        const std::string &UpgradeVersion() const { return upgradeVersion_; }
        const std::string &UpgradeVersionDisplayName() const { return upgradeVersionDisplayName_; }
        const std::string &AssetName() const { return assetName_; }
        const std::string &UpdateUrl() const { return updateUrl_; }
        const std::string &GpgSignatureUrl() const { return gpgSignatureUrl_;}
        bool operator==(const UpdateRelease &other) const;
        DECLARE_JSON_MAP(UpdateRelease);
    };

    class UpdateStatus
    {
    private:
        friend class Updater;
        std::chrono::system_clock::time_point::rep lastUpdateTime_ = 0;

        bool isValid_ = false;
        std::string errorMessage_;
        bool isOnline_ = false;
        std::string currentVersion_;
        std::string currentVersionDisplayName_;

        int32_t updatePolicy_ = (int32_t)(UpdatePolicyT::ReleaseOrBeta);
        UpdateRelease releaseOnlyRelease_;
        UpdateRelease releaseOrBetaRelease_;
        UpdateRelease devRelease_;

    public:
        UpdateStatus();
        std::chrono::system_clock::time_point LastUpdateTime() const;
        void LastUpdateTime(const std::chrono::system_clock::time_point &timePoint);

        void ResetCurrentVersion();
        bool IsValid() const { return isValid_; }
        const std::string &ErrorMessage() const { return errorMessage_; }
        bool IsOnline() const { return isOnline_; }
        const std::string &CurrentVersion() const { return currentVersion_; }
        const std::string &CurrentDisplayVersion() const { return currentVersionDisplayName_; }
        UpdatePolicyT UpdatePolicy() const { return (UpdatePolicyT)updatePolicy_; }
        void UpdatePolicy(UpdatePolicyT updatePreference) { this->updatePolicy_ = (int32_t)updatePreference; }

        const UpdateRelease &ReleaseOnlyRelease() const { return releaseOnlyRelease_; }
        const UpdateRelease &ReleaseOrBetaRelease() const { return releaseOrBetaRelease_; }
        const UpdateRelease &DevRelease() const { return devRelease_; }
        bool operator==(const UpdateStatus &other) const;

        DECLARE_JSON_MAP(UpdateStatus);
    };

    class Updater
    {
    public:
        Updater();
        ~Updater();

        static void ValidateSignature(const std::filesystem::path&file, const std::filesystem::path&signatureFile);
        
        using UpdateListener = std::function<void(const UpdateStatus &upateResult)>;

        void SetUpdateListener(UpdateListener &&listener);
        void CheckNow();
        void Stop();

        UpdatePolicyT GetUpdatePolicy();
        void SetUpdatePolicy(UpdatePolicyT updatePolicy);
        void ForceUpdateCheck();
        void DownloadUpdate(const std::string &url, std::filesystem::path*file, std::filesystem::path*signatureFile);
    private:
        std::string GetUpdateFilename(const std::string &url);
        std::string GetSignatureUrl(const std::string &url);

        UpdatePolicyT updatePolicy = UpdatePolicyT::ReleaseOrBeta;
        using UpdateReleasePredicate = std::function<bool(const GithubRelease &githubRelease)>;

        UpdateRelease getUpdateRelease(
            const std::vector<GithubRelease> &githubReleases,
            const std::string &currentVersion,
            const UpdateReleasePredicate &predicate);

        UpdateStatus cachedUpdateStatus;
        bool stopped = false;
        using clock = std::chrono::steady_clock;

        int event_reader = -1;
        int event_writer = -1;
        void ThreadProc();
        void CheckForUpdate(bool useCache);
        UpdateListener listener;

        std::unique_ptr<std::thread> thread;
        std::mutex mutex;

        bool hasInfo = false;
        UpdateStatus currentResult;
        static clock::duration updateRate;
    };
}