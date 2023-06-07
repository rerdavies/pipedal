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


#pragma once

#include <lilv/lilv.h> 
//#include "lv2.h"

#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include "lv2/atom/atom.h"
#include "lv2/worker/worker.h"
#include "condition_variable"

#include <map>
#include <string>
#include <mutex>
#include <thread>
#include "RingBuffer.hpp"
#include <memory>
#include "inverting_mutex.hpp"


namespace pipedal {

    class Worker;

    class HostWorkerThread {
    public:
        HostWorkerThread();
        ~HostWorkerThread();

        void Close();
        LV2_Worker_Status ScheduleWork(Worker*worker, size_t size, const void*data);
    private:
        bool closed = false;
        std::unique_ptr<std::thread> pThread;
        void ThreadProc() noexcept;

        RingBuffer<false,true> requestRingBuffer;
        bool exiting = false;
        inverting_mutex submitMutex;

        std::vector<uint8_t> dataBuffer;

        

    };

	class Worker {

	private:
        std::shared_ptr<HostWorkerThread> pHostWorker = nullptr;
        LilvInstance*lilvInstance;
        const LV2_Worker_Interface*workerInterface;

        bool closed = false;
        bool exiting = false;
        RingBuffer<true,false> responseRingBuffer;

        std::vector<uint8_t> responseBuffer;

        static LV2_Worker_Status worker_respond_fn(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data);


        LV2_Worker_Status WorkerRespond(uint32_t size,const void*data);

        inverting_mutex outstandingRequestMutex;
        std::condition_variable cvOutstandingRequests;
        int64_t outstandingRequests = 0;
        int64_t outstandingResponses = 0;
        void WaitForAllResponses();
	public:
		Worker(const std::shared_ptr<HostWorkerThread>& pHostWorker,LilvInstance *instance, const LV2_Worker_Interface *iface);
        ~Worker();
        void Close();

        LV2_Worker_Status ScheduleWork(
            uint32_t size,
            const void *data);

        void RunBackgroundTask(size_t size, uint8_t*data);
        
        bool EmitResponses();


	};
}