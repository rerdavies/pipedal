#pragma once

#include <string.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include <cstring>


#include <mutex> 
#include <thread>
#include "Uri.hpp"
#include <string_view>
#include <filesystem>
#include "TemporaryFile.hpp"





// forward declaration.

class websocket_session;

namespace pipedal {


class  HttpRequest {
public:
    //virtual const std::string&body() const = 0;
    virtual std::istream &get_body_input_stream() = 0;
    virtual const std::filesystem::path& get_body_temporary_file() = 0;
    virtual size_t content_length() const = 0;
    virtual const std::string &method() const = 0;
    virtual const std::string&get(const std::string&key) const = 0;
    virtual bool keepAlive() const = 0;
};

class HttpResponse {
public:
    virtual void set(const std::string&key, const std::string&value) = 0;
    virtual void setContentLength(size_t size) = 0;
    virtual void setBody(const std::string&body) = 0;
    virtual void setBodyFile(std::shared_ptr<TemporaryFile>&temporaryFile) = 0;
    virtual void setBodyFile(std::filesystem::path&path, bool deleteWhenDone) = 0;
    virtual void clearBody() = 0; // but leave the file size intact (e.g for a HEAD request).

    virtual void  keepAlive(bool value)  = 0;
};


struct HttpVerb {
    constexpr static const char * options = "OPTIONS";
    constexpr static const char * get = "GET";
    constexpr static const char * post = "POST";
    constexpr static const char * head = "HEAD";
};


class HttpField {
public:
    constexpr static const char * LastModified = "Last-Modified";
    constexpr static const char* content_length = "Content-Length";
    constexpr static const char* content_type = "Content-Type";
    constexpr static const char* cache_control = "Cache-Control";
    constexpr static const char* content_disposition = "Content-Disposition";
    constexpr static const char* access_control_allow_origin = "Access-Control-Allow-Origin";
    constexpr static const char* access_control_allow_methods= "Access-Control-Allow-Methods";
    constexpr static const char* access_control_allow_headers = "Access-Control-Allow-Headers";
    constexpr static const char* access_control_request_headers = "Access-Control-Request-Headers";
    constexpr static const char* origin = "Origin";
    constexpr static const char* date = "Date";
    constexpr static const char* referer = "Referer";
    constexpr static const char * location = "Location";
    constexpr static const char* accept_encoding = "Accept-Encoding";
    constexpr static const char* content_encoding = "Content-Encoding";
};


//xxx move this to HtmlHelpers.
std::string last_modified(const std::filesystem::path& path);

class WebServerImpl;

class SocketHandler {
    friend class WebServerImpl;
public:
    class IWriteCallback {
    public:
        virtual void close() = 0;

        virtual void writeCallback(const std::string& text) = 0;
        virtual std::string getFromAddress() const = 0;
    };

private:


    IWriteCallback *writeCallback_;
public:
    void setWriteCallback(IWriteCallback *writeCallback) {
        writeCallback_ = writeCallback;
    }

public:
    virtual void onSocketClosed() = 0;
    virtual void onReceive(const std::string_view&text) = 0;
public:
    std::string getFromAddress() const { return writeCallback_->getFromAddress(); }
    void receive(const std::string_view&text) {
        onReceive(text);
    }
    void send(const std::string &text) {
        if (writeCallback_ != nullptr)
        {
            writeCallback_->writeCallback(text);
        }
    }
    virtual void OnSocketClosed()
    {
        writeCallback_ = nullptr;
    }
    virtual void Close()
    {
        if (writeCallback_ != nullptr)
        {
            writeCallback_->close();
            writeCallback_ =  nullptr;
        }
    }

    virtual ~SocketHandler() {}

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

    virtual bool wants(const std::string& method,const uri &request_uri) const {
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
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) = 0;

    virtual void head_response(
        const std::string&fromAddress,
        const uri&request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) {
        head_response(request_uri,req,res,ec);
    }



    virtual void get_response(
        const uri&request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) = 0;

    virtual void get_response(
        const std::string&fromAddress,
        const uri&request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec)
        {
            get_response(request_uri,req,res,ec);
        } 


    virtual void post_response(
        const uri&request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) 
    {
        get_response(request_uri,req,res,ec);    
    }
    virtual void post_response(
        const std::string&fromAddress,
        const uri&request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) 
    {
        post_response(
            request_uri,req,res,ec);
    }

};

class WebServer {
public:
    virtual ~WebServer() { }

    virtual void SetLogHttpRequests(bool enableLogging) = 0;
    virtual void DisplayIpAddresses() = 0;

    virtual void AddRequestHandler(std::shared_ptr<RequestHandler> requestHandler) = 0;
    virtual void AddSocketFactory(std::shared_ptr<ISocketFactory> &socketHandler) = 0;

    virtual void StopListening() = 0;
    virtual void ShutDown(int timeoutMs) = 0;
    virtual void Join() = 0;

    // signalOnDone: fire the specified POSIX signal when the service thread terminates. -1 for no signal.
    virtual void RunInBackground(int signalOnDone = -1) = 0;

    static std::shared_ptr<WebServer> create(
        const boost::asio::ip::address &address, 
        int port, 
        const char *rootPath, 
        int threads,
        size_t maxUploadsize);


};






} // namespace pipedal;