/*
    Copyright 2008 Rutgers University
    Copyright 2005, 2008 John Asmuth

    This file is part of strxml.

    strxml is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    strxml is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with strxml.  If not, see <http://www.gnu.org/licenses/>.
 */
 
//#include <config.h>
#include <iostream>
#include <stdlib.h>
//#if HAVE_SSTREAM
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <stack>
/*
#else
#include <strstream>
namespace std {
  typedef std::ostrstream ostringstream;
}
#endif
*/
#include "strxml.hpp"

class XMLNode
{
public:
	int type;
	virtual ~XMLNode() {}
	virtual XMLNodePtr getChild(int i);
	virtual XMLNodePtr getChild(std::string s);
	virtual int size();
	virtual void print(std::ostream& os) = 0;
	virtual std::string getText() = 0;
	virtual std::string getName() = 0;
	virtual std::string getParam(std::string s) = 0;
};

typedef boost::shared_ptr<XMLNode> XMLNodePtr;

typedef std::pair<std::string, std::string> str_pair;
typedef std::vector<str_pair> str_pair_vec;
typedef std::vector<std::string> str_vec;
typedef std::map<std::string, std::string> str_str_map;
typedef std::vector<XMLNodePtr> node_vec;
typedef std::stack<XMLNodePtr> node_stk;

class XMLText : public XMLNode
{
public:
	std::string text;
	XMLText();
	virtual ~XMLText() {}
	virtual void print(std::ostream& os);
	virtual std::string getText();
	virtual std::string getName();
	virtual std::string getParam(std::string s);
};
typedef boost::shared_ptr<XMLText> XMLTextPtr;

class XMLParent : public XMLNode {
public:
	std::string name;
	str_str_map params;
	node_vec children;
	XMLParent();
	virtual ~XMLParent();
	virtual void print(std::ostream& os);
	XMLNodePtr getChild(int i);
	XMLNodePtr getChild(std::string s);
	virtual int size();
	virtual std::string getText();
	virtual std::string getName();
	virtual std::string getParam(std::string s);
};
typedef boost::shared_ptr<XMLParent> XMLParentPtr;

class PSink
{
	node_stk s;
public:
	int error;
	XMLNodePtr top;
	virtual void pushNode(std::string name, str_pair_vec params);
	virtual void popNode(std::string name);
	virtual void pushText(std::string text);

	virtual void formaterror() {error=1;}
	virtual void streamerror() {error=2;}

	PSink() {error=0;}
	virtual ~PSink() {}
};

int dissectNode(XMLNodePtr p, std::string child, std::string& destination);

int parseStream(std::istream& is, PSink& ps);
XMLNodePtr getNodeFromStream(std::istream& is);

std::string next_token(std::istream& is) {
	static char last_char = 0;

	std::string res;
	if (last_char)
		res += last_char;

	if (last_char == '<') {
		last_char = 0;
		return res;
	}

	if (last_char == '>') {
		last_char = 0;
		return res;
	}

	int next_char;
	while (1) {
		next_char = is.get();
		if (next_char == 1 || next_char == -1 || next_char == 0) {
			return std::string("");
		}
		if (next_char == '>' || next_char == '<') {
			if (res.length() == 0) {
				res += next_char;
				last_char = 0;
				return res;
			}
			last_char = next_char;
			break;
		}
		res += next_char;
	}
	return res;
}

int token_type(char c) {
	if (c == '=')
		return 2;
	if (c == '"')
		return 3;
	if (c == '/')
		return 4;
	if (isspace(c))
		return 0;
	return 1;
}

str_vec tokenize_string(std::string str) {
	str_vec v;
	std::string last;
	int t_type = 0;
	for (const char *s = str.c_str(); *s; s++) {
		int n_type = token_type(*s);

		if (t_type != n_type && last.length() != 0) {
			if (t_type)
				v.push_back(last);
			last.erase();
		}
		last += *s;
		t_type = n_type;
	}
	if (last.length())
		v.push_back(last);
	return v;
}

int do_node(std::string token, PSink& ps) {
	str_vec node_tokens = tokenize_string(token);

	if (node_tokens.size() == 0) {
		//cerr << "e1a" << endl;
		return -2;
	}
    
	if (node_tokens[0] == "/") {
		ps.popNode(node_tokens[1]);
		return -1;
	}
	std::string name = node_tokens[0];
	str_pair_vec v;
	for (size_t i=1; i<node_tokens.size(); i+=5) {
		if (node_tokens[i] == "/") {
			ps.pushNode(name, v);
			ps.popNode(name);
			return 0;
		}
		if (i+5 > node_tokens.size() ||
		    node_tokens[i+1] != "=" ||
		    node_tokens[i+2] != "\"" ||
		    node_tokens[i+4] != "\"") {
			//cerr << "e1b" << endl;
			return -2;
		}
		str_pair p(node_tokens[i], node_tokens[i+3]);
		v.push_back(p);
	}
	ps.pushNode(name, v);
	return 1;
}

int parseStream(std::istream& is, PSink& ps) {
	std::string token = next_token(is);
	int depth = 0;
	while (token.length() != 0) {
		if (token == "<") {
			int delta = do_node(next_token(is), ps);
			if (delta == -2) {
			    ps.formaterror();
			    return 1;
			}
			depth += delta;
			token = next_token(is);
			if (token != ">") {
			    ps.formaterror();
			    return 1;
			}
			if (depth == 0)
				return 0;
		}
		else
			ps.pushText(token);
		token = next_token(is);
	}
	ps.streamerror();
	return 1;
}

void PSink::pushNode(std::string name, str_pair_vec params) {
	XMLParentPtr p(new XMLParent);
	p->type = 2;
	p->name = name;
	for (size_t i=0; i<params.size(); i++)
		p->params[params[i].first] = params[i].second;
	if (s.size() == 0) {
		top = p;
	}
	else {
		XMLParentPtr x = boost::shared_polymorphic_downcast<XMLParent>(s.top());
		x->children.push_back(p);
	}
	s.push(p);
}

void PSink::popNode(std::string name) {
	s.pop();
}

void PSink::pushText(std::string text) {
	XMLTextPtr t(new XMLText);
	t->type = 1;
	t->text = text;
	if (s.size() == 0)
		top = t;
	else {
		XMLParentPtr p = boost::shared_polymorphic_downcast<XMLParent>(s.top());
		p->children.push_back(t);
	}
}
  
XMLText::XMLText() {
	type = 2;
}

void XMLText::print(std::ostream& os) {
	os << text;
}

XMLParent::XMLParent() {
	type = 2;
}

void XMLParent::print(std::ostream& os) {
	os << "<" << name;
	for (str_str_map::iterator itr = params.begin(); itr != params.end(); itr++)
		os << " " << itr->first << "=\"" << itr->second << "\"";
	os << ">";
	for (size_t i=0; i<children.size(); i++)
		children[i]->print(os);
	os << "</" << name << ">";
}

XMLNodePtr XMLParent::getChild(int i) {
	return children[i];
}

XMLNodePtr XMLParent::getChild(std::string s) {
	for (size_t i=0; i<children.size(); i++) {
		if (children[i]->type == 2) {
			XMLNodePtr p = children[i];
			if (p->getName() == s)
				return p;
		}
	}
	return XMLNodePtr();
}

int XMLParent::size() {
	return children.size();
}

std::string XMLText::getText() {
	return text;
}

std::string XMLText::getName() {
	return text;
}

std::string XMLText::getParam(std::string s) {
	return "";
}

std::string XMLParent::getText() {
	std::ostringstream os;
	if (children.size() == 1 && children[0]->type == 1)
		children[0]->print(os);
	else
		print(os);
	std::string s = os.str();
	return s;
}

std::string XMLParent::getName() {
	return name;
}

std::string XMLParent::getParam(std::string s) {
	return params[s];
}

XMLParent::~XMLParent() {

}

int dissectNode(XMLNodePtr p, std::string child, std::string& destination) {
	if (!p)
		return 0;
	XMLNodePtr c = p->getChild(child);
	if (!c)
		return 0;
	destination = c->getText();
	return 1;
}

XMLNodePtr getNodeFromStream(std::istream& is) {
	PSink ps;
	int ret = parseStream(is, ps);
	if (ret) {
		return XMLNodePtr();
	}
	return ps.top;
}

XMLNodePtr XMLNode::getChild(int i) {
	return XMLNodePtr();
}
XMLNodePtr XMLNode::getChild(std::string s) {
	return XMLNodePtr();
}
int XMLNode::size() {
	return 0;
}


XMLObject::XMLObject() {

}

XMLObject::XMLObject(std::istream& is) {
	node = getNodeFromStream(is);
}

int XMLObject::size() const {
	return node->size();
}

XMLObject XMLObject::operator[](int index) const {
	XMLNodePtr child = node->getChild(index);
	if (!child) {
		std::ostringstream err;
		err << "Element '" << node->getName() << "' has no child " << index; 
		throw xml_exception(err.str());
	}
	return XMLObject(child);
}

XMLObject XMLObject::operator[](std::string name) const {
	XMLNodePtr child = node->getChild(name);
	if (!child) {
		std::ostringstream err;
		err << "Element '" << node->getName() << "' has no child '" << name << "'"; 
		throw xml_exception(err.str());
	}
	return XMLObject(child);
}

std::string XMLObject::operator()(std::string key) const {
	return node->getParam(key);
}

std::string XMLObject::getName() const {
	return node->getName();
}

std::string XMLObject::getText() const {
	return node->getText();
}

std::ostream& operator << (std::ostream& os, const XMLObject& xo) {
	os << xo.node;
	return os;
}

std::istream& operator >> (std::istream& is, XMLObject& xo) {
	xo.node = getNodeFromStream(is);
	return is;
}

/*
int main(int argc, char **argv)
{
	XMLObject xobj;
	std::cin >> xobj;
	std::cout << xobj << std::endl;
	std::cout << (std::string)xobj["b"] << std::endl;
	return 0;
}
*/
