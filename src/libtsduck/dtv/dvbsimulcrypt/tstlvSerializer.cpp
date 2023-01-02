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

#include "tstlvSerializer.h"


//----------------------------------------------------------------------------
// Open a TLV structure. Cannot be nested in the same serializer.
// Use nested factories but not nested TLV into one serializer.
//----------------------------------------------------------------------------

void ts::tlv::Serializer::openTLV(TAG tag)
{
    // Bug if TLV already open
    assert(_length_offset < 0);

    // Insert tag value
    putUInt16(tag);

    // Save position of length field. Must use offsets and not pointers
    // because of potential reallocations before closeTLV().
    _length_offset = int(_bb->size());

    // Insert dummy length. Will be updated by closeTLV()
    putUInt16(0);
}


//----------------------------------------------------------------------------
// Close the current TLV structure
//----------------------------------------------------------------------------

void ts::tlv::Serializer::closeTLV()
{
    // Bug if no TLV open
    assert(_length_offset >= 0);

    // Compute actual length of TLV "value" field
    int length = int(_bb->size() - _length_offset - sizeof(LENGTH));
    assert(length >= 0);

    // Rewrite length in previously saved location
    PutUInt16(_bb->data() + _length_offset, uint16_t(length));

    // Mark TLV as closed.
    _length_offset = -1;
}


//----------------------------------------------------------------------------
// Insert each integer in the vector as one TLV field.
//----------------------------------------------------------------------------

void ts::tlv::Serializer::putUInt8(TAG tag, const std::vector<uint8_t>& val)
{
    for (auto i : val) {
        putUInt8(tag, i);
    }
}

void ts::tlv::Serializer::putUInt16(TAG tag, const std::vector<uint16_t>& val)
{
    for (auto i : val) {
        putUInt16(tag, i);
    }
}

void ts::tlv::Serializer::putUInt32(TAG tag, const std::vector<uint32_t>& val)
{
    for (auto i : val) {
        putUInt32(tag, i);
    }
}

void ts::tlv::Serializer::putUInt64(TAG tag, const std::vector<uint64_t>& val)
{
    for (auto i : val) {
        putUInt64(tag, i);
    }
}

void ts::tlv::Serializer::putInt8(TAG tag, const std::vector<int8_t>& val)
{
    for (auto i : val) {
        putInt8(tag, i);
    }
}

void ts::tlv::Serializer::putInt16(TAG tag, const std::vector<int16_t>& val)
{
    for (auto i : val) {
        putInt16(tag, i);
    }
}

void ts::tlv::Serializer::putInt32(TAG tag, const std::vector<int32_t>& val)
{
    for (auto i : val) {
        putInt32(tag, i);
    }
}

void ts::tlv::Serializer::putInt64(TAG tag, const std::vector<int64_t>& val)
{
    for (auto i : val) {
        putInt64(tag, i);
    }
}


//----------------------------------------------------------------------------
// Insert a vector of booleans as TLV structures
//----------------------------------------------------------------------------

void ts::tlv::Serializer::putBool(TAG tag, const std::vector<bool>& val)
{
    for (auto i : val) {
        putBool(tag, i);
    }
}


//----------------------------------------------------------------------------
// Insert a vector of strings as TLV structures
//----------------------------------------------------------------------------

void ts::tlv::Serializer::put(TAG tag, const std::vector<std::string>& val)
{
    for (auto i : val) {
        put(tag, i);
    }
}


//----------------------------------------------------------------------------
// Convert to a string (for debug purpose)
//----------------------------------------------------------------------------

ts::UString ts::tlv::Serializer::toString() const
{
    UString prefix;
    if (_bb.isNull()) {
        return u"(null)";
    }
    prefix = UString::Format(u"{%d bytes, ", {_bb->size()});
    if (_length_offset >= 0) {
        prefix += UString::Format(u"length at offset %d, ", {_length_offset});
    }
    return prefix + u"data: " + UString::Dump(*_bb, UString::SINGLE_LINE) + u"}";
}
