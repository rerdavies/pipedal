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

#include "catch.hpp"
#include "UnixSocket.hpp"
#include "ss.hpp"
#include <unistd.h>
#include <cassert>

using namespace pipedal;


TEST_CASE( "Unix Socket Test", "[unix_socket_test][dev]" ) {
    UnixSocket serverSocket;
    UnixSocket clientSocket;

    std::string serverSocketName = SS("/tmp/ServerSocketTest-Server-" << getpid() << "-0");

    char buffer[1024];
    serverSocket.Bind(serverSocketName,"pipedal_d");
    clientSocket.Connect(serverSocketName,"pipedal_d");

    std::string testData = "abc123";
    clientSocket.Send(testData.c_str(),testData.length());

    size_t length = serverSocket.Receive(buffer,sizeof(buffer));
    assert(length == testData.length());

    buffer[length] = 0;
    assert(testData == buffer);

    clientSocket.Send(testData.c_str(),testData.length());

    std::string fromAddress;
    length = serverSocket.ReceiveFrom(buffer,sizeof(buffer),&fromAddress);

    serverSocket.SendTo(buffer,length,fromAddress);

    size_t length2 = clientSocket.Receive(buffer,length);
    assert(length == length2);
    buffer[length2] = 0;
    assert(testData == std::string(buffer));





    

}