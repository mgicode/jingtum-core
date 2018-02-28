//------------------------------------------------------------------------------
/*
    This file is part of skywelld: https://github.com/skywell/skywelld
    Copyright (c) 2012, 2013 Skywell Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef SKYWELL_COMMON_MISC_UTILITY_H_
#define SKYWELL_COMMON_MISC_UTILITY_H_

#include <boost/asio.hpp>
#include <sstream>
#include <vector>
#include <string>
#include <type_traits>
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <iterator>
#include <time.h> //clock_gettime

static std::string
convert_endpoint_to_string (const boost::asio::ip::tcp::endpoint& t)
{
    std::string str;
    std::stringstream ss;
    ss << t;
    ss >> str;

    return str;
}

static std::pair<boost::asio::ip::tcp::endpoint, bool>
endpoint_from_string(const std::string& s)
{
    std::vector<std::string> v;
    if(std::find(std::begin(s), std::end(s), ':') != std::end(s))
    {
        boost::split(v, s, boost::is_any_of(":"));

        if(v.size() != 2)
            std::runtime_error("Input string's format must be [x.x.x.x:30 or x.x.x.x 30]");

        boost::asio::ip::address address(boost::asio::ip::address::from_string(v[0]));
        unsigned short port = static_cast<unsigned short>(std::stoi(v[1]));
        boost::asio::ip::tcp::endpoint endpoint(address, port);
        
        return std::make_pair(endpoint, true);
    }
    else if(std::find(std::begin(s), std::end(s), ' ') != std::end(s))
    {
        boost::split(v, s, boost::is_any_of(" "));    

        if(v.size() != 2)
            std::runtime_error("Input string's format must be [x.x.x.x:30 or x.x.x.x 30]");

        boost::asio::ip::address address(boost::asio::ip::address::from_string(v[0]));
        unsigned short port = static_cast<unsigned short>(std::stoi(v[1]));
        boost::asio::ip::tcp::endpoint endpoint(address, port);

        return std::make_pair(endpoint, true);
    }

    //default
    boost::asio::ip::tcp::endpoint endpoint{};

    return std::make_pair(endpoint, false);
}

static std::pair<boost::asio::ip::address, bool>
address_from_string(const std::string& s)
{
    std::vector<std::string> v;
    boost::split(v, s, boost::is_any_of("."));
    if(std::all_of(std::begin(s), std::end(s), [](char c) { return c >= 0 && c <= 255; }))
    {
        boost::asio::ip::address address(boost::asio::ip::address::from_string(s));
        return std::make_pair(address, true);
    }

    return std::make_pair(boost::asio::ip::address{}, false);
}

static bool 
is_broadcast(const boost::asio::ip::address& address)
{   
    return address.to_v4() == boost::asio::ip::address_v4::broadcast();
}

static bool
is_private(const boost::asio::ip::address& address)
{ 
    std::array<unsigned char, 4> byte = address.to_v4().to_bytes();
    unsigned char _1 = byte[0];
    unsigned char _2 = byte[1];
    unsigned char _3 = byte[2];
    unsigned char _4 = byte[3];

    return (((_1 << 12) & 0xff000000) == 0x0a000000) || //A class private address 
           (((_2 << 8) & 0xfff00000) == 0xac100000) || //B class private address
           (((_3 << 4) & 0xffff0000) == 0xc0a80000);   //C class private address
}

static bool 
is_public(const boost::asio::ip::address& address)
{
    return ! is_private(address) &&
           ! address.is_multicast() &&
           ! address.is_loopback() &&
             is_broadcast(address);
}

#ifndef STL_NO_CXX14_MAKE_UNIQUE
    #if __cplusplus >= 201305
        #define STL_NO_CXX14_MAKE_UNIQUE 1
    #else
        #define STL_NO_CXX14_MAKE_UNIQUE 0
    #endif
#endif

#if !STL_NO_CXX14_MAKE_UNIQUE
namespace std {

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T> (new T(std::forward<Args>(args)...));
}

template <typename Iterator>
std::reverse_iterator<Iterator> make_reverse_iterator( Iterator i )
{
    return std::reverse_iterator<Iterator>(i);
}

}
#endif

static std::string
fileLocation(const std::string& filepath, const int line)
{
    std::size_t pos = filepath.find_last_of('/');
    std::string location;
    if(pos + 1 < filepath.length())
    {
        location = filepath.substr(pos+1) + "(" + std::to_string(line) + ")";
    }

    return location;
}

static int64_t 
getHighResolutionTicks()
{
    timespec t;
    clock_gettime (CLOCK_MONOTONIC, &t);

    return (t.tv_sec * (std::int64_t) 1000000) + (t.tv_nsec / 1000);
}

template <typename Function>
static double 
measureFunctionCallTime (Function f)
{
    const std::int64_t start(getHighResolutionTicks());

    f();

    const std::int64_t elapse(getHighResolutionTicks() - start);
    
    return elapse;
}

#endif
