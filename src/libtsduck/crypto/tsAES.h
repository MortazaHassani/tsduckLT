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
//!  AES block cipher
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //! AES block cipher
    //! @ingroup crypto
    //!
    class TSDUCKDLL AES: public BlockCipher
    {
        TS_NOCOPY(AES);
    public:
        AES();                                        //!< Constructor.
        static constexpr size_t BLOCK_SIZE = 16;      //!< AES block size in bytes.
        static constexpr size_t MIN_KEY_SIZE = 16;    //!< AES minimum key size in bytes.
        static constexpr size_t MAX_KEY_SIZE = 32;    //!< AES maximum key size in bytes.
        static constexpr size_t MIN_ROUNDS = 10;      //!< AES minimum number of rounds.
        static constexpr size_t MAX_ROUNDS = 14;      //!< AES maximum number of rounds.
        static constexpr size_t DEFAULT_ROUNDS = 10;  //!< AES default number of rounds, actually depends on key size.

        // Implementation of BlockCipher interface:
        virtual UString name() const override;
        virtual size_t blockSize() const override;
        virtual size_t minKeySize() const override;
        virtual size_t maxKeySize() const override;
        virtual bool isValidKeySize(size_t size) const override;
        virtual size_t minRounds() const override;
        virtual size_t maxRounds() const override;
        virtual size_t defaultRounds() const override;

    protected:
        // Implementation of BlockCipher interface:
        virtual bool setKeyImpl(const void* key, size_t key_length, size_t rounds) override;
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;

    private:
        int      _Nr;     //!< Number of rounds
        uint32_t _eK[60]; //!< Scheduled encryption keys
        uint32_t _dK[60]; //!< Scheduled decryption keys
    };
}
