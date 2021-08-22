#pragma once
#include <boost/filesystem.hpp>

namespace pipedal {

class SystemConfigFile {
    boost::filesystem::path currentPath;
    std::vector<std::string> lines;
    void SetLine(int64_t lineIndex,const std::string&key,const std::string &value );
    bool LineMatches(const std::string &line, const std::string&key) const;
public:
    SystemConfigFile() {

    }
    SystemConfigFile(const boost::filesystem::path& path)
    {
        Load(path);
    }

    void Load(const boost::filesystem::path&path);

    void Save(std::ostream &os);
    void Save(const boost::filesystem::path&path);
    void Save();

    int64_t GetLine(const std::string &key) const;
    bool HasValue(const std::string&key) const;
    bool Get(const std::string&key,std::string*pResult) const;
    std::string Get(const std::string&key) const;
    void Set(const std::string&key,const std::string &value);
    void Set(const std::string&key,const std::string &value, bool overwrite);
    void Set(const std::string&key,const std::string &value, const std::string&comment);
    bool Erase(const std::string&key);
    int64_t Insert(int64_t position, const std::string&key, const std::string&value);
    int64_t Insert(int64_t position, const std::string&line);

};

};