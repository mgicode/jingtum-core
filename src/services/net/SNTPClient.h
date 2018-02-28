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

#ifndef SKYWELL_NET_SNTPCLIENT_H_INCLUDED
#define SKYWELL_NET_SNTPCLIENT_H_INCLUDED

#include <beast/threads/Stoppable.h>

namespace skywell {

class SNTPClient : public beast::Stoppable
{
protected:
    explicit SNTPClient (beast::Stoppable& parent);

public:
    static SNTPClient* New (beast::Stoppable& parent);
    virtual ~SNTPClient() { }
    virtual void init (std::vector <std::string> const& servers) = 0;
    virtual void addServer (std::string const& mServer) = 0;
    virtual void queryAll () = 0;
    virtual bool getOffset (int& offset) = 0;
};

} // skywell

#endif
