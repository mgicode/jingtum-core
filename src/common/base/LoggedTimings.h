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

#ifndef SKYWELL_BASICS_LOGGEDTIMINGS_H_INCLUDED
#define SKYWELL_BASICS_LOGGEDTIMINGS_H_INCLUDED

#include <common/base/Log.h>
#include <common/misc/Utility.h>

namespace skywell {

namespace detail {

/** Template class that performs destruction of an object.
    Default implementation simply calls delete
*/
template <typename Object>
struct Destroyer;

/** Specialization for std::shared_ptr.
*/
template <typename Object>
struct Destroyer <std::shared_ptr <Object> >
{
    static void destroy (std::shared_ptr <Object>& p)
    {
        p.reset ();
    }
};

/** Specialization for std::unordered_map
*/
template <typename Key, typename Value, typename Hash, typename Alloc>
struct Destroyer <std::unordered_map <Key, Value, Hash, Alloc> >
{
    static void destroy (std::unordered_map <Key, Value, Hash, Alloc>& v)
    {
        v.clear ();
    }
};

/** Cleans up an elaspsed time so it prints nicely */
inline double cleanElapsed (double seconds) noexcept
{
    if (seconds >= 10)
        return std::floor (seconds + 0.5);

    return static_cast <int> ((seconds * 10 + 0.5) / 10);
}

} // detail

//------------------------------------------------------------------------------

/** Measure the time required to destroy an object.
*/

template <typename Object>
double timedDestroy (Object& object)
{
    std::int64_t const startTime (getHighResolutionTicks ());

    detail::Destroyer <Object>::destroy (object);

    return (getHighResolutionTicks() - startTime ) / 1000000;
}

/** Log the timed destruction of an object if the time exceeds a threshold.
*/
template <typename PartitionKey, typename Object>
void logTimedDestroy (
    Object& object,
    std::string const& objectDescription,
    double thresholdSeconds = 1)
{
    double const seconds = timedDestroy (object);

    if (seconds > thresholdSeconds)
    {
        deprecatedLogs().journal("LoggedTimings").warning <<
            objectDescription << " took " <<
            detail::cleanElapsed (seconds) <<
            " seconds to destroy";
    }
}

//------------------------------------------------------------------------------

/** Log a timed function call if the time exceeds a threshold. */
template <typename Function>
void logTimedCall (beast::Journal::Stream stream,
                   std::string const& description,
                   char const* fileName,
                   int lineNumber,
                   Function f,
                   double thresholdSeconds = 1)
{
    double const seconds = measureFunctionCallTime (f);

    if (seconds > thresholdSeconds)
    {
        stream << description 
               << " took "
               << detail::cleanElapsed (seconds) 
               << " seconds to execute at "
               << fileLocation(fileName, lineNumber);
    }
}

} // skywell

#endif
