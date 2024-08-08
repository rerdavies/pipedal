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
#include <vector>



namespace p2p {

    template <typename T> class autoptr_vector
    {
        std::vector<T*> v;
    private:
        // no copy no move.
        autoptr_vector(const autoptr_vector<T> &) = delete;
        autoptr_vector(autoptr_vector<T> && ) = delete;
    public:
        using iterator = std::vector<T*>::const_iterator;

        autoptr_vector() { }
        autoptr_vector(std::initializer_list<T*> l)
        :v(l)
        {

        }
        ~autoptr_vector() {
            for (T *i: v)
            {
                delete i;
            }
        }
        void push_back(T*value) { v.push_back(value);}

        iterator begin() const { return v.begin(); }
        iterator end() const { return v.end(); }

        T*operator[](size_t index) const { return v[index]; }

        size_t size() const { return v.size(); }
    };
}