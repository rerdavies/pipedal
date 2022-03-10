
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

#include "BeastServer.hpp"

#include "Uri.hpp"

#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

using namespace pipedal;
using namespace std;

using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

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

static std::string getHostName()
{
    char buff[512];
    if (gethostname(buff,sizeof(buff)) == 0)
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
    memset(&ifr,0,sizeof(ifr));

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want an IP address attached to "eth0" */
    strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ-1);

    int result = ioctl(fd, SIOCGIFADDR, &ifr);
    if (result == -1) return "";


    close(fd);
    char *name = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    if (name == nullptr) return "";
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
        return extensionsToMimeType.at(ext);
    }
    catch (const std::exception &)
    {
        return "application/octet-stream";
    }
}

std::string GetFromAddress(const tcp::socket &socket)
{
    std::stringstream s;
    s << socket.remote_endpoint().address().to_string() << ':' << socket.remote_endpoint().port();
    return s.str();
}
namespace pipedal
{

    class BeastServerImpl : public BeastServer
    {
    private:
        int signalOnDone = -1;
        std::string address;
        int port = -1;
        std::filesystem::path rootPath;
        int threads = 1;

        std::thread *pBgThread = nullptr;
        std::recursive_mutex io_mutex;

        boost::asio::io_context *pIoContext = nullptr;

        typedef websocketpp::connection_hdl connection_hdl;
        typedef websocketpp::server<websocketpp::config::asio> server;

        class HttpRequestImpl : public HttpRequest
        {
        private:
            const server::connection_type::request_type &m_request;

        public:
            HttpRequestImpl(const server::connection_type::request_type &request)
                : m_request(request)
            {
            }

            virtual const std::string &body() const { return m_request.get_body(); }
            virtual const std::string &method() const { return m_request.get_method(); }
            virtual const std::string &get(const std::string &key) const { return m_request.get_header(key); }
            virtual bool keepAlive() const
            {
                m_request.get_version() != "1.0" || m_request.get_header("Connection") == "keep-alive";
                return true;
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
                if (!value)
                {
                    set("Connection", "close");
                }
            }
        };

        server m_endpoint;
        bool logHttpRequests = false;

        class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>, public SocketHandler::IWriteCallback
        {
            BeastServerImpl *pServer;
            server::connection_ptr webSocket;
            std::string fromAddress;
            std::shared_ptr<SocketHandler> socketHandler;

        private:
            // IWriteCallback
            virtual void close() { webSocket->close(websocketpp::close::status::normal, ""); }

            virtual void writeCallback(const std::string &text)
            {
                webSocket->send(text, websocketpp::frame::opcode::text);
            }
            virtual std::string getFromAddress() const
            {
                return fromAddress;
            }

        public:
            ~WebSocketSession()
            {
            }
            using ptr = std::shared_ptr<WebSocketSession>;
            WebSocketSession(BeastServerImpl *pServer, server::connection_ptr &webSocket)
                : pServer(pServer),
                  webSocket(webSocket)
            {
                uri requestUri(webSocket->get_uri()->str().c_str());
                fromAddress = SS(webSocket->get_socket().remote_endpoint());


                auto pFactory = pServer->GetSocketFactory(requestUri);
                if (!pFactory)
                {
                    Lv2Log::error("Invalid request: " + requestUri.str());
                    webSocket->close(websocketpp::close::status::normal, "Invalid request.");
                    return;
                }
                else
                {
                    using websocketpp::lib::bind;
                    using websocketpp::lib::placeholders::_1;
                    using websocketpp::lib::placeholders::_2;

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

        void on_session_closed(WebSocketSession::ptr session, connection_hdl hConnection)
        {
            m_sessions.erase(session);
            m_connections.erase(hConnection);
        }
        void NotFound(server::connection_type &connection, const std::string &filename)
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
            return;
        };
        void ServerError(server::connection_type &connection, const std::string &error)
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

            if (logHttpRequests)
            {
                tcp::endpoint fromAddress = con->get_socket().remote_endpoint();
                stringstream ss;
                ss << "http - " << fromAddress << "; " << request.get_method() << "; " << con->get_uri()->str();
            }
            std::shared_ptr<websocketpp::uri> connectionUri = con->get_uri();

            uri requestUri(con->get_uri()->str().c_str());

            HttpRequestImpl req(con->get_request());
            HttpResponseImpl res((*con));

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
                    std::string fromAddress = SS(con->get_remote_endpoint());

                    try
                    {
                        if (req.method() == HttpVerb::head)
                        {
                            std::error_code ec;
                            res.set(HttpField::date, HtmlHelper::timeToHttpDate(time(nullptr)));
                            res.set(HttpField::access_control_allow_origin, origin);

                            requestHandler->head_response(fromAddress,requestUri, req, res, ec);
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

                            res.set(HttpField::cache_control, "max-age: 31104000"); // cache for a year.

                            requestHandler->get_response(fromAddress,requestUri, req, res, ec);
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

                            requestHandler->post_response(fromAddress,requestUri, req, res, ec);

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
                        ServerError(*con, SS("Unexpected error. " <<  e.what()));
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

            server::connection_ptr webSocket = m_endpoint.get_con_from_hdl(hdl);

            WebSocketSession::ptr socketSession = std::make_shared<WebSocketSession>(this, webSocket);
            m_sessions.insert(socketSession);
        }

        void on_fail(connection_hdl hdl)
        {
        }
        void on_close(connection_hdl hdl)
        {
            m_connections.erase(hdl);
        }

        void Run()
        {
            try
            {
                // The io_context is required for all I/O
                boost::asio::io_service ioc{threads};
                //*********************************

                m_endpoint.set_reuse_addr(true);

                m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
                // m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
                m_endpoint.set_access_channels(websocketpp::log::alevel::fail);

                m_endpoint.init_asio(&ioc);

                // Bind the handlers we are using
                using websocketpp::lib::bind;
                using websocketpp::lib::placeholders::_1;
                using websocketpp::lib::placeholders::_2;

                m_endpoint.set_open_handler(bind(&BeastServerImpl::on_open, this, _1));
                m_endpoint.set_close_handler(bind(&BeastServerImpl::on_close, this, _1));
                m_endpoint.set_http_handler(bind(&BeastServerImpl::on_http, this, _1));

                std::string hostName = getHostName();
                if (hostName.length() != 0)
                {
                    std::stringstream ss;
                    ss << "Listening on " << hostName << ".local:" << this->port;
                    Lv2Log::info(ss.str());
                }
                std::string ipv4Address = getIpv4Address("eth0");
                if (ipv4Address.length() != 0)
                {
                    Lv2Log::info(SS("Listening on " << ipv4Address << ":" << this->port));
                }
                std::string wifiAddress = getIpv4Address("wlan0");
                if (wifiAddress.length() != 0)
                {
                    Lv2Log::info(SS("Listening on Wi-Fi address " << wifiAddress << ":" << this->port));
                }

                std::stringstream ss;
                ss << port;
                // m_endpoint.listen(this->address, ss.str());
                m_endpoint.listen(tcp::v6(),(uint16_t)port);
                m_endpoint.start_accept();

                // Start IOC service threads.
                std::vector<std::thread> v;
                v.reserve(threads - 1);
                for (auto i = threads - 1; i > 0; --i)
                    v.emplace_back(
                        [&ioc]
                        {
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
                std::cout << e.what() << std::endl;
            }
            if (this->signalOnDone != -1)
            {
                kill(getpid(), this->signalOnDone);
            }
        }

        static void ThreadProc(BeastServerImpl *server)
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
                    m_endpoint.close(*it, websocketpp::close::status::normal, "");
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

        BeastServerImpl(const std::string &address, int port, const char *rootPath, int threads)
            : address(address),
              rootPath(rootPath),
              port(port),
              threads(threads)
        {
        }
    };
} // namespace pipedal

std::shared_ptr<ISocketFactory> BeastServerImpl::GetSocketFactory(const uri &requestUri)
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

std::shared_ptr<BeastServer> pipedal::createBeastServer(const boost::asio::ip::address &address, int port, const char *rootPath, int threads)
{
    return std::shared_ptr<BeastServer>(new BeastServerImpl(address.to_string(), port, rootPath, threads));
}
