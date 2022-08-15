#include <string>
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/processdata.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include "public.sdk/samples/vst-hosting/audiohost/source/media/iparameterclient.h"
#include "public.sdk/samples/vst-hosting/audiohost/source/media/imediaserver.h"

using namespace std;
using namespace VST3::Hosting;
using namespace Steinberg;
using namespace Steinberg::Vst;

PluginFactory::ClassInfos GetVst3ClassInfos(const std::string& pluginPath)
{
    std::string error;
    Module::Ptr module = Module::create(pluginPath.c_str(), error);

    if (!module)
    {
        assert(false && "Vst3 load failed.");
    }
    const PluginFactory &factory = module->getFactory();

    return factory.classInfos();
}

static std::string HomePath() { return getenv("HOME"); }

void BugReportTest()
{
    auto _ = GetVst3ClassInfos(HomePath() + "/.vst3/adelay.vst3");
    PluginFactory::ClassInfos classInfos = GetVst3ClassInfos(HomePath() + "/.vst3/mda-vst3.vst3");

    // right size.
    assert(classInfos.size() == 68); // succeeds

    // wrong classInfo (from adelay.vst3)
    assert(classInfos[0].name() == "mda Ambience"); // actual result "ADelay"
}
