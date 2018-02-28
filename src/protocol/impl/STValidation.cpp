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
#include <protocol/STValidation.h>
#include <protocol/HashPrefix.h>
#include <common/base/Log.h>
#include <common/json/to_string.h>

namespace skywell {

STValidation::STValidation (SerialIter& sit, bool checkSignature)
    : STObject (getFormat (), sit, sfValidation)
    , mTrusted (false)
{
    mNodeID = SkywellAddress::createNodePublic (getFieldVL (sfSigningPubKey)).getNodeID ();
    assert (mNodeID.isNonZero ());

    if  (checkSignature && !isValid ())
    {
        WriteLog (lsTRACE, Ledger) << "Invalid validation " << getJson (0);
        throw std::runtime_error ("Invalid validation");
    }
}

STValidation::STValidation (
    uint256 const& ledgerHash, std::uint32_t signTime,
    SkywellAddress const& raPub, bool isFull)
    : STObject (getFormat (), sfValidation)
    , mTrusted (false)
{
    // Does not sign
    setFieldH256 (sfLedgerHash, ledgerHash);
    setFieldU32 (sfSigningTime, signTime);

    setFieldVL (sfSigningPubKey, raPub.getNodePublic ());
    mNodeID = raPub.getNodeID ();
    assert (mNodeID.isNonZero ());

    if (!isFull)
        setFlag (kFullFlag);
}

void STValidation::sign (SkywellAddress const& raPriv)
{
    uint256 signingHash;
    sign (signingHash, raPriv);
}

void STValidation::sign (uint256& signingHash, SkywellAddress const& raPriv)
{
    setFlag (vfFullyCanonicalSig);

    signingHash = getSigningHash ();
    Blob signature;
    raPriv.signNodePrivate (signingHash, signature);
    setFieldVL (sfSignature, signature);
}

uint256 STValidation::getSigningHash () const
{
    return STObject::getSigningHash (HashPrefix::validation);
}

uint256 STValidation::getLedgerHash () const
{
    return getFieldH256 (sfLedgerHash);
}

std::uint32_t STValidation::getSignTime () const
{
    return getFieldU32 (sfSigningTime);
}

std::uint32_t STValidation::getFlags () const
{
    return getFieldU32 (sfFlags);
}

bool STValidation::isValid () const
{
    return isValid (getSigningHash ());
}

bool STValidation::isValid (uint256 const& signingHash) const
{
    try
    {
        const ECDSA fullyCanonical = getFlags () & vfFullyCanonicalSig ?
                                            ECDSA::strict : ECDSA::not_strict;
        SkywellAddress   raPublicKey = SkywellAddress::createNodePublic (getFieldVL (sfSigningPubKey));
        return raPublicKey.isValid () &&
            raPublicKey.verifyNodePublic (signingHash, getFieldVL (sfSignature), fullyCanonical);
    }
    catch (...)
    {
        WriteLog (lsINFO, Ledger) << "exception validating validation";
        return false;
    }
}

SkywellAddress STValidation::getSignerPublic () const
{
    SkywellAddress a;
    a.setNodePublic (getFieldVL (sfSigningPubKey));
    return a;
}

bool STValidation::isFull () const
{
    return (getFlags () & kFullFlag) != 0;
}

Blob STValidation::getSignature () const
{
    return getFieldVL (sfSignature);
}

Blob STValidation::getSigned () const
{
    Serializer s;
    add (s);
    return s.peekData ();
}

SOTemplate const& STValidation::getFormat ()
{
    struct FormatHolder
    {
        SOTemplate format;

        FormatHolder ()
        {
            format.push_back (SOElement (sfFlags,           SOE_REQUIRED));
            format.push_back (SOElement (sfLedgerHash,      SOE_REQUIRED));
            format.push_back (SOElement (sfLedgerSequence,  SOE_OPTIONAL));
            format.push_back (SOElement (sfCloseTime,       SOE_OPTIONAL));
            format.push_back (SOElement (sfLoadFee,         SOE_OPTIONAL));
            format.push_back (SOElement (sfAmendments,      SOE_OPTIONAL));
            format.push_back (SOElement (sfBaseFee,         SOE_OPTIONAL));
            format.push_back (SOElement (sfReserveBase,     SOE_OPTIONAL));
            format.push_back (SOElement (sfReserveIncrement, SOE_OPTIONAL));
            format.push_back (SOElement (sfFeeAccountID, SOE_OPTIONAL));
            format.push_back (SOElement (sfIssuerAccountID, SOE_OPTIONAL));
            format.push_back (SOElement (sfBlackListAccountID, SOE_OPTIONAL));
            format.push_back (SOElement (sfSigningTime,     SOE_REQUIRED));
            format.push_back (SOElement (sfSigningPubKey,   SOE_REQUIRED));
            format.push_back (SOElement (sfSignature,       SOE_OPTIONAL));
        }
    };

    static FormatHolder holder;

    return holder.format;
}

} // skywell
