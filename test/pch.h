#pragma once

// Diagnostic type used to report calculated type T in an error message.
template <typename T> class TypeDisplay;


#include <string>
#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <mutex>
#include <string>
#include <vector>
#include <string_view>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
