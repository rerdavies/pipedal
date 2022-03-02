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
#include "lv2.h"

#include "lv2/atom.lv2/atom.h"
#include "lv2/atom.lv2/util.h"
#include "lv2.h"
#include "lv2/log.lv2/log.h"
#include "lv2/log.lv2/logger.h"
#include "lv2/midi.lv2/midi.h"
#include "lv2/urid.lv2/urid.h"
#include "lv2/atom.lv2/atom.h"
#include "lv2/worker.lv2/worker.h"

#include <map>
#include <string>
#include <mutex>
#include <thread>
#include "RingBuffer.hpp"


namespace pipedal {
	class Worker {

	private:
        LilvInstance*lilvInstance;
        const LV2_Worker_Interface*workerInterface;

        sem_t requestSemaphore;
        bool exiting = false;
        RingBuffer<false,true> requestRingBuffer;
        RingBuffer<true,false> responseRingBuffer;

        char *responseBuffer;

        std::thread* pThread = nullptr;

        void ThreadProc();
        void StartWorkerThread();
        void StopWorkerThread();

        static LV2_Worker_Status worker_respond_fn(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data);


        LV2_Worker_Status WorkerResponse(uint32_t size,const void*data);

	public:
		Worker(LilvInstance *instance, const LV2_Worker_Interface *iface);
        ~Worker();

        LV2_Worker_Status ScheduleWork(
            uint32_t size,
            const void *data);

        void EmitResponses();


	};
}