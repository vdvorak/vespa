// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "itablefactory.h"
#include "itablemanager.h"
#include <vespa/vespalib/util/sync.h>
#include <map>
#include <vector>

namespace search {
namespace fef {

/**
 * This class manages a set of tables and contains an ordered list of table factories used to create tables,
 * and a cache of allready created tables. A table is accessed by a unique name.
 **/
class TableManager : public ITableManager
{
private:
    TableManager(const TableManager &);
    TableManager &operator=(const TableManager &);

    typedef std::map<vespalib::string, Table::SP> TableCache;
    std::vector<ITableFactory::SP> _factories;
    mutable TableCache             _cache;
    vespalib::Lock                 _lock;

public:
    TableManager();
    ~TableManager();

    /**
     * Adds a table factory to this manager.
     * The table factories are used in the order they where added to create tables.
     **/
    void addFactory(ITableFactory::SP factory) { _factories.push_back(factory); }

    /**
     * Retrieves the table with the given name using the following strategy:
     * 1. Try to find the table in the cache.
     * 2. Iterate over the table factories and try to create the table.
     *    The first table that is successfully created is added it to the cache and returned.
     * 3. Return NULL.
     **/
    const Table * getTable(const vespalib::string & name) const override;
};

} // namespace fef
} // namespace search

