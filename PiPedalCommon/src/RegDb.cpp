// Copyright (c) 2022 Robin Davies
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

#include "RegDb.hpp"

#include "ss.hpp"
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <bit>
#include <algorithm>
#include <array>
#include <memory>
#include "json_variant.hpp"
#include "json.hpp"

#include <iostream>
#include <fstream>

#define REGDB_MAGIC 0x52474442
#define REGDB_VERSION 19   // pre-bookwork
#define REGDB_VERSION_2 20 // bookworm and later.

using namespace pipedal;
using namespace std;

static std::unique_ptr<RegDb> g_Instance;
RegDb &RegDb::GetInstance()
{
    if (!g_Instance)
    {
        g_Instance = std::make_unique<RegDb>();
    }
    return *g_Instance;
}

const WifiRegulations &RegDb::getWifiRegulations(const std::string &countryIso3661) const
{
    for (const auto &regulation : this->regulations)
    {
        if (regulation.reg_alpha2 == countryIso3661)
        {
            return regulation;
        }
    }
    throw std::runtime_error("Invalid country code.");
}

template <typename T>
T NlSwap(T value)
{
    if constexpr (std::endian::native == std::endian::big)
    {
        return value;
    }
    else
    {
        auto value_rep = std::bit_cast<std::array<std::uint8_t, sizeof(T)>, T>(value);
        std::ranges::reverse(value_rep);
        return std::bit_cast<T>(value_rep);
    }
}

template <typename T>
class BigEndian
{
public:
    BigEndian() : m_value(0) {}
    explicit BigEndian(T value) : m_value(NlSwap(value)) {}
    T value() const { return NlSwap(m_value); }
    bool operator==(const BigEndian<T> &other) const { return m_value == other.m_value; }
    explicit operator bool() const { return m_value != 0; }

private:
    T m_value;
};

struct RegDbFileHeader19
{
    uint32_t magic;
    uint32_t version;
    uint32_t countryOffset;
    uint32_t countryCount;
    uint32_t signatureLength;

    void toNs()
    {
        this->magic = NlSwap(this->magic);
        this->version = NlSwap(this->version);
        this->countryOffset = NlSwap(this->countryOffset);
        this->countryCount = NlSwap(this->countryCount);
        this->signatureLength = NlSwap(this->signatureLength);
    }
};

struct CountryHeader
{
    char alpha2[2];
    uint8_t pad;
    uint8_t dfsRegion; // first 2 bits only.
    uint32_t rulesOffset;

    void toNs()
    {
        this->rulesOffset = NlSwap(rulesOffset);
    }
};

struct Rule
{
    uint32_t frequencyRangeOffset;
    uint32_t powerRuleOffset;
    uint32_t flags;

    void toNs()
    {
        frequencyRangeOffset = NlSwap(frequencyRangeOffset);
        powerRuleOffset = NlSwap(powerRuleOffset);
        flags = NlSwap(flags);
    }
};

struct RulesCollection19
{
    uint32_t ruleCount;
    uint32_t ruleOffsets[1];

    void toNs()
    {
        this->ruleCount = NlSwap(this->ruleCount);
    }
    Rule *getRule(
        uint32_t nRule,
        char *pData)
    {
        uint32_t ruleOffset = NlSwap(ruleOffsets[nRule]);
        return (Rule *)(pData + ruleOffset);
    }
};

struct FrequencyRange
{
    uint32_t startFrequency; // in khz.
    uint32_t endFrequency;   // in khz.
    uint32_t maxBandwidth;   // in khz.
    void toNs()
    {
        startFrequency = NlSwap(startFrequency);
        endFrequency = NlSwap(endFrequency);
        maxBandwidth = NlSwap(maxBandwidth);
    }
};
struct PowerRule
{
    uint32_t maximumAntennaGain;
    uint32_t maximumEirp;
    void toNs()
    {
        maximumAntennaGain = NlSwap(maximumAntennaGain);
        maximumEirp = NlSwap(maximumEirp);
    }
};

const char *filePaths[] =
    {
        "/lib/firmware/regulatory.db", // bookworm, bullseye.
        "/usr/lib/crda/regulatory.bin" // previous versions?.
};

static std::filesystem::path getFilePath()
{
    for (size_t i = 0; i < sizeof(filePaths) / sizeof(filePaths[0]); ++i)
    {
        std::filesystem::path path(filePaths[i]);
        if (std::filesystem::exists(path))
        {
            return path;
        }
    }
    throw std::runtime_error("Could not find 'regulatory.bin' file.");
}

RegDb::~RegDb()
{
}

RegDb::RegDb()
    : RegDb(getFilePath())
{
}
RegDb::RegDb(const std::filesystem::path &path)
    : pData(nullptr)
{
    ifstream f(path);
    if (!f.is_open())
    {
        stringstream s;
        s << "Can't read " << path;
        throw std::runtime_error(s.str());
    }
    size_t fileSize = std::filesystem::file_size(path);
    data.resize(fileSize);
    f.read((char *)&data[0], fileSize);
    if (!f)
    {
        stringstream s;
        s << "Can't read " << path;
        throw std::runtime_error(s.str());
    }

    RegDbFileHeader19 header = *(RegDbFileHeader19 *)ptr();
    header.toNs();

    if (header.magic != REGDB_MAGIC)
    {
        throw std::runtime_error(SS("Invalid file format. " << path));
    }
    if (header.version == 19)
    {
        Load19();
    }
    else if (header.version == 20)
    {
        Load20();
    }
    else
    {
        throw std::runtime_error(SS("Unknown file version. " << path));
    }
}
void RegDb::Load19()
{
    char *pData = (char *)ptr();
    RegDbFileHeader19 fileHeader = *(RegDbFileHeader19 *)ptr();
    fileHeader.toNs();

    this->regulations.resize(0);

    CountryHeader *pCountryHeader = (CountryHeader *)(pData + fileHeader.countryOffset);
    for (uint32_t i = 0; i < fileHeader.countryCount; ++i)
    {
        WifiRegulations wifiRegulations;

        CountryHeader countryHeader = pCountryHeader[i];
        countryHeader.toNs();
        wifiRegulations.reg_alpha2 = SS(countryHeader.alpha2[0] << countryHeader.alpha2[1]);
        wifiRegulations.dfs_region = (DfsRegion)(countryHeader.dfsRegion & 0x03);

        RulesCollection19 *pRules = (RulesCollection19 *)(pData + countryHeader.rulesOffset);
        uint32_t ruleCount = NlSwap(pRules->ruleCount);
        for (uint32_t nRule = 0; nRule < ruleCount; ++nRule)
        {
            Rule *pRule = pRules->getRule(nRule, pData);
            Rule rule = *pRule;
            rule.toNs();

            FrequencyRange fr = *(FrequencyRange *)(pData + rule.frequencyRangeOffset);
            fr.toNs();

            PowerRule pr = *(PowerRule *)(pData + rule.powerRuleOffset);
            pr.toNs();

            WifiRule wifiRegulation;
            wifiRegulation.flags = (RegRuleFlags)rule.flags;
            wifiRegulation.start_freq_khz = fr.startFrequency;
            wifiRegulation.end_freq_khz = fr.endFrequency;
            wifiRegulation.max_bandwidth_khz = fr.maxBandwidth;
            wifiRegulation.max_antenna_gain = pr.maximumAntennaGain;
            wifiRegulation.max_eirp = pr.maximumEirp;
            wifiRegulations.rules.push_back(wifiRegulation);
        }
        this->regulations.push_back(std::move(wifiRegulations));
    }
    this->isValid = true;
}

////////////////////////////////////////////////////////////////////////////////
// glue
#define IEEE80211_NUM_ACS 4 // linux/ieee80211.h  Number of hardware queues?

#define __aligned(n)
#define __packed

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using s__be32 = BigEndian<uint32_t>;
using s__be16 = BigEndian<uint16_t>;
#define BIT(n) (1 << n)

template <typename T>
T ALIGN(T value, int bits)
{
    intptr_t v = (intptr_t)value;

    intptr_t mask = (1 << bits) - 1;
    v = (v + mask) & (~mask);
    return (T)v;
}

inline BigEndian<uint32_t> cpu_to_be32(uint32_t value)
{
    return BigEndian<uint32_t>(value);
}

inline BigEndian<uint16_t> cpu_to_be16(uint16_t value)
{
    return BigEndian<uint16_t>(value);
}

inline uint16_t be16_to_cpu(const BigEndian<uint16_t> &value)
{
    return value.value();
}
inline uint32_t be32_to_cpu(const BigEndian<uint32_t> &value)
{
    return value.value();
}

bool regdb_has_valid_signature(const u8 *data, unsigned int size) { return true; }

#define offsetofend(TYPE, FIELD) ((uint32_t)(offsetof(TYPE, FIELD) + sizeof(((TYPE *)0)->FIELD)))
#define ASSERT_RTNL() // in the original, assert that rtnl is not locked.

/////////////////////////
// shamelessly lifted from linux net/wireless/reg.c

struct fwdb_country
{
    u8 alpha2[2];
    s__be16 coll_ptr;
    /* this struct cannot be extended */
} __packed __aligned(4);

struct fwdb_collection
{
    u8 len;
    u8 n_rules;
    u8 dfs_region;
    /* no optional data yet */
    /* aligned to 2, then followed by s__be16 array of rule pointers */
} __packed __aligned(4);

struct fwdb_header
{
    s__be32 magic;
    s__be32 version;
    struct fwdb_country country[];
} __packed __aligned(4);

enum fwdb_flags
{
    FWDB_FLAG_NO_OFDM = BIT(0),
    FWDB_FLAG_NO_OUTDOOR = BIT(1),
    FWDB_FLAG_DFS = BIT(2),
    FWDB_FLAG_NO_IR = BIT(3),
    FWDB_FLAG_AUTO_BW = BIT(4),
};

struct fwdb_wmm_ac
{
    u8 ecw;
    u8 aifsn;
    s__be16 cot;
} __packed;

struct fwdb_wmm_rule
{
    struct fwdb_wmm_ac client[IEEE80211_NUM_ACS];
    struct fwdb_wmm_ac ap[IEEE80211_NUM_ACS];
} __packed;

struct fwdb_rule
{
    u8 len;
    u8 flags;
    s__be16 max_eirp;
    s__be32 start, end, max_bw;
    /* start of optional data */
    s__be16 cac_timeout;
    s__be16 wmm_ptr;
} __packed __aligned(4);

#define FWDB_MAGIC 0x52474442
#define FWDB_VERSION 20

static int ecw2cw(int ecw)
{
    return (1 << ecw) - 1;
}

static bool valid_wmm(struct fwdb_wmm_rule *rule)
{
    struct fwdb_wmm_ac *ac = (struct fwdb_wmm_ac *)rule;
    int i;

    for (i = 0; i < IEEE80211_NUM_ACS * 2; i++)
    {
        u16 cw_min = ecw2cw((ac[i].ecw & 0xf0) >> 4);
        u16 cw_max = ecw2cw(ac[i].ecw & 0x0f);
        u8 aifsn = ac[i].aifsn;

        if (cw_min >= cw_max)
            return false;

        if (aifsn < 1)
            return false;
    }

    return true;
}

static bool valid_rule(const u8 *data, unsigned int size, u16 rule_ptr)
{
    struct fwdb_rule *rule = (fwdb_rule *)(data + (rule_ptr << 2));

    if ((u8 *)rule + sizeof(rule->len) > data + size)
        return false;

    /* mandatory fields */
    if (rule->len < offsetofend(struct fwdb_rule, max_bw))
        return false;
    if (rule->len >= offsetofend(struct fwdb_rule, wmm_ptr))
    {
        u32 wmm_ptr = be16_to_cpu(rule->wmm_ptr) << 2;
        struct fwdb_wmm_rule *wmm;

        if (wmm_ptr + sizeof(struct fwdb_wmm_rule) > size)
            return false;

        wmm = (fwdb_wmm_rule *)(data + wmm_ptr);

        if (!valid_wmm(wmm))
            return false;
    }
    return true;
}

static bool valid_country(const u8 *data, unsigned int size,
                          const struct fwdb_country *country)
{
    unsigned int ptr = be16_to_cpu(country->coll_ptr) << 2;
    struct fwdb_collection *coll = (struct fwdb_collection *)(data + ptr);
    s__be16 *rules_ptr;
    unsigned int i;

    /* make sure we can read len/n_rules */
    if ((u8 *)coll + offsetofend(typeof(*coll), n_rules) > data + size)
        return false;

    /* make sure base struct and all rules fit */
    if ((u8 *)coll + ALIGN(coll->len, 2) +
            (coll->n_rules * 2) >
        data + size)
        return false;

    /* mandatory fields must exist */
    if (coll->len < offsetofend(struct fwdb_collection, dfs_region))
        return false;

    rules_ptr = (s__be16 *)((u8 *)coll + ALIGN(coll->len, 2));

    for (i = 0; i < coll->n_rules; i++)
    {
        u16 rule_ptr = be16_to_cpu(rules_ptr[i]);

        if (!valid_rule(data, size, rule_ptr))
            return false;
    }

    return true;
}

static bool valid_regdb(const u8 *data, unsigned int size)
{
    const struct fwdb_header *hdr = (fwdb_header *)data;
    const struct fwdb_country *country;

    if (size < sizeof(*hdr))
        return false;

    if (hdr->magic != cpu_to_be32(FWDB_MAGIC))
        return false;

    if (hdr->version != cpu_to_be32(FWDB_VERSION))
        return false;

    if (!regdb_has_valid_signature(data, size))
        return false;

    country = &hdr->country[0];
    while ((u8 *)(country + 1) <= data + size)
    {
        if (!country->coll_ptr)
            break;
        if (!valid_country(data, size, country))
            return false;
        country++;
    }

    return true;
}

static void set_wmm_rule(const struct fwdb_header *db,
                         const struct fwdb_country *country,
                         const struct fwdb_rule *rule,
                         WifiRule *rrule)
{
    // not implemented.
    return;
}
// 	struct ieee80211_wmm_rule *wmm_rule = &rrule->wmm_rule;
// 	struct fwdb_wmm_rule *wmm;
// 	unsigned int i, wmm_ptr;

// 	wmm_ptr = be16_to_cpu(rule->wmm_ptr) << 2;
// 	wmm = (fwdb_wmm_rule *)((u8 *)db + wmm_ptr);

// 	if (!valid_wmm(wmm)) {
//         std::cout
//             << "Invalid regulatory WMM rule "
//             << be32_to_cpu(rule->start)
//             << "-" << be32_to_cpu(rule->end)
//             << " in domain " << country->alpha2[0] <<  country->alpha2[1]
//             << std::endl;
// 		return;
// 	}

// 	for (i = 0; i < IEEE80211_NUM_ACS; i++) {
// 		wmm_rule->client[i].cw_min =
// 			ecw2cw((wmm->client[i].ecw & 0xf0) >> 4);
// 		wmm_rule->client[i].cw_max = ecw2cw(wmm->client[i].ecw & 0x0f);
// 		wmm_rule->client[i].aifsn =  wmm->client[i].aifsn;
// 		wmm_rule->client[i].cot =
// 			1000 * be16_to_cpu(wmm->client[i].cot);
// 		wmm_rule->ap[i].cw_min = ecw2cw((wmm->ap[i].ecw & 0xf0) >> 4);
// 		wmm_rule->ap[i].cw_max = ecw2cw(wmm->ap[i].ecw & 0x0f);
// 		wmm_rule->ap[i].aifsn = wmm->ap[i].aifsn;
// 		wmm_rule->ap[i].cot = 1000 * be16_to_cpu(wmm->ap[i].cot);
// 	}

// 	rrule->has_wmm = true;
// }

static WifiRegulations regdb_load_country(const struct fwdb_header *db,
                                          const struct fwdb_country *country)
{
    unsigned int ptr = be16_to_cpu(country->coll_ptr) << 2;
    struct fwdb_collection *coll = (fwdb_collection *)((u8 *)db + ptr);
    unsigned int i;

    WifiRegulations wifiRegulations;

    wifiRegulations.reg_alpha2 = SS(country->alpha2[0] << country->alpha2[1]);
    wifiRegulations.dfs_region = (DfsRegion)(coll->dfs_region);
    unsigned int nRules = coll->n_rules;

    for (i = 0; i < nRules; i++)
    {
        const s__be16 *rules_ptr = (const s__be16 *)((u8 *)coll + ALIGN(coll->len, 2));
        unsigned int rule_ptr = be16_to_cpu(rules_ptr[i]) << 2;
        struct fwdb_rule *rule = (fwdb_rule *)((u8 *)db + rule_ptr);

        WifiRule wifiRule;
        wifiRule.start_freq_khz = be32_to_cpu(rule->start);
        wifiRule.end_freq_khz = be32_to_cpu(rule->end);
        wifiRule.max_bandwidth_khz = be32_to_cpu(rule->max_bw);
        wifiRule.max_antenna_gain = 0;
        wifiRule.max_eirp = be16_to_cpu(rule->max_eirp);
        wifiRule.flags = RegRuleFlags::NONE;

        if (rule->flags & FWDB_FLAG_NO_OFDM)
            wifiRule.flags |= RegRuleFlags::NO_OFDM;
        if (rule->flags & FWDB_FLAG_NO_OUTDOOR)
            wifiRule.flags |= RegRuleFlags::NO_OUTDOOR;
        if (rule->flags & FWDB_FLAG_DFS)
            wifiRule.flags |= RegRuleFlags::DFS;
        if (rule->flags & FWDB_FLAG_NO_IR)
            wifiRule.flags |= RegRuleFlags::NO_IR;
        if (rule->flags & FWDB_FLAG_AUTO_BW)
            wifiRule.flags |= RegRuleFlags::AUTO_BW;

        wifiRule.dfs_cac_ms = 0;

        /* handle optional data */
        if (rule->len >= offsetofend(struct fwdb_rule, cac_timeout))
            wifiRule.dfs_cac_ms =
                1000 * be16_to_cpu(rule->cac_timeout);
        // if (rule->len >= offsetofend(struct fwdb_rule, wmm_ptr))
        // 	set_wmm_rule(db, country, rule, &wifiRule);
        wifiRegulations.rules.push_back(std::move(wifiRule));
    }

    return wifiRegulations;
}

static std::vector<WifiRegulations> load_regdb(u8 *pData)
{
    const struct fwdb_header *hdr = (const fwdb_header *)pData;
    const struct fwdb_country *country;

    ASSERT_RTNL();
    if (!pData)
    {
        throw std::runtime_error("Regdb not loaded.");
    }
    std::vector<WifiRegulations> result;

    country = &hdr->country[0];
    while (country->coll_ptr)
    {
        result.push_back(regdb_load_country(hdr, country));
        country++;
    }
    return result;
}

void RegDb::Load20()
{
    u8 *data = (u8 *)ptr();
    unsigned int size = (unsigned int)this->data.size();
    if (!valid_regdb(data, size))
    {
        throw std::runtime_error("Invalid file format.");
    }
    this->regulations = load_regdb(data);
    this->isValid = true;
}


static std::map<std::string,std::string> getIsoNamesDict(const std::filesystem::path&filename)
{
    std::map<std::string,std::string> result;
    std::ifstream f(filename);
    if (!f.is_open())
    {
        throw std::runtime_error("Can't open WiFi regulatory domain names file.");
    }
    json_reader reader(f);
    reader.read(&result);
    return result;
}
std::map<std::string,std::string>  RegDb::getRegulatoryDomains(const std::filesystem::path&isoFilenamesfile) const
{
    std::map<std::string,std::string> keyToNameDict = getIsoNamesDict(isoFilenamesfile);
    std::map<std::string,std::string>  result;

    for (const auto &regulation : this->regulations)
    {
        if (keyToNameDict.contains(regulation.reg_alpha2))
        {
            result[regulation.reg_alpha2] = keyToNameDict[regulation.reg_alpha2];
        }
    }
    return result;
}
