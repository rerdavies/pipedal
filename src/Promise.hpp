// Copyright (c) 2023 Robin Davies
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

#include <functional>
#include <mutex>
#include <stdexcept>
#include <condition_variable>

namespace pipedal
{

    // thread-safe javascript-like future.

    template <typename T>
    class Promise;

    template <typename T>
    class PromiseInner;

    

    template <typename T>
    using ResolveFunction = std::function<void(const T &value)>;

    using RejectFunction = std::function<void(const std::string &message)>;

    template <typename T>
    using WorkerFunction = std::function<void(ResolveFunction<T>, RejectFunction reject)>;
    using CatchFunction = std::function<void(const std::string &message)>;

    class PromiseInnerBase
    {
    public:
        static int64_t allocationCount; // not thread-safe. for testing purposes only.

    public:
        virtual void Cancel() = 0;


    protected:

        virtual ~PromiseInnerBase()
        {
            if (this->inputPromise)
            {
                this->inputPromise->ReleaseRef();
            }
        }
        void AddRef()
        {
            std::lock_guard lock{mutex};
            ++referenceCount;
        }
        void ReleaseRef()
        {
            bool deleteMe = false;
            {
                std::lock_guard lock{mutex};
                if (referenceCount == 0)
                {
                    throw std::logic_error("Reference count blown.");
                }
                --referenceCount;
                deleteMe = (referenceCount == 0); // can't do it while holding the mutex.
            }
            if (deleteMe)
            {
                delete this;
            }
        }
        void SetInputPromise(PromiseInnerBase *promise)
        {

            PromiseInnerBase *oldPromise;
            {
                std::lock_guard lock{mutex};
                oldPromise = this->inputPromise;
                this->inputPromise = promise;
            }
            if (promise) 
            {
                promise->AddRef();
            }
            if (oldPromise)
            {
                oldPromise->ReleaseRef();
            }
        }

    protected:
        PromiseInnerBase *inputPromise = nullptr;
        int referenceCount = 0;
        std::mutex mutex;
    };
    inline int64_t PromiseInnerBase::allocationCount = 0;

    template <typename T>
    class PromiseInner : PromiseInnerBase
    {
    private:
        friend class Promise<T>;
        using ResolveFunction = pipedal::ResolveFunction<T>;
        using WorkerFunction = pipedal::WorkerFunction<T>;
        using ThenFunction = std::function<void(const T &value)>;

        PromiseInner()
        {
            ++allocationCount;
            referenceCount = 1; // one for the owner, none for the worker (yet)
        }
        void Work(WorkerFunction work)
        {
            {
                std::lock_guard lock{mutex};
                if (cancelled)
                    return;
                ++referenceCount;
                this->worker = work;
                this->hasWorker = true;
            }
            try
            {
                worker(
                    [this](const T &value)
                    { this->Resolve(value); },
                    [this](const std::string &message)
                    { this->Reject(message); });
            }
            catch (const std::exception &e)
            {
                Reject(e.what());
            }
        }
        PromiseInner(WorkerFunction work)
        {
            ++allocationCount;
            referenceCount = 2; // one for the owner, one for the worker.
            this->worker = work;
            this->hasWorker = true;
            try
            {
                worker(
                    [this](const T &value)
                    { this->Resolve(value); },
                    [this](const std::string &message)
                    { this->Reject(message); });
            }
            catch (const std::exception &e)
            {
                Reject(e.what());
            }
        }
        ~PromiseInner()
        {
            delete conditionVariable;
            --allocationCount;
        }

        T Get()
        {
            while (true)
            {
                std::unique_lock lock{mutex};
                if (this->resolved || this->cancelled)
                    break;
                if (conditionVariable != nullptr)
                {
                    conditionVariable = new std::condition_variable();
                }
                conditionVariable->wait(lock);
            }
            ReleaseFunctions();
            if (cancelled)
            {
                throw std::logic_error("Cancelled.");
            }
            if (rejected)
            {
                throw std::logic_error(catchMessage);
            }
            return resolvedValue;
        }

        void Resolve(const T &value)
        {
            resolvedValue = value;
            bool fireResult = false;
            std::condition_variable *conditionVariable = nullptr;
            bool releaseRef = false;
            {
                std::lock_guard lock{mutex};
                if (!resolved)
                {
                    resolved = true;
                    releaseRef = true;
                    if (hasThenFunction && !cancelled)
                    {
                        fireResult = true;
                    }
                    conditionVariable = this->conditionVariable;
                }
            }
            if (fireResult)
            {
                thenFunction(resolvedValue);
                ReleaseFunctions();
                thenFunction = nullptr;
                catchFunction = nullptr;
            }
            if (conditionVariable)
            {
                conditionVariable->notify_all();
            }
            if (releaseRef)
            {
                ReleaseRef();
            }
        }
        void Reject(const std::string &message)
        {
            bool fireCatch = false;
            bool releaseRef = false;
            std::condition_variable *conditionVariable = nullptr;

            {
                std::lock_guard lock{mutex};
                if (!resolved)
                {
                    releaseRef = hasWorker;
                    resolved = true;
                    rejected = true;
                    this->catchMessage = message;

                    if (hasCatchFunction && !cancelled)
                    {
                        fireCatch = true;
                    }
                    conditionVariable = this->conditionVariable;
                    resolved = true;
                }
            }
            if (fireCatch)
            {
                catchFunction(this->catchMessage);
                ReleaseFunctions();
            }
            if (conditionVariable)
            {
                conditionVariable->notify_all();
            }
            if (releaseRef)
            {
                ReleaseRef();
            }
        }
        virtual void Cancel()
        {
            std::condition_variable *conditionVariable = nullptr;
            {
                std::lock_guard lock{mutex};
                if (!cancelled)
                {
                    cancelled = true;
                    thenFunction = nullptr;
                    catchFunction = nullptr;
                    conditionVariable = this->conditionVariable;
                    if (inputPromise)
                    {
                        inputPromise->Cancel();
                        SetInputPromise(nullptr);
                    }
                }
            }
            ReleaseFunctions();
            if (conditionVariable)
            {
                conditionVariable->notify_all();
            }
        }
        bool IsCancelled()
        {
            std::lock_guard lock{mutex};
            return cancelled;
        }
        void Then(ThenFunction &&handler)
        {
            bool fireThen;
            {
                std::lock_guard lock{mutex};
                if (hasThenFunction)
                {
                    throw std::logic_error("Then handler already set.");
                }
                hasThenFunction = true;
                thenFunction = std::move(handler);
                fireThen = resolved && (!rejected) && (!cancelled);
            }
            if (fireThen)
            {
                thenFunction(resolvedValue);
                ReleaseFunctions();
            }
        }
        void Then(const ThenFunction &handler)
        {
            ThenFunction fn = handler;
            Then(std::move(fn));
        }

        void Catch(CatchFunction handler)
        {
            bool fireCatch;
            {
                std::lock_guard lock{mutex};
                if (hasCatchFunction)
                {
                    throw std::logic_error("Catch handler already set.");
                }
                if (!hasThenFunction)
                {
                    ++referenceCount;
                }
                hasCatchFunction = true;
                catchFunction = handler;

                fireCatch = resolved && rejected && !cancelled;
            }
            if (fireCatch)
            {
                catchFunction(this->catchMessage);
                
                ReleaseFunctions();


            }
        }
        bool IsReady() const
        {
            std::lock_guard lock{mutex};
            return this->resolved;
        }

    private:
        void ReleaseFunctions()
        {
            // Most importantly, release capture variables that reference a promise.
            thenFunction = nullptr;
            catchFunction = nullptr;
            worker = nullptr;
            SetInputPromise(nullptr);
        }

    private:
        bool hasWorker = false;
        bool cancelled = false;
        bool resolved = false;
        bool rejected = false;
        WorkerFunction worker;
        std::condition_variable *conditionVariable = nullptr;
        bool hasThenFunction = false;
        ThenFunction thenFunction;
        bool hasCatchFunction = false;
        CatchFunction catchFunction;

        T resolvedValue;
        std::string catchMessage;
    };

    template <typename T>
    class Promise
    {
    public:
        
        using ResolveFunction = PromiseInner<T>::ResolveFunction;
        using RejectFunction = pipedal::RejectFunction;
        using WorkerFunction = PromiseInner<T>::WorkerFunction;
        using ThenFunction = PromiseInner<T>::ThenFunction;
        using CatchFunction = pipedal::CatchFunction;

        Promise() { p = new PromiseInner<T>(); }

        Promise(const Promise<T> &other)
        {
            p = other.p;
            p->AddRef();
        }
        Promise(Promise<T> &&other)
        {
            this->p = other.p;
            other.p = nullptr;
        }
        Promise(WorkerFunction worker) { p = new PromiseInner(worker); }
        ~Promise()
        {
            if (p)
                p->ReleaseRef();
        }

        Promise(Promise &other)
        {
            other.p->AddRef();
            this->p = other.p;
        }
        Promise<T> Then(std::function<void(const T &v)> &&thenFn)
        {
            p->Then(std::move(thenFn));
            return Promise<T>(*this);
        }
        Promise<T> Then(const std::function<void(const T &v)> &thenFn)
        {
            std::function<void(const T &v)> t(thenFn);
            p->Then(std::move(t));
            return Promise<T>(*this);
        }

        template <typename U>
        Promise<U> Then(std::function<void(const T &value, pipedal::ResolveFunction<U> result, RejectFunction reject)> thenFn)
        {

            Promise<U> t{};
            t.SetInputPromise(this->p);
            this->Then([t, thenFn](const T &value) mutable
                       { t.Work([thenFn, value](pipedal::ResolveFunction<U> resolveU, RejectFunction rejectU) mutable
                                { thenFn(value, resolveU, rejectU); }); })
                .Catch([t](const std::string &message) mutable
                       { t.Reject(message); });
            return t;
        }

        Promise<T> Catch(const CatchFunction &catchFn)
        {
            p->Catch(catchFn);
            return Promise<T>(*this);
        }

        Promise<T> &operator=(Promise<T> &&other)
        {
            std::swap(this->p, other.p);
            return *this;
        }
        Promise<T> &operator=(const Promise<T> &other)
        {
            this->p = other.p;
            p->AddRef();
            return *this;
        }
        T Get()
        {
            return p->Get();
        }

        class Test
        {
            // not thread-safe. for testing purposes only.
            int GetAllocationCount() { return PromiseInnerBase::allocationCount; }
        };

        void Work(std::function<void(ResolveFunction resolve, RejectFunction reject)> workFunction)
        {
            p->Work(workFunction);
        }
        void Reject(const std::string &message)
        {
            p->Reject(message);
        }

        void SetInputPromise(PromiseInnerBase *inputPromise)
        {
            p->SetInputPromise(inputPromise);
        }
    private:
        PromiseInner<T> *p = nullptr;
    };

}