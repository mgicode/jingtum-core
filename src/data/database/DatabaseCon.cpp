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
#include <data/database/DatabaseCon.h>
#include <data/database/SociDB.h>
#include <common/base/Log.h>
#include <common/misc/Utility.h>

namespace skywell {

DatabaseCon::DatabaseCon (
    Setup const& setup,
    std::string const& strName,
    const char* initStrings[],
    int initCount)
{
    auto const useTempFiles  // Use temporary files or regular DB files?
        = setup.standAlone &&
          setup.startUp != Config::LOAD &&
          setup.startUp != Config::LOAD_FILE &&
          setup.startUp != Config::REPLAY;

    boost::filesystem::path pPath = useTempFiles ? "" : (setup.dataDir / strName);

	//std::string dbpath = "host=localhost user=jingtum pass=good db=jingtum";
	std::string dbPath;
	if (strName.compare("transaction") == 0)
		dbPath = setup.mysqlStrings[0];
	else if (strName.compare("ledger") == 0)
		dbPath = setup.mysqlStrings[1];
	else if (strName.compare("wallet") == 0)
		dbPath = setup.mysqlStrings[2];

	open(session_, "mysql", dbPath);

    for (int i = 0; i < initCount; ++i)
    {
        try
        {
            session_ << initStrings[i];
        }
        catch (soci::soci_error& err)
        {
            // ignore errors
			std::string errstring = err.what();
        }
    }
}

DatabaseCon::Setup setup_DatabaseCon (Config const& c)
{
    DatabaseCon::Setup setup;

    setup.startUp = c.START_UP;
    setup.standAlone = c.RUN_STANDALONE;
    setup.dataDir = c.legacy ("database_path");
	//add by frank for mysql config
	try
	{
		int idx = 0;
		for (auto const& val : c.MYSQL_CONFIG)
		{
			if (val.find_first_not_of(" \t\n\v\f\r") != std::string::npos)
			{
				setup.mysqlStrings[idx++] = val;

				if (idx >= 3) break;
			}
		}
	}
	catch (...)
	{
		WriteLog(lsERROR, DatabaseCon) << "Mysql Config Error";

		assert(false);
	}

	return setup;
}

void DatabaseCon::setupCheckpointing (JobQueue* q)
{
    if (! q)
        throw std::logic_error ("No JobQueue");

    checkpointer_ = makeCheckpointer (session_, *q);
}

} // skywell
