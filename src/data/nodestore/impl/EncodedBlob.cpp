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

#include <BeastConfig.h>
#include <data/nodestore/impl/EncodedBlob.h>
#include <cstring>

namespace skywell {
namespace NodeStore {

void
EncodedBlob::resize(std::size_t nSize)
{
    if(nSize <= 0)
    {
        std::free(m_data);

        m_data = nullptr;
        m_size = 0;
    }
    else
    {
        if(m_data != nullptr)
        {
            m_data = (char*)std::realloc(m_data, nSize * sizeof(char));

            if(nSize > m_size)
            {
                memset(m_data, 0, nSize);
            }
        }
        else
        {
            m_data = (char*)std::calloc(nSize, sizeof(char));
        }

        m_size = nSize;
    }
}

void
EncodedBlob::prepare (NodeObject::Ptr const& object)
{
    m_key = object->getHash().begin ();

    // This is how many bytes we need in the flat data
    m_size = object->getData ().size () + 9;

    resize(m_size);

    {
        // these 8 bytes are unused
        //std::uint64_t* buf = static_cast<std::uint64_t*> (m_data);

        //*buf = 0;

        std::memset(m_data, 0, 8);
    }

    {
        char* buf = static_cast<char*> (m_data);

        buf[8] = static_cast<unsigned char> (object->getType ());

        memcpy (&buf[9], object->getData ().data (), object->getData ().size ());
    }
}

} // NodeStore
} // skywell
