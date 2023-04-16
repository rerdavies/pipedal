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

#pragma once

#include "PiPedalMath.hpp"

namespace pipedal
{
    class DbDezipper
    {
    public:
        void SetSampleRate(double rate);
        void SetRate(float seconds);
        void Reset(float db = -96);
        void SetMinDb(float db = -96);

        void SetTarget(float db)
        {
            if (db < minDb)
                db = minDb;
            if (db != targetDb)
            {
                targetDb = db;
                count = 0;
            }
        }

        bool IsIdle() const { return count < 0;}
        inline float Tick()
        {
            if (count >= 0)
            {
                if (count-- <= 0)
                {
                    NextSegment();
                }
                float result = x;
                x += dx;

                return result;
            }
            return x;
        }

    private:
        float minDb = -96;
        double sampleRate = 44100;
        float rate = 0.1;
        float targetDb = -96;
        float currentDb = -96;
        float targetX = 0;
        float x = 0;
        float dx = 0;
        int32_t count = -1;
        float dbPerSegment;
        void NextSegment();
    };
}