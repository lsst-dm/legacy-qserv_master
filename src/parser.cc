#include <sstream>
#include <iostream>
#include "SqlSQL2Parser.hpp"
#include "SqlSQL2Lexer.hpp"
#include "antlr/AST.hpp"
#include "antlr/CommonAST.hpp"
#include "boost/shared_ptr.hpp"
#include <list>
#include "parser.h"

namespace { // Anonymous

std::string tokenText(RefAST r) {
    if(r.get()) {
	return r->getText();
    } else return std::string();
}

class ColumnHandler : public VoidFourRefFunc {
public:    
    virtual ~ColumnHandler() {}
    virtual void operator()(RefAST a, RefAST b, RefAST c, RefAST d) {
	std::cout << "col _" << tokenText(a) 
		  << "_ _" << tokenText(b) 
		  << "_ _" << tokenText(c) 
		  << "_ _" << tokenText(d) 
		  << "_ "; 
	a->setText("AWESOMECOLUMN");
    }

};

class TableHandler : public VoidThreeRefFunc {
public: 
    virtual ~TableHandler() {}
    virtual void operator()(RefAST a, RefAST b, RefAST c)  {
	std::cout << "qualname " << tokenText(a) 
		  << " " << tokenText(b) << " " 
		  << tokenText(c) << " "; 
	a->setText("AwesomeTable");
    }
};

class Templater {
public:
    class ColumnHandler : public VoidFourRefFunc {
    public:    
	ColumnHandler(Templater& t) : _templater(t) {}
	virtual ~ColumnHandler() {}
	virtual void operator()(RefAST a, RefAST b, RefAST c, RefAST d) {
	    if(d.get()) {
		_templater._processName(c);
	    } else if(c.get()) {
		_templater._processName(b);
	    } else if(b.get()) {
		_templater._processName(a);
	    }
	    // std::cout << "col _" << tokenText(a) 
	    // 	      << "_ _" << tokenText(b) 
	    // 	      << "_ _" << tokenText(c) 
	    // 	      << "_ _" << tokenText(d) 
	    // 	      << "_ "; 
	    // a->setText("AWESOMECOLUMN");
	}
    private:
	Templater& _templater;
	
    };
    
    class TableHandler : public VoidThreeRefFunc {
    public: 
	TableHandler(Templater& t) : _templater(t) {}
	virtual ~TableHandler() {}
	virtual void operator()(RefAST a, RefAST b, RefAST c)  {
	    // right-most is the table name.
	    if(c.get()) {
		_templater._processName(c);
	    } else if(b.get()) {
		_templater._processName(b);
	    } else if(a.get()) {
		_templater._processName(a);
	    }
	    // std::cout << "qualname " << tokenText(a) 
	    // 	      << " " << tokenText(b) << " " 
	    // 	      << tokenText(c) << " "; 
	}
    private:
	Templater& _templater;
    };

    typedef std::map<std::string, char> ReMap;
    
    Templater(std::string const& delimiter="*?*") 
	: _delimiter(delimiter) {
    }
    ~Templater() {
    }
    template <typename Iter>
    void setKeynames(Iter begin, Iter end) {
	// Clear the map, then fill it.
	_map.clear();
	for(Iter i = begin; i != end; ++i) {
	    _map[*i] = true;
	}
    }

    std::string mungeName(std::string const& name) {
	return _delimiter + name + _delimiter;
    }

    bool isSpecial(std::string const& s) {
	return _map.find(s) != _map.end();
    }
    boost::shared_ptr<TableHandler> getTableHandler() {
	return boost::shared_ptr<TableHandler>(new TableHandler(*this));
    }
    boost::shared_ptr<ColumnHandler> getColumnHandler() {
	return boost::shared_ptr<ColumnHandler>(new ColumnHandler(*this));
    }
    template <typename AnAst>
    std::string walkTree(AnAst r) {
	//DFS walk?
	// Print child (child will print siblings)
	std::string result;
	RefAST c = r->getFirstChild();
	if(c.get()) {
	    result = walkTree(c);
	}
	// Now print sibling(s)
	RefAST s = r->getNextSibling();
	if(s.get()) {
	    if(!result.empty()) result += " ";
	    result += walkTree(s);
	}
	if(!result.empty()) result = " " + result;
	return r->getText() + result;
	
    }
private:
    void _processName(RefAST n) {
	if(isSpecial(n->getText())) {
	    n->setText(mungeName(n->getText()));
	}
    }

    ReMap _map;
    std::string _delimiter;

    friend class Templater::TableHandler;
    friend class Templater::ColumnHandler;
};




class SqlParseRunner {
public:
    SqlParseRunner(std::string const& statement, std::string const& delimiter) :
	_statement(statement),
	_stream(statement, stringstream::in | stringstream::out),
	_lexer(_stream),
	_parser(_lexer),
	_delimiter(delimiter),
	_templater(_delimiter)
    { }

    void setup(std::list<std::string> const& names) {
	_templater.setKeynames(names.begin(), names.end());
	_parser._columnRefHandler = _templater.getColumnHandler();
	_parser._qualifiedNameHandler = _templater.getTableHandler();
    }

    std::string getParseResult() {
	try {
	    _parser.initializeASTFactory(_factory);
	    _parser.setASTFactory(&_factory);
	    _parser.sql_stmt();
	    RefCommonAST ast = RefCommonAST(_parser.getAST());
	    
	    if (ast) {
		// ";" is not in the AST, so add it back.
		return ast->toStringList() +";"; 
	    } else {
		_errorMsg = "Error: no AST from parse";
	    }
	} catch( ANTLRException& e ) {
	    _errorMsg =  "Parse exception: " + e.getMessage();
	} catch( exception& e ) {
	    _errorMsg = std::string("General exception: ") + e.what();
	}
	return std::string(); // Error.

    }

    std::string const& getError() const {
	return _errorMsg;
    }
private:
    std::string _statement;
    std::stringstream _stream;
    ASTFactory _factory;
    SqlSQL2Lexer _lexer;
    SqlSQL2Parser _parser;
    std::string _delimiter;
    Templater _templater;

    std::string _errorMsg;

};
} // Anonymous namespace

///////////////////////////////////////////////////////////////////////////
// class Substitution
///////////////////////////////////////////////////////////////////////////
Substitution::Substitution(std::string template_, std::string const& delim) 
    : _template(template_) {
    _build(delim);
}
    
std::string Substitution::transform(Mapping const& m) {
    // This can be made more efficient by pre-sizing the result buffer
    // copying directly into it, rather than creating
    // intermediate string objects and appending.
    //
    unsigned pos = 0;
    std::string result;
    // No re-allocations if transformations are constant-size.
    result.reserve(_template.size()); 

    for(std::vector<Item>::const_iterator i = _index.begin();
	i != _index.end(); ++i) {
	// Copy bits since last match
	result += _template.substr(pos, i->position - pos);
	// Copy substitution
	Mapping::const_iterator s = m.find(i->name);
	result += s->second;
	// Update position
	pos = i->position + i->length;
    }
    // Copy remaining.
    if(pos < _template.length()) {
	result += _template.substr(pos);
    }
    return result;
}

// Let delim = ***
// blah blah ***Name*** blah blah
//           |         |
//         pos       endpos
//           |-length-|
//        name = Name
void Substitution::_build(std::string const& delim) {
    //int maxLength = _max(names.begin(), names.end());
    int delimLength = delim.length();
    for(unsigned pos=_template.find(delim); 
	pos < _template.length(); 
	pos = _template.find(delim, pos+1)) {
	unsigned endpos = _template.find(delim, pos + delimLength);
	Item newItem;
	newItem.position = pos;
	newItem.length = (endpos - pos) + delimLength;
	newItem.name.assign(_template, pos + delimLength,
			    newItem.length - delimLength - delimLength);
	// Note: length includes two delimiters.
	_index.push_back(newItem);
	pos = endpos;

	// Sanity check:
	// Check to see if the name is in names.
	    
    }

}

///////////////////////////////////////////////////////////////////////////
// class SqlSubstitution
///////////////////////////////////////////////////////////////////////////
SqlSubstitution::SqlSubstitution(std::string const& sqlStatement, 
				 Mapping const& mapping) 
    : _delimiter("*?*") {
    _build(sqlStatement, mapping);
    //
}
    
std::string SqlSubstitution::transform(Mapping const& m) {
    return _substitution->transform(m);
}

void SqlSubstitution::_build(std::string const& sqlStatement, 
			     Mapping const& mapping) {
    // 
    Mapping::const_iterator end = mapping.end();
    std::list<std::string> names;
    for(Mapping::const_iterator i=mapping.begin(); i != end; ++i) {
	names.push_back(i->first);
    }
    SqlParseRunner spr(sqlStatement, _delimiter);
    spr.setup(names);
    std::string template_ = spr.getParseResult();
	
    if(template_.empty()) {
	_errorMsg = spr.getError();
    }
    _substitution = SubstPtr(new Substitution(template_, _delimiter));
	
}

///////////////////////////////////////////////////////////////////////////
//class ChunkMapping
///////////////////////////////////////////////////////////////////////////
ChunkMapping::Map ChunkMapping::getMapping(int chunk, int subChunk) {
    Map m;
    ModeMap::const_iterator end = _map.end();
    std::string chunkStr = _toString(chunk);
    std::string subChunkStr = _toString(subChunk);
    for(ModeMap::const_iterator i = _map.begin(); i != end; ++i) {
	if(i->second == CHUNK) {
	    m.insert(MapValue(i->first, i->first + "_" + chunkStr));
	} else if (i->second == CHUNK_WITH_SUB) {
	    m.insert(MapValue(i->first, i->first + "_" 
			      + chunkStr + "_" + subChunkStr));
	}
    }
    return m;
}