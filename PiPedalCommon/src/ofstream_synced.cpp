#include "ofstream_synced.hpp"
#include "unistd.h"

using namespace pipedal;


void pipedal::FileSystemSync()
{
    ::sync();
}
void ofstream_synced::close() {
    if (is_open())
    {
        super::close();
        ::sync();
    }
}
ofstream_synced::~ofstream_synced()
{
    ofstream_synced::close();
}