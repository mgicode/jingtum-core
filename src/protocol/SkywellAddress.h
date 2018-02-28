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

#ifndef SKYWELL_PROTOCOL_SKYWELLADDRESS_H_INCLUDED
#define SKYWELL_PROTOCOL_SKYWELLADDRESS_H_INCLUDED

#include <common/base/base_uint.h>
#include <common/json/json_value.h>
#include <crypto/Base58Data.h>
#include <crypto/ECDSACanonical.h>
#include <crypto/KeyType.h>
#include <protocol/SkywellPublicKey.h>
#include <protocol/UInt160.h>
#include <protocol/UintTypes.h>
#include <protocol/ErrorCodes.h>

namespace skywell {

//
// Used to hold addresses and parse and produce human formats.
//
// XXX This needs to be reworked to store data in uint160 and uint256.

class SkywellAddress : private CBase58Data
{
private:
    typedef enum
    {
        VER_NONE                = 1,
        VER_NODE_PUBLIC         = 28,
        VER_NODE_PRIVATE        = 32,
        VER_ACCOUNT_ID          = 0,
        VER_ACCOUNT_PUBLIC      = 35,
        VER_ACCOUNT_PRIVATE     = 34,
        VER_FAMILY_GENERATOR    = 41,
        VER_FAMILY_SEED         = 33,
    } VersionEncoding;

    bool    mIsValid;

public:
    SkywellAddress ();

    void const*
    data() const noexcept
    {
        return vchData.data();
    }

    std::size_t
    size() const noexcept
    {
        return vchData.size();
    }

    // For public and private key, checks if they are legal.
    bool isValid () const
    {
        return mIsValid;
    }

    void clear ();
    bool isSet () const;

    static void clearCache ();

    /** Returns the public key.
        Precondition: version == VER_NODE_PUBLIC
    */
    SkywellPublicKey
    toPublicKey() const;

    //
    // Node Public - Also used for Validators
    //
    NodeID getNodeID () const;
    Blob const& getNodePublic () const;

    std::string humanNodePublic () const;

    bool setNodePublic (std::string const& strPublic);
    void setNodePublic (Blob const& vPublic);
    bool verifyNodePublic (uint256 const& hash, Blob const& vchSig,
                           ECDSA mustBeFullyCanonical) const;
    bool verifyNodePublic (uint256 const& hash, std::string const& strSig,
                           ECDSA mustBeFullyCanonical) const;

    static SkywellAddress createNodePublic (SkywellAddress const& naSeed);
    static SkywellAddress createNodePublic (Blob const& vPublic);
    static SkywellAddress createNodePublic (std::string const& strPublic);

    //
    // Node Private
    //
    Blob const& getNodePrivateData () const;
    uint256 getNodePrivate () const;

    std::string humanNodePrivate () const;

    bool setNodePrivate (std::string const& strPrivate);
    void setNodePrivate (Blob const& vPrivate);
    void setNodePrivate (uint256 hash256);
    void signNodePrivate (uint256 const& hash, Blob& vchSig) const;

    static SkywellAddress createNodePrivate (SkywellAddress const& naSeed);

    //
    // Accounts IDs
    //
    Account getAccountID () const;

    std::string humanAccountID () const;

    bool setAccountID (
        std::string const& strAccountID,
        Base58::Alphabet const& alphabet = Base58::getSkywellAlphabet());
    void setAccountID (Account const& hash160In);

    static SkywellAddress createAccountID (Account const& uiAccountID);

    //
    // Accounts Public
    //
    Blob const& getAccountPublic () const;

    std::string humanAccountPublic () const;

    bool setAccountPublic (std::string const& strPublic);
    void setAccountPublic (Blob const& vPublic);
    void setAccountPublic (SkywellAddress const& generator, int seq);

    bool accountPublicVerify (Blob const& message, Blob const& vucSig,
                              ECDSA mustBeFullyCanonical) const;

    static SkywellAddress createAccountPublic (Blob const& vPublic)
    {
        SkywellAddress naNew;
        naNew.setAccountPublic (vPublic);
        return naNew;
    }

    static std::string createHumanAccountPublic (Blob const& vPublic)
    {
        return createAccountPublic (vPublic).humanAccountPublic ();
    }

    // Create a deterministic public key from a public generator.
    static SkywellAddress createAccountPublic (
        SkywellAddress const& naGenerator, int iSeq);

    //
    // Accounts Private
    //
    uint256 getAccountPrivate () const;

    bool setAccountPrivate (std::string const& strPrivate);
    void setAccountPrivate (Blob const& vPrivate);
    void setAccountPrivate (uint256 hash256);
    void setAccountPrivate (SkywellAddress const& naGenerator,
                            SkywellAddress const& naSeed, int seq);

    Blob accountPrivateSign (Blob const& message) const;

    // Encrypt a message.
    Blob accountPrivateEncrypt (
        SkywellAddress const& naPublicTo, Blob const& vucPlainText) const;

    // Decrypt a message.
    Blob accountPrivateDecrypt (
        SkywellAddress const& naPublicFrom, Blob const& vucCipherText) const;

    static SkywellAddress createAccountPrivate (
        SkywellAddress const& generator, SkywellAddress const& seed, int iSeq);

    static SkywellAddress createAccountPrivate (Blob const& vPrivate)
    {
        SkywellAddress   naNew;

        naNew.setAccountPrivate (vPrivate);

        return naNew;
    }

    //
    // Generators
    // Use to generate a master or regular family.
    //
    Blob const& getGenerator () const;

    std::string humanGenerator () const;

    bool setGenerator (std::string const& strGenerator);
    void setGenerator (Blob const& vPublic);
    // void setGenerator(SkywellAddress const& seed);

    // Create generator for making public deterministic keys.
    static SkywellAddress createGeneratorPublic (SkywellAddress const& naSeed);

    //
    // Seeds
    // Clients must disallow reconizable entries from being seeds.
    uint128 getSeed () const;

    std::string humanSeed () const;
    std::string humanSeed1751 () const;

    bool setSeed (std::string const& strSeed);
    int setSeed1751 (std::string const& strHuman1751);
    bool setSeedGeneric (std::string const& strText);
    void setSeed (uint128 hash128);
    void setSeedRandom ();

    static SkywellAddress createSeedRandom ();
    static SkywellAddress createSeedGeneric (std::string const& strText);

    std::string ToString () const
        {return static_cast<CBase58Data const&>(*this).ToString();}

    template <class Hasher>
    friend
    void
    hash_append(Hasher& hasher, SkywellAddress const& value)
    {
        using beast::hash_append;
        hash_append(hasher, static_cast<CBase58Data const&>(value));
    }

    friend
    bool
    operator==(SkywellAddress const& lhs, SkywellAddress const& rhs)
    {
        return static_cast<CBase58Data const&>(lhs) ==
               static_cast<CBase58Data const&>(rhs);
    }

    friend
    bool
    operator <(SkywellAddress const& lhs, SkywellAddress const& rhs)
    {
        return static_cast<CBase58Data const&>(lhs) <
               static_cast<CBase58Data const&>(rhs);
    }
};

//------------------------------------------------------------------------------

inline
bool
operator!=(SkywellAddress const& lhs, SkywellAddress const& rhs)
{
    return !(lhs == rhs);
}

inline
bool
operator >(SkywellAddress const& lhs, SkywellAddress const& rhs)
{
    return rhs < lhs;
}

inline
bool
operator<=(SkywellAddress const& lhs, SkywellAddress const& rhs)
{
    return !(rhs < lhs);
}

inline
bool
operator>=(SkywellAddress const& lhs, SkywellAddress const& rhs)
{
    return !(lhs < rhs);
}

struct KeyPair
{
    SkywellAddress secretKey;
    SkywellAddress publicKey;
};

uint256 keyFromSeed (uint128 const& seed);

SkywellAddress getSeedFromRPC (Json::Value const& params);

KeyPair generateKeysFromSeed (KeyType keyType, SkywellAddress const& seed);

} // skywell

#endif
