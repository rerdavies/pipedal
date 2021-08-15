#include "pch.h"
#include "catch.hpp"


#include "BeastServer.hpp"

using namespace piddle;

TEST_CASE( "BeastServer shutodwn", "[beastServerShutdown]" ) {

    auto const address = boost::asio::ip::make_address("0.0.0.0");
    auto const port = 8081;
    std::string doc_root = "/bad";
    auto const threads = 3;

    auto server = std::make_shared<BeastServer>(
        address,port,doc_root.c_str(),threads);
    server->RunInBackground();
    sleep(3);
    server->Stop();
    server->Join();

}

