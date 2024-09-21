/*
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
    Based on websocketpp/logger/pipedal_logger.hpp, Copyright Peter Thorson.
*/



/// Logger that redirects webserverpp messages to Lv2Log.
#ifndef WEBSOCKETPP_LOGGER_PIPEDAL_HPP
#define WEBSOCKETPP_LOGGER_PIPEDAL_HPP


/* Need a way to print a message to the log
 *
 * - timestamps
 * - channels
 * - thread safe
 * - output to stdout or file
 * - selective output channels, both compile time and runtime
 * - named channels
 * - ability to test whether a log message will be printed at compile time
 *
 */

#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include "Lv2Log.hpp"
#include "ss.hpp"
#include <websocketpp/logger/levels.hpp>




namespace pipedal {




class pipedal_elog {
public:
    using level = websocketpp::log::level;
    using elevel = websocketpp::log::elevel;
    using channel_type_hint = websocketpp::log::channel_type_hint;
    pipedal_elog()
      : m_channels(0xffffffff)
      {

      }

    /// Destructor
    ~pipedal_elog() {}

    /// Copy constructor
    pipedal_elog(pipedal_elog const & other)
     : m_channels(other.m_channels)
    {}

    pipedal_elog(level level,channel_type_hint::value hint = channel_type_hint::error)
    {
        this->m_channels = level;
    }
    
#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    // no copy assignment operator because of const member variables
    pipedal_elog & operator=(pipedal_elog const &) = delete;
#endif // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#ifdef _WEBSOCKETPP_MOVE_SEMANTICS_
    /// Move constructor
    pipedal_elog(pipedal_elog && other)
     : m_channels(other.m_channels)
    {}

#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    // no move assignment operator because of const member variables
    pipedal_elog & operator=(pipedal_elog &&) = delete;
#endif // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#endif // _WEBSOCKETPP_MOVE_SEMANTICS_

    void set_channels(level channels) {
        m_channels |= channels;
    }

    void clear_channels(level channels) {
        m_channels &= ~m_channels;

    }

    /// Write a string message to the given channel
    /**
     * @param channel The channel to write tosu
     * @param msg The message to write
     */
    void write(level channel, std::string const & msg) {
        if (!dynamic_test(channel)) return;
        switch (channel)
        {
            case elevel::devel:
            case elevel::library:
                Lv2Log::debug("WebServer: %s",msg.c_str());
                break;
            case elevel::info:
                Lv2Log::info("WebServer: %s",msg.c_str());
                break;
            case elevel::warn:
                Lv2Log::warning("WebServer: %s",msg.c_str());
                break;
            case elevel::rerror:
                Lv2Log::error("WebServer: %s",msg.c_str());
                break;
            case elevel::fatal:
                Lv2Log::error("WebServer fatal error: %s",msg.c_str());
                break;
            default:
                break;
        }
    }

    /// Write a cstring message to the given channel
    /**
     * @param channel The channel to write to
     * @param msg The message to write
     */
    void write(level channel, char const * msg) {
        write(channel,std::string(msg));
    }

    constexpr bool static_test(level channel) const {
        return m_static_channels;
    }

    bool dynamic_test(level channel) {
        return (m_channels & channel) != 0;
    }

protected:
private:
    static constexpr level m_static_channels = elevel::warn | elevel::rerror | elevel::fatal;
    level m_channels;
};

class pipedal_alog {
public:
    using level = websocketpp::log::level;
    using alevel = websocketpp::log::alevel;
    using channel_type_hint = websocketpp::log::channel_type_hint;


    pipedal_alog()
    :m_channels(alevel::fail)
    {
    }

    pipedal_alog(level  level, channel_type_hint::value hint = channel_type_hint::access)
    : m_channels(level)
    {

    }
    /// Destructor
    ~pipedal_alog() {}

    /// Copy constructor
    pipedal_alog(pipedal_alog const & other)
    :m_channels(other.m_channels)
    {}
    
#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    // no copy assignment operator because of const member variables
    pipedal_alog & operator=(pipedal_elog const &) = delete;
#endif // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#ifdef _WEBSOCKETPP_MOVE_SEMANTICS_
    /// Move constructor
    pipedal_alog(pipedal_alog && other)
    {}

#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    // no move assignment operator because of const member variables
    pipedal_alog & operator=(pipedal_elog &&) = delete;
#endif // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#endif // _WEBSOCKETPP_MOVE_SEMANTICS_

    void set_channels(level channels) {
        m_channels |= channels;
    }

    void clear_channels(level channels) {
        m_channels = m_channels & ~channels;
    }

    /// Write a string message to the given channel
    /**
     * @param channel The channel to write to
     * @param msg The message to write
     */
    void write(level channel, std::string const & msg) {
        if (dynamic_test(channel))
        {
            Lv2Log::info(SS('[' << alevel::channel_name(channel) << "] " << msg));
        }
    }

    /// Write a cstring message to the given channel
    /**
     * @param channel The channel to write to
     * @param msg The message to write
     */
    void write(level channel, char const * msg) {
        write(channel,std::string(msg));
    }

    constexpr bool static_test(level channel) const {
        return  (this->m_static_channels & channel) != 0;
    }

    bool dynamic_test(level channel) {
        return false;
    }

protected:
private:
    static constexpr level m_static_channels = alevel::access_core;
    level m_channels;
};

} //namespace pipedal

#endif // WEBSOCKETPP_LOGGER_PIPEDAL_HPP
