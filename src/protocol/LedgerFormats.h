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

#ifndef SKYWELL_PROTOCOL_LEDGERFORMATS_H_INCLUDED
#define SKYWELL_PROTOCOL_LEDGERFORMATS_H_INCLUDED

#include <protocol/KnownFormats.h>

namespace skywell {

/** Ledger entry types.

    These are stored in serialized data.

    @note Changing these values results in a hard fork.

    @ingroup protocol
*/
// Used as the type of a transaction or the type of a ledger entry.
enum LedgerEntryType
{
    ltINVALID           = -1,

    ltACCOUNT_ROOT      = 'a',

    /** Directory node.

        A directory is a vector 256-bit values. Usually they represent
        hashes of other objects in the ledger.

        Used in an append-only fashion.

        (There's a little more information than this, see the template)
    */
    ltDIR_NODE          = 'd',

    /** Describes a trust line.
    */
    ltSKYWELL_STATE      = 'r',

    ltTrust_STATE      = 't',

    ltTICKET            = 'T',
    
    ltMANAGE_FEE      = 'm',

    ltMANAGE_ISSUER      = 'i',
    
    ltBLACKLIST      = 'l',
    
    /* Deprecated. */
    ltOFFER             = 'o',

    ltLEDGER_HASHES     = 'h',

    ltAMENDMENTS        = 'f',

    ltFEE_SETTINGS      = 's',

    // No longer used or supported. Left here to prevent accidental
    // reassignment of the ledger type.
    ltNICKNAME          = 'n',

    ltNotUsed01         = 'c',

	ltSIGNER_LIST		= 'S',
};

/**
    @ingroup protocol
*/
// Used as a prefix for computing ledger indexes (keys).
enum LedgerNameSpace
{
    spaceAccount        = 'a',
    spaceDirNode        = 'd',
	spaceGenerator = 'g',
    spaceSkywell         = 'r',
    spaceTrust         = 't',
    spaceOffer          = 'o',  // Entry for an offer.
    spaceOwnerDir       = 'O',  // Directory of things owned by an account.
    spaceBookDir        = 'B',  // Directory of order books.
    spaceContract       = 'c',
    spaceSkipList       = 's',
    spaceAmendment      = 'f',
    spaceFee            = 'e',
    spaceIssuer            = 'i',
    spaceTicket         = 'T',
    spaceManageFee            = 'm',
    spaceBlackList            = 'l',
	spaceSign			= 'S',
    // No longer used or supported. Left here to reserve the space and
    // avoid accidental reuse of the space.
    spaceNickname       = 'n',
};

/**
    @ingroup protocol
*/
enum LedgerSpecificFlags
{
    // ltACCOUNT_ROOT
    lsfPasswordSpent    = 0x00010000,   // True, if password set fee is spent.
    lsfRequireDestTag   = 0x00020000,   // True, to require a DestinationTag for payments.
    lsfRequireAuth      = 0x00040000,   // True, to require a authorization to hold IOUs.
    lsfDisallowSWT      = 0x00080000,   // True, to disallow sending SWT.
    lsfDisableMaster    = 0x00100000,   // True, force regular key
    lsfNoFreeze         = 0x00200000,   // True, cannot freeze skywell states
    lsfGlobalFreeze     = 0x00400000,   // True, all assets frozen
    lsfDefaultSkywell    = 0x00800000,   // True, trust lines allow rippling by default

    // ltOFFER
    lsfPassive          = 0x00010000,
    lsfSell             = 0x00020000,   // True, offer was placed as a sell.

    // ltSKYWELL_STATE
    lsfLowReserve       = 0x00010000,   // True, if entry counts toward reserve.
    lsfHighReserve      = 0x00020000,
    lsfLowAuth          = 0x00040000,
    lsfHighAuth         = 0x00080000,
    lsfLowNoSkywell      = 0x00100000,
    lsfHighNoSkywell     = 0x00200000,
    lsfLowFreeze        = 0x00400000,   // True, low side has set freeze flag
    lsfHighFreeze       = 0x00800000,   // True, high side has set freeze flag
    lsfTie       = 0x01000000,
};

//------------------------------------------------------------------------------

/** Holds the list of known ledger entry formats.
*/
class LedgerFormats : public KnownFormats <LedgerEntryType>
{
private:
    LedgerFormats ();

public:
    static LedgerFormats const& getInstance ();

private:
    void addCommonFields (Item& item);
};

} // skywell

#endif
