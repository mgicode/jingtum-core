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

#ifndef SKYWELL_NODESTORE_ENCODEDBLOB_H_INCLUDED
#define SKYWELL_NODESTORE_ENCODEDBLOB_H_INCLUDED

#include <data/nodestore/NodeObject.h>
#include <cstddef> //std::size_t
#include <cstdlib> //std::free

namespace skywell {
namespace NodeStore {

/** Utility for producing flattened node objects.
    @note This defines the database format of a NodeObject!
*/
//  TODO Make allocator aware and use short_alloc
struct EncodedBlob
{
public:
    EncodedBlob() : m_data(nullptr)
    {}

    ~EncodedBlob()
    {
        if(m_data != nullptr)
        {
            std::free(m_data);
            m_data = nullptr;
        }
    }

public:
    void prepare (NodeObject::Ptr const& object);

    void const* getKey  () const noexcept { return m_key; }
    std::size_t getSize () const noexcept { return m_size; }
    void const* getData () const noexcept { return m_data; }

private:
    void resize(std::size_t nSize);

private:
    void const* m_key;
    std::size_t m_size;

    char* m_data;
};

} // NodeStore
} // skywell

#endif
