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
//!  Declaration of class DVBCharTableUTF16.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDVBCharTable.h"
#include "tsDVBCharset.h"

TS_PUSH_WARNING()
TS_GCC_NOWARNING(ctor-dtor-privacy) // private constructor here

namespace ts {
    //!
    //! Definition of the UTF-8 DVB character set.
    //! @see ETSI EN 300 468, Annex A.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL DVBCharTableUTF16 : public DVBCharTable
    {
        TS_NOCOPY(DVBCharTableUTF16);
    public:
        static const DVBCharTableUTF16 RAW_UNICODE;  //!< Raw UNICODE (UTF-16) character set.
        static const DVBCharset DVB_UNICODE;         //!< Non-standard DVB encoding using UNICODE (UTF-16) character set as default.

        // Inherited methods.
        virtual bool decode(UString& str, const uint8_t* dvb, size_t dvbSize) const override;
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = NPOS) const override;
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const override;

    private:
        // Private constructor since only local instances are available.
        explicit DVBCharTableUTF16(const UChar* name = nullptr);
   };
}

TS_POP_WARNING()
