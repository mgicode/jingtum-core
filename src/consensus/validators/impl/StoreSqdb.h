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

#ifndef SKYWELL_VALIDATORS_STORESQDB_H_INCLUDED
#define SKYWELL_VALIDATORS_STORESQDB_H_INCLUDED

#include <beast/utility/Journal.h>
#include <consensus/validators/impl/Store.h>
#include <data/database/SociDB.h>

namespace skywell {
namespace Validators {

/** Database persistence for Validators using SQLite */
class StoreSqdb : public Store
{
private:
    beast::Journal m_journal;
    soci::session  m_session;

public:
    enum
    {
        // This affects the format of the data!
        currentSchemaVersion = 1
    };

    explicit
    StoreSqdb (beast::Journal journal);

    ~StoreSqdb();

    void
    open (SociConfig const& sociConfig);
};

}
}

#endif
