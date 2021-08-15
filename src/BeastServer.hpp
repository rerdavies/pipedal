#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/network_v4.hpp>

#include <mutex> 
#include <thread>
#include "Uri.hpp"
#include <string_view>
#include <filesystem>


// forward declaration.

class websocket_session;

namespace pipedal {
using HttpRequest =boost::beast::http::request<boost::beast::http::string_body>;


std::string last_modified(const std::filesystem::path& path);

class SocketHandler {
public:

    class IWriteCallback {
    public:
        virtual void close() = 0;

        virtual void writeCallback(const std::string& text) = 0;
    };

private:
    friend class ::websocket_session;


    IWriteCallback *writeCallback_;

    void setWriteCallback(IWriteCallback *writeCallback) {
        writeCallback_ = writeCallback;
    }

protected:
    virtual void onReceive(const std::string_view&text) = 0;
public:
    void receive(const std::string_view&text) {
        onReceive(text);
    }
    void send(const std::string &text) {
        if (writeCallback_ != nullptr)
        {
            writeCallback_->writeCallback(text);
        }
    }
    virtual void Close()
    {
        writeCallback_->close();
    }

    virtual ~SocketHandler() = default;

    virtual void onAttach() { }

};

class ISocketFactory {
public:
    virtual bool wants(const uri &request) = 0;
    virtual std::shared_ptr<SocketHandler> CreateHandler(const uri& request) = 0;
};

class RequestHandler {
private:
    uri target_url_;
public:
    RequestHandler(const char*target_url)
    : target_url_(target_url)
    {

    }
    ~RequestHandler() = default;

    virtual bool wantsRedirect(const uri& requestUri)
    {
        return false;
    }
    uri GetRedirect(const uri& requestUri)
    {
        return requestUri;
    }

    virtual bool wants(boost::beast::http::verb verb,const uri &request_uri) const {
        if (request_uri.segment_count() < target_url_.segment_count())
        {
            return false;
        }
        for (int i = 0; i < target_url_.segment_count(); ++i)
        {
            if (request_uri.segment(i) != target_url_.segment(i))
            {
                return false;
            }
        }
        return true;
    }

    virtual void head_response(
        const uri&request_uri,
        const boost::beast::http::request<boost::beast::http::string_body> &req,
        boost::beast::http::response<boost::beast::http::empty_body> &res,
        boost::beast::error_code &ec) = 0;

    virtual void get_response(
        const uri&request_uri,
        const boost::beast::http::request<boost::beast::http::string_body> &req,
        boost::beast::http::response<boost::beast::http::string_body> &res,
        boost::beast::error_code &ec) = 0;

    virtual void post_response(
        const uri&request_uri,
        const boost::beast::http::request<boost::beast::http::string_body> &req,
        boost::beast::http::response<boost::beast::http::string_body> &res,
        boost::beast::error_code &ec) 
    {
        get_response(request_uri,req,res,ec);    
    }


};
class BeastServer {
private:
    boost::asio::ip::address address;
    unsigned short port;
    std::string doc_root;
    int threads;
    std::mutex log_mutex;
    std::vector<std::shared_ptr<RequestHandler> > request_handlers;
    std::vector<std::shared_ptr<ISocketFactory> > socket_factories;
    std::mutex io_mutex;
    boost::asio::io_context *pIoContext = nullptr;
    void*pListener = nullptr;
    std::thread*pBgThread = nullptr;

    bool logHttpRequests = false;

    bool hasAccessPointGateway = false;
    boost::asio::ip::network_v4 accessPointGateway;
    std::string accessPointServerAddress;
    bool stopping = false;


public:

    BeastServer(
        const boost::asio::ip::address& address, 
        int port, 
        const char*rootPath,
        int threads = 10);

    void Run();
    void RunInBackground();
    bool Stopping() const { return this->stopping; }
    void Stop();
    void Join();

    void SetLogHttpRequests(bool logRequests) { this->logHttpRequests = logRequests; }
    bool LogHttpRequests() const { return this->logHttpRequests; }
    boost::asio::io_context& IoContext() { 
        return *pIoContext;
    }

    void AddRequestHandler(std::shared_ptr<RequestHandler> requestHandler)
    {
        request_handlers.push_back(requestHandler);
    }

    void AddSocketFactory(const std::shared_ptr<ISocketFactory> &socketHandler)
    {
        socket_factories.push_back(socketHandler);
    }

    std::vector<std::shared_ptr<RequestHandler> > &RequestHandlers() { return request_handlers; }

    const std::string&GetDocRoot() { return doc_root; }

    virtual ~BeastServer();

    std::shared_ptr<ISocketFactory> GetSocketFactory(const uri &request_uri);

    void SetAccessPointGateway(const boost::asio::ip::network_v4 &gateway, const std::string&accessPointServerAddress)
    {
        this->accessPointGateway = gateway;
        this->hasAccessPointGateway = true;
        this->accessPointServerAddress = accessPointServerAddress;
    }
    bool HasAccessPointGateway() const { return hasAccessPointGateway; }
    boost::asio::ip::network_v4 &GetAccessPointGateway() { return this->accessPointGateway; }
    const std::string & GetAccessPointServerAddress() const { return this->accessPointServerAddress; }

public:
    virtual void onError(boost::system::error_code ec, char const* what);
    virtual void onWarning(boost::system::error_code ec, char const* what);
    virtual void onInfo(char const* what);

    virtual void onListenerClosed();
};

} // namespace pipedal;