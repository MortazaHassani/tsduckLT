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

#include "tsT2MIPacket.h"
#include "tsCRC32.h"
#include "tsPSI.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::T2MIPacket::T2MIPacket() :
    SuperClass(),
    _is_valid(false)
{
}

ts::T2MIPacket::T2MIPacket(const T2MIPacket& pp, ShareMode mode) :
    SuperClass(pp, mode),
    _is_valid(pp._is_valid)
{
}

ts::T2MIPacket::T2MIPacket(T2MIPacket&& pp) noexcept :
    SuperClass(std::move(pp)),
    _is_valid(pp._is_valid)
{
}

ts::T2MIPacket::T2MIPacket(const void* content, size_t content_size, PID source_pid) :
    SuperClass(content, content_size, source_pid),
    _is_valid(false)
{
    validate();
}

ts::T2MIPacket::T2MIPacket(const ByteBlock& content, PID source_pid) :
    SuperClass(content, source_pid),
    _is_valid(false)
{
    validate();
}

ts::T2MIPacket::T2MIPacket(const ByteBlockPtr& content_ptr, PID source_pid) :
    SuperClass(content_ptr, source_pid),
    _is_valid(false)
{
    validate();
}


//----------------------------------------------------------------------------
// Validate binary content.
//----------------------------------------------------------------------------

void ts::T2MIPacket::validate()
{
    // Check fixed header size
    const uint8_t* const daddr = content();
    const size_t dsize = size();
    if (dsize < T2MI_HEADER_SIZE) {
        clear();
        return;
    }

    // Check packet size.
    const uint16_t payload_bytes = (GetUInt16(daddr + 4) + 7) / 8;
    if (T2MI_HEADER_SIZE + payload_bytes + SECTION_CRC32_SIZE != dsize) {
        clear();
        return;
    }

    // Get CRC from packet and recompute CRC from header + payload.
    const uint32_t pktCRC = GetUInt32(daddr + T2MI_HEADER_SIZE + payload_bytes);
    const uint32_t compCRC = CRC32(daddr, T2MI_HEADER_SIZE + payload_bytes);
    if (pktCRC != compCRC) {
        // Invalid CRC in T2-MI packet.
        clear();
        return;
    }

    // Passed all checks
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Clear packet content.
//----------------------------------------------------------------------------

void ts::T2MIPacket::clear()
{
    SuperClass::clear();
    _is_valid = false;
}


//----------------------------------------------------------------------------
// Reload from full binary content.
//----------------------------------------------------------------------------

void ts::T2MIPacket::reload(const void* content, size_t content_size, PID source_pid)
{
    SuperClass::reload(content, content_size, source_pid);
    validate();
}

void ts::T2MIPacket::reload(const ByteBlock& content, PID source_pid)
{
    SuperClass::reload(content, source_pid);
    validate();
}

void ts::T2MIPacket::reload(const ByteBlockPtr& content_ptr, PID source_pid)
{
    SuperClass::reload(content_ptr, source_pid);
    validate();
}


//----------------------------------------------------------------------------
// Assignment.
//----------------------------------------------------------------------------

ts::T2MIPacket& ts::T2MIPacket::operator=(const T2MIPacket& pp)
{
    if (&pp != this) {
        SuperClass::operator=(pp);
        _is_valid = pp._is_valid;
    }
    return *this;
}

ts::T2MIPacket& ts::T2MIPacket::operator=(T2MIPacket&& pp) noexcept
{
    if (&pp != this) {
        SuperClass::operator=(std::move(pp));
        _is_valid = pp._is_valid;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Duplication.
//----------------------------------------------------------------------------

ts::T2MIPacket& ts::T2MIPacket::copy(const T2MIPacket& pp)
{
    if (&pp != this) {
        SuperClass::copy(pp);
        _is_valid = pp._is_valid;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Comparison.
//----------------------------------------------------------------------------

bool ts::T2MIPacket::operator==(const T2MIPacket& pp) const
{
    return _is_valid && pp._is_valid && SuperClass::operator==(pp);
}


//----------------------------------------------------------------------------
// T2-MI packet characteristics
///----------------------------------------------------------------------------

size_t ts::T2MIPacket::payloadSize() const
{
    // Size of the payload of the packet in bytes.
    assert(!_is_valid || size() >= T2MI_HEADER_SIZE + SECTION_CRC32_SIZE);
    return _is_valid ? size() - T2MI_HEADER_SIZE - SECTION_CRC32_SIZE : 0;
}

bool ts::T2MIPacket::interleavingFrameStart() const
{
    // Get the interleaving frame start flag.
    return packetType() == T2MIPacketType::BASEBAND_FRAME && payloadSize() >= 3 && (content()[T2MI_HEADER_SIZE + 2] & 0x80) != 0;
}

const uint8_t* ts::T2MIPacket::basebandFrame() const
{
    // Access to the baseband frame inside the packet.
    return packetType() == T2MIPacketType::BASEBAND_FRAME && payloadSize() >= 3 ? content() + T2MI_HEADER_SIZE + 3 : nullptr;
}

size_t ts::T2MIPacket::basebandFrameSize() const
{
    // Size of the baseband frame in bytes.
    return packetType() == T2MIPacketType::BASEBAND_FRAME && payloadSize() >= 3 ? payloadSize() - 3 : 0;
}
