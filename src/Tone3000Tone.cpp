/*
 *   Copyright (c) Robin E.R. Davies
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

#include "Tone3000Tone.hpp"
#include "ss.hpp"



using namespace pipedal;
using namespace pipedal::tone3000;
namespace fs = std::filesystem;


Tone::Tone(const std::string &json)
{
    std::istringstream ss(json);
    json_reader reader(ss);
    reader.read(this);
}

namespace
{
    static std::string mdSanitize(const std::string &str, const std::string &illegalCharacters = "")
    {
        std::ostringstream os;
        for (char c : str)
        {
            if ((unsigned char)c < 0x20)
                continue;
            if (c == '<')
            {
                os << "&lt;";
                continue;
            }
            auto npos = illegalCharacters.find_first_of(c);
            if (npos != std::string::npos)
            {
                continue;
            }
            os << c;
        }
        return os.str();
    }

    static std::string mdDate(const tone3000_time_point &date_)
    {
        std::ostringstream os;
        // C++ doesn't handle timezones. Just zap the timezone, and replace it with "Z"
        std::string date = date_;

        // Remove timezone offset if present and replace with Z
        size_t plusPos = date.find('+');
        if (plusPos != std::string::npos)
        {
            date = date.substr(0, plusPos) + "Z";
        }
        // Parse ISO 8601 date string into tm
        std::tm tm = {};
        std::istringstream ss(date);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail())
        {
            os << date;
            return os.str();
        }

        os << std::put_time(&tm, "%x");
        return os.str();
    }

    static std::string mdEnum(const std::string &value, const std::map<std::string, std::string> &enumValues)
    {
        auto ff = enumValues.find(value);
        if (ff == enumValues.end())
        {
            return value;
        }
        return ff->second;
    }

    static std::string mdEnumList(
        const std::vector<std::string> &values,
        const std::map<std::string, std::string> &enumValues)
    {
        if (values.size() == 1)
        {
            return mdEnum(values[0], enumValues);
        }
        std::ostringstream os;
        os << '[';
        bool first = true;
        for (const auto &value : values)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                os << ", ";
            }
            os << mdEnum(value, enumValues);
        }
        os << ']';
        return os.str();
    }

    static std::map<std::string, std::string> sizeEnumValues =
        {
            {"standard", "Standard"},
            {"lite", "Lite"},
            {"feather", "Feather"},
            {"nano", "Nano"},
            {"custom", "Custom"}};
    static std::string mdSizes(const std::vector<std::string> &sizes)
    {
        return mdEnumList(sizes, sizeEnumValues);
    }

    static std::map<std::string, std::string> gearEnumValues =
        {
            {"amp", "Amp only"},
            {"full-rig", "Full rig"},
            {"pedal", "Pedal"},
            {"outboard", "Outboard"},
            {"ir", "IR"}};

    static std::string mdGear(const std::string &gear)
    {
        return mdEnum(gear, gearEnumValues);
    }

    namespace
    {
        enum class LicenseFlags
        {
            None = 0,
            Cc = 1,
            By = 2,
            CcBy = 3, // = Cc | By
            Sa = 4,
            Nc = 8,
            Nd = 16,
            Cc0 = 32,
        };

        static bool operator&(LicenseFlags v1, LicenseFlags v2)
        {
            return (((int)v1) & ((int)v2)) != 0;
        }
        static LicenseFlags operator|(LicenseFlags v1, LicenseFlags v2)
        {
            return (LicenseFlags)(((int)(v1)) | ((int)(v2)));
        }

        struct LicenseInfo
        {
            std::string key;
            std::string displayName;
            std::string url;
            LicenseFlags licenseFlags;
        };
    }

    static std::vector<LicenseInfo> licenses{
        {.key = "t3k",
         .displayName = "T3K",
         .url = "licenses/t3k_license.html",
         .licenseFlags = LicenseFlags::None},
        {.key = "cc-by",
         .displayName = "CC BY 4.0",
         .url = "https://creativecommons.org/licenses/by/4.0/",
         .licenseFlags = LicenseFlags::CcBy},
        {.key = "cc-by-sa",
         .displayName = "CC BY-SA 4.0",
         .url = "https://creativecommons.org/licenses/by-sa/4.0/",
         .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Sa},
        {.key = "cc-by-nc",
         .displayName = "CC BY-NC 4.0",
         .url = "https://creativecommons.org/licenses/by-nc/4.0/",
         .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Nc},
        {.key = "cc-by-nc-sa",
         .displayName = "CC BY-NC-SA 4.0",
         .url = "https://creativecommons.org/licenses/by-nc-sa/4.0/",
         .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Nc | LicenseFlags::Sa},
        {.key = "cc-by-nd",
         .displayName = "CC BY-ND 4.0",
         .url = "https://creativecommons.org/licenses/by-nd/4.0/",
         .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Nd},
        {.key = "cc-by-nc-nd",
         .displayName = "CC BY-NC-ND 4.0",
         .url = "https://creativecommons.org/licenses/by-nc-nd/4.0/",
         .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Nc | LicenseFlags::Nd},
        {
            .key = "cco",
            .displayName = "CC0",
            .url = "https://creativecommons.org/publicdomain/zero/1.0/",
            .licenseFlags = LicenseFlags::Cc | LicenseFlags::Cc0,

        },
    };

    static std::map<std::string, LicenseInfo> makeLicenseEnum()
    {
        std::map<std::string, LicenseInfo> result;
        for (const auto &license : licenses)
        {
            result[license.key] = license;
        }
        return result;
    }

    static std::map<std::string, LicenseInfo> licenseEnum = makeLicenseEnum();

    static std::string mdLicenseIcon(LicenseFlags licenseFlag)
    {
        std::ostringstream os;
        std::string url;

        switch (licenseFlag)
        {
        case LicenseFlags::Cc:
            url = "img/cc.svg";
            break;
        case LicenseFlags::By:
            url = "img/by.svg";
            break;
        case LicenseFlags::Sa:
            url = "img/sa.svg";
            break;
        case LicenseFlags::Nc:
            url = "img/nc.svg";
            break;
        case LicenseFlags::Nd:
            url = "img/nd.svg";
            break;
        case LicenseFlags::Cc0:
            url = "img/cc0.svg";
            break;

        case LicenseFlags::None:
            throw std::runtime_error("Invalid argument.");
        }
        os << "<img id='cc_img' src='" << url << "'/>";
        return os.str();
    }

    static void mdEscapeString(std::ostream &f, const std::string &str)
    {
        size_t ix = 0;
        while (ix < str.size())
        {
            char c = str[ix++];
            if (c == '\n')
            {
                if (ix < str.size() && str[ix] == '\n')
                {
                    f << "\n\n";
                }
                else
                {
                    f << "  \n"; // a <br> in md.
                }
            }
            else
            {
                f << c;
            }
        }
    }

    static std::string mdHref(const std::string &url)
    {
        std::ostringstream os;
        os << '\'';
        for (char c : url)
        {
            if (c == '\'')
            {
                continue;
            }
            os << c;
        }
        os << '\'';
        return os.str();
    }
    static std::string mdLicense(const std::string &license)
    {

        auto ff = licenseEnum.find(license);
        LicenseInfo licenseInfo;
        if (ff != licenseEnum.end())
        {
            licenseInfo = ff->second;
        }
        else
        {
            licenseInfo.displayName = license;
        }

        std::ostringstream os;

        if (licenseInfo.licenseFlags != LicenseFlags::None)
        {
            for (LicenseFlags licenseFlag : std::vector<LicenseFlags>{LicenseFlags::Cc, LicenseFlags::By, LicenseFlags::Cc0, LicenseFlags::Nc, LicenseFlags::Sa, LicenseFlags::Nd})
            {
                if (licenseInfo.licenseFlags & licenseFlag)
                {
                    os << mdLicenseIcon(licenseFlag);
                }
            }
            os << " ";
        }

        if (licenseInfo.url != "")
        {
            os << "<a href="
               << mdHref(licenseInfo.url)
               << " target='_blank' rel='noopener noreferrer' >" << mdSanitize(licenseInfo.displayName) << "</a>";
        }
        else
        {
            os << mdSanitize(licenseInfo.displayName);
        }
        return os.str();
    }
    // static std::string mdTestLicenses()
    // {
    //     std::ostringstream os;
    //     for (auto license: licenseEnum)
    //     {
    //         os << "<br/>" << mdLicense(license.first) << "\n";
    //     }
    //     return os.str();
    // }
    static std::map<std::string, std::string> platformEnumValues =
        {
            {"nam", "NAM"},
            {"ir", "IR"},
            {"aida-x", "Aida X"},
            {"aa-snapshot", "aa-snapshot"},
            {"proteus", "Proteus"}};
    static std::string mdPlatform(const std::string &platform)
    {
        return mdEnum(platform, platformEnumValues);
    }
    static std::string mdUser(const tone3000::User &user)
    {
        return mdSanitize(user.username());
    }

    static void mdLink(std::ostream &f, const std::string &label, const std::string &url)
    {
        f << "[" << mdSanitize(label, "]") << "](" << mdSanitize(url, ")") << ")";
    }

};

void pipedal::tone3000::WriteTone3000Readme(const std::filesystem::path &filePath, const tone3000::Tone &tone, const std::string &thumbnailUrl_)
{
    std::string thumbnailUrl = SS("/var/tone3000_thumbnail?id=" << tone.id());
    std::ofstream of{filePath};
    if (!of.is_open())
    {
        throw std::runtime_error(SS("Unable to create file " << filePath));
    }

    // clang-format off
        of <<

                "<div id='tone3000_float'  >\n";
        if (!thumbnailUrl.empty())
        {

            of <<  "    <img id='tone3000_thumbnail' src='"
                            << thumbnailUrl << "' alt=''  />\n";
        }
        of  <<  "    <div id='tone3000_float_content' >\n"
            <<  "        <h3 id='tone3000_title'>" << mdSanitize(tone.title()) << "</h3>\n"
            <<  "        <div>\n"
            <<  "            <p id='tone3000_subtitle'>\n"
                            << mdDate(tone.updated_at())
                            << " "
                            << mdUser(tone.user())
            <<  "            <br/>\n"
            <<  "            ";
            if (tone.sizes().has_value() && tone.sizes().value().size() != 0) {
                of
                    << mdSizes(tone.sizes().value()) << ", "
                ;
            }
            if (tone.platform() == "ir")
            {
                of
                        << (mdPlatform(tone.platform()));
            } else {
                of
                        << mdGear(tone.gear()) << ", "
                        << mdPlatform(tone.platform());
            }
            of           << "<br/>\n";
            if (tone.license() != "") {
                of <<  "            License: " << mdLicense(tone.license()) << "\n";
            }
            of
    //        << "          " << mdTestLicenses()
            <<  "        </p>\n"
            <<  "        </div>\n"
            <<  "    </div>\n"
            << "</div>\n";
    // clang-format on
    of << "\n\n";
    mdEscapeString(of, tone.description());

    if (tone.links().size() > 0)
    {
        of << std::endl;
        of << "## Links" << std::endl
           << std::endl;
        for (auto &link : tone.links())
        {
            of << "- " << "[" << link << "](" << link << ")" << std::endl;
        }
    }
}

JSON_MAP_BEGIN(Model)
JSON_MAP_REFERENCE(Model, id)
JSON_MAP_REFERENCE(Model, name)
JSON_MAP_REFERENCE(Model, model_url)
JSON_MAP_REFERENCE(Model, created_at)
JSON_MAP_REFERENCE(Model, size)
JSON_MAP_REFERENCE(Model, user_id)
JSON_MAP_END()

JSON_MAP_BEGIN(User)
JSON_MAP_REFERENCE(User, id)
JSON_MAP_REFERENCE(User, username)
JSON_MAP_REFERENCE(User, avatar_url)
JSON_MAP_REFERENCE(User, url)
JSON_MAP_END()

JSON_MAP_BEGIN(Tone)
JSON_MAP_REFERENCE(Tone, id)
JSON_MAP_REFERENCE(Tone, user_id)
JSON_MAP_REFERENCE(Tone, title)
JSON_MAP_REFERENCE(Tone, description)
JSON_MAP_REFERENCE(Tone, created_at)
JSON_MAP_REFERENCE(Tone, updated_at)
JSON_MAP_REFERENCE(Tone, gear)
JSON_MAP_REFERENCE(Tone, images)
JSON_MAP_REFERENCE(Tone, is_public)
JSON_MAP_REFERENCE(Tone, links)
JSON_MAP_REFERENCE(Tone, platform)
JSON_MAP_REFERENCE(Tone, models_count)
JSON_MAP_REFERENCE(Tone, favorites_count)
JSON_MAP_REFERENCE(Tone, downloads_count)
JSON_MAP_REFERENCE(Tone, license)
JSON_MAP_REFERENCE(Tone, sizes)
JSON_MAP_REFERENCE(Tone, a1_models_count)
JSON_MAP_REFERENCE(Tone, a2_models_count)
JSON_MAP_REFERENCE(Tone, irs_count)
JSON_MAP_REFERENCE(Tone, custom_models_count)
JSON_MAP_REFERENCE(Tone, user)
JSON_MAP_REFERENCE(Tone, url)
JSON_MAP_REFERENCE(Tone, user)
JSON_MAP_END();
