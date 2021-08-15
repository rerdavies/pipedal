/*
  Borrows heavily from worker.c by David Robillard.

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

#include "Worker.hpp"
#include <mutex>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include "Lv2Log.hpp"

using namespace pipedal;

const int RING_BUFFER_SIZE = 16 * 1024;

Worker::Worker(LilvInstance *lilvInstance_, const LV2_Worker_Interface *workerInterface_)
    : lilvInstance(lilvInstance_),
      requestRingBuffer(RING_BUFFER_SIZE),
      responseRingBuffer(RING_BUFFER_SIZE),
      workerInterface(workerInterface_)
{

    responseBuffer = (char *)malloc(RING_BUFFER_SIZE);

    StartWorkerThread();
}

Worker::~Worker()
{
    StopWorkerThread();
    sem_destroy(&requestSemaphore);
    free(responseBuffer);
}

LV2_Worker_Status Worker::worker_respond_fn(LV2_Worker_Respond_Handle handle, uint32_t size, const void *data)
{
    Worker *this_ = (Worker *)handle;
    return this_->WorkerResponse(size, data);
}

LV2_Worker_Status Worker::WorkerResponse(uint32_t size, const void *data)
{
    if (responseRingBuffer.write(sizeof(size), (uint8_t *)&size))
    {
        return LV2_WORKER_ERR_NO_SPACE;
    }
    if (!responseRingBuffer.write(size, (uint8_t *)data))
    {
        return LV2_WORKER_ERR_NO_SPACE;
    }
    return LV2_WORKER_SUCCESS;
}

void Worker::EmitResponses()
{
    while (true)
    {
        uint32_t available = responseRingBuffer.readSpace();
        uint32_t size = 0;
        if (available <= sizeof(size)) // i.e. we need a size AND a response.
            break;

        responseRingBuffer.read(sizeof(size), (uint8_t *)&size);
        responseRingBuffer.read(size, (uint8_t *)responseBuffer);

        workerInterface->work_response(lilvInstance->lv2_handle, size, responseBuffer);
    }
}
void Worker::ThreadProc()
{
    try
    {
        while (true)
        {
            if (!requestRingBuffer.readWait())
            {
                return;
            }
            size_t size;
            while (requestRingBuffer.readSpace() > sizeof(size))
            {
                if (!requestRingBuffer.read(sizeof(size), (uint8_t *)&size))
                {
                    throw PiPedalStateException("Working ringbuffer read failed.");
                }
                void *data = malloc(size);
                if (!requestRingBuffer.read(size, (uint8_t *)data))
                {
                    throw PiPedalStateException("Working ringbuffer read failed.");
                }
                workerInterface->work(lilvInstance->lv2_handle, worker_respond_fn, (LV2_Handle)this, size, data);
                free(data);
            }
        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error("Lv2 Worker thread proc exited abnormally. (%s)", e.what());
    }
}

void Worker::StopWorkerThread()
{
    if (pThread)
    {
        auto pThread = this->pThread;
        this->pThread = nullptr;
        exiting = true;
        requestRingBuffer.close();
        pThread->join();
        delete pThread;
    }
}
void Worker::StartWorkerThread()
{
    auto fn = [this]()
    { this->ThreadProc(); };
    this->pThread = new std::thread(fn);
}

LV2_Worker_Status Worker::ScheduleWork(
    uint32_t size,
    const void *data)
{
    if (!exiting)
    {
        size_t space = requestRingBuffer.writeSpace();
        if (space < sizeof(size) + size)
        {
            return LV2_WORKER_ERR_NO_SPACE;
        }
        if (!requestRingBuffer.write(sizeof(size), (uint8_t *)&size))
        {
            Lv2Log::debug("Not enough space in Worker ring buffer. Request failed.");
            return LV2_WORKER_ERR_NO_SPACE;
        }
        if (!requestRingBuffer.write(size, (uint8_t *)data))
        {
            // probably not going to survive. :-(
            Lv2Log::error("Not enough space in Worker ring buffer. Request ring buffer probably lost sync.");
            return LV2_WORKER_ERR_NO_SPACE;
        }
    }
    return LV2_WORKER_ERR_NO_SPACE;
}
