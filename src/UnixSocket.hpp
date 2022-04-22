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

#include <string>
#include <exception>

struct sockaddr_un;

namespace pipedal {

    class UnixSocketException : public std::exception {
    public:
        UnixSocketException(int errno_);
        UnixSocketException(const std::string what_,int errno_);
        UnixSocketException(const std::string what_);

        const char*what() const noexcept { return what_.c_str(); }

        int get_errno() const { return errno_; }
    private:
        int errno_;
        std::string what_;

    };

    namespace detail {
        class UnixSocketData;
    }
    class UnixSocket {
    public:
        using sockaddr_t = sockaddr_un;

        UnixSocket();
        ~UnixSocket();

        bool IsOpen() const;

        /**
         * @brief Prepare a server socket to recive data.
         * 
         * permissionGroup controls permissions on the socekt.
         * Proceses that wish to read or write to the socket must belong to permissionGroup.
         * @param socketName Server socket name.
         * @param permissionGroup The group which is allowed to connect to this socket.
         */
        void Bind(const std::string&socketName,const std::string&permissionGroup);

        /**
         * @brief Prepare a socket to write to a server socket.
         * 
         * The optional groupPermission is applied to the client socket to allow 
         * server process that are not highly privileged to write replies. The 
         * server process must belong the group named by permissionGroup.
         * 
         * @param remoteSocketName The name of the server socket.
         * @param permissionGroup` The permission to appy to the client socket.
         */
        void Connect(const std::string&remoteSocketName, const std::string &groupPermission = "");

        void Close();


        size_t Send(const void*buffer, size_t length);
        size_t Receive(void *buffer, size_t length);

        size_t ReceiveFrom(void*buffer, size_t length,std::string*fromAddress);
        size_t SendTo(const void*buffer, size_t length,const std::string&toAddress);


    private:
        detail::UnixSocketData *data;
    };

};