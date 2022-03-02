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

template <typename FUNC>
class deferred_call
{
private:
    deferred_call(const deferred_call& other) = delete;
    deferred_call& operator=(const deferred_call& other) = delete;
public:
    deferred_call(FUNC&& f) 
        : fn(std::forward<FUNC>(f)), owned(true) 
    {
    }

    deferred_call(deferred_call&& other)
        : fn(std::move(other.fn)), owned(other.owned)
    {
        other.owned = false;
    }

    ~deferred_call()
    {
        execute();
    }

    bool execute()
    {
        if (owned)
        {
            owned = false;
            fn();
            return true;
        }
        return false;
    }

private:
    FUNC fn;
    bool owned;
};

template <typename F>
deferred_call<F> defer(F&& f)
{
    return deferred_call<F>(std::forward<F>(f));
}