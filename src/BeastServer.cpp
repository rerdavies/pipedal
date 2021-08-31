//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: Advanced server
//
//------------------------------------------------------------------------------

#include "pch.h"

#include <boost/beast/core.hpp>
#include <queue>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include "HtmlHelper.hpp"
#include "Lv2Log.hpp"

#include "BeastServer.hpp"

#include "Uri.hpp"

using namespace pipedal;

using tcp = boost::asio::ip::tcp;              // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
namespace websocket = boost::beast::websocket; // from <boost/beast/websocket.hpp>

const size_t MAX_READ_SIZE = 1 * 1024 * 204;
using namespace boost;

std::string
pipedal::last_modified(const std::filesystem::path &path)
{
    auto cPath = path.c_str();

    struct stat fStat;
    if (stat(cPath, &fStat) == 0)
    {
        return HtmlHelper::timeToHttpDate(fStat.st_mtim.tv_sec);
    }
    else
    {
        return HtmlHelper::timeToHttpDate(0);
    }
}

std::map<std::string, std::string> extensionsToMimeType = {
    {".htm", "text/html"},
    {".html", "text/html"},
    {".php", "text/html"},
    {".css", "text/css"},
    {".txt", "text/plain"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".png", "image/png"},
    {".jpe", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".ico", "image/x-icon"},
    {".tiff", "image/tiff"},
    {".tif", "image/tiff"},
    {".svg", "image/svg+xml"},
    {".svgz", "image/svg+xml"},
    {".woff", "font/woff2" },
    {".woff2", "font/woff2"}
};
// Return a reasonable mime type based on the extension of a file.
std::string
mime_type(const std::filesystem::path &path)
{
    auto const ext = path.extension();
    try
    {
        return extensionsToMimeType.at(ext);
    }
    catch (const std::exception &)
    {
        return "application/octet-stream";
    }
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::filesystem::path
path_cat(
    boost::beast::string_view base,
    const std::vector<std::string> &segments)
{

    std::filesystem::path result = base.to_string();

    for (const std::string &segment : segments)
    {
        if (segment.find("..") != -1)
        {
            throw std::invalid_argument("Invalid segment.");
        }
        if (segment.find('/') != -1)
            throw std::invalid_argument("Invalid segment.");
        if (segment.find('\\') != -1)
            throw std::invalid_argument("Invalid segment.");
        if (segment.find('/') != -1)
            throw std::invalid_argument("Invalid segment.");
        if (segment.find(':') != -1)
            throw std::invalid_argument("Invalid segment.");

        result /= segment;
    }
    return result;
}

std::string GetFromAddress(const tcp::socket &socket)
{
    std::stringstream s;
    s << socket.remote_endpoint().address().to_string() << ':' << socket.remote_endpoint().port();
    return s.str();
}
// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template <
    class Body, class Allocator,
    class Send>
void handle_request(
    const tcp::socket &socket_,
    BeastServer *server,
    http::request<Body, http::basic_fields<Allocator>> &&req,
    Send &&send)
{


    // Returns a bad request response
    auto const bad_request =
        [&req, &socket_](boost::beast::string_view why)
    {
        Lv2Log::error("http - (%s) Bad request: %s,", GetFromAddress(socket_).c_str(), why.to_string().c_str());
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = why.to_string();
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
        [&req, &socket_](boost::beast::string_view target)
    {
        Lv2Log::error("http - (%s) Not found: %s,", GetFromAddress(socket_).c_str(), target.to_string().c_str());
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + target.to_string() + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
        [&req, &socket_](boost::beast::string_view what)
    {
        Lv2Log::error("http - (%s) Server error: %s,", GetFromAddress(socket_).c_str(), what.to_string().c_str());
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = what.to_string();
        res.prepare_payload();
        return res;
    };

    if (server->HasAccessPointGateway())
    {
        auto &gateway = server->GetAccessPointGateway();
        auto remoteAddress = socket_.remote_endpoint().address();
        if (remoteAddress.is_v4())
        {
            auto v4Address = remoteAddress.to_v4();
            unsigned int gwAddr = (gateway.netmask().to_uint() & gateway.address().to_uint());
            unsigned int ipAddr = (gateway.netmask().to_uint() & v4Address.to_uint());

            if ((gateway.netmask().to_uint() & gateway.address().to_uint()) == (gateway.netmask().to_uint() & v4Address.to_uint()))
            {
                std::string host = req[http::field::host].to_string();
                std::string gatewayHost = server->GetAccessPointServerAddress();
                if (host != gatewayHost)
                {

                    uri requestUri;
                    requestUri.set(req.target().begin(), req.target().end());
                    if (requestUri.segment_count() != 0) {
                        return send(not_found(req.target()));
                    }
                    uri_builder builder(requestUri);
                    builder.set_authority(gatewayHost);

                    std::string redirectUri = builder.str();
                    Lv2Log::info("Access point redirect from" + host + " to " + redirectUri);


                    // redirect to the gateway host.
                    http::response<http::empty_body> res{http::status::temporary_redirect, req.version()};
                    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.set(http::field::location, redirectUri);
                    res.prepare_payload();
                    return send(std::move(res));

                }
            }
        }
    }

    if (req.method() == http::verb::options)
    {
        http::response<http::empty_body> res{http::status::no_content, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::access_control_allow_origin, req[http::field::origin]);
        res.set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, HEADERS");
        res.set(http::field::access_control_allow_headers, "Content-Type");
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    uri request_uri;
    try
    {
        request_uri.set(req.target().begin(), req.target().end());
        std::string host = req[http::field::host].to_string();
        int pos = host.find_last_of(':');
        if (pos != std::string::npos)
        {
            host = host.substr(0, pos);
        }
        uri_builder builder(request_uri);
        builder.set_authority(host);
        builder.set_port(socket_.local_endpoint().port());
        // stub: set htts scheme once we implement it.
        builder.set_scheme("http");

        std::string strUri = builder.str();
        request_uri = uri(strUri.c_str());
    }
    catch (const std::exception &_)
    {
        return send((bad_request("Malformed target.")));
    }

    for (auto requestHandler : server->RequestHandlers())
    {

        if (requestHandler->wants(req.method(), request_uri))
        {

            if (req.method() == http::verb::head)
            {
                Lv2Log::info("http - (%s) head %s", GetFromAddress(socket_), request_uri.str().c_str());

                boost::beast::error_code ec;
                http::response<http::empty_body> res{http::status::ok, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::date, HtmlHelper::timeToHttpDate(time(nullptr)));
                res.set(http::field::access_control_allow_origin, req[http::field::origin]);
                requestHandler->head_response(request_uri, req, res, ec);
                res.keep_alive(req.keep_alive());
                // Handle the case where the file doesn't exist

                if (ec == boost::system::errc::no_such_file_or_directory)
                    return send(not_found(req.target()));

                if (ec)
                {
                    return send(server_error(ec.message()));
                }
                req.prepare_payload();
                return send(std::move(res));
            }
            else if (req.method() == http::verb::get)
            {
                if (server->LogHttpRequests())
                {
                    Lv2Log::info("http - (%s) get %s", GetFromAddress(socket_).c_str(), request_uri.str().c_str());
                }

                boost::beast::error_code ec;
                http::response<http::string_body> res{http::status::ok, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::date, HtmlHelper::timeToHttpDate(time(nullptr)));
                res.set(http::field::access_control_allow_origin, req[http::field::origin]);
                requestHandler->get_response(request_uri, req, res, ec);
                res.keep_alive(req.keep_alive());
                if (ec == boost::system::errc::no_such_file_or_directory)
                    return send(not_found(req.target()));

                if (ec)
                {
                    return send(server_error(ec.message()));
                }
                res.prepare_payload();
                return send(std::move(res));
            }
            else if (req.method() == http::verb::post)
            {
                if (server->LogHttpRequests())
                {
                    Lv2Log::info("http - (%s) post %s", GetFromAddress(socket_).c_str(), request_uri.str().c_str());
                }

                boost::beast::error_code ec;
                http::response<http::string_body> res{http::status::ok, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::date, HtmlHelper::timeToHttpDate(time(nullptr)));
                res.set(http::field::access_control_allow_origin, req[http::field::origin]);
                requestHandler->post_response(request_uri, req, res, ec);
                res.keep_alive(false); // bug in Beast Server -- posted body not reset.
                if (ec == boost::system::errc::no_such_file_or_directory)
                    return send(not_found(req.target()));

                if (ec)
                {
                    return send(server_error(ec.message()));
                }
                res.prepare_payload();
                return send(std::move(res));
            }
            return send(bad_request("Unknown HTTP-method"));
        }
    }
    // Make sure we can handle the method
    if (req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if (req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return send(not_found(req.target())); // security: report not found to avoid disclosing info.

    // Build the path to the requested file
    auto path = path_cat(server->GetDocRoot().c_str(), request_uri.segments());
    if (req.target().back() == '/' || req.target().size() == 0)
        path /= "index.html";

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // Handle an unknown error
    if (ec)
        return send(server_error(ec.message()));

    // Respond to HEAD request
    if (req.method() == http::verb::head)
    {
        if (server->LogHttpRequests())
        {
            Lv2Log::info("http - (%s) head %s", GetFromAddress(socket_).c_str(), request_uri.str().c_str());
        }

        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::date, HtmlHelper::timeToHttpDate(time(nullptr)));
        res.set(http::field::content_type, mime_type(path));
        res.set(http::field::last_modified, last_modified(path));
        res.content_length(body.size());
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }
    else if (req.method() == http::verb::get)
    {
        if (server->LogHttpRequests())
        {
            Lv2Log::info("http - (%s) get %s", GetFromAddress(socket_).c_str(), request_uri.str().c_str());
        }

        // Respond to GET request
        http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(http::status::ok, req.version())};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::date, HtmlHelper::timeToHttpDate(time(nullptr)));
        res.set(http::field::content_type, mime_type(path));
        res.set(http::field::last_modified, last_modified(path));
        res.content_length(body.size());
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }
    else
    {
        return send(bad_request("Unknown HTTP-method"));
    }
}

//------------------------------------------------------------------------------

// Echoes back all received WebSocket messages
class websocket_session : public std::enable_shared_from_this<websocket_session>, private SocketHandler::IWriteCallback
{
    websocket::stream<tcp::socket> ws_;
    boost::asio::strand<
        boost::asio::io_context::executor_type>
        strand_;
    boost::asio::steady_timer timer_;
    boost::beast::multi_buffer read_buffer_;
    boost::beast::multi_buffer write_buffer_;

    BeastServer *beastServer_;
    std::shared_ptr<SocketHandler> socket_handler_;

    std::queue<std::string> writeQueue;

    bool writePending = false;
    ;
    std::mutex writeQueueMutex;
    std::string fromAddress;

public:
    ~websocket_session()
    {
        Lv2Log::info("http - (%s) Websocket closed.", fromAddress.c_str());
    }
    virtual void close()
    {
        this->ws_.close(websocket::close_reason());
    }
    virtual std::string getFromAddress() const {
        return this->fromAddress;
    }
    virtual void writeCallback(const std::string &text)
    {
        {
            std::lock_guard guard(writeQueueMutex);
            if (writePending)
            {
                writeQueue.push(text);
                return;
            }
            writePending = true;
        }
        do_write(text);
    }
    void do_write(const std::string &text)
    {
        size_t n = buffer_copy(write_buffer_.prepare(text.size()), boost::asio::buffer(text));
        write_buffer_.commit(n);
        ws_.text(true);

        ws_.async_write(
            write_buffer_.data(),
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_write,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }
    // Take ownership of the socket
    explicit websocket_session(
        const std::string &fromAddress,
        BeastServer *beastServer,
        std::shared_ptr<SocketHandler> &&socketHandler,
        tcp::socket socket)
        : beastServer_(beastServer), socket_handler_(socketHandler),
          fromAddress(fromAddress),
          ws_(std::move(socket)),
          strand_(ws_.get_executor()),
          read_buffer_(MAX_READ_SIZE),
          timer_(ws_.get_executor().context(),
                 (std::chrono::steady_clock::time_point::max)())
    {
    }

    // Start the asynchronous operation
    template <class Body, class Allocator>
    void
    run(http::request<Body, http::basic_fields<Allocator>> req)
    {
        // Run the timer. The timer is operated
        // continuously, this simplifies the code.
        on_timer({});

        // Set the timer
        timer_.expires_after(std::chrono::seconds(15));
        // Accept the websocket handshake

        ws_.async_accept(
            req,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_accept,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    // Called when the timer expires.
    void
    on_timer(boost::system::error_code ec)
    {
        if (ec && ec != boost::asio::error::operation_aborted)
            this->beastServer_->onError(ec, "timer");
        return;

        // Verify that the timer really expired since the deadline may have moved.
        if (timer_.expiry() <= std::chrono::steady_clock::now())
        {
            // Closing the socket cancels all outstanding operations. They
            // will complete with boost::asio::error::operation_aborted
            ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
            ws_.next_layer().close(ec);
            return;
        }

        // Wait on the timer
        timer_.async_wait(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_timer,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void
    on_accept(boost::system::error_code ec)
    {
        // Happens when the timer closes the socket
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (ec)
        {
            beastServer_->onError(ec, "accept");
            return;
        }
        if (socket_handler_)
        {
            auto t = socket_handler_.get();
            t->setWriteCallback(this);
            socket_handler_->onAttach();
        }

        // Read a message
        do_read();
    }

    void
    do_read()
    {
        // Set the timer
        timer_.expires_after(std::chrono::seconds(15));

        // Read a message into our buffer
        ws_.async_read(
            read_buffer_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_read,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }

    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {

        // Happens when the timer closes the socket
        if (ec == boost::asio::error::operation_aborted)
            return;

        // This indicates that the websocket_session was closed
        if (ec == websocket::error::closed)
            return;

        if (ec)
        {
            beastServer_->onError(ec, "read");
            return;
        }

        // Echo the message
        if (ws_.got_text())
        {
            std::string data(beast::buffers_to_string(read_buffer_.data()));
            this->socket_handler_->onReceive(data);
            read_buffer_.consume(bytes_transferred);
        }
        else
        {
            read_buffer_.consume(bytes_transferred); // ignore binary messages
        }

        do_read();
        // ws_.text(ws_.got_text());
        // ws_.async_write(
        //     buffer_.data(),
        //     boost::asio::bind_executor(
        //         strand_,
        //         std::bind(
        //             &websocket_session::on_write,
        //             shared_from_this(),
        //             std::placeholders::_1,
        //             std::placeholders::_2)));
    }

    void fail(boost::system::error_code ec, const char *what)
    {
        beastServer_->onError(ec, what);
    }
    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // Happens when the timer closes the socket
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (ec)
        {
            fail(ec, "write");
        }

        // Clear the buffer
        std::string nextWrite;
        write_buffer_.consume(write_buffer_.size());

        {
            std::lock_guard guard(this->writeQueueMutex);
            if (writeQueue.size() != 0)
            {
                nextWrite = writeQueue.front();
                writeQueue.pop();
            }
            else
            {
                writePending = false;
                return;
            }
        }
        do_write(nextWrite);
    }
};

// Handles an HTTP server connection
class http_session : public std::enable_shared_from_this<http_session>
{
    // This queue is used for HTTP pipelining.
    class queue
    {
        enum
        {
            // Maximum number of responses we will queue
            limit = 8
        };

        // The type-erased, saved work item
        struct work
        {
            virtual ~work() = default;
            virtual void operator()() = 0;
        };

        http_session &self_;
        std::vector<std::unique_ptr<work>> items_;

    public:
        explicit queue(http_session &self)
            : self_(self)
        {
            static_assert(limit > 0, "queue limit must be positive");
            items_.reserve(limit);
        }

        // Returns `true` if we have reached the queue limit
        bool
        is_full() const
        {
            return items_.size() >= limit;
        }

        // Called when a message finishes sending
        // Returns `true` if the caller should initiate a read
        bool
        on_write()
        {
            BOOST_ASSERT(!items_.empty());
            auto const was_full = is_full();
            items_.erase(items_.begin());
            if (!items_.empty())
                (*items_.front())();
            return was_full;
        }

        // Called by the HTTP handler to send a response.
        template <bool isRequest, class Body, class Fields>
        void
        operator()(http::message<isRequest, Body, Fields> &&msg)
        {
            // This holds a work item
            struct work_impl : work
            {
                http_session &self_;
                http::message<isRequest, Body, Fields> msg_;

                work_impl(
                    http_session &self,
                    http::message<isRequest, Body, Fields> &&msg)
                    : self_(self), msg_(std::move(msg))
                {
                }

                void
                operator()()
                {
                    http::async_write(
                        self_.socket_,
                        msg_,
                        boost::asio::bind_executor(
                            self_.strand_,
                            std::bind(
                                &http_session::on_write,
                                self_.shared_from_this(),
                                std::placeholders::_1,
                                msg_.need_eof())));
                }
            };

            // Allocate and store the work
            items_.emplace_back(new work_impl(self_, std::move(msg)));

            // If there was no previous work, start this one
            if (items_.size() == 1)
                (*items_.front())();
        }
    };

    tcp::socket socket_;
    boost::asio::strand<
        boost::asio::io_context::executor_type>
        strand_;
    boost::asio::steady_timer timer_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    queue queue_;
    BeastServer *beastServer_;

public:
    // Take ownership of the socket
    explicit http_session(
        BeastServer *beastServer,
        tcp::socket socket)
        : beastServer_(beastServer), socket_(std::move(socket)), strand_(socket_.get_executor()), timer_(socket_.get_executor().context(),
                                                                                                         (std::chrono::steady_clock::time_point::max)()),
          queue_(*this)
    {
    }

    // Start the asynchronous operation
    void
    run()
    {
        // Run the timer. The timer is operated
        // continuously, this simplifies the code.
        on_timer({});

        do_read();
    }

    void
    do_read()
    {
        // make the request empty before reading
        // otherwise the operation behavour is undefined.
        req_ = {};

        // Set the time
        timer_.expires_after(std::chrono::seconds(15));

        // Read a request
        http::async_read(socket_, buffer_, req_,
                         boost::asio::bind_executor(
                             strand_,
                             std::bind(
                                 &http_session::on_read,
                                 shared_from_this(),
                                 std::placeholders::_1)));
    }

    // Called when the timer expires.
    void
    on_timer(boost::system::error_code ec)
    {
        if (ec && ec != boost::asio::error::operation_aborted)
            return fail(ec, "timer");

        // Verify that the timer really expired since the deadline may have moved.
        if (timer_.expiry() <= std::chrono::steady_clock::now())
        {
            // Closing the socket cancels all outstanding operations. They
            // will complete with boost::asio::error::operation_aborted
            if (!beastServer_->Stopping())
            {
                socket_.shutdown(tcp::socket::shutdown_both, ec);
            }
            socket_.close(ec);
            return;
        }

        // Wait on the timer
        timer_.async_wait(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &http_session::on_timer,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void fail(boost::system::error_code ec, const char *what)
    {
        beastServer_->onError(ec, what);
    }

    void
    on_read(boost::system::error_code ec)
    {
        // Happens when the timer closes the socket
        if (ec == boost::asio::error::operation_aborted)
            return;

        // This means they closed the connection
        if (ec == http::error::end_of_stream)
            return do_close();

        if (ec)
        {
            fail(ec, "read");
            return;
        }
        // See if it is a WebSocket Upgrade
        if (websocket::is_upgrade(req_))
        {
            uri request_uri;
            try
            {
                request_uri.set(req_.target().begin(), req_.target().end());
            }
            catch (const std::exception &_)
            {
                fail(ec, "socket uri");
                return;
            }

            std::shared_ptr<ISocketFactory> socketFactory = beastServer_->GetSocketFactory(request_uri);
            if (!socketFactory)
            {
                fail(http::error::bad_target, "Not found");
                return;
            }

            Lv2Log::info("http - (%s) Websocket opened: %s", GetFromAddress(socket_).c_str(), request_uri.str().c_str());
            // Create a WebSocket websocket_session by transferring the socket
            std::string fromName = GetFromAddress(socket_);
            std::make_shared<websocket_session>(
                fromName,
                this->beastServer_,
                socketFactory ? socketFactory->CreateHandler(request_uri) : nullptr,
                std::move(socket_))
                ->run(std::move(req_));
            return;
        }
        // Send the response
        handle_request(
            this->socket_,
            beastServer_,
            std::move(req_),
            queue_);

        // If we aren't at the queue limit, try to pipeline another request
        if (!queue_.is_full())
            do_read();
    }

    void
    on_write(boost::system::error_code ec, bool close)
    {
        // Happens when the timer closes the socket
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (ec)
            return fail(ec, "write");

        if (close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // Inform the queue that a write completed
        if (queue_.on_write())
        {
            // Read another request
            do_read();
        }
    }

    void
    do_close()
    {
        // Send a TCP shutdown
        boost::system::error_code ec;
        if (!beastServer_->Stopping())
        {
            socket_.shutdown(tcp::socket::shutdown_send, ec);
        }
        socket_.close();

        
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    BeastServer *beastServer_;

public:
    ~listener()
    {
        beastServer_->onListenerClosed();
    }
    listener(
        BeastServer *server,
        boost::asio::io_context &ioc,
        tcp::endpoint endpoint)
        : beastServer_(server), acceptor_(ioc), socket_(ioc)
    {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        boost::asio::socket_base::reuse_address option(true);
        acceptor_.set_option(option);
        // Lv2Log::warning("http - Accepting connections with reuse_adddress to allow faster debug turns. Should NOT be enabled in production use. ");

        // Leftover client sockets in TIME_WAIT state prevent binding of the server address.
        // Retry for up to 140 seconds in order to allow the TIME_WAIT sockets to expire.
        // If we still can't establish a connection, we can be sure that somebody else or an existing instance is servicing the address, so
        // quit after that.
        int retries = 0;
        int retryTime = 1;
        int totalRetryTime = 0;
        int MAX_RETRY_TIME = 240; // socket-timeout-ish.

        while (true)
        {
            // Bind to the server address
            acceptor_.bind(endpoint, ec);
            if (!ec)
            {
                break;
            }
            else
            {
                ++retries;
                if (totalRetryTime > MAX_RETRY_TIME)
                {
                    fail(ec, "bind");
                    return;
                }
                Lv2Log::info("http - bind: %s. (port %d) Retrying...", ec.message().c_str(), endpoint.port());
                sleep(retryTime);
                retryTime *= 2;
                if (retryTime > 10)
                    retryTime = 10;
                totalRetryTime += retryTime;
            }
        }

        Lv2Log::info("http - Server listening on %s:%d", endpoint.address().to_string().c_str(), (int)endpoint.port());

        // Start listening for connections
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    void
    stop()
    {
        acceptor_.cancel();
    }

    // Start accepting incoming connections
    void
    run()
    {
        if (!acceptor_.is_open())
            return;
        do_accept();
    }

    void
    do_accept()
    {
        acceptor_.async_accept(
            socket_,
            std::bind(
                &listener::on_accept,
                shared_from_this(),
                std::placeholders::_1));
    }

    void fail(boost::system::error_code ec, const char *what)
    {
        beastServer_->onError(ec, what);
    }
    void
    on_accept(boost::system::error_code ec)
    {
        if (ec)
        {
            fail(ec, "accept");
            return;
        }
        else
        {
            // Create the http_session and run it
            std::make_shared<http_session>(
                beastServer_,
                std::move(socket_))
                ->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

BeastServer::BeastServer(const boost::asio::ip::address &address, int port, const char *rootPath, int threads)
    : address(address),
      port((unsigned short)port),
      doc_root(rootPath),
      threads(threads),
      pBgThread(nullptr)
{
}

void BeastServer::Stop()
{
    if (stopping) return;
    stopping = true;
    listener *p = nullptr;
    {
        std::unique_lock<std::mutex> lk(io_mutex);
        p = (listener*)this->pListener;
        this->pListener = nullptr;
    }
    if (p != nullptr)
    {
        p->stop();
    }
    boost::asio::io_context *pIoContext = nullptr;
    {
        std::unique_lock<std::mutex> lk(io_mutex);
        pIoContext = this->pIoContext;
        this->pIoContext = nullptr;
    }
    if (pIoContext != nullptr)
    {
        pIoContext->stop();
    };
}

void BeastServer::onListenerClosed()
{
    std::unique_lock<std::mutex> lk(io_mutex);
    this->pListener = nullptr;
}

static void ThreadProc(BeastServer *server)
{
    server->Run();
}

void BeastServer::RunInBackground()
{
    if (this->pBgThread != nullptr)
    {
        throw std::runtime_error("Bad state.");
    }
    this->pBgThread = new std::thread(ThreadProc, this);
}

void BeastServer::Join()
{
    if (this->pBgThread != nullptr)
    {
        Stop();
        this->pBgThread->join();
        this->pBgThread = nullptr;
    }
}

void BeastServer::Run()
{
    // The io_context is required for all I/O
    boost::asio::io_context ioc{threads};

    {
        // save a reference for use by stop()
        std::unique_lock<std::mutex> lk(io_mutex);
        pIoContext = &ioc;
    }

    // Create and launch a listening port
    std::shared_ptr<listener> listenerInstance = std::make_shared<listener>(
        this,
        ioc,
        tcp::endpoint{address, port});
    listenerInstance->run();

    // save a reference for use by stop()
    {
        std::unique_lock<std::mutex> lk(io_mutex);
        this->pListener = (void *)&(*listenerInstance);
    }

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        v.emplace_back(
            [&ioc]
            {
                ioc.run();
            });
    ioc.run();

    ioc.stop();
    for (auto &thread : v)
    {
        thread.join();
    }
    {
        std::unique_lock<std::mutex> lk(io_mutex);
        this->pListener = nullptr;
        this->pIoContext = nullptr;
    }
}

BeastServer::~BeastServer()
{
    // Shutdown if running in background.
    Stop();
    Join();
}

void BeastServer::onWarning(boost::system::error_code ec, char const *what)
{
    std::stringstream s;
    s << "http - " << what << ": " << ec.message() << " (" << ec.value() << ')';
    Lv2Log::warning(s.str().c_str());
}
void BeastServer::onInfo(char const *message)
{
    Lv2Log::info("%s", message);
}

void BeastServer::onError(boost::system::error_code ec, char const *what)
{
    std::stringstream s;
    s << "http - " << what << ": " << ec.message() << " (" << ec.value() << ')';
    Lv2Log::warning(s.str().c_str());
}

std::shared_ptr<ISocketFactory> BeastServer::GetSocketFactory(const uri &request_uri)
{
    for (auto factory : this->socket_factories)
    {
        if (factory->wants(request_uri))
        {
            return factory;
        }
    }
    return nullptr;
}
