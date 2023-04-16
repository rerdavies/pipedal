/*
 *   Copyright (c) 2022 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "DbDezipper.hpp"

using namespace pipedal;

static const int SEGMENT_SIZE = 64;

void DbDezipper::SetSampleRate(double sampleRate)
{
    this->sampleRate = sampleRate;
    this->dbPerSegment = 96/rate * SEGMENT_SIZE / sampleRate;
}

void DbDezipper::SetRate(float seconds)
{
    this->rate = seconds;
    this->dbPerSegment = 96/rate * SEGMENT_SIZE / sampleRate;
    
}


void DbDezipper::SetMinDb(float minDb)
{
    this->minDb = minDb;
    if (targetDb < minDb)
    {
        targetDb = minDb;
        count = 0;
    }
    if (currentDb < minDb)
    {
        currentDb = minDb;
        x = targetX = 0;
        dx = 0;
        count = 0;
    }
}

void DbDezipper::NextSegment()
{
    if (targetDb == currentDb)
    {
        x = targetX;
        dx = 0;
        if (targetDb <= minDb)
        {
            x = 0;
        }
        count = -1;
        return;
    }
    else if (targetDb < currentDb)
    {
        currentDb -= dbPerSegment;
        if (currentDb < targetDb)
        {
            currentDb = targetDb;
        }
    }
    else
    {
        currentDb += dbPerSegment;
        if (currentDb > targetDb)
        {
            currentDb = targetDb;
        }
    }
    this->targetX = db2a(currentDb);
    this->dx = (targetX - x) / SEGMENT_SIZE;
    this->count = SEGMENT_SIZE;
}

void DbDezipper::Reset(float db)
{
    float value;
    if (db <= minDb)
    {
        value = 0;
    } else {
        value = db2a(db);
    }
    x = targetX = value;
    dx = 0;
    currentDb = db;
    targetDb = db;
    count = -1;
}
