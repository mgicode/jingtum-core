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

#ifndef SKYWELL_SERVER_HANDOFF_H_INCLUDED
#define SKYWELL_SERVER_HANDOFF_H_INCLUDED

#include <services/server/Writer.h>
#include <memory>

namespace skywell {

/** Used to indicate the result of a server connection handoff. */
struct Handoff
{
    // When `true`, the Session will close the socket. The
    // Handler may optionally take socket ownership using std::move
    bool moved = false;

    // If response is set, this determines the keep alive
    bool keep_alive = false;

    // When set, this will be sent back
    std::shared_ptr<HTTP::Writer> response;

    bool handled() const
    {
        return moved || response;
    }
};

} // skywell

#endif
