/*
  Copyright 2007-2012 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
// (Borrows heavily from worker.c by David Robillard.)

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

#include "Worker.hpp"
#include <mutex>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include "Lv2Log.hpp"
#include <iostream>
#include <utility>
#include "util.hpp"
#include "SchedulerPriority.hpp"

using namespace pipedal;

const int RING_BUFFER_SIZE = 64 * 1024;

Worker::Worker(const std::shared_ptr<HostWorkerThread> &pHostWorker, LilvInstance *lilvInstance_, const LV2_Worker_Interface *workerInterface_)
    : lilvInstance(lilvInstance_),
      pHostWorker(pHostWorker),
      responseRingBuffer(RING_BUFFER_SIZE),
      workerInterface(workerInterface_)
{

    responseBuffer.resize(16 * 1024);
}

void Worker::Close()
{
    {
        std::lock_guard lock(outstandingRequestMutex);
        if (closed)
            return;
        closed = true;
        exiting = true;
    }
    WaitForAllResponses();
}
Worker::~Worker()
{
    Close();
}

LV2_Worker_Status Worker::worker_respond_fn(LV2_Worker_Respond_Handle handle, uint32_t size, const void *data)
{
    Worker *this_ = (Worker *)handle;
    return this_->WorkerRespond(size, data);
}

LV2_Worker_Status Worker::WorkerRespond(uint32_t size, const void *data)
{
    {
        std::lock_guard lock(outstandingRequestMutex);
        ++outstandingResponses;
    }
    LV2_Worker_Status status;
    if (responseRingBuffer.writeSpace() < sizeof(size) + size)
    {
        {
            Lv2Log::warning(SS("LV2 Worker response too large: " << size << " bytes."));
            std::lock_guard lock(outstandingRequestMutex);
            --outstandingResponses;
            // no need to notify, because outstandingRequests will be decremented after return.
        }
        return LV2_WORKER_ERR_NO_SPACE;
    }
    else
    {
        if (!responseRingBuffer.write(sizeof(size), (uint8_t *)&size))
        {
            throw std::logic_error("Response queue sync lost.");
        }
        if (!responseRingBuffer.write(size, (uint8_t *)data))
        {
            throw std::logic_error("Response queue sync lost.");
        }
        return LV2_WORKER_SUCCESS;
    }
}

bool Worker::EmitResponses()
{
    bool emitted = false;
    while (true)
    {
        if (!responseRingBuffer.isReadReady())
        {
            break;
        }

        emitted = true;
        uint32_t size;
        responseRingBuffer.read(sizeof(size), (uint8_t *)&size);
        if (size > responseBuffer.size())
        {
            responseBuffer.resize(size); // allocation on the RT thread! But it's rare, and we have no choice.
        }
        uint8_t *pResponse = &(responseBuffer[0]);

        responseRingBuffer.read(size, pResponse);

        workerInterface->work_response(lilvInstance->lv2_handle, size, pResponse);
        {
            std::lock_guard lock(outstandingRequestMutex);
            --outstandingResponses;
            cvOutstandingRequests.notify_all(); // must be done WITH the lock, since we may be destructed after the mutex is released.
        }
    }
    return emitted;
}

void Worker::WaitForAllResponses()
{
    using Clock = std::chrono::steady_clock;
    auto startTime = Clock::now();
    while (true)
    {
        // can't do condition_variable::wait_until due to OS restrictions.
        // instead, sleep briefly, waiting for wait tasks to complete.
        bool gotResponse = EmitResponses();
        {
            std::unique_lock lock(outstandingRequestMutex);
            if (outstandingRequests == 0 && outstandingResponses == 0)
            {
                break;
            }
        }
        std::chrono::seconds waitDuration = std::chrono::duration_cast<std::chrono::seconds>(Clock::now()-startTime);
        if (waitDuration.count() > 5) {
            throw std::logic_error("Timed out waiting for a Worker task to complete.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

LV2_Worker_Status Worker::ScheduleWork(
    uint32_t size,
    const void *data)
{
    {
        std::lock_guard lock(outstandingRequestMutex);
        ++outstandingRequests;
    }
    LV2_Worker_Status status = this->pHostWorker->ScheduleWork(this, size, data);
    if (status != LV2_Worker_Status::LV2_WORKER_SUCCESS)
    {
        {
            std::lock_guard lock(outstandingRequestMutex);
            --outstandingRequests;
            cvOutstandingRequests.notify_all(); // must be done WITH the lock!
            return status; 
        }
    }
    return status;
}

void HostWorkerThread::ThreadProc() noexcept
{
    // run nice +2 (priority -2 on Windows)
    SetThreadName("lv2_worker");
    SetThreadPriority(SchedulerPriority::Lv2Scheduler);


    try
    {
        while (true)
        {
            if (!requestRingBuffer.readWait())
            {
                return;
            }
            uint32_t size;

            Worker *pWorker;

            if (!requestRingBuffer.read(sizeof(size), (uint8_t *)&size))
            {
                throw PiPedalStateException("Working ringbuffer read failed.");
            }

            if (!requestRingBuffer.read(sizeof(pWorker), (uint8_t *)&pWorker))
            {
                throw PiPedalStateException("Worker ringbuffer read failed.");
            }
            size -= sizeof(pWorker);

            if (size > dataBuffer.size())
            {
                dataBuffer.resize(size);
            }
            uint8_t *pData = &(dataBuffer[0]);
            if (!requestRingBuffer.read(size, pData))
            {
                throw PiPedalStateException("Worker ringbuffer read failed.");
            }
            if (pWorker == nullptr) // signals close.
            {
                break;
            }
            pWorker->RunBackgroundTask(size, pData);
        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error("Lv2 Worker thread proc exited abnormally. (%s)", e.what());
    }
    requestRingBuffer.close();
}

HostWorkerThread::HostWorkerThread()
{
    this->dataBuffer.resize(16*1024);
    pThread = std::make_unique<std::thread>([this]()
                                            { this->ThreadProc(); });
}

void HostWorkerThread::Close()
{

    bool sendClose = false;
    {
        std::lock_guard lock{submitMutex};
        if (!closed)
        {
            closed = true;
            sendClose = true;
        }
    }
    if (sendClose)
    {
        ScheduleWork(nullptr, 0, nullptr);
    }
}
HostWorkerThread::~HostWorkerThread()
{
    if (pThread)
    {
        // ask worker thread to terminate.
        Close();
        pThread->join();
        pThread = nullptr;
    }
}

LV2_Worker_Status HostWorkerThread::ScheduleWork(Worker *worker, size_t size, const void *data)
{
    std::lock_guard lock(submitMutex);

    if (exiting)
    {
        return LV2_Worker_Status::LV2_WORKER_ERR_NO_SPACE;
    }
    if (worker == nullptr)
    {
        exiting = true;
    }

    if (requestRingBuffer.writeSpace() < sizeof(worker) + sizeof(size) + size)
    {
        return LV2_Worker_Status::LV2_WORKER_ERR_NO_SPACE;
    }
    size_t packetSizeL = (size + sizeof(worker));
    uint32_t packetSize = (uint32_t)(packetSizeL);
    if (packetSizeL != packetSize)
    {
        return LV2_Worker_Status::LV2_WORKER_ERR_NO_SPACE;
    }

    requestRingBuffer.write(sizeof(packetSize), (uint8_t *)&packetSize);
    requestRingBuffer.write(sizeof(worker), (uint8_t *)&worker);
    requestRingBuffer.write(size, (uint8_t *)data);
    
    return LV2_Worker_Status::LV2_WORKER_SUCCESS;
}

void Worker::RunBackgroundTask(size_t size, uint8_t *data)
{
    workerInterface->work(lilvInstance->lv2_handle, worker_respond_fn, (LV2_Handle)this, size, data);
    {
        std::lock_guard lock { this->outstandingRequestMutex};
        --this->outstandingRequests;
        if (this->outstandingRequests == 0)
        {
            this->cvOutstandingRequests.notify_all();
        }

    }

}
