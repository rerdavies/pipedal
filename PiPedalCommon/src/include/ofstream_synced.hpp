
#pragma once

#include <fstream>

namespace pipedal
{
    void FileSystemSync();

    class ofstream_synced : public std::ofstream
    {
    public:
        ofstream_synced() {}

        explicit ofstream_synced(const std::string &filename, ios_base::openmode mode = ios_base::out)
            : std::ofstream(filename, mode)
        {
        }
        explicit ofstream_synced(const char *filename, ios_base::openmode mode = ios_base::out)
            : std::ofstream(filename, mode)
        {
        }
        ~ofstream_synced();
    };

}