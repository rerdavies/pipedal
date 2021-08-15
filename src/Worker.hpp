#pragma once

#include <lilv/lilv.h>
#include "lv2/core/lv2.h"

#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include "lv2/atom/atom.h"
#include "lv2/worker/worker.h"

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