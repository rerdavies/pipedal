#pragma once

#include <stdint.h>

namespace pipedal {

// sync with <linux/nl80211.h
enum class RegRuleFlags {
	NO_OFDM		= 1<<0,  
	NO_CCK		= 1<<1,
	NO_INDOOR		= 1<<2,
	NO_OUTDOOR		= 1<<3,
	DFS			= 1<<4,
	PTP_ONLY		= 1<<5,
	PTMP_ONLY		= 1<<6,
	NO_IR		= 1<<7,
	__NO_IBSS		= 1<<8,
	AUTO_BW		= 1<<11,
	IR_CONCURRENT	= 1<<12,
	NO_HT40MINUS	= 1<<13,
	NO_HT40PLUS		= 1<<14,
	NO_80MHZ		= 1<<15,
	NO_160MHZ		= 1<<16,
	NO_HE		= 1<<17,
};

enum class DfsRegion {
    Unset = 0,
    Fcc = 1,
    Etsi = 2,
    Japan = 3
};

struct FrequencyRule {
    uint32_t maximumAntennaGain; // in mBiu (100*dBi)
    uint32_t maximumEirp; // in mBm (100*dBm)
    RegRuleFlags flags;

    bool hasFlag(RegRuleFlags flag) const { return (((int)flag) & (int)this->flags) != 0; }
};

class RegDb;

class CrdbRules {
    friend class RegDb;
    RegDb *regDb = nullptr;
    uint32_t countryOffset;
    DfsRegion dfsRegion;
    CrdbRules(RegDb *regDb,uint32_t countryOffset, DfsRegion dfsRegion );
public:
    virtual DfsRegion GetDfsRegion() const;
    virtual bool GetFrequencyRule(uint32_t frequencyMHz,FrequencyRule*pResult);
};

class RegDb {
private:
    friend class CrdbRules;
    uint8_t*pData;
    
public:
    RegDb();
    ~RegDb();
    CrdbRules *GetCrdbRules(const char*countryCodeIso6333);
    bool isValid() const { return pData != nullptr; }
};

}