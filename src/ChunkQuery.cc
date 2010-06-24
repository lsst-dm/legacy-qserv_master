
// Standard
#include <iostream>
#include <fcntl.h> // for O_RDONLY, O_WRONLY, etc.

// Boost
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

// Package
#include "lsst/qserv/master/ChunkQuery.h"
#include "lsst/qserv/master/xrootd.h"
#include "lsst/qserv/master/AsyncQueryManager.h"

// Namespace modifiers
using boost::make_shared;
namespace qMaster = lsst::qserv::master;


//////////////////////////////////////////////////////////////////////
// class ChunkQuery 
//////////////////////////////////////////////////////////////////////
void qMaster::ChunkQuery::Complete(int Result) {
    bool isReallyComplete = false;
    if(_shouldSquash) {
        
        _squashAtCallback(Result);
        return; // Anything else to do?
    }
    switch(_state) {
    case WRITE_OPEN: // Opened, so we can send off the query
	{
	    boost::lock_guard<boost::mutex> lock(_mutex);
	    _result.open = Result;
	}
	if(Result < 0) { // error? 
	    _result.open = Result;
	    isReallyComplete = true;
	    _state = COMPLETE;
	} else {
	    _state = WRITE_WRITE;
	    _sendQuery(Result);	   
	}
	break;
    case READ_OPEN: // Opened, so we can read-back the results.
	if(Result < 0) { // error? 
	    _result.read = Result;
	    std::cout << "Problem reading result: open returned " 
		      << _result.read << std::endl;
	    isReallyComplete = true;
	    _state = COMPLETE;
	} else {
	    _state = READ_READ;
	    _readResults(Result);
	}
	break;
    default:
	isReallyComplete = true;
	_state = CORRUPT;
    }

    if(isReallyComplete) { _notifyManager(); }
}

qMaster::ChunkQuery::ChunkQuery(qMaster::TransactionSpec const& t, int id, 
				qMaster::AsyncQueryManager* mgr) 
    : _spec(t), _manager(mgr), _id(id), _shouldSquash(false), 
      XrdPosixCallBack() {
    assert(_manager != NULL);
    _result.open = 0;
    _result.queryWrite = 0;
    _result.read = 0;
    _result.localWrite = 0;
    // Patch the spec to include the magic query terminator.
    _spec.query.append(4,0); // four null bytes.

}

void qMaster::ChunkQuery::run() {
    // This lock ensures that the remaining ChunkQuery::Complete() calls
    // do not proceed until this initial step completes.
    boost::lock_guard<boost::mutex> lock(_mutex);

    _state = WRITE_OPEN;
    std::cout << "Opening " << _spec.path << "\n";
    int result = qMaster::xrdOpenAsync(_spec.path.c_str(), O_WRONLY, this);
    if(result != -EINPROGRESS) {
	// don't continue, set result with the error.
	std::cout << "Not EINPROGRESS, should not continue with " 
		  << _spec.path << "\n";
	_result.open = result;
	_state = COMPLETE;
	_notifyManager(); // manager should delete me.
    } else {
	std::cout << "Waiting for " << _spec.path << "\n";
	_hash = qMaster::hashQuery(_spec.query.c_str(), 
				   _spec.query.size());
	
    }
    // Callback(Complete) will handle the rest.
}

std::string qMaster::ChunkQuery::getDesc() const {
    std::stringstream ss;
    ss << "Query " << _id << " (" << _hash << ") " << _resultUrl
       << " " << _queryHostPort << " state=";
    switch(_state) {
    case WRITE_OPEN:
	ss << "openingWrite";
	break;
    case WRITE_WRITE:
	ss << "writing";
	break;
    case READ_OPEN:
	ss << "openingRead";
	break;
    case READ_READ:
	ss << "reading";
	break;
    case COMPLETE:
	ss << "complete";
	break;
    case CORRUPT:
	ss << "corrupted";
	break;
    case ABORTED:
	ss << "aborted/squashed";
	break;

    default:
	break;
    }
    return ss.str();
}

void qMaster::ChunkQuery::_squashAtCallback(int result) {
    // squash this query so that it stops running.
    bool badState = false;
    switch(_state) {
    case WRITE_OPEN:
        // Just close the channel w/o sending a query.
	qMaster::xrdClose(result);
        
	break;
    case WRITE_WRITE:
	// Shouldn't get called in this state.
        badState = true;
	break;
    case READ_OPEN:
        // Close the channel w/o reading the result (which might be faulty)
	qMaster::xrdClose(result);
        
	break;
    case READ_READ:
	// Shouldn't get called in this state.
        badState = true;
	break;
    case COMPLETE:
        // Shouldn't get called here, but doesn't matter.
        badState = true;
	break;
    case CORRUPT:
        // Shouldn't get called here either.
        badState = true;
	break;
    default:
        // Unknown state.
        badState = true;
	break;
    }
    _state = ABORTED;
    _notifyManager();
    if(badState) {
        std::cout << "Unexpected state at squashing. Expecting READ_OPEN "
                  << "or WRITE_OPEN, got:" << getDesc() << std::endl;
    }
}
    

void qMaster::ChunkQuery::_sendQuery(int fd) {
    bool isReallyComplete = false;
    // Now write
    int len = _spec.query.length();
    int writeCount = qMaster::xrdWrite(fd, _spec.query.c_str(), len);
    if(writeCount != len) {
	_result.queryWrite = -errno;
	isReallyComplete = true;
	// To be safe, close file anyway.
        std::cout << (std::string() + "Error-caused closing of " 
                      + boost::lexical_cast<std::string>(fd)  + " dumpPath "
                      + _spec.savePath + "\n");
	qMaster::xrdClose(fd);
    } else {
	_result.queryWrite = writeCount;
	_queryHostPort = qMaster::xrdGetEndpoint(fd);
	_resultUrl = qMaster::makeUrl(_queryHostPort.c_str(), "result", 
				      _hash);
        std::cout << (std::string() + "Normal closing of " 
                      + boost::lexical_cast<std::string>(fd)  + " dumpPath "
                      + _spec.savePath + "\n");
	qMaster::xrdClose(fd); 
	_state = READ_OPEN;
	std::cout  << "opening async read to " << _resultUrl << "\n";
	int result = qMaster::xrdOpenAsync(_resultUrl.c_str(), 
					   O_RDONLY, this);
	if(result != -EINPROGRESS) {
	    _result.read = result;
	    isReallyComplete = true;
	}  // open for read in progress.
    } // Write ok
	    
    if(isReallyComplete) { 
	_state=COMPLETE;
	_notifyManager(); 
    }
}

void qMaster::ChunkQuery::_readResults(int fd) {
	int const fragmentSize = 4*1024*1024; // 4MB fragment size
	// Now read.
	qMaster::xrdReadToLocalFile(fd, fragmentSize, _spec.savePath.c_str(), 
			   &(_result.localWrite), &(_result.read));
	qMaster::xrdClose(fd);
	_state = COMPLETE;

	_notifyManager(); // This is a successful completion.
}
    
void qMaster::ChunkQuery::_notifyManager() {
    _manager->finalizeQuery(_id, _result, (_state == ABORTED));
}
