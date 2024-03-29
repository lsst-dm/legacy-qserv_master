/* 
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
 * 
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the LSST License Statement and 
 * the GNU General Public License along with this program.  If not, 
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */
 
#ifndef LSST_QSERV_MASTER_CHUNKMAPPING_H
#define LSST_QSERV_MASTER_CHUNKMAPPING_H

// C++ 
#include <map>
#include <string>
#include <sstream>
// Boost
#include "boost/shared_ptr.hpp"

// Package
#include "lsst/qserv/master/common.h"

namespace lsst {
namespace qserv {
namespace master {

// class ChunkMapping is a helper class that generates mappings from
// placeholder table references to physical table names. 
// 
class ChunkMapping {
public:
    typedef StringMap Map;
    typedef Map::value_type MapValue;
    
    ChunkMapping() :_subPrefix("_sc") {}
    
    // Get a StringMapping for use with the Substitution class
    Map getMapping(int chunk, int subChunk);

    // Get a reference to this instance's Mapping, which is overwritten
    // each time this method is called.
    std::map<std::string,std::string> const& getMapReference(int chunk, int subChunk);

    // ChunkKeys: tables partitioned into chunks (not subc)
    // SubChunkKeys: tables partitioned into chunks and subchunks.
    void addChunkKey(std::string const& key) { _map[key] = CHUNK; }
    void addSubChunkKey(std::string const& key) { _map[key] = CHUNK_WITH_SUB; }
private:
    enum Mode {UNKNOWN, CHUNK, CHUNK_WITH_SUB};
    typedef std::map<std::string, Mode> ModeMap;
    typedef ModeMap::value_type ModeMapValue;

    template <typename T>
    std::string _toString(T const& t) {
	std::ostringstream oss;
	oss << t;
	return oss.str();
    }

    ModeMap _map;
    Map _instanceMap;
    std::string _subPrefix;
};

}}} // namespace lsst::qserv::master

#endif // LSST_QSERV_MASTER_CHUNKMAPPING_H
