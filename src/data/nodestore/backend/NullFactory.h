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

#ifndef SKYWELL_APP_DATA_NODESTORE_NULL_H_INCLUDED
#define SKYWELL_APP_DATA_NODESTORE_NULL_H_INCLUDED

#include <BeastConfig.h>
#include <data/nodestore/Factory.h>
#include <data/nodestore/Manager.h>
#include <common/misc/Utility.h>

namespace skywell {
namespace NodeStore {

class NullBackend : public Backend
{
public:
    NullBackend ()
    {
    }

    ~NullBackend ()
    {
    }

    std::string
    getName()
    {
        return std::string ();
    }

    void
    close() override
    {
    }

    Status
    fetch (void const*, NodeObject::Ptr*)
    {
        return notFound;
    }

    bool
    canFetchBatch() override
    {
        return false;
    }

    std::vector<std::shared_ptr<NodeObject>>
    fetchBatch (std::size_t n, void const* const* keys) override
    {
        throw std::runtime_error("pure virtual called");
        return {};
    }

    void
    store (NodeObject::ref object)
    {
    }

    void
    storeBatch (Batch const& batch)
    {
    }

    void
    for_each (std::function <void(NodeObject::Ptr)> f)
    {
    }

    int
    getWriteLoad ()
    {
        return 0;
    }

    void
    setDeletePath() override
    {
    }

    void
    verify() override
    {
    }

private:
};

//------------------------------------------------------------------------------

class NullFactory : public Factory
{
public:
    NullFactory()
    {
        Manager::instance().insert(*this);
    }

    ~NullFactory()
    {
        Manager::instance().erase(*this);
    }

    std::string
    getName () const
    {
        return "none";
    }

    std::unique_ptr <Backend>
    createInstance (
        size_t,
        Section const&,
        Scheduler&, beast::Journal)
    {
        return std::make_unique <NullBackend> ();
    }
};

static NullFactory nullFactory;

}
}

#endif
