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

#include "tsIPv4Address.h"
#include "tsIPUtils.h"
#include "tsMemory.h"

// Local host address
const ts::IPv4Address ts::IPv4Address::LocalHost(127, 0, 0, 1);


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::IPv4Address::IPv4Address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) :
    _addr((uint32_t(b1) << 24) | (uint32_t(b2) << 16) | (uint32_t(b3) << 8) | uint32_t(b4))
{
}

ts::IPv4Address::IPv4Address(const ::sockaddr& s) :
    _addr(AnyAddress)
{
    if (s.sa_family == AF_INET) {
        assert(sizeof(::sockaddr) >= sizeof(::sockaddr_in));
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(&s);
        _addr = ntohl(sp->sin_addr.s_addr);
    }
}

ts::IPv4Address::IPv4Address(const ::sockaddr_in& s) :
    _addr(AnyAddress)
{
    if (s.sin_family == AF_INET) {
        _addr = ntohl(s.sin_addr.s_addr);
    }
}

ts::IPv4Address::~IPv4Address()
{
}


//----------------------------------------------------------------------------
// Copy into socket structures
//----------------------------------------------------------------------------

void ts::IPv4Address::copy(::sockaddr& s, uint16_t port) const
{
    TS_ZERO(s);
    assert(sizeof(::sockaddr) >= sizeof(::sockaddr_in));
    ::sockaddr_in* sp = reinterpret_cast<::sockaddr_in*> (&s);
    sp->sin_family = AF_INET;
    sp->sin_addr.s_addr = htonl(_addr);
    sp->sin_port = htons(port);
}

void ts::IPv4Address::copy(::sockaddr_in& s, uint16_t port) const
{
    TS_ZERO(s);
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(_addr);
    s.sin_port = htons(port);
}


//----------------------------------------------------------------------------
// Set/get address
//----------------------------------------------------------------------------

size_t ts::IPv4Address::binarySize() const
{
    return BYTES;
}

void ts::IPv4Address::clearAddress()
{
    _addr = AnyAddress;
}

bool ts::IPv4Address::hasAddress() const
{
    return _addr != AnyAddress;
}

bool ts::IPv4Address::isMulticast() const
{
    return IN_MULTICAST(_addr);
}

bool ts::IPv4Address::setAddress(const void* addr, size_t size)
{
    if (addr == nullptr || size < BYTES) {
        return false;
    }
    else {
        _addr = GetUInt32BE(addr);
        return true;
    }
}

size_t ts::IPv4Address::getAddress(void* addr, size_t size) const
{
    if (addr == nullptr || size < BYTES) {
        return 0;
    }
    else {
        PutUInt32BE(addr, _addr);
        return BYTES;
    }
}

void ts::IPv4Address::setAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    _addr = (uint32_t(b1) << 24) | (uint32_t(b2) << 16) | (uint32_t(b3) << 8) | uint32_t(b4);
}


//----------------------------------------------------------------------------
// Decode a string or hostname which is resolved.
//----------------------------------------------------------------------------

bool ts::IPv4Address::resolve(const UString& name, Report& report)
{
    _addr = AnyAddress;

    // Try the trivial case of an IPv4 address.
    uint8_t b1, b2, b3, b4;
    if (name.scan(u"%d.%d.%d.%d", {&b1, &b2, &b3, &b4})) {
        setAddress(b1, b2, b3, b4);
        return true;
    }

    // The best way to resolve a name is getaddrinfo(). However, this function
    // tries to activate shared objects to lookup services. Consequently, it
    // cannot be used when the application is statically linked. With statically
    // linked application, we need a plain address string "x.x.x.x".

#if defined(TSDUCK_STATIC)

    report.error(u"error resolving %s: must be an IPv4 address x.x.x.x (statically linked application)", {name});
    return false;

#else

    ::addrinfo hints;
    TS_ZERO(hints);
    hints.ai_family = AF_INET;
    ::addrinfo* res = nullptr;

    const int status = ::getaddrinfo(name.toUTF8().c_str(), nullptr, &hints, &res);

    if (status != 0) {
#if defined(TS_WINDOWS)
        const SysSocketErrorCode code = LastSysSocketErrorCode();
        report.error(name + u": " + SysSocketErrorCodeMessage(code));
#else
        const SysErrorCode code = LastSysErrorCode();
        UString errmsg;
        if (status == EAI_SYSTEM) {
            errmsg = SysErrorCodeMessage(code);
        }
        else {
            errmsg = UString::FromUTF8(gai_strerror(status));
        }
        report.error(name + u": " + errmsg);
#endif
        return false;
    }

    // Look for an IPv4 address. All returned addresses should be IPv4 since
    // we specfied the family in hints, but check to be sure.

    ::addrinfo* ai = res;
    while (ai != nullptr && (ai->ai_family != AF_INET || ai->ai_addr == nullptr || ai->ai_addr->sa_family != AF_INET)) {
        ai = ai->ai_next;
    }
    if (ai != nullptr) {
        assert(sizeof(::sockaddr) >= sizeof(::sockaddr_in));
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*> (ai->ai_addr);
        _addr = ntohl(sp->sin_addr.s_addr);
    }
    else {
        report.error(u"no IPv4 address found for " + name);
    }
    ::freeaddrinfo(res);
    return ai != nullptr;

#endif // TSDUCK_STATIC
}


//----------------------------------------------------------------------------
// Check if this address "matches" another one.
//----------------------------------------------------------------------------

bool ts::IPv4Address::match(const IPv4Address& other) const
{
    return _addr == AnyAddress || other._addr == AnyAddress || _addr == other._addr;
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::IPv4Address::toString() const
{
    return UString::Format(u"%d.%d.%d.%d", {(_addr >> 24) & 0xFF, (_addr >> 16) & 0xFF, (_addr >> 8) & 0xFF, _addr & 0xFF});
}
