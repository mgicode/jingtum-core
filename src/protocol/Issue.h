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

#ifndef SKYWELL_PROTOCOL_ISSUE_H_INCLUDED
#define SKYWELL_PROTOCOL_ISSUE_H_INCLUDED

#include <cassert>
#include <functional>
#include <type_traits>

#include <protocol/UintTypes.h>

namespace skywell {

/** A currency issued by an account.

    When ByValue is `false`, this only stores references, and the caller
    is responsible for managing object lifetime.

    @see Currency, Account, Issue, IssueRef, Book
*/
template <bool ByValue>
class IssueType
{
public:
    typedef typename
    std::conditional <ByValue, Currency, Currency const&>::type
    IssueCurrency;

    typedef typename
    std::conditional <ByValue, Account, Account const&>::type
    IssueAccount;

    IssueCurrency currency;
    IssueAccount account;

    IssueType ()
    {
    }

    IssueType (Currency const& c, Account const& a)
            : currency (c), account (a)
    {
    }

    template <bool OtherByValue>
    IssueType (IssueType <OtherByValue> const& other)
        : currency (other.currency)
        , account (other.account)
    {
    }

    /** Assignment. */
    template <bool MaybeByValue = ByValue, bool OtherByValue>
    typename std::enable_if <MaybeByValue, IssueType&>::type
    operator= (IssueType <OtherByValue> const& other)
    {
        currency = other.currency;
        account = other.account;
        return *this;
    }
};

template <bool ByValue>
bool isConsistent(IssueType<ByValue> const& ac)
{
    return isSWT (ac.currency) == isSWT (ac.account);
}

template <bool ByValue>
std::string to_string (IssueType<ByValue> const& ac)
{
    if (isSWT (ac.account))
        return to_string (ac.currency);

    return to_string(ac.account) + "/" + to_string(ac.currency);
}

template <bool ByValue>
std::ostream& operator<< (
    std::ostream& os, IssueType<ByValue> const& x)
{
    os << to_string (x);
    return os;
}

template <bool ByValue, class Hasher>
void hash_append (Hasher& h, IssueType<ByValue> const& r)
{
    using beast::hash_append;
    hash_append (h, r.currency, r.account);
}

/** Ordered comparison.
    The assets are ordered first by currency and then by account,
    if the currency is not SWT.
*/
template <bool LhsByValue, bool RhsByValue>
int compare (IssueType <LhsByValue> const& lhs,
    IssueType <RhsByValue> const& rhs)
{
    int diff = compare (lhs.currency, rhs.currency);
    if (diff != 0)
        return diff;
    if (isSWT (lhs.currency))
        return 0;
    return compare (lhs.account, rhs.account);
}

/** Equality comparison. */
/** @{ */
template <bool LhsByValue, bool RhsByValue>
bool operator== (IssueType <LhsByValue> const& lhs,
    IssueType <RhsByValue> const& rhs)
{
    return compare (lhs, rhs) == 0;
}

template <bool LhsByValue, bool RhsByValue>
bool operator!= (IssueType <LhsByValue> const& lhs,
    IssueType <RhsByValue> const& rhs)
{
    return ! (lhs == rhs);
}
/** @} */

/** Strict weak ordering. */
/** @{ */
template <bool LhsByValue, bool RhsByValue>
bool operator< (IssueType <LhsByValue> const& lhs,
    IssueType <RhsByValue> const& rhs)
{
    return compare (lhs, rhs) < 0;
}

template <bool LhsByValue, bool RhsByValue>
bool operator> (IssueType <LhsByValue> const& lhs,
    IssueType <RhsByValue> const& rhs)
{
    return rhs < lhs;
}

template <bool LhsByValue, bool RhsByValue>
bool operator>= (IssueType <LhsByValue> const& lhs,
    IssueType <RhsByValue> const& rhs)
{
    return ! (lhs < rhs);
}

template <bool LhsByValue, bool RhsByValue>
bool operator<= (IssueType <LhsByValue> const& lhs,
    IssueType <RhsByValue> const& rhs)
{
    return ! (rhs < lhs);
}
/** @} */

//------------------------------------------------------------------------------

typedef IssueType <true> Issue;
typedef IssueType <false> IssueRef;

//------------------------------------------------------------------------------

/** Returns an asset specifier that represents SWT. */
inline Issue const& xrpIssue ()
{
    static Issue issue {xrpCurrency(), xrpAccount()};
    return issue;
}

/** Returns an asset specifier that represents no account and currency. */
inline Issue const& noIssue ()
{
    static Issue issue {noCurrency(), noAccount()};
    return issue;
}

}

#endif
