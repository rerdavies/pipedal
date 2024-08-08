#include "pch.h"
#include "CommandLineParser.hpp"
#include "PiPedalException.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string.h>
#include <set>
#include "HtmlHelper.hpp"

using namespace pipedal;
using namespace std;

#define UNKNOWN_LICENSE "~unknown" // primary purpose: show this group last, not first.

std::vector<std::string> unknownLicense ={
    "Unknown License",
    "."
    "These copyright notices were found, but we were unable to automatically identify an associated license."
};

bool ContainsName(const std::string &copyright, const char*name)
{
    return copyright.find(name) != -1;
}

class Copyrights {
    struct License {
        std::string tag;
        std::vector<std::string> licenseText;
        std::set<std::string> copyrights;
    };
    std::map<std::string,std::shared_ptr<License> > licenseMap;
    static std::string trim(const std::string&value)
    {
        size_t start = 0;
        while (start < value.length() && value[start] == ' ' || value[start] == '\t')
        {
            ++start;
        }
        size_t end = value.length();
        while (end > start && value[end-1] == ' ' || value[end-1] == '\t')
        {
            --end;
        }
        return value.substr(start,end-start);
    }
    void addLicense(const std::string&license,const std::vector<std::string> &licenseText)
    {
        auto currentLicense = licenseMap.find(license);
        if (currentLicense == licenseMap.end())
        {
            std::shared_ptr<License> l = std::make_shared<License>();
            l->tag = license;
            l->licenseText = licenseText;
            licenseMap[license] = std::move(l);
        } else {
            (*currentLicense).second->licenseText = licenseText;
        }

    }

    void addCopyright(std::string license,std::string copyright)
    {
        if (copyright.length() == 0) return;
        if (copyright ==  "no-info-found") return;
        if (copyright == "info-missing") return;
        if (license == "")
        {
            license = UNKNOWN_LICENSE;
        }
        const static std::string UNSPECIFIED = "(unspecified) ";
        if (copyright.starts_with(UNSPECIFIED))
        {
            copyright = copyright.substr(UNSPECIFIED.length());
        }
        auto currentLicense = licenseMap.find(license);
        if (currentLicense == licenseMap.end())
        {
            std::shared_ptr<License> l = std::make_shared<License>();
            l->tag = license;
            l->copyrights.insert(copyright);
            licenseMap[license] = l;
        } else {
            auto & copyrights = (*currentLicense).second->copyrights;
            copyrights.insert(copyright);
        }
    }
    bool splitLicense(const std::string & license, const std::string &match, std::string*pLeft, std::string *pRight)
    {
        auto pos = license.find(match);
        if (pos != std::string::npos)
        {
            *pLeft = trim(license.substr(0,pos));
            *pRight = trim(license.substr(pos+match.length()));
            return true;
        }
        return false;
    }
    void parseLicenses(const std::string &license,std::vector<std::string>*pLicenses)
    {
        std::string l,r;
        if (splitLicense(license," and ",&l,&r))
        {
            parseLicenses(l,pLicenses);
            parseLicenses(r,pLicenses);

        } else if (splitLicense(license," or ",&l,&r))
        {
            parseLicenses(l,pLicenses);
            parseLicenses(r,pLicenses);

        } else {
            auto pos = license.find_last_of(',');
            if (pos != std::string::npos) {
                pLicenses->push_back(license.substr(0,pos));
            } else {
                pLicenses->push_back(license);
            }
        }
    }
    void addCopyright(
        std::string license, 
        const std::vector<std::string> &licenseText,
        const std::vector<std::string> &copyrights)
    {
        if (license.length() == 0  && licenseText.size() != 0)
        {
            std::stringstream uniqueName;
            uniqueName << "unique-" << licenseMap.size();
            license = uniqueName.str();
        }
        std::vector<std::string> licenses;
        parseLicenses(license,&licenses);
        for (size_t i = 0; i < licenses.size(); ++i)
        {
            for (size_t j = 0; j < copyrights.size(); ++j)
            {
                addCopyright(licenses[i], copyrights[j]);
            }
        }
        if (licenseText.size() != 0)
        {
            addLicense(license,licenseText);
        }
    }
    void writeLicenseText(std::ostream &os, const std::vector<std::string>&licenseText)
    {
        bool open = false;
        for (const std::string&line: licenseText)
        {
            if (line == ".")
            {
                if (open) {
                    os << "</p>" << endl;
                } else {
                    os << "<p>&nbsp;</p>" << endl;
                }
                open = false;
            } else {
                if (!open)
                {
                    open = true;
                    os << "<p>";
                }
                os << HtmlHelper::HtmlEncode(line) << ' ';
            }
        }
        if (open)
        {
            os << "</p>" << endl;
        }
    }
    static std::vector<std::string> copyrightPrefixes;

    std::string stripCopyrightPrefix(std::string text)
    {
        while (true)
        {
            bool found = false;
            for (auto & prefix: copyrightPrefixes)
            {
                if (text.rfind(prefix,0) != std::string::npos)
                {
                    text = trim(text.substr(prefix.length()));
                    found = true;
                    break;
                }
            }
            if (!found) {
                return text;
            }
        }
    }
public:
    void write(const std::filesystem::path&path)
    {
        ofstream f(path);
        if (!f.is_open())
        {
            throw PiPedalException(SS("Can't write to " << path));
        }
        f << "<div class='fossLicenses'>" << endl;

        f << "<div class='fossIntro'>" << endl;
        f << "<p>PiPedal uses open-source software covered by the following copyright notices and licenses.</p>" << endl;
        f << "</div>" << endl;
        for (auto &iter: licenseMap)
        {
            auto license = (iter).second;
            if (license->copyrights.size() != 0)
            {
                f << "<div class='fossCopyrights'>" << endl;
                for (auto&copyright: license->copyrights) {
                    std::string text = stripCopyrightPrefix(copyright);
                    f << " <p>\u00A9 " << HtmlHelper::HtmlEncode(text) << "</p>" << endl;
                }
                f << "</div>" << endl;
                f << "<div class='fossLicense' tag='" << license->tag << "'>" << endl;
                if (license->tag == UNKNOWN_LICENSE) {
                    writeLicenseText(f,unknownLicense);
                } else {
                    writeLicenseText(f,license->licenseText);
                }
                f << "</div>" << endl;
            }
        }
    }
    void loadFile(const std::filesystem::path&path)
    {
        std::ifstream f(path);
        if (!f.is_open()) 
        {
            stringstream s;
            s << "Can't open file " << path;
            throw PiPedalException(s.str());
        }
        std::vector<std::string> copyrights;
        std::vector<std::string> licenseText;
        std::string license;
        enum class State {
            none,
            copyright,
            license,
            other
        };
        State state = State::none;

        while (true)
        {
            if (f.eof()) break;
            std::string line;
            std::getline(f,line);
            bool isContinuation = (line[0] == ' ' || line[0] == '\t');
            line = trim(line);


            if (line.length() == 0)
            {
                if (copyrights.size() != 0 || licenseText.size() != 0)
                {
                    addCopyright(license,licenseText,copyrights);
                }
                copyrights.clear();
                license.clear();
                licenseText.clear();
            } else {
                if (isContinuation)
                {
                    switch (state)
                    {
                    case State::copyright:
                        copyrights.push_back(line);
                        break;
                    case State::none:
                        break; // permissively ignore it.
                    case State::license:
                        licenseText.push_back(line);
                        break;
                    case State::other:
                        // ignore
                        break;
                    }
                } else {
                    int nPos = line.find(':');
                    if (nPos != std::string::npos)
                    {
                        std::string tag = line.substr(0,nPos);
                        std::string arg = trim(line.substr(nPos+1));
                        if (tag == "Copyright")
                        {
                            copyrights.push_back(arg);
                            state = State::copyright;
                        } else if (tag == "License")
                        {
                            license = arg;
                            state = State::license;
                        } else {
                            state = State::other;
                        }
                    }
                }


            }

        }

    }
};

std::vector<std::string> Copyrights::copyrightPrefixes = {
    "Copyright",
    "copyright",
    "(c)",
    "(C)",
    "\u00A9"
};

int main(int argc, const char*argv[])
{
    CommandLineParser parser;
    bool help = false;
    bool helpError = false;
    std::string copyrightFile;
    std::string outputFile;
    parser.AddOption("-h",&help);
    parser.AddOption("--help",&help);   
    parser.AddOption("--projectCopyright",&copyrightFile);
    parser.AddOption("--output", &outputFile);

    try {
        parser.Parse(argc,argv);
        if (copyrightFile == "")
        {
            throw PiPedalException("--projectCopyright not specified.");
        }
        if (outputFile == "")
        {
            throw PiPedalException("--output file not specified.");
        }

    } catch (const std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        helpError = true;
    }
    if (help || helpError)
    {
        if (helpError)
        {
            cerr << endl;
        }
        cout << "Syntax: processcopyrights --projectCopyright debian/copyright -output <outputfile> [<dependents>]*" << endl;
        return helpError? EXIT_FAILURE: EXIT_SUCCESS;
    }

    Copyrights copyrights;
    copyrights.loadFile(copyrightFile);
    for (std::string dependent: parser.Arguments())
    {
        cout << "Processing copyrights for " << dependent << endl;
        std::filesystem::path dependentCopyrightFile = 
            std::filesystem::path("/usr/share/doc")
            / dependent / "copyright";
        copyrights.loadFile(dependentCopyrightFile);
    }

    // our file last, so that our notices take precedence.
    cout << "Processing copyrights for " << copyrightFile << endl;
    copyrights.loadFile(copyrightFile);


    copyrights.write(outputFile);

    cout << "Copyright notices written to " << outputFile << endl;


    return EXIT_SUCCESS;
}



