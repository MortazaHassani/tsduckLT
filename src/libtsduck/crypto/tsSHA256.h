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
//!
//!  @file
//!  SHA-256 hash.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsHash.h"

namespace ts {
    //!
    //! SHA-256 hash.
    //! @ingroup crypto
    //!
    class TSDUCKDLL SHA256: public Hash
    {
        TS_NOCOPY(SHA256);
    public:
        // Sizes in bytes
        static const size_t HASH_SIZE  = 32;  //!< SHA-256 hash size in bytes.
        static const size_t BLOCK_SIZE = 64;  //!< SHA-256 block size in bytes.

        // Implementation of Hash interface:
        virtual UString name() const override;
        virtual size_t hashSize() const override;
        virtual size_t blockSize() const override;
        virtual bool init() override;
        virtual bool add(const void* data, size_t size) override;
        virtual bool getHash(void* hash, size_t bufsize, size_t* retsize = nullptr) override;

        //! Constructor
        SHA256();

    private:
        void compress(const uint8_t* buf);
        uint64_t _length;
        uint32_t _state[8];
        size_t   _curlen;
        uint8_t  _buf[BLOCK_SIZE];
    };
}
