/*
 * MIT License
 *
 * Copyright (c) 2022 Robin E. R. Davies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <chrono>
#include <mutex>

namespace pipedal
{

    enum ProfileCategory
    {
        Init,
        Read,
        Driver,
        Execute,
        Write,

        MaxCategory
    };
    constexpr size_t NUM_PROFILE_CATEGORIES = (size_t)ProfileCategory::MaxCategory;

    class CpuUse
    {

    private:
        using ClockT = std::chrono::steady_clock;
        using TimeT = ClockT::time_point;
        using DurationT = ClockT::duration;
        using SampleT = DurationT::rep;

    public:
        class CpuUseAverager
        {
        private:
            static constexpr size_t NUM_SAMPLES = 20; // average over N samples.
            SampleT sampleTotal = 0;
            SampleT samples[NUM_SAMPLES];
            size_t sampleIndex = 0;

        public:

            CpuUseAverager();

            SampleT GetAverage() const
            {
                return sampleTotal / NUM_SAMPLES;
            }
            SampleT GetTotal() const {
                return sampleTotal;
            }
            void AddSample(DurationT duration)
            {
                SampleT sample = duration.count();
                sampleTotal = sampleTotal + sample-samples[sampleIndex];
                samples[sampleIndex] = sample;
                ++sampleIndex;
                if (sampleIndex >= NUM_SAMPLES)
                {
                    sampleIndex = 0;
                }
            }
        };
    private:

        std::mutex sync;

        TimeT lastSample;

        CpuUseAverager profileTimes[NUM_PROFILE_CATEGORIES];
        float currentCpuUse = 0;
        float currentOverhead = 0;

        CpuUseAverager &GetCategory(ProfileCategory category) {
            return profileTimes[(size_t)category];
        }
        
    public:

        TimeT Now() {
            return ClockT::now();
        }
        void SetStartTime(TimeT time)
        {
            lastSample = time;
        }
        void AddSample(ProfileCategory category, TimeT time)
        {
            profileTimes[(size_t)category].AddSample((time-lastSample));
            lastSample = time;
        }
        void AddSample(ProfileCategory category) 
        {
            AddSample(category,Now());
        }
        void AddSample(ProfileCategory category, TimeT startTime, TimeT endTime)
        {
            profileTimes[(size_t)category].AddSample((endTime-startTime));
        }

        void UpdateCpuUse() {
            
            SampleT readTime = GetCategory(ProfileCategory::Read).GetTotal();
            SampleT writeTime = GetCategory(ProfileCategory::Driver).GetTotal();

            SampleT processingTime = GetCategory(ProfileCategory::Execute).GetTotal() + GetCategory(ProfileCategory::Driver).GetTotal();

            // most of the waiting occurs in the write. Assume that the difference between readTime and write Time is how long we spent waiting (i.e. free time)

            SampleT waitTime;
            SampleT overheadTime;
            if (readTime > writeTime)
            {
                waitTime = readTime-writeTime;
                overheadTime = writeTime;
            } else {
                waitTime = writeTime-readTime;
                overheadTime = readTime;
            }

            SampleT totalTime = writeTime+ readTime + processingTime;
            SampleT maxTime = waitTime+processingTime;
            

            float result = 100.0f*(processingTime)/(maxTime);
            float overhead = 100.0F*(overheadTime*2)/totalTime;
            {
                std::lock_guard lock { sync};
                currentCpuUse = result;
                currentOverhead = overhead;
            }
        }
        float GetCpuUse() 
        {
            std::lock_guard lock { sync};
            return currentCpuUse;
        }
        float GetCpuOverhead() 
        {
            std::lock_guard lock { sync};
            return currentOverhead;
        }
    };

}
