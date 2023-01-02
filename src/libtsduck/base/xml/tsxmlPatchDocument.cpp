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

#include "tsxmlPatchDocument.h"
#include "tsxmlElement.h"

#define X_ATTR          u"x-" // prefix of special attribute names
#define X_NODE_ATTR     X_ATTR u"node"
#define X_ADD_PREFIX    X_ATTR u"add-"
#define X_DELETE_PREFIX X_ATTR u"delete-"
#define X_UPDATE_PREFIX X_ATTR u"update-"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::PatchDocument::PatchDocument(Report& report) :
    Document(report)
{
}

ts::xml::PatchDocument::~PatchDocument()
{
}


//----------------------------------------------------------------------------
// Patch an XML document.
//----------------------------------------------------------------------------

void ts::xml::PatchDocument::patch(Document& doc) const
{
    patchElement(rootElement(), doc.rootElement());
}


//----------------------------------------------------------------------------
// Patch an XML tree of elements.
//----------------------------------------------------------------------------

bool ts::xml::PatchDocument::patchElement(const Element* patch, Element* doc) const
{
    // If the node name do not match, no need to go further.
    if (doc == nullptr || !doc->haveSameName(patch)) {
        return true;
    }

    // Get all attributes in the patch element.
    std::map<UString, UString> attr;
    patch->getAttributes(attr);

    // Check if all attributes in doc element match the specific attributes in the patch element.
    for (const auto& it : attr) {
        // Ignore attributes starting with the special prefix.
        if (!it.first.startWith(X_ATTR, CASE_INSENSITIVE) && !doc->hasAttribute(it.first, it.second)) {
            // No, the doc node does not meet the patch requirements.
            return true;
        }
    }

    // Now process all attribute modifications on the node attributes.
    for (auto it : attr) {
        if (it.first.startWith(X_ADD_PREFIX, CASE_INSENSITIVE)) {
            // Add or replace an attribute.
            UString name(it.first);
            name.removePrefix(X_ADD_PREFIX, CASE_INSENSITIVE);
            if (!name.empty()) {
                doc->setAttribute(name, it.second);
            }
        }
        else if (it.first.startWith(X_DELETE_PREFIX, CASE_INSENSITIVE)) {
            // Delete an attribute.
            UString name(it.first);
            name.removePrefix(X_DELETE_PREFIX, CASE_INSENSITIVE);
            if (!name.empty()) {
                doc->deleteAttribute(name);
            }
        }
        else if (it.first.startWith(X_UPDATE_PREFIX, CASE_INSENSITIVE)) {
            // Update an exiting attribute.
            UString name(it.first);
            name.removePrefix(X_UPDATE_PREFIX, CASE_INSENSITIVE);
            if (!name.empty() && doc->hasAttribute(name)) {
                doc->setAttribute(name, it.second);
            }
        }
        else if (it.first.similar(X_NODE_ATTR) && it.second.similar(u"delete")) {
            // Remove this node from parent.
            // Deallocating the element call its destructor which removes it from parent.
            delete doc;
            return false;
        }
        else if (it.first.startWith(X_ATTR, CASE_INSENSITIVE)) {
            report().error(u"invalid special attribute '%s' in <%s>, line %d", {it.first, patch->name(), patch->lineNumber()});
        }
    }

    // Now recurse on all children elements in the document to patch.
    // We need to get the list of elements first and then process them because each processing may add or remove children.
    std::vector<Element*> docChildren;
    for (Element* child = doc->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        docChildren.push_back(child);
    }

    // Get the children of the patch node.
    std::vector<const Element*> patchChildren;
    for (const Element* child = patch->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        if (child->hasAttribute(X_NODE_ATTR, u"add")) {
            // This is a node to add directly. Create a clone.
            Element* e = new Element(*child);
            // Remove all "x-" attributes (especially the "x-node" one).
            cleanupAttributes(e);
            // Add the new child in the document.
            e->reparent(doc);
        }
        else {
            // This is a patch to apply.
            patchChildren.push_back(child);
        }
    }

    // Now apply all patches on all doc children.
    for (size_t di = 0; di < docChildren.size(); ++di) {
        for (size_t pi = 0; pi < patchChildren.size(); ++pi) {
            if (!patchElement(patchChildren[pi], docChildren[di])) {
                // Stop processing this doc child (probably deleted).
                break;
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Cleanup a cloned XML tree from all "x-" attributes.
//----------------------------------------------------------------------------

void ts::xml::PatchDocument::cleanupAttributes(Element* e) const
{
    // Get all attribute names.
    UStringList attrNames;
    e->getAttributesNames(attrNames);

    // Remove all attributes starting with the special prefix.
    for (const auto& it : attrNames) {
        if (it.startWith(X_ATTR, CASE_INSENSITIVE)) {
            e->deleteAttribute(it);
        }
    }

    // Recurse on all children.
    for (Element* child = e->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        cleanupAttributes(child);
    }
}
