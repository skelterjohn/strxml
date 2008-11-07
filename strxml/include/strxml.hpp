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
 
#ifndef _STRXML_H
#define _STRXML_H

#include <stdexcept>
#include <boost/shared_ptr.hpp>

class XMLNode;
typedef boost::shared_ptr<XMLNode> XMLNodePtr;

class XMLObject {
private:
	XMLNodePtr node;
	XMLObject(XMLNodePtr node)
	: node(node) { }
public:
	XMLObject();
	/**
	 * Reads the XML from the input stream
	 */
	XMLObject(std::istream& is);
	int size() const;
	/**
	 * Gets a child element specified by index
	 */
	XMLObject operator[](int index) const;
	/**
	 * Gets the first child element specified by name
	 */
	XMLObject operator[](std::string name) const;
	/**
	 * Gets an element attribute specified by key
	 */
	std::string operator()(std::string key) const;
	/**
	 * Gets the element name
	 */
	std::string getName() const;
	/**
	 * Gets the XML code for the element and children, unless
	 * there is a single child, in which case its contents
	 * are returned
	 */
	std::string getText() const;
	
	friend std::ostream& operator << (std::ostream& os, const XMLObject& xn);
	friend std::istream& operator >> (std::istream& is, XMLObject& xn);
};

class xml_exception : public std::runtime_error {
public:
	explicit xml_exception(const std::string& what)
	: std::runtime_error(what) { }
	virtual ~xml_exception() throw() {}
};

std::ostream& operator << (std::ostream& os, const XMLObject& xn);
std::istream& operator >> (std::istream& is, XMLObject& xn);

#endif
