#include "pch.h"
#include "RegDb.hpp"
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <arpa/inet.h>
#include "PiPedalException.hpp"
#include <cstring>
#include <fcntl.h>

#define REGDB_MAGIC 0x52474442
#define REGDB_VERSION 19


using namespace pipedal;
using namespace std;

struct RegDbFileHeader {
    uint32_t magic; 
    uint32_t version;
    uint32_t countryOffset;
    uint32_t countryCount;
    uint32_t signatureLength;

    void toNs() {
        this->magic = htonl(this->magic);
        this->version = htonl(this->version);
        this->countryOffset = htonl(this->countryOffset);
        this->countryCount = htonl(this->countryCount);
        this->signatureLength = htonl(this->signatureLength);
    }
};

struct CountryHeader {
    char alpha2[2];
    uint8_t pad;
    uint8_t dfsRegion; // first 2 bits only.
    uint32_t rulesOffset;

    void toNs() {
        this->rulesOffset = htonl(rulesOffset);
    }

};
struct RulesCollection {
    uint32_t ruleCount;
    uint32_t ruleOffsets[1];

    void toNs() {
        this->ruleCount = htonl(this->ruleCount);
        uint32_t *p = ruleOffsets;
        for (uint32_t i = 0; i < this->ruleCount; ++i)
        {
            *p = htonl(*p);
            ++p;
        }
    }
};

struct Rule {
    uint32_t frequencyRangeOffset;
    uint32_t powerRuleOffset;
    uint32_t flags;

    void toNs() {
        frequencyRangeOffset = htonl(frequencyRangeOffset);
        powerRuleOffset = htonl(powerRuleOffset);
        flags = htonl(flags);
    }
};

struct FrequencyRange {
    uint32_t startFrequency; // in khz.
    uint32_t endFrequency;   // in khz.
    uint32_t maxBandwidth;   // in khz.
    void toNs() {
        startFrequency = htonl(startFrequency);
        endFrequency = htonl(endFrequency);
        maxBandwidth = htonl(maxBandwidth);
    }
};
struct PowerRule {
    uint32_t maximumAntennaGain;
    uint32_t maximumEirp;
    void toNs() {
        maximumAntennaGain = htonl(maximumAntennaGain);
        maximumEirp = htonl(maximumEirp);
    }
};



const char*filePaths[] = 
{
    "/usr/lib/crda/regulatory.bin"
};

static std::filesystem::path getFilePath()
{
    for (size_t i = 0; i < sizeof(filePaths)/sizeof(filePaths[0]); ++i)
    {
        std::filesystem::path path(filePaths[i]);
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    throw PiPedalException("Could not find 'regulatory.bin' file.");
}

RegDb::~RegDb()
{
    delete[] pData;
}

RegDb::RegDb()
:pData(nullptr)
{
    RegDbFileHeader header;
    std::filesystem::path path = getFilePath();
    ifstream f(path);
    if (!f.is_open()) {
        stringstream s;
        s << "Can't read " << path;
        throw PiPedalException(s.str());
    }
    size_t fileSize = std::filesystem::file_size(path);
    this->pData = new uint8_t[fileSize];
    f.read((char*)this->pData,fileSize);

    // convert from big-endian to local byte order;
    RegDbFileHeader *fileHeader = (RegDbFileHeader*)pData;
    fileHeader->toNs();
    if (fileHeader->magic != REGDB_MAGIC)
    {
        throw PiPedalException("Invalid file format.");
    }
    std::unordered_set<uint32_t> visited;

    CountryHeader*countryHeader = (CountryHeader*)(pData + fileHeader->countryOffset);
    for (uint32_t i = 0; i < fileHeader->countryCount; ++i)
    {
        countryHeader->toNs();
        if (visited.count(countryHeader->rulesOffset) == 0)
        {
            visited.insert(countryHeader->rulesOffset);
            RulesCollection*pRules = (RulesCollection*)(pData + countryHeader->rulesOffset);
            pRules->toNs();
            for (uint32_t nRule = 0; nRule < pRules->ruleCount; ++nRule)
            {
                uint32_t ruleOffset = pRules->ruleOffsets[nRule];
                if (visited.count(ruleOffset) == 0)
                {
                    visited.insert(ruleOffset);
                    Rule *pRule = (Rule*)(pData + ruleOffset);
                    pRule->toNs();

                    if (visited.count(pRule->frequencyRangeOffset) == 0)
                    {
                        visited.insert(pRule->frequencyRangeOffset);

                        FrequencyRange*pf = (FrequencyRange*)(pData + pRule->frequencyRangeOffset);
                        pf->toNs();
                    }
                    if (visited.count(pRule->powerRuleOffset) == 0)
                    {
                        visited.insert(pRule->powerRuleOffset);

                        PowerRule *pr = (PowerRule*)(pData + pRule->powerRuleOffset);
                        pr->toNs();
                    }
                }
            }
        }
        ++countryHeader;
    }
 
}
bool CrdbRules::GetFrequencyRule(uint32_t frequencyRuleMHz, FrequencyRule*pResult)
{
    memset(pResult,0,sizeof(FrequencyRule));
    RulesCollection*pRules = (RulesCollection*)(regDb->pData + countryOffset);
    uint32_t frequencyRuleKHz = frequencyRuleMHz*1000;
    for (uint32_t i = 0; i < pRules->ruleCount; ++i)
    {
        Rule *pRule = (Rule*)(regDb->pData + pRules->ruleOffsets[i]);
        FrequencyRange*pf = (FrequencyRange*)(regDb->pData + pRule->frequencyRangeOffset);
        if (frequencyRuleKHz >= pf->startFrequency && frequencyRuleKHz < pf->endFrequency)
        {
            PowerRule *pr = (PowerRule*)(regDb->pData + pRule->powerRuleOffset);
            pResult->flags = (RegRuleFlags)(pRule->flags);
            pResult->maximumAntennaGain = pr->maximumAntennaGain;
            pResult->maximumEirp = pr->maximumEirp;
            return true;
        }
    }
    return false;
}
CrdbRules*RegDb::GetCrdbRules(const char*countryCodeIso633)
{
    const char c0 = countryCodeIso633[0];
    if (c0 == '\0') return nullptr;
    const char c1 = countryCodeIso633[1];

    RegDbFileHeader *fileHeader = (RegDbFileHeader*)pData;
    CountryHeader*countryHeader = (CountryHeader*)(pData + fileHeader->countryOffset);
    for (uint32_t i = 0; i < fileHeader->countryCount; ++i)
    {
        if (countryHeader->alpha2[0] == c0 && countryHeader->alpha2[1] == c1) {
            return new CrdbRules(
                this,
                countryHeader->rulesOffset,
                (DfsRegion)(countryHeader->dfsRegion & 0x03)
            );
        }
        ++countryHeader;
    }
    return nullptr;
}

CrdbRules::CrdbRules(RegDb *regDb,uint32_t countryOffset, DfsRegion dfsRegion )
:   regDb(regDb),
    countryOffset(countryOffset),
    dfsRegion(dfsRegion)
{
}

DfsRegion CrdbRules::GetDfsRegion() const {
    return this->dfsRegion;
}

