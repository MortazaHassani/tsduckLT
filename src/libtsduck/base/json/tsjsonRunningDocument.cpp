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

#include "tsjsonRunningDocument.h"
#include "tsjsonValue.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::json::RunningDocument::RunningDocument(Report& report) :
    _text(report),
    _open_array(false),
    _empty_array(true),
    _obj_count(0)
{
}

ts::json::RunningDocument::~RunningDocument()
{
    close();
}


//----------------------------------------------------------------------------
// Initialize the document.
//----------------------------------------------------------------------------

bool ts::json::RunningDocument::open(const ValuePtr& root, const UString& fileName, std::ostream& strm)
{
    // Cleanup previous state.
    close();

    // Locate the array that must remain open.
    ValuePtrVector path;
    if (!root.isNull() && !searchArray(root, path)) {
        _text.report().error(u"internal error, no array in JSON tree, cannot build a dynamic JSON document");
        return false;
    }

    // Open either a file or stream.
    if (fileName.empty() || fileName == u"-") {
        _text.setStream(strm);
    }
    else if (!_text.setFile(fileName)) {
        return false;
    }

    // Print all open objects up to the open array.
    if (root.isNull()) {
        // Emulate empty array.
        _text << "[" << ts::indent;
        _empty_array = true;
        _obj_count = 0;
    }
    else {
        // The path is made of objects only, except the last one which is the array.
        assert(!path.empty());
        _obj_count = path.size() - 1;
        // Print all parent objects.
        for (size_t pi = 0; pi < _obj_count; ++pi) {
            const ValuePtr& value(path[pi]);
            assert(value->isObject());
            UStringList names;
            value->getNames(names);
            _text << "{" << ts::indent;
            // Print all fields, except the one containing the array.
            UString last_name;
            size_t count = 0;
            for (const auto& it : names) {
                const ValuePtr subval(value->valuePtr(it));
                if (subval == path[pi+1]) {
                    // Field containing the array will be printed last.
                    last_name = it;
                }
                else {
                    if (count++ > 0) {
                        _text << ",";
                    }
                    _text << ts::endl << ts::margin << '"' << it.toJSON() << "\": ";
                    subval->print(_text);
                }
            }
            // Print the name of the last field.
            if (count > 0) {
                _text << ",";
            }
            _text << ts::endl << ts::margin << '"' << last_name.toJSON() << "\": ";
        }
        // Print the start of the array.
        const ValuePtr& value(path.back());
        assert(value->isArray());
        const size_t count = value->size();
        _empty_array = count == 0;
        _text << "[" << ts::indent;
        for (size_t i = 0; i < count; ++i) {
            if (i > 0) {
                _text << ",";
            }
            _text << ts::endl << ts::margin;
            value->at(i).print(_text);
        }
    }

    _open_array = true;
    return true;
}


//----------------------------------------------------------------------------
// Add one JSON value in the open array of the running document.
//----------------------------------------------------------------------------

void ts::json::RunningDocument::add(const Value& value)
{
    // Add object only if the array is already open and the provided object is not null.
    if (_open_array) {
        if (!_empty_array) {
            // There are already some elements in the array.
            _text << ",";
        }
        _text << ts::endl << ts::margin;
        value.print(_text);
        _empty_array = false;
    }
}


//----------------------------------------------------------------------------
// Close the running document.
//----------------------------------------------------------------------------

void ts::json::RunningDocument::close()
{
    // Close array and parent objects.
    if (_open_array) {
        // Unindent and closing sequence for the open array.
        _text << ts::endl << ts::unindent << ts::margin << "]";
        _open_array = false;
        _empty_array = true;

        // Close all parent objects.
        while (_obj_count > 0) {
            // Unindent and closing sequence for each parent object.
            _text << ts::endl << ts::unindent << ts::margin << "}";
            _obj_count--;
        }
        _text << std::endl;
    }
    assert(_obj_count == 0);

    // Close the associated text formatter.
    _text.close();
}


//----------------------------------------------------------------------------
// Look for a JSON array in a tree. Return true if one is found.
// Build a path of objects, one per level. The last one is the array.
//----------------------------------------------------------------------------

bool ts::json::RunningDocument::searchArray(const ValuePtr& root, ValuePtrVector& path)
{
    // Assume that the root is part of the path.
    path.push_back(root);

    if (root->isArray()) {
        // Directly found the array. This is the last segment in the path.
        return true;
    }
    else if (root->isObject()) {
        // Lookup all fields in the object.
        UStringList names;
        root->getNames(names);
        for (const auto& it : names) {
            const ValuePtr val(root->valuePtr(it));
            if (!val.isNull() && searchArray(val, path)) {
                // Found an array in that branch.
                return true;
            }
        }
    }

    // The root is not in the path.
    path.pop_back();
    return false;
}
