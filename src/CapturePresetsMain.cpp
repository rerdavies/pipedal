#include "Storage.hpp"
#include "CommandLineParser.hpp"
#include <filesystem>
#include <fstream>
#include "Uri.hpp"
#include "ss.hpp"
#include <iomanip>
#include "util.hpp"

using namespace pipedal;
using namespace std;
using namespace std::filesystem;

Storage storage;    

static std::string GetPluginid(const std::string &pluginUrl)
{
    uri uri_(pluginUrl);
    return uri_.segments()[uri_.segment_count() - 1];
}

static char hexC[] = "0123456788ABCDEF";

static void EncodeTurtleStringBody(std::stringstream&ss,const std::string &value)
{
    std::u32string u32String = ToUtf32(value);
    for (auto c : u32String)
    {
        if (c >= ' ' && c <= 0x7F && c != '\"' && c != '\'' && c != '<' && c != '>' && c != '\\' && c != '%')
        {
            ss << (char)c;
        }
        else if (c < 0x10000)
        {
            ss << "\\u"
               << hexC[(c >> 12) & 0x0F]
               << hexC[(c >> 8) & 0x0F]
               << hexC[(c >> 4) & 0x0F]
               << hexC[(c >> 0) & 0x0F];
        }
        else
        {
            ss << "\\U"
               << hexC[(c >> 28) & 0x0F]
               << hexC[(c >> 24) & 0x0F]
               << hexC[(c >> 20) & 0x0F]
               << hexC[(c >> 16) & 0x0F]
               << hexC[(c >> 12) & 0x0F]
               << hexC[(c >> 8) & 0x0F]
               << hexC[(c >> 4) & 0x0F]
               << hexC[(c >> 0) & 0x0F];
        }
    }

}

static std::string EncodeTurtleString(const std::string &value)
{
    std::stringstream ss;
    ss << '"';
    EncodeTurtleStringBody(ss,value);
    ss << '"';
    return ss.str();
}

static std::string ToString(const std::vector<uint8_t>&value)
{
    const char*start = (const char*)(&value[0]);
    const char*end = start+value.size();
    while (end != start && end[-1] == 0)
    {
        --end;
    }

    return std::string(start,end);

}
static std::string EncodeTurtleString(const std::vector<uint8_t> &value)
{
    return EncodeTurtleString(ToString(value));
}



static std::string EncodeTurtlePath(const std::string &value)
{
    std::stringstream ss;
    ss << '<';
    EncodeTurtleStringBody(ss,value);
    ss << '>';
    return ss.str();
}

static std::string EncodeTurtlePath(const std::vector<uint8_t>&value)
{
    return EncodeTurtlePath(ToString(value));
}

std::string GetFileName(const std::string &uri)
{
    std::string id = GetPluginid(uri);
    return id + "-presets.ttl";
}

void WritePrefixes(ofstream &f)
{
    f
        << "@prefix lv2: <http://lv2plug.in/ns/lv2core#> ." << endl
        << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> ." << endl
        << "@prefix pset: <http://lv2plug.in/ns/ext/presets#> ." << endl
        << "@prefix tpset: <http://two-play.com/plugins/preset#> ." << endl
        << "@prefix state: <http://lv2plug.in/ns/ext/state#> ." << endl

        << endl;
}

static void WriteSeeAlso(ofstream &f, const std::string &uri)
{
    std::string id = GetPluginid(uri);
    PluginPresets presets = storage.GetPluginPresets(uri);

    f << "###" << endl;

    for (size_t i = 0; i < presets.presets_.size(); ++i)
    {
        std::string presetId = SS("tpset:" << id << "-preset-" << i);
        f << presetId << endl
          << "    a pset:Preset ;" << endl
          << "    lv2:appliesTo <" << uri << "> ;" << endl
          << "    rdfs:seeAlso "
          << "<" << GetFileName(uri) << "> ." << endl;
        f << endl;
    }
}

std::string EncodeTurtleFloat(float value)
{
    int64_t iValue = (int64_t)value;
    if (iValue == value)
    {
        return SS(iValue << ".0");
    }
    std::stringstream s;
    s << setprecision(7) << value;
    return s.str();
}
std::string EncodeTurtleDouble(double value)
{
    int64_t iValue = (int64_t)value;
    std::stringstream s;
    s << '\"';
    if (iValue == value)
    {
    if (iValue == value)
    {
        s << iValue << ".0";
    }

    } else {
        s << setprecision(15) << value;
    }
    s << "\"^^xsd:double";
    return s.str();
}

void WritePresetFile(const path &outputDirectory, const std::string &uri)
{
    std::string id = GetPluginid(uri);
    path fileName = outputDirectory / GetFileName(uri);
    ofstream f(fileName);
    if (!f.is_open())
    {
        throw logic_error(SS("Can't write to " << fileName));
    }

    cout << "Writing preset file  " << fileName << endl;


    WritePrefixes(f);

    PluginPresets presets = storage.GetPluginPresets(uri);

    for (size_t i = 0; i < presets.presets_.size(); ++i)
    {
        std::string presetId = SS("tpset:" << id << "-preset-" << i);
        const auto &preset = presets.presets_[i];

        f << presetId << endl
          << "    a pset:Preset ; " << endl
          << "    lv2:appliesTo <" << uri << "> ;" << endl
          << "    rdfs:label " << EncodeTurtleString(preset.label_) << " ;" << endl;
        if (preset.controlValues_.size() != 0)
        {
            f << "    lv2:port ";
            bool firstTime = true;
            for (const auto &value : preset.controlValues_)
            {
                if (!firstTime)
                {
                    f << " , ";
                }
                firstTime = false;
                f << "[" << endl
                  << "        lv2:symbol " << EncodeTurtleString(value.first) << " ;" << endl
                  << "        pset:value " << EncodeTurtleFloat(value.second) << " ;" << endl;
                f << "    ]";
            }
            if (preset.state_.isValid_ && preset.state_.values_.size() != 0)
            {
                f << " ;" << endl;
                f << "    state:state [" << endl;
                for (const auto &stateValue : preset.state_.values_)
                {
                    f << "        <" 
                        << stateValue.first << "> ";
                    const auto& t = stateValue.second;
                    if (t.atomType_ == LV2_ATOM__String)
                    {
                        f << EncodeTurtleString(t.value_);
                    } else if (t.atomType_ == LV2_ATOM__Path)
                    {
                        // encode it as a string, because Turtle paths have additional sematics.
                        // The type will be corrected at restore time.
                        f << EncodeTurtleString(t.value_);
                    } else if (t.atomType_ == LV2_ATOM__Float)
                    {
                        float *v = (float*)(&(t.value_[0]));
                        f << EncodeTurtleFloat(*v);
                    } else if (t.atomType_ == LV2_ATOM__Double)
                    {
                        double *v = (double*)(&(t.value_[0]));
                        f << EncodeTurtleDouble(*v);
                    } else if (t.atomType_ == LV2_ATOM__Int)
                    {
                        int32_t *v = (int32_t*)(&t.value_[0]);
                        f << *v;
                    } else if (t.atomType_ == LV2_ATOM__Long)
                    {
                        int64_t *v = (int64_t*)(&t.value_[0]);
                        f << *v;
                    }
                    f << " ;" << endl;

                }
                f << "    ] ." << endl;

            } else {
                f << " ." << endl;
            }
        }
        f << endl;
    }
}

int main(int argc, const char *argv[])
{
    // A utility for the ToobAmp project which converts Pipedal presets to LV2 presets so
    // that they can be imported as pre-declared presets respective TTL files.

    path outputPath = "/tmp/ToobPresets";

    // Capture all ToobAmp plugin presets in LV2 format.
    // CommandLineParser commandLineParser;
    // commandLineParser.Parse(argc,argv);
    try
    {

        storage.SetConfigRoot("/etc/pipedal/config");
        storage.SetDataRoot("/var/pipedal");
        storage.Initialize();

        const PluginPresetIndex &index = storage.GetPluginPresetIndex();

        std::filesystem::create_directories(outputPath);

        {
            ofstream f(outputPath / "add_to_manifest.ttl");

            WritePrefixes(f);

            for (const auto &entry : index.entries_)
            {
                if (entry.pluginUri_.starts_with("http://two-play.com/plugins/"))
                {
                    cout << "Writing manifest.ttl declerations: " << entry.pluginUri_ << endl;
                    WriteSeeAlso(f, entry.pluginUri_);
                }
            }
        }

        for (const auto &entry : index.entries_)
        {
            if (entry.pluginUri_.starts_with("http://two-play.com/plugins/"))
            {
                WritePresetFile(outputPath, entry.pluginUri_);
            }
        }
    }
    catch (const std::exception &e)
    {
        cout << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
