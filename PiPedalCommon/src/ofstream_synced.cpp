#include "ofstream_synced.hpp"
#include "unistd.h"

using namespace pipedal;


void pipedal::FileSystemSync()
{
    ::sync();
}

ofstream_synced::~ofstream_synced()
{
    if (is_open())
    {
        close();
        ::sync();
    }
}