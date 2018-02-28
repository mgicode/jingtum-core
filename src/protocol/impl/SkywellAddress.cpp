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
#include <common/base/Log.h>
#include <common/base/StringUtilities.h>
#include <crypto/ECDSA.h>
#include <crypto/ECIES.h>
#include <crypto/GenerateDeterministicKey.h>
#include <crypto/RandomNumbers.h>
#include <crypto/RFC1751.h>
#include <protocol/JsonFields.h>
#include <protocol/SkywellAddress.h>
#include <protocol/Serializer.h>
#include <protocol/SkywellPublicKey.h>
#include <crypto/ed25519-donna/ed25519.h>
#include <openssl/ripemd.h>
#include <openssl/pem.h>
#include <algorithm>
#include <mutex>

namespace skywell {

static
bool isCanonicalEd25519Signature (std::uint8_t const* signature)
{
    using std::uint8_t;

    // Big-endian `l`, the Ed25519 subgroup order
    char const* const order = "\x10\x00\x00\x00\x00\x00\x00\x00"
                              "\x00\x00\x00\x00\x00\x00\x00\x00"
                              "\x14\xDE\xF9\xDE\xA2\xF7\x9C\xD6"
                              "\x58\x12\x63\x1A\x5C\xF5\xD3\xED";

    uint8_t const* const l = reinterpret_cast<uint8_t const*> (order);

    // Take the second half of signature and byte-reverse it to big-endian.
    uint8_t const* S_le = signature + 32;
    uint8_t S[32];
    std::reverse_copy (S_le, S_le + 32, S);

    return std::lexicographical_compare (S, S + 32, l, l + 32);
}

// <-- seed
static
uint128 PassPhraseToKey (std::string const& passPhrase)
{
    Serializer s;

    s.addRaw (passPhrase);
    // NIKB TODO this calling sequence is a bit ugly; this should be improved.
    uint256 hash256 = s.getSHA512Half ();
    uint128 ret (uint128::fromVoid (hash256.data()));

    s.secureErase ();

    return ret;
}

static
bool verifySignature (Blob const& pubkey, uint256 const& hash, Blob const& sig,
                      ECDSA fullyCanonical)
{
    if (! isCanonicalECDSASig (sig, fullyCanonical))
    {
        return false;
    }

    return ECDSAVerify (hash, sig, &pubkey[0], pubkey.size());
}

SkywellAddress::SkywellAddress ()
    : mIsValid (false)
{
    nVersion = VER_NONE;
}

void SkywellAddress::clear ()
{
    nVersion = VER_NONE;
    vchData.clear ();
    mIsValid = false;
}

bool SkywellAddress::isSet () const
{
    return nVersion != VER_NONE;
}

//
// NodePublic
//

static
uint160 Hash160 (Blob const& vch)
{
    uint256 hash1;
    SHA256 (vch.data (), vch.size (), hash1.data ());

    uint160 hash2;
    RIPEMD160 (hash1.data (), hash1.size (), hash2.data ());

    return hash2;
}

SkywellAddress SkywellAddress::createNodePublic (SkywellAddress const& naSeed)
{
    SkywellAddress   naNew;

    // YYY Should there be a GetPubKey() equiv that returns a uint256?
    naNew.setNodePublic (generateRootDeterministicPublicKey (naSeed.getSeed()));

    return naNew;
}

SkywellAddress SkywellAddress::createNodePublic (Blob const& vPublic)
{
    SkywellAddress   naNew;

    naNew.setNodePublic (vPublic);

    return naNew;
}

SkywellAddress SkywellAddress::createNodePublic (std::string const& strPublic)
{
    SkywellAddress   naNew;

    naNew.setNodePublic (strPublic);

    return naNew;
}

SkywellPublicKey
SkywellAddress::toPublicKey() const
{
    assert (nVersion == VER_NODE_PUBLIC);
    return SkywellPublicKey (vchData.begin(), vchData.end());
}

static
std::runtime_error badSourceError (int nVersion)
{
    return std::runtime_error ("bad source: " + std::to_string (nVersion));
}

NodeID SkywellAddress::getNodeID () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getNodeID");

    case VER_NODE_PUBLIC: {
        // Note, we are encoding the left.
        NodeID node;
        node.copyFrom(Hash160 (vchData));
        return node;
    }

    default:
        throw badSourceError (nVersion);
    }
}

Blob const& SkywellAddress::getNodePublic () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getNodePublic");

    case VER_NODE_PUBLIC:
        return vchData;

    default:
        throw badSourceError (nVersion);
    }
}

std::string SkywellAddress::humanNodePublic () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanNodePublic");

    case VER_NODE_PUBLIC:
        return ToString ();

    default:
        throw badSourceError (nVersion);
    }
}

bool SkywellAddress::setNodePublic (std::string const& strPublic)
{
    mIsValid = SetString (
        strPublic, VER_NODE_PUBLIC, Base58::getSkywellAlphabet ());

    return mIsValid;
}

void SkywellAddress::setNodePublic (Blob const& vPublic)
{
    mIsValid        = true;

    SetData (VER_NODE_PUBLIC, vPublic);
}

bool SkywellAddress::verifyNodePublic (
    uint256 const& hash, Blob const& vchSig, ECDSA fullyCanonical) const
{
    return verifySignature (getNodePublic(), hash, vchSig, fullyCanonical);
}

bool SkywellAddress::verifyNodePublic (
    uint256 const& hash, std::string const& strSig, ECDSA fullyCanonical) const
{
    Blob vchSig (strSig.begin (), strSig.end ());

    return verifyNodePublic (hash, vchSig, fullyCanonical);
}

//
// NodePrivate
//

SkywellAddress SkywellAddress::createNodePrivate (SkywellAddress const& naSeed)
{
    SkywellAddress   naNew;

    naNew.setNodePrivate (generateRootDeterministicPrivateKey (naSeed.getSeed()));

    return naNew;
}

Blob const& SkywellAddress::getNodePrivateData () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getNodePrivateData");

    case VER_NODE_PRIVATE:
        return vchData;

    default:
        throw badSourceError (nVersion);
    }
}

uint256 SkywellAddress::getNodePrivate () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source = getNodePrivate");

    case VER_NODE_PRIVATE:
        return uint256 (vchData);

    default:
        throw badSourceError (nVersion);
    }
}

std::string SkywellAddress::humanNodePrivate () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanNodePrivate");

    case VER_NODE_PRIVATE:
        return ToString ();

    default:
        throw badSourceError (nVersion);
    }
}

bool SkywellAddress::setNodePrivate (std::string const& strPrivate)
{
    mIsValid = SetString (
        strPrivate, VER_NODE_PRIVATE, Base58::getSkywellAlphabet ());

    return mIsValid;
}

void SkywellAddress::setNodePrivate (Blob const& vPrivate)
{
    mIsValid = true;

    SetData (VER_NODE_PRIVATE, vPrivate);
}

void SkywellAddress::setNodePrivate (uint256 hash256)
{
    mIsValid = true;

    SetData (VER_NODE_PRIVATE, hash256);
}

void SkywellAddress::signNodePrivate (uint256 const& hash, Blob& vchSig) const
{
    vchSig = ECDSASign (hash, getNodePrivate());

    if (vchSig.empty())
        throw std::runtime_error ("Signing failed.");
}

//
// AccountID
//

Account SkywellAddress::getAccountID () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getAccountID");

    case VER_ACCOUNT_ID:
        return Account(vchData);

    case VER_ACCOUNT_PUBLIC: {
        // Note, we are encoding the left.
        // TODO(tom): decipher this comment.
        Account account;
        account.copyFrom (Hash160 (vchData));
        return account;
    }

    default:
        throw badSourceError (nVersion);
    }
}

typedef std::mutex StaticLockType;
typedef std::lock_guard <StaticLockType> StaticScopedLockType;

static StaticLockType s_lock;
static hash_map <Blob, std::string> rncMapOld, rncMapNew;

void SkywellAddress::clearCache ()
{
    StaticScopedLockType sl (s_lock);

    rncMapOld.clear ();
    rncMapNew.clear ();
}

std::string SkywellAddress::humanAccountID () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanAccountID");

    case VER_ACCOUNT_ID:
    {
        std::string ret;

        {
            StaticScopedLockType sl (s_lock);

            auto it = rncMapNew.find (vchData);

            if (it != rncMapNew.end ())
            {
                // Found in new map, nothing to do
                ret = it->second;
            }
            else
            {
                it = rncMapOld.find (vchData);

                if (it != rncMapOld.end ())
                {
                    ret = it->second;
                    rncMapOld.erase (it);
                }
                else
                    ret = ToString ();

                if (rncMapNew.size () >= 128000)
                {
                    rncMapOld = std::move (rncMapNew);
                    rncMapNew.clear ();
                    rncMapNew.reserve (128000);
                }

                rncMapNew[vchData] = ret;
            }
        }

        return ret;
    }

    case VER_ACCOUNT_PUBLIC:
    {
        SkywellAddress   accountID;

        (void) accountID.setAccountID (getAccountID ());

        return accountID.ToString ();
    }

    default:
        throw badSourceError (nVersion);
    }
}

bool SkywellAddress::setAccountID (
    std::string const& strAccountID, Base58::Alphabet const& alphabet)
{
    if (strAccountID.empty ())
    {
        setAccountID (Account ());

        mIsValid    = true;
    }
    else
    {
        mIsValid = SetString (strAccountID, VER_ACCOUNT_ID, alphabet);
    }

    return mIsValid;
}

void SkywellAddress::setAccountID (Account const& hash160)
{
    mIsValid        = true;
    SetData (VER_ACCOUNT_ID, hash160);
}

//
// AccountPublic
//

SkywellAddress SkywellAddress::createAccountPublic (
    SkywellAddress const& generator, int iSeq)
{
    SkywellAddress   naNew;

    naNew.setAccountPublic (generator, iSeq);

    return naNew;
}

Blob const& SkywellAddress::getAccountPublic () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getAccountPublic");

    case VER_ACCOUNT_ID:
        throw std::runtime_error ("public not available from account id");
        break;

    case VER_ACCOUNT_PUBLIC:
        return vchData;

    default:
        throw badSourceError (nVersion);
    }
}

std::string SkywellAddress::humanAccountPublic () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanAccountPublic");

    case VER_ACCOUNT_ID:
        throw std::runtime_error ("public not available from account id");

    case VER_ACCOUNT_PUBLIC:
        return ToString ();

    default:
        throw badSourceError (nVersion);
    }
}

bool SkywellAddress::setAccountPublic (std::string const& strPublic)
{
    mIsValid = SetString (
        strPublic, VER_ACCOUNT_PUBLIC, Base58::getSkywellAlphabet ());

    return mIsValid;
}

void SkywellAddress::setAccountPublic (Blob const& vPublic)
{
    mIsValid = true;

    SetData (VER_ACCOUNT_PUBLIC, vPublic);
}

void SkywellAddress::setAccountPublic (SkywellAddress const& generator, int seq)
{
    setAccountPublic (generatePublicDeterministicKey (
        generator.getGenerator(), seq));
}

bool SkywellAddress::accountPublicVerify (
    Blob const& message, Blob const& vucSig, ECDSA fullyCanonical) const
{
    if (vchData.size() == 33  &&  vchData[0] == 0xED)
    {
        if (vucSig.size() != 64)
        {
            return false;
        }

        uint8_t const* publicKey = &vchData[1];
        uint8_t const* signature = &vucSig[0];

        return !ed25519_sign_open (message.data(), message.size(),
                                   publicKey, signature)
                && isCanonicalEd25519Signature (signature);
    }

    uint256 const uHash = getSHA512Half (message);
    return verifySignature (getAccountPublic(), uHash, vucSig, fullyCanonical);
}

SkywellAddress SkywellAddress::createAccountID (Account const& account)
{
    SkywellAddress   na;
    na.setAccountID (account);

    return na;
}

//
// AccountPrivate
//

SkywellAddress SkywellAddress::createAccountPrivate (
    SkywellAddress const& generator, SkywellAddress const& naSeed, int iSeq)
{
    SkywellAddress   naNew;

    naNew.setAccountPrivate (generator, naSeed, iSeq);

    return naNew;
}

uint256 SkywellAddress::getAccountPrivate () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getAccountPrivate");

    case VER_ACCOUNT_PRIVATE:
        return uint256::fromVoid (vchData.data() + (vchData.size() - 32));

    default:
        throw badSourceError (nVersion);
    }
}

bool SkywellAddress::setAccountPrivate (std::string const& strPrivate)
{
    mIsValid = SetString (
        strPrivate, VER_ACCOUNT_PRIVATE, Base58::getSkywellAlphabet ());

    return mIsValid;
}

void SkywellAddress::setAccountPrivate (Blob const& vPrivate)
{
    mIsValid = true;
    SetData (VER_ACCOUNT_PRIVATE, vPrivate);
}

void SkywellAddress::setAccountPrivate (uint256 hash256)
{
    mIsValid = true;
    SetData (VER_ACCOUNT_PRIVATE, hash256);
}

void SkywellAddress::setAccountPrivate (
    SkywellAddress const& generator, SkywellAddress const& naSeed, int seq)
{
    uint256 secretKey = generatePrivateDeterministicKey (
        generator.getGenerator(), naSeed.getSeed(), seq);

    setAccountPrivate (secretKey);
}

Blob SkywellAddress::accountPrivateSign (Blob const& message) const
{
    if (vchData.size() == 33  &&  vchData[0] == 0xED)
    {
        uint8_t const*      secretKey = &vchData[1];
        ed25519_public_key  publicKey;
        Blob                signature (sizeof (ed25519_signature));

        ed25519_publickey (secretKey, publicKey);

        ed25519_sign (
            message.data(), message.size(), secretKey, publicKey,
            &signature[0]);

        assert (isCanonicalEd25519Signature (signature.data()));

        return signature;
    }

    uint256 const uHash = getSHA512Half (message);

    Blob result = ECDSASign (uHash, getAccountPrivate());
    bool const ok = !result.empty();

    CondLog (!ok, lsWARNING, SkywellAddress)
            << "accountPrivateSign: Signing failed.";

    return result;
}

Blob SkywellAddress::accountPrivateEncrypt (
    SkywellAddress const& naPublicTo, Blob const& vucPlainText) const
{
    uint256 secretKey = getAccountPrivate();
    Blob    publicKey = naPublicTo.getAccountPublic();

    Blob vucCipherText;

    {
        try
        {
            vucCipherText = encryptECIES (secretKey, publicKey, vucPlainText);
        }
        catch (...)
        {
            // TODO: log this or explain why this is unimportant!
        }
    }

    return vucCipherText;
}

Blob SkywellAddress::accountPrivateDecrypt (
    SkywellAddress const& naPublicFrom, Blob const& vucCipherText) const
{
    uint256 secretKey = getAccountPrivate();
    Blob    publicKey = naPublicFrom.getAccountPublic();

    Blob    vucPlainText;

    {
        try
        {
            vucPlainText = decryptECIES (secretKey, publicKey, vucCipherText);
        }
        catch (...)
        {
            // TODO: log this or explain why this is unimportant!
        }
    }

    return vucPlainText;
}

//
// Generators
//

Blob const& SkywellAddress::getGenerator () const
{
    // returns the public generator
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getGenerator");

    case VER_FAMILY_GENERATOR:
        // Do nothing.
        return vchData;

    default:
        throw badSourceError (nVersion);
    }
}

std::string SkywellAddress::humanGenerator () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanGenerator");

    case VER_FAMILY_GENERATOR:
        return ToString ();

    default:
        throw badSourceError (nVersion);
    }
}

void SkywellAddress::setGenerator (Blob const& vPublic)
{
    mIsValid        = true;
    SetData (VER_FAMILY_GENERATOR, vPublic);
}

SkywellAddress SkywellAddress::createGeneratorPublic (SkywellAddress const& naSeed)
{
    SkywellAddress   naNew;
    naNew.setGenerator (generateRootDeterministicPublicKey (naSeed.getSeed()));
    return naNew;
}

//
// Seed
//

uint128 SkywellAddress::getSeed () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getSeed");

    case VER_FAMILY_SEED:
        return uint128 (vchData);

    default:
        throw badSourceError (nVersion);
    }
}

std::string SkywellAddress::humanSeed1751 () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanSeed1751");

    case VER_FAMILY_SEED:
    {
        std::string strHuman;
        std::string strLittle;
        std::string strBig;
        uint128 uSeed   = getSeed ();

        strLittle.assign (uSeed.begin (), uSeed.end ());

        strBig.assign (strLittle.rbegin (), strLittle.rend ());

        RFC1751::getEnglishFromKey (strHuman, strBig);

        return strHuman;
    }

    default:
        throw badSourceError (nVersion);
    }
}

std::string SkywellAddress::humanSeed () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanSeed");

    case VER_FAMILY_SEED:
        return ToString ();

    default:
        throw badSourceError (nVersion);
    }
}

int SkywellAddress::setSeed1751 (std::string const& strHuman1751)
{
    std::string strKey;
    int         iResult = RFC1751::getKeyFromEnglish (strKey, strHuman1751);

    if (1 == iResult)
    {
        Blob    vchLittle (strKey.rbegin (), strKey.rend ());
        uint128     uSeed (vchLittle);

        setSeed (uSeed);
    }

    return iResult;
}

bool SkywellAddress::setSeed (std::string const& strSeed)
{
    mIsValid = SetString (strSeed, VER_FAMILY_SEED, Base58::getSkywellAlphabet ());

    return mIsValid;
}

bool SkywellAddress::setSeedGeneric (std::string const& strText)
{
    SkywellAddress   naTemp;
    bool            bResult = true;
    uint128         uSeed;

    if (strText.empty ()
            || naTemp.setAccountID (strText)
            || naTemp.setAccountPublic (strText)
            || naTemp.setAccountPrivate (strText)
            || naTemp.setNodePublic (strText)
            || naTemp.setNodePrivate (strText))
    {
        bResult = false;
    }
    else if (strText.length () == 32 && uSeed.SetHex (strText, true))
    {
        setSeed (uSeed);
    }
    else if (setSeed (strText))
    {
        // Log::out() << "Recognized seed.";
    }
    else if (1 == setSeed1751 (strText))
    {
        // Log::out() << "Recognized 1751 seed.";
    }
    else
    {
        setSeed (PassPhraseToKey (strText));
    }

    return bResult;
}

void SkywellAddress::setSeed (uint128 hash128)
{
    mIsValid = true;

    SetData (VER_FAMILY_SEED, hash128);
}

void SkywellAddress::setSeedRandom ()
{
    // XXX Maybe we should call MakeNewKey
    uint128 key;

    random_fill (key.begin (), key.size ());

    SkywellAddress::setSeed (key);
}

SkywellAddress SkywellAddress::createSeedRandom ()
{
    SkywellAddress   naNew;

    naNew.setSeedRandom ();

    return naNew;
}

SkywellAddress SkywellAddress::createSeedGeneric (std::string const& strText)
{
    SkywellAddress   naNew;

    naNew.setSeedGeneric (strText);

    return naNew;
}

uint256 keyFromSeed (uint128 const& seed)
{
    Serializer s;

    s.add128 (seed);
    uint256 result = s.getSHA512Half();

    s.secureErase ();

    return result;
}

SkywellAddress getSeedFromRPC (Json::Value const& params)
{
    // This function is only called when `key_type` is present.
    assert (params.isMember (jss::key_type));

    bool const has_passphrase = params.isMember (jss::passphrase);
    bool const has_seed       = params.isMember (jss::seed);
    bool const has_seed_hex   = params.isMember (jss::seed_hex);

    int const n_secrets = has_passphrase + has_seed + has_seed_hex;

    if (n_secrets > 1)
    {
        // `passphrase`, `seed`, and `seed_hex` are mutually exclusive.
        return SkywellAddress();
    }

    SkywellAddress result;

    if (has_seed)
    {
        std::string const seed = params[jss::seed].asString();

        result.setSeed (seed);
    }
    else if (has_seed_hex)
    {
        uint128 seed;
        std::string const seed_hex = params[jss::seed_hex].asString();

        if (seed_hex.size() != 32  ||  !seed.SetHex (seed_hex, true))
        {
            return SkywellAddress();
        }

        result.setSeed (seed);
    }
    else if (has_passphrase)
    {
        std::string const passphrase = params[jss::passphrase].asString();

        // Given `key_type`, `passphrase` is always the passphrase.
        uint128 const seed = PassPhraseToKey (passphrase);
        result.setSeed (seed);
    }

    return result;
}

KeyPair generateKeysFromSeed (KeyType type, SkywellAddress const& seed)
{
    KeyPair result;

    if (! seed.isSet())
    {
        return result;
    }

    if (type == KeyType::secp256k1)
    {
        SkywellAddress generator = SkywellAddress::createGeneratorPublic (seed);
        result.secretKey.setAccountPrivate (generator, seed, 0);
        result.publicKey.setAccountPublic (generator, 0);
    }
    else if (type == KeyType::ed25519)
    {
        uint256 secretkey = keyFromSeed (seed.getSeed());

        Blob ed25519_key (33);
        ed25519_key[0] = 0xED;

        assert (secretkey.size() + 1 == ed25519_key.size());
        memcpy (&ed25519_key[1], secretkey.data(), secretkey.size());
        result.secretKey.setAccountPrivate (ed25519_key);

        ed25519_publickey (secretkey.data(), &ed25519_key[1]);
        result.publicKey.setAccountPublic (ed25519_key);

        secretkey.zero();  // security erase
    }
    else
    {
        assert (false);  // not reached
    }

    return result;
}

} // skywell
