// Copyright (c) 2022-2024 Robin Davies
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

#include "pch.h"

#include <queue>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <filesystem>
#include "HtmlHelper.hpp"
#include "Lv2Log.hpp"
#include <set>
#include <strings.h>
#include "Ipv6Helpers.hpp"
#include "util.hpp"
#include "ofstream_synced.hpp"

#include "WebServer.hpp"

#include "Uri.hpp"
#include "ss.hpp"

#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

#include "WebServerLog.hpp"
#include "TemporaryFile.hpp"

using namespace pipedal;
using namespace std;

const bool ENABLE_KEEP_ALIVE = true;
const std::filesystem::path WEB_TEMP_DIR{"/var/pipedal/web_temp"};

using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

const size_t MAX_READ_SIZE = 512 * 1024 * 1024;
;
using namespace boost;

class request_with_file_upload : public websocketpp::http::parser::parser
{
public:
    using super = websocketpp::http::parser::parser;

    typedef request_with_file_upload type;
    typedef std::shared_ptr<type> ptr;

    request_with_file_upload()
        : m_buf(std::make_shared<std::string>()), m_ready(false) {}

    size_t consume(char const *buf, size_t len, std::error_code &ec);

    /// Returns whether or not the request is ready for reading.
    bool ready() const
    {
        return m_ready;
    }
    std::istream &get_body_input_stream();

    const std::filesystem::path &get_body_input_file();
    size_t content_length() const { return m_content_length; }

    /// Returns the full raw request (including the body)
    std::string raw() const;

    /// Returns the raw request headers only (similar to an HTTP HEAD request)
    std::string raw_head() const;

    /// Set the HTTP method.
    /**
     * Must be a valid HTTP token
     *
     * @since 0.9.0 added return value and removed exception
     *
     * @param [in] method The value to set the method to.
     * @return A status code describing the outcome of the operation.
     */
    std::error_code set_method(std::string const &method);

    /// Return the request method
    std::string const &get_method() const
    {
        return m_method;
    }

    /// Set the HTTP uri.
    /**
     * Must be a valid HTTP uri
     *
     * @since 0.9.0 Return value added
     *
     * @param uri The URI to set
     * @return A status code describing the outcome of the operation.
     */
    std::error_code set_uri(std::string const &uri);

    /// Return the requested URI
    std::string const &get_uri() const
    {
        return m_uri;
    }
    /// Helper function for message::consume. Process request line
    /**
     * @since 0.9.0 (ec parameter added, exceptions removed)
     *
     * @param [in] begin An iterator to the beginning of the sequence.
     * @param [in] end An iterator to the end of the sequence.
     * @return A status code describing the outcome of the operation.
     */

    bool prepare_body(std::error_code &ec);

    std::error_code process(std::string::iterator begin, std::string::iterator end);
    size_t process_body(char const *buf, size_t len,
                        std::error_code &ec);
    std::shared_ptr<std::string> m_buf;
    std::string m_method;
    std::string m_uri;
    size_t m_content_length;
    bool m_ready;
    bool m_uploading_to_file = false;
    size_t m_max_in_memory_upload = 0; // saver to have one code path. And any input body is going to be an upload anyway.
    std::shared_ptr<TemporaryFile> m_temporaryFile;
    std::ofstream m_outputStream;
    std::ifstream m_inputStream;
    std::stringstream m_stringInputStream;
    bool m_outputOpen = false;
};
const std::filesystem::path &request_with_file_upload::get_body_input_file()
{
    if (!this->m_temporaryFile)
        throw std::runtime_error("Request does not have a body.");
    return this->m_temporaryFile->Path();
}
std::istream &request_with_file_upload::get_body_input_stream()
{
    if (!m_outputOpen)
    {
        m_outputOpen = true;
        if (m_uploading_to_file)
        {
            m_inputStream.open(this->m_temporaryFile->Path(), std::ios_base::in | std::ios_base::binary);
            return m_inputStream;
        }
        else
        {
            auto body = super::get_body();
            m_stringInputStream.write(body.c_str(), body.length());
            m_stringInputStream.flush();
            m_stringInputStream.seekg(0);
            return m_stringInputStream;
        }
    }
    else
    {
        if (m_uploading_to_file)
        {
            return m_inputStream;
        }
        else
        {
            return m_stringInputStream;
        }
    }
}
bool request_with_file_upload::prepare_body(std::error_code &ec)
{
    using namespace websocketpp::http;
    bool result = super::prepare_body(ec);
    m_content_length = 0;
    if (!result)
        return result;
    m_content_length = m_body_bytes_needed;
    if (m_body_bytes_needed > m_max_in_memory_upload)
    {
        m_uploading_to_file = true;
        try
        {
            this->m_temporaryFile = std::make_shared<TemporaryFile>(WEB_TEMP_DIR);
            m_outputStream.open(this->m_temporaryFile->Path(), std::ios_base::trunc | std::ios_base::out | std::ios_base::binary);
            if (!m_outputStream)
            {
                throw std::runtime_error(SS("Unable to open file " << this->m_temporaryFile->Path()));
            }
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("Unabe to create upload file. " << e.what()));
            ec = error::make_error_code(error::istream_bad);
        }
    }
    return true;
}

inline size_t request_with_file_upload::process_body(char const *buf, size_t len,
                                                     std::error_code &ec)
{
    using namespace websocketpp::http;
    using namespace websocketpp::http::parser;

    if (!this->m_uploading_to_file)
    {
        return super::process_body(buf, len, ec);
    }
    if (m_body_encoding == body_encoding::plain)
    {
        size_t processed = (std::min)(m_body_bytes_needed, len);
        try
        {
            m_outputStream.write(buf, processed);
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("Can't write to web temporary file " << m_temporaryFile->Path() << ". " << e.what()));
            ec = error::make_error_code(error::istream_bad);
            return 0;
        }
        m_body_bytes_needed -= processed;
        ec = std::error_code();
        return processed;
    }
    else if (m_body_encoding == body_encoding::chunked)
    {
        ec = error::make_error_code(error::unsupported_transfer_encoding);
        return 0;
        // TODO: support for chunked transfers?
    }
    else
    {
        ec = error::make_error_code(error::unknown_transfer_encoding);
        return 0;
    }
}

inline size_t request_with_file_upload::consume(char const *buf, size_t len, std::error_code &ec)
{
    using namespace websocketpp::http;

    size_t bytes_processed = 0;

    if (m_ready)
    {
        // the request is already complete. End immediately without reading.
        ec = std::error_code();
        return 0;
    }

    if (m_body_bytes_needed > 0)
    {
        // The headers are complete, but we are still expecting more body
        // bytes. Process body bytes.
        bytes_processed = process_body(buf, len, ec);
        if (ec)
        {
            return bytes_processed;
        }

        // if we have ready all the expected body bytes set the ready flag
        if (body_ready())
        {
            m_outputStream.close();
            m_ready = true;
        }
        return bytes_processed;
    }

    // at this point we have an incomplete request still waiting for headers

    // copy new candidate bytes into our local buffer. This buffer may have
    // leftover bytes from previous calls. Not all of these bytes are
    // necessarily header bytes (they might be body or even data after this
    // request entirely for a keepalive request)
    m_buf->append(buf, len);

    // Search for delimiter in buf. If found read until then. If not read all
    std::string::iterator begin = m_buf->begin();
    std::string::iterator end;

    for (;;)
    {
        // search for line delimiter in our local buffer
        end = std::search(
            begin,
            m_buf->end(),
            header_delimiter,
            header_delimiter + sizeof(header_delimiter) - 1);

        if (end == m_buf->end())
        {
            // we didn't find the delimiter

            // check that the confirmed header bytes plus the outstanding
            // candidate bytes do not put us over the header size limit.
            if (m_header_bytes + (end - begin) > max_header_size)
            {
                ec = error::make_error_code(error::request_header_fields_too_large);
                return 0;
            }

            // We are out of bytes but not over any limits yet. Discard the
            // processed bytes and copy the remaining unprecessed bytes to the
            // beginning of the buffer in prep for another call to consume.

            // If there are no processed bytes in the buffer right now don't
            // copy the unprocessed ones over themselves.
            if (begin != m_buf->begin())
            {
                std::copy(begin, end, m_buf->begin());
                m_buf->resize(static_cast<std::string::size_type>(end - begin));
            }

            ec = std::error_code();
            return len;
        }

        // at this point we have found a delimiter and the range [begin,end)
        // represents a line to be processed

        // update count of header bytes read so far
        m_header_bytes += (end - begin + sizeof(header_delimiter));

        if (m_header_bytes > max_header_size)
        {
            // This read exceeded max header size
            ec = error::make_error_code(error::request_header_fields_too_large);
            return 0;
        }

        if (end - begin == 0)
        {
            // we got a blank line, which indicates the end of the headers

            // If we never got a valid method or are missing a host header then
            // this request is invalid.
            if (m_method.empty() || get_header("Host").empty())
            {
                ec = error::make_error_code(error::incomplete_request);
                return 0;
            }

            // any bytes left over in the local buffer are bytes we didn't use.
            // When we report how many bytes we consumed we need to subtract
            // these so the caller knows that they need to be processed by some
            // other logic.
            bytes_processed = (len - static_cast<std::string::size_type>(m_buf->end() - end) + sizeof(header_delimiter) - 1);

            // frees memory used temporarily during request parsing
            m_buf.reset();

            // if this was not an upgrade request and has a content length
            // continue capturing content-length bytes and expose them as a
            // request body.

            bool need_more = prepare_body(ec);
            if (ec)
            {
                return 0;
            }

            if (need_more)
            {
                bytes_processed += process_body(buf + bytes_processed, len - bytes_processed, ec);
                if (ec)
                {
                    return 0;
                }
                if (body_ready())
                {
                    m_ready = true;
                }
                ec = std::error_code();
                return bytes_processed;
            }
            else
            {
                m_ready = true;

                // return number of bytes processed (starting bytes - bytes left)
                ec = std::error_code();
                return bytes_processed;
            }
        }
        else
        {
            // we got a line with content
            if (m_method.empty())
            {
                // if we haven't found a method yet process this line as a first line
                ec = this->process(begin, end);
            }
            else
            {
                // this is a second (or later) line, process as a header
                ec = this->process_header(begin, end);
            }
            if (ec)
            {
                return 0;
            }
        }

        // if we got here it means there is another header line to read.
        // advance our cursor to the first character after the most recent
        // delimiter found.
        begin = end + (sizeof(header_delimiter) - 1);
    }
}

inline std::string request_with_file_upload::raw() const
{
    // TODO: validation. Make sure all required fields have been set?
    std::stringstream ret;

    ret << m_method << " " << m_uri << " " << get_version() << "\r\n";
    ret << raw_headers() << "\r\n"
        << m_body;

    return ret.str();
}

inline std::string request_with_file_upload::raw_head() const
{
    // TODO: validation. Make sure all required fields have been set?
    std::stringstream ret;

    ret << m_method << " " << m_uri << " " << get_version() << "\r\n";
    ret << raw_headers() << "\r\n";

    return ret.str();
}

inline std::error_code request_with_file_upload::set_method(std::string const &method)
{
    using namespace websocketpp::http;

    if (std::find_if(method.begin(), method.end(), is_not_token_char) != method.end())
    {
        return error::make_error_code(error::invalid_format);
    }

    m_method = method;
    return std::error_code();
}

inline std::error_code request_with_file_upload::set_uri(std::string const &uri)
{
    // TODO: validation?
    m_uri = uri;

    return std::error_code();
}

inline std::error_code request_with_file_upload::process(std::string::iterator begin, std::string::iterator
                                                                                          end)
{
    using namespace websocketpp::http;
    std::error_code ec;

    std::string::iterator cursor_start = begin;
    std::string::iterator cursor_end = std::find(begin, end, ' ');

    if (cursor_end == end)
    {
        return error::make_error_code(error::incomplete_request);
    }

    ec = set_method(std::string(cursor_start, cursor_end));
    if (ec)
    {
        return ec;
    }

    cursor_start = cursor_end + 1;
    cursor_end = std::find(cursor_start, end, ' ');

    if (cursor_end == end)
    {
        return error::make_error_code(error::incomplete_request);
    }

    ec = set_uri(std::string(cursor_start, cursor_end));
    if (ec)
    {
        return ec;
    }

    return set_version(std::string(cursor_end + 1, end));
}

class CustomPpConfig : public websocketpp::config::asio
{
public:
    typedef CustomPpConfig type;
    typedef websocketpp::config::asio base;

    static size_t max_http_body_size; // websocketpp::config::asio::max_http_body_size;
    typedef pipedal_elog elog_type;
    typedef pipedal_alog alog_type;

    typedef request_with_file_upload request_type;

    struct transport_config : public base::transport_config
    {
        typedef type::concurrency_type concurrency_type;
        typedef type::alog_type alog_type;
        typedef type::elog_type elog_type;
        typedef type::request_type request_type;
        typedef type::response_type response_type;
        typedef websocketpp::transport::asio::basic_socket::endpoint
            socket_type;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config>
        transport_type;
};

size_t CustomPpConfig::max_http_body_size = MAX_READ_SIZE;

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

static std::string getHostName()
{
    char buff[512];
    if (gethostname(buff, sizeof(buff)) == 0)
    {
        buff[511] = '\0';
        return buff;
    }
    return "";
}

static std::string getIpv4Address(const std::string interface)
{
    int fd = -1;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want an IP address attached to "eth0" */
    strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);

    int result = ioctl(fd, SIOCGIFADDR, &ifr);
    if (result == -1)
        return "";

    close(fd);
    char *name = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    if (name == nullptr)
        return "";
    return name;
}
static std::map<std::string, std::string> extensionsToMimeType = {
    {".htm", "text/html; charset=UTF-8"},
    {".html", "text/html; charset=UTF-8"},
    {".php", "text/html"},
    {".css", "text/css"},
    {".txt", "text/plain; charset=UTF-8"},
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
    {".woff", "font/woff2"},
    {".woff2", "font/woff2"}};

// Return a reasonable mime type based on the extension of a file.
std::string
mime_type(const std::filesystem::path &path)
{
    auto const ext = path.extension();
    try
    {
        if (extensionsToMimeType.find(ext) != extensionsToMimeType.end())
        {
            return extensionsToMimeType.at(ext);
        }
    }
    catch (const std::exception &)
    {
    }
    return "application/octet-stream";
}

std::string GetFromAddress(const tcp::socket &socket)
{
    std::stringstream s;
    s << socket.remote_endpoint().address().to_string() << ':' << socket.remote_endpoint().port();
    return s.str();
}
namespace pipedal
{

    class WebServerImpl : public WebServer
    {
    private:
        int signalOnDone = -1;
        std::string address;
        int port = -1;
        std::filesystem::path rootPath;
        int threads = 1;
        size_t maxUploadSize = 512 * 1024 * 1024;

        std::thread *pBgThread = nullptr;
        std::recursive_mutex io_mutex;

        boost::asio::io_context *pIoContext = nullptr;

        typedef websocketpp::connection_hdl connection_hdl;
        typedef websocketpp::server<CustomPpConfig> server;

        class HttpRequestImpl : public HttpRequest
        {
        private:
            server::connection_type::request_type &m_request;

        public:
            HttpRequestImpl(const server::connection_type::request_type &request)
                : m_request(const_cast<server::connection_type::request_type &>(request)) // so we can get access to get_body_input_stream
            {
            }

            virtual std::istream &get_body_input_stream() override { return m_request.get_body_input_stream(); }
            virtual const std::filesystem::path &get_body_temporary_file() override
            {
                return m_request.get_body_input_file();
            }

            virtual size_t content_length() const { return m_request.content_length(); }

            virtual const std::string &method() const { return m_request.get_method(); }
            virtual const std::string &get(const std::string &key) const { return m_request.get_header(key); }
            virtual bool keepAlive() const
            {
                return ENABLE_KEEP_ALIVE && (m_request.get_version() != "1.0" || m_request.get_header("Connection") == "keep-alive");
            }
        };
        class HttpResponseImpl : public HttpResponse
        {
            server::connection_type &request;

        public:
            HttpResponseImpl(server::connection_type &request)
                : request(request)
            {
            }
            virtual void set(const std::string &key, const std::string &value) { request.replace_header(key, value); }
            virtual void setContentLength(size_t size)
            {
                std::stringstream ss;
                ss << size;
                request.replace_header(HttpField::content_length, ss.str());
            }
            virtual void setBody(const std::string &body) { request.set_body(body); }
            virtual void keepAlive(bool value)
            {
                if ((!value) || (!ENABLE_KEEP_ALIVE))
                {
                    set("Connection", "close");
                }
            }
        };

        server m_endpoint;
        bool logHttpRequests = false;

        class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>, public SocketHandler::IWriteCallback
        {
            WebServerImpl *pServer;
            server::connection_ptr webSocket;
            std::string fromAddress;
            std::shared_ptr<SocketHandler> socketHandler;

        private:
            // IWriteCallback
            virtual void close()
            {
                webSocket->close(websocketpp::close::status::normal, "");
                webSocket = nullptr;
            }

            virtual void writeCallback(const std::string &text)
            {
                if (webSocket)
                {
                    webSocket->send(text, websocketpp::frame::opcode::text);
                }
            }
            virtual std::string getFromAddress() const
            {
                return fromAddress;
            }

        public:
            ~WebSocketSession()
            {
                this->socketHandler = nullptr;
                webSocket = nullptr;
                pServer = nullptr;
                Lv2Log::info(SS("WebSocketSession closed. " << fromAddress));
            }
            using ptr = std::shared_ptr<WebSocketSession>;
            WebSocketSession(WebServerImpl *pServer, server::connection_ptr &webSocket)
                : pServer(pServer),
                  webSocket(webSocket)
            {
            }
            void Open()
            {
                uri requestUri(webSocket->get_uri()->str().c_str());
                fromAddress = SS(webSocket->get_socket().remote_endpoint());

                Lv2Log::info(SS("WebSocketSession opened. (" << fromAddress << ")"));

                auto pFactory = pServer->GetSocketFactory(requestUri);
                if (!pFactory)
                {
                    Lv2Log::error("Invalid request: " + requestUri.str());
                    webSocket->close(websocketpp::close::status::normal, "Invalid request.");
                    return;
                }
                else
                {
                    using std::bind;
                    using std::placeholders::_1;
                    using std::placeholders::_2;

                    webSocket->set_message_handler(bind(&WebSocketSession::on_message, this, _1, _2));
                    webSocket->set_close_handler(bind(&WebSocketSession::on_close, this, _1));

                    socketHandler = pFactory->CreateHandler(requestUri);
                    socketHandler->setWriteCallback(this);
                }
            }
            void on_close(connection_hdl hdl)
            {
#ifndef NDEBUG

#endif
                auto shThis = this->shared_from_this(); // we will destruct as we return.
                pServer->on_session_closed(shThis, hdl);
            }
            void on_message(connection_hdl hdl, server::message_ptr msg)
            {
                const std::string &data = msg->get_payload();
                if (socketHandler)
                {
                    std::string_view stringView(data.c_str());
                    socketHandler->onReceive(stringView);
                }
            }
        };

        std::set<WebSocketSession::ptr, std::owner_less<WebSocketSession::ptr>> m_sessions;

        void on_session_closed(WebSocketSession::ptr &session, connection_hdl hConnection)
        {
            m_sessions.erase(session);
            session = nullptr; // probably delete here.
            m_connections.erase(hConnection);
        }

        void NotFound(server::connection_type &connection, const std::string &filename)
        {
            try
            {
                // 404 error
                std::stringstream ss;

                ss << "<!doctype html><html><head>"
                   << "<title>Error 404 (Resource not found)</title><body>"
                   << "<h1>Error 404</h1>"
                   << "<p>The requested URL " << HtmlHelper::HtmlEncode(filename) << " was not found on this server.</p>"
                   << "</body></head></html>";

                std::string body = ss.str();
                connection.set_body(body);
                std::stringstream ssLen;
                ssLen << body.length();
                connection.replace_header(HttpField::content_length, ssLen.str());
                connection.set_status(websocketpp::http::status_code::not_found);
            }
            catch (const std::exception &)
            {
                // ignored. Things weren't going well anyway.
            }
            return;
        };
        void ServerError(server::connection_type &connection, const std::string &error)
        {
            try
            {
                // 404 error
                std::stringstream ss;

                ss << "<!doctype html><html><head>"
                   << "<title>Error 500 (Server error)</title><body>"
                   << "<h1>Error 500</h1>"
                   << "<p>" << HtmlHelper::HtmlEncode(error) << "</p>"
                   << "</body></head></html>";
                std::string body = ss.str();
                connection.set_body(body);
                std::stringstream ssLen;
                ssLen << body.length();
                connection.replace_header(HttpField::content_length, ssLen.str());

                connection.set_status(websocketpp::http::status_code::internal_server_error);
            }
            catch (const std::exception &)
            {
            }
            return;
        };

        static bool isAllowedHeader(const std::string &header)
        {
            return strcasecmp(header.c_str(), HttpField::content_type) == 0;
        }
        static std::vector<std::string> splitHeaders(const std::string &list)
        {
            std::vector<std::string> result;
            std::stringstream s(list);
            while (s.peek() != -1)
            {
                while (s.peek() == ' ' && s.peek() != -1)
                {
                    s.get();
                }
                std::stringstream header;
                while (true)
                {
                    int c = s.peek();
                    if (c == -1 || c == ' ' || c == ',')
                        break;
                    header << (char)s.get();
                }
                while (s.peek() == ' ')
                    s.get();
                if (s.peek() == ',')
                    s.get();
                std::string t = s.str();
                if (t.length() != 0)
                {
                    result.push_back(std::move(t));
                }
            }
            return result;
        }
        static std::string filterCorsHeaders(const std::string &requestedHeaders)
        {
            std::vector<std::string> headers = splitHeaders(requestedHeaders);
            std::stringstream result;
            bool firstTime = true;
            for (size_t i = 0; i < headers.size(); ++i)
            {
                std::string t = headers[i];
                if (isAllowedHeader(t))
                {
                    if (!firstTime)
                    {
                        result << ",";
                    }
                    firstTime = false;
                    result << t;
                }
            }
            return result.str();
        }

        void on_http(connection_hdl hdl)
        {
            // Upgrade our connection handle to a full connection_ptr

            server::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
            auto &request = con->get_request();

            std::string origin = con->get_request_header(HttpField::origin);
            if (origin.size() == 0)
            {
                origin = "*";
            }

            if (logHttpRequests)
            {
                tcp::endpoint fromAddress = con->get_socket().remote_endpoint();
                stringstream ss;
                ss << "http - " << fromAddress << "; " << request.get_method() << "; " << con->get_uri()->str();
                Lv2Log::info(ss.str());
            }
            std::shared_ptr<websocketpp::uri> connectionUri = con->get_uri();

            HttpRequestImpl req(con->get_request());
            HttpResponseImpl res((*con));

            uri requestUri;
            try
            {
                requestUri.set(con->get_uri()->str().c_str());
            }
            catch (const std::exception &e)
            {
                ServerError(*con, SS("Unexpected error. " << e.what()));
                return;
            }

            std::string fromAddress = SS(con->get_remote_endpoint());

            if (req.method() == HttpVerb::options)
            {
                res.set(HttpField::access_control_allow_origin, origin);
                res.set(HttpField::access_control_allow_methods, "POST, GET, OPTIONS, HEADERS");
                res.set(HttpField::access_control_allow_headers, HttpField::content_type);
                res.keepAlive(req.keepAlive());
                con->set_status(websocketpp::http::status_code::ok);
                return;
            }

            for (auto requestHandler : this->request_handlers)
            {

                if (requestHandler->wants(req.method(), requestUri))
                {
                    try
                    {

                        if (req.method() == HttpVerb::head)
                        {
                            std::error_code ec;
                            res.set(HttpField::date, HtmlHelper::timeToHttpDate(time(nullptr)));
                            res.set(HttpField::access_control_allow_origin, origin);

                            requestHandler->head_response(fromAddress, requestUri, req, res, ec);
                            res.keepAlive(req.keepAlive());
                            if (ec == std::errc::no_such_file_or_directory)
                            {
                                NotFound(*con, requestUri.str());
                                return;
                            }

                            if (ec)
                            {
                                ServerError(*con, ec.message());
                                return;
                            }
                            con->set_status(websocketpp::http::status_code::ok);
                            return;
                        }
                        else if (req.method() == HttpVerb::get)
                        {
                            std::error_code ec;
                            res.set(HttpField::date, HtmlHelper::timeToHttpDate(time(nullptr)));
                            res.set(HttpField::access_control_allow_origin, origin);

                            requestHandler->get_response(fromAddress, requestUri, req, res, ec);
                            res.keepAlive(req.keepAlive());

                            if (ec == std::errc::no_such_file_or_directory)
                            {
                                NotFound(*con, requestUri.str());
                                return;
                            }

                            if (ec)
                            {
                                ServerError(*con, ec.message());
                                return;
                            }
                            con->set_status(websocketpp::http::status_code::ok);
                            return;
                        }
                        else if (req.method() == HttpVerb::post)
                        {
                            std::error_code ec;
                            res.keepAlive(req.keepAlive());
                            res.set(HttpField::date, HtmlHelper::timeToHttpDate(time(nullptr)));
                            res.set(HttpField::access_control_allow_origin, origin);

                            requestHandler->post_response(fromAddress, requestUri, req, res, ec);

                            if (ec == std::errc::no_such_file_or_directory)
                            {
                                NotFound(*con, requestUri.str());
                                return;
                            }

                            if (ec)
                            {
                                ServerError(*con, ec.message());
                                return;
                            }
                            con->set_status(websocketpp::http::status_code::ok);

                            return;
                        }
                        ServerError(*con, "Unknown HTTP-Method");
                        return;
                    }
                    catch (std::exception &e)
                    {
                        ServerError(*con, SS("Unexpected error. " << e.what()));
                        return;
                    }
                }
            }

            std::ifstream file;
            std::filesystem::path filename = con->get_resource();
            std::string response;
            if (requestUri.segment_count() == 0)
            {
                filename = this->rootPath / "index.html";
            }
            else
            {
                filename = this->rootPath;
                for (size_t i = 0; i < requestUri.segment_count(); ++i)
                {
                    filename /= requestUri.segment(i);
                }
            }

            if (req.method() != HttpVerb::get)
            {
                ServerError(*con, "Unknown HTTP-Method");
                return;
            }
            std::string mimeType = mime_type(filename);

            file.open(filename.c_str(), std::ios::in);
            if (!file)
            {
                NotFound(*con, requestUri.str());
                return;
            }

            file.seekg(0, std::ios::end);
            response.reserve(file.tellg());
            file.seekg(0, std::ios::beg);

            response.assign((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

            res.set("Content-Type", mimeType);

            if (mimeType.starts_with("image/") || mimeType.starts_with("font/"))
            {
                res.set(HttpField::cache_control, "public, max-age=864000"); // cache for a ten days.
            }

            res.set(HttpField::access_control_allow_origin, origin);
            res.set(HttpField::date, HtmlHelper::timeToHttpDate(time(nullptr)));
            res.setContentLength(response.length());

            con->set_body(response);
            con->set_status(websocketpp::http::status_code::ok);
        }

        typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;

        con_list m_connections;

        void on_open(connection_hdl hdl)
        {
            m_connections.insert(hdl);

            try
            {
                server::connection_ptr webSocket = m_endpoint.get_con_from_hdl(hdl);
                WebSocketSession::ptr socketSession = std::make_shared<WebSocketSession>(this, webSocket);
                socketSession->Open();
                m_sessions.insert(socketSession);
            }
            catch (const std::exception &e)
            {
                Lv2Log::error("Failed to open session: %s", e.what());
            }
        }

        void on_fail(connection_hdl hdl)
        {
            m_connections.erase(hdl);
        }
        void on_close(connection_hdl hdl)
        {
            m_connections.erase(hdl);
        }

        void Run()
        {
            try
            {
                SetThreadName("webMain");
                // The io_context is required for all I/O
                boost::asio::io_service ioc{threads};
                //*********************************

                m_endpoint.set_reuse_addr(true);

                m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
                m_endpoint.set_access_channels(websocketpp::log::alevel::fail);

                m_endpoint.init_asio(&ioc);

                // Bind the handlers we are using
                using std::bind;
                using std::placeholders::_1;
                using std::placeholders::_2;

                m_endpoint.set_open_handler(bind(&WebServerImpl::on_open, this, _1));
                m_endpoint.set_close_handler(bind(&WebServerImpl::on_close, this, _1));
                m_endpoint.set_http_handler(bind(&WebServerImpl::on_http, this, _1));

                DisplayIpAddresses();

                std::stringstream ss;
                ss << port;
                // m_endpoint.listen(this->address, ss.str());
                m_endpoint.listen(tcp::v6(), (uint16_t)port);
                m_endpoint.start_accept();

                // Start IOC service threads.
                std::vector<std::thread> v;
                v.reserve(threads - 1);
                for (auto i = threads - 1; i > 0; --i)
                    v.emplace_back(
                        [&ioc, i]
                        {
                            SetThreadName(SS("web_" << i));
                            ioc.run();
                        });

                // Start the ASIO io_service run loop

                m_endpoint.run();
                // ioc.run();
                /****************** */

                ioc.stop();
                for (auto &thread : v)
                {
                    thread.join();
                }
                Lv2Log::info("Web server terminated.");
            }
            catch (websocketpp::exception const &e)
            {
                Lv2Log::error(SS("Web server: " << e.what()));
            }
            if (this->signalOnDone != -1)
            {
                kill(getpid(), this->signalOnDone);
            }
        }

        static void ThreadProc(WebServerImpl *server)
        {
            server->Run();
        }
        std::shared_ptr<ISocketFactory> GetSocketFactory(const uri &requestUri);

        std::vector<std::shared_ptr<RequestHandler>> request_handlers;
        std::vector<std::shared_ptr<ISocketFactory>> socket_factories;

        virtual void AddRequestHandler(std::shared_ptr<RequestHandler> requestHandler)
        {
            request_handlers.push_back(requestHandler);
        }

        virtual void AddSocketFactory(std::shared_ptr<ISocketFactory> &socketHandler)
        {
            socket_factories.push_back(socketHandler);
        }

    public:
        virtual void SetLogHttpRequests(bool enableLogging)
        {
            this->logHttpRequests = enableLogging;
        }

        virtual void ShutDown(int timeoutMs)
        {

            m_endpoint.stop_listening();
            for (auto &connection : m_connections)
            {

                for (auto it = m_connections.begin(); it != m_connections.end(); ++it)
                {
                    try
                    {
                        m_endpoint.close(*it, websocketpp::close::status::normal, "");
                    }
                    catch (const std::exception &ignored)
                    {
                    }
                }
            }
        }
        virtual void Join()
        {
            if (this->pBgThread)
            {
                this->pBgThread->join();
            }
            this->pBgThread = nullptr;
        }

        virtual void RunInBackground(int signalOnDone)
        {
            if (this->pBgThread != nullptr)
            {
                throw std::runtime_error("Bad state.");
            }
            this->signalOnDone = signalOnDone;
            this->pBgThread = new std::thread(ThreadProc, this);
        }

        virtual void DisplayIpAddresses() override;

        WebServerImpl(const std::string &address, int port, const char *rootPath, int threads, size_t maxUploadSize);
    };
} // namespace pipedal

std::shared_ptr<ISocketFactory> WebServerImpl::GetSocketFactory(const uri &requestUri)
{

    for (auto factory : this->socket_factories)
    {
        if (factory->wants(requestUri))
        {
            return factory;
        }
    }
    return nullptr;
}
WebServerImpl::WebServerImpl(const std::string &address, int port, const char *rootPath, int threads, size_t maxUploadSize)
    : address(address),
      rootPath(rootPath),
      port(port),
      threads(threads),
      maxUploadSize(maxUploadSize)
{
    ::CustomPpConfig::max_http_body_size = maxUploadSize;
}

std::shared_ptr<WebServer> pipedal::WebServer::create(
    const boost::asio::ip::address &address,
    int port,
    const char *rootPath, int threads,
    size_t maxUploadSize)
{
    return std::shared_ptr<WebServer>(new WebServerImpl(address.to_string(), port, rootPath, threads, maxUploadSize));
}

void WebServerImpl::DisplayIpAddresses()
{
    std::string hostName = getHostName();
    if (hostName.length() != 0)
    {
        std::stringstream ss;
        ss << "Listening on mDns address " << hostName << ":" << this->port;
        Lv2Log::info(ss.str());
    }
    std::string ipv4Address = getIpv4Address("eth0");
    if (ipv4Address.length() != 0)
    {
        Lv2Log::info(SS("Listening on eth0 address " << ipv4Address << ":" << this->port));
    }
    std::string wifiAddress = getIpv4Address("wlan0");
    if (wifiAddress.length() != 0)
    {
        if (wifiAddress == "10.42.0.1")
        {
            Lv2Log::info(SS("Listening on Wi-Fi hotspot address " << wifiAddress << ":" << this->port));
        }
        else
        {
            Lv2Log::info(SS("Listening on Wi-Fi address " << wifiAddress << ":" << this->port));
        }
    }
}