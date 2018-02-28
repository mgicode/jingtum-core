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
#include <boost/optional.hpp>
#include <iostream>

#include <main/Application.h>
#include <main/LocalCredentials.h>
#include <data/database/DatabaseCon.h>
#include <network/peers/UniqueNodeList.h>
#include <common/base/Log.h>
#include <common/base/StringUtilities.h>
#include <common/base/make_SSLContext.h>
#include <common/core/Config.h>

namespace skywell {

void LocalCredentials::start ()
{
    // We need our node identity before we begin networking.
    // - Allows others to identify if they have connected multiple times.
    // - Determines our CAS routing and responsibilities.
    // - This is not our validation identity.
    if (!nodeIdentityLoad ())
    {
        nodeIdentityCreate ();

        if (!nodeIdentityLoad ())
            throw std::runtime_error ("unable to retrieve new node identity.");
    }

    if (!getConfig ().QUIET)
        std::cerr << "NodeIdentity: " << mNodePublicKey.humanNodePublic () << std::endl;

    getApp().getUNL ().start ();
}

// Retrieve network identity.
bool LocalCredentials::nodeIdentityLoad ()
{
    auto db = getApp().getWalletDB ().checkoutDb ();
    bool bSuccess = false;

    boost::optional<std::string> pubKO, priKO;
    soci::statement st = (db->prepare <<
                          "SELECT PublicKey, PrivateKey "
                          "FROM NodeIdentity;",
                          soci::into(pubKO),
                          soci::into(priKO));
    st.execute ();

    while (st.fetch ())
    {
        mNodePublicKey.setNodePublic (pubKO.value_or(""));
        mNodePrivateKey.setNodePrivate (priKO.value_or(""));

        bSuccess = true;
    }

    if (getConfig ().NODE_PUB.isValid () && getConfig ().NODE_PRIV.isValid ())
    {
        mNodePublicKey  = getConfig ().NODE_PUB;
        mNodePrivateKey = getConfig ().NODE_PRIV;
    }

    return bSuccess;
}

// Create and store a network identity.
bool LocalCredentials::nodeIdentityCreate ()
{
    if (!getConfig ().QUIET)
        std::cerr << "NodeIdentity: Creating." << std::endl;

    //
    // Generate the public and private key
    //
    SkywellAddress   naSeed          = SkywellAddress::createSeedRandom ();
    SkywellAddress   naNodePublic    = SkywellAddress::createNodePublic (naSeed);
    SkywellAddress   naNodePrivate   = SkywellAddress::createNodePrivate (naSeed);

    // Make new key.
    std::string strDh512 (getRawDHParams (2048));

    std::string strDh1024 = strDh512;

    //
    // Store the node information
    //
    auto db = getApp().getWalletDB ().checkoutDb ();

    std::string qstr = str (boost::format ("INSERT INTO NodeIdentity (PublicKey,PrivateKey,Dh512,Dh1024) VALUES ('%s','%s',%s,%s);")
                         % naNodePublic.humanNodePublic ()
                         % naNodePrivate.humanNodePrivate ()
                         % sqlEscape (strDh512)
                         % sqlEscape (strDh1024));

    *db << qstr;

    if (!getConfig ().QUIET)
        std::cerr << "NodeIdentity: Created." << std::endl;

    return true;
}

} // skywell
