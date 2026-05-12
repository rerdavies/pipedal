/*
 *   Copyright (c) 2026 Robin E. R. Davies
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

#include <memory>
#include <queue>
#include <condition_variable>
#include <functional>

namespace pipedal
{
    template <typename T>
    class ThreadedQueue
    {
    public:
        ~ThreadedQueue() { Close(); }

        void Put(std::shared_ptr<T> value)
        {
            {
                std::lock_guard lock(mutex);
                if (!closed)
                {
                    queue.push(value);
                }
            }
            read_cv.notify_one();
        }
        std::shared_ptr<T> Take()
        {
            while (true)
            {
                {
                    std::unique_lock  lock(mutex);
                    read_cv.wait(lock, 
                    [this]() {return !queue.empty() || closed;});
                    if (!queue.empty())
                    {
                        auto result = queue.front();
                        queue.pop();
                        return result;
                    }
                    else
                    {
                        if (closed)
                        {
                            return nullptr;
                        }
                    }
                }
            }
        }
        void Close()
        {
            {
                std::lock_guard lock{mutex};
                closed = true;
            }
            read_cv.notify_all();
        }
        void CloseAndClear()
        {
            {
                std::lock_guard lock{mutex};
                closed = true;
                while (!queue.empty())
                {
                    queue.pop();
                }
            }
            read_cv.notify_all();
        }
        void Erase(std::function<bool(std::shared_ptr<T> queueEntry)> predicate)
        {
            std::lock_guard lock{mutex};
            std::queue<std::shared_ptr<T>> newQueue;
            while (!queue.empty())
            {
                auto item = queue.front();
                queue.pop();
                if (!predicate(item))
                {
                    newQueue.push(item);
                }
            }
            queue = newQueue;
        }

    private:
        bool closed = 0;
        std::queue<std::shared_ptr<T>> queue;
        std::mutex mutex;
        std::condition_variable read_cv;
    };
}