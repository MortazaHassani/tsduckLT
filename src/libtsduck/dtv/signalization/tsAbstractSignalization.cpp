//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsAbstractSignalization.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

const ts::UChar* const ts::AbstractSignalization::XML_GENERIC_DESCRIPTOR  = u"generic_descriptor";
const ts::UChar* const ts::AbstractSignalization::XML_GENERIC_SHORT_TABLE = u"generic_short_table";
const ts::UChar* const ts::AbstractSignalization::XML_GENERIC_LONG_TABLE  = u"generic_long_table";


//----------------------------------------------------------------------------
// Constructors, assignments and destructors.
//----------------------------------------------------------------------------

ts::AbstractSignalization::AbstractSignalization(const UChar* xml_name, Standards standards, const UChar* xml_legacy_name) :
    AbstractDefinedByStandards(),
    _is_valid(true),
    _xml_name(xml_name),
    _xml_legacy_name(xml_legacy_name),
    _standards(standards)
{
}

ts::AbstractSignalization& ts::AbstractSignalization::operator=(const AbstractSignalization& other)
{
    if (this != &other) {
        // Don't copy the pointer to XML name or the list of standards, they are const values.
        // In debug mode, check that we have the same XML name.
        assert(_standards == other._standards);
        assert((_xml_name == nullptr && other._xml_name == nullptr) ||
               (_xml_name != nullptr && other._xml_name != nullptr && UString(_xml_name) == UString(other._xml_name)));
        _is_valid = other._is_valid;
    }
    return *this;
}

ts::AbstractSignalization::~AbstractSignalization()
{
}


//----------------------------------------------------------------------------
// Implementation of AbstractDefinedByStandards.
//----------------------------------------------------------------------------

ts::Standards ts::AbstractSignalization::definingStandards() const
{
    return _standards;
}


//----------------------------------------------------------------------------
// Get the XMl node name representing this table.
//----------------------------------------------------------------------------

ts::UString ts::AbstractSignalization::xmlName() const
{
    return UString(_xml_name);
}


//----------------------------------------------------------------------------
// This method clears the content of the table or descriptor.
//----------------------------------------------------------------------------

void ts::AbstractSignalization::clear()
{
    _is_valid = true;
    clearContent();
}


//----------------------------------------------------------------------------
// XML serialization and deserialization (default implementations).
//----------------------------------------------------------------------------

ts::xml::Element* ts::AbstractSignalization::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* root = _is_valid && parent != nullptr ? parent->addElement(_xml_name) : nullptr;
    if (root != nullptr) {
        buildXML(duck, root);
    }
    return root;
}

void ts::AbstractSignalization::fromXML(DuckContext& duck, const xml::Element* element)
{
    // Make sure the object is cleared before analyzing the XML.
    clear();

    // The object is valid if the XML node name is correct and the subclass correctly analyzes the XML node.
    _is_valid = checkXMLName(element) && analyzeXML(duck, element);

    // If the object is invalid, clear it again to avoid letting partial objects being used.
    if (!_is_valid) {
        clear();
        _is_valid = false; // was set to true by clear()
    }
}


//----------------------------------------------------------------------------
// Check that an XML element has the right name for this table.
//----------------------------------------------------------------------------

bool ts::AbstractSignalization::checkXMLName(const xml::Element* element) const
{
    if (element == nullptr) {
        return false;
    }
    else if (element->name().similar(_xml_name)) {
        return true;
    }
    else if (_xml_legacy_name != nullptr && element->name().similar(_xml_legacy_name)) {
        return true;
    }
    else {
        element->report().error(u"Incorrect <%s>, expected <%s>", {element->name(), _xml_name});
        return false;
    }
}
