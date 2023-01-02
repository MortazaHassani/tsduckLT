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

#include "tsBlockCipher.h"
#include "tsBlockCipherAlertInterface.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::BlockCipher::BlockCipher() :
    _key_set(false),
    _cipher_id(0),
    _key_encrypt_count(0),
    _key_decrypt_count(0),
    _key_encrypt_max(UNLIMITED),
    _key_decrypt_max(UNLIMITED),
    _current_key(),
    _alert(nullptr)
{
}

ts::BlockCipher::~BlockCipher()
{
}


//----------------------------------------------------------------------------
// Get the current key.
//----------------------------------------------------------------------------

bool ts::BlockCipher::getKey(ByteBlock& key) const
{
    key = _current_key;
    return _key_set && isValidKeySize(key.size());
}


//----------------------------------------------------------------------------
// Schedule a new key.
//----------------------------------------------------------------------------

bool ts::BlockCipher::setKey(const void* key, size_t key_length, size_t rounds)
{
    _key_encrypt_count = _key_decrypt_count = 0;
    _current_key.copy(key, key_length);
    _key_set = setKeyImpl(key, key_length, rounds);
    return _key_set;
}


//----------------------------------------------------------------------------
// Check if encryption or decryption is allowed. Increment counters.
//----------------------------------------------------------------------------

bool ts::BlockCipher::allowEncrypt()
{
    // Check that a key was successfully set.
    if (!_key_set) {
        return false;
    }

    // Check encryption limitations.
    if (_key_encrypt_count >= _key_encrypt_max &&
        (_alert == nullptr || _alert->handleBlockCipherAlert(*this, BlockCipherAlertInterface::ENCRYPTION_EXCEEDED)))
    {
        // Disallow encryption if no handler present or handler did not cancel the alert.
        return false;
    }

    // Notify first encryption.
    if (_key_encrypt_count == 0 && _alert != nullptr) {
        // Informational only.
        _alert->handleBlockCipherAlert(*this, BlockCipherAlertInterface::FIRST_ENCRYPTION);
    }

    // Encryption allowed.
    _key_encrypt_count++;
    return true;
}

bool ts::BlockCipher::allowDecrypt()
{
    // Check that a key was successfully set.
    if (!_key_set) {
        return false;
    }

    // Check decryption limitations.
    if (_key_decrypt_count >= _key_decrypt_max &&
        (_alert == nullptr || _alert->handleBlockCipherAlert(*this, BlockCipherAlertInterface::DECRYPTION_EXCEEDED)))
    {
        // Disallow decryption if no handler present or handler did not cancel the alert.
        return false;
    }

    // Notify first decryption.
    if (_key_decrypt_count == 0 && _alert != nullptr) {
        // Informational only.
        _alert->handleBlockCipherAlert(*this, BlockCipherAlertInterface::FIRST_DECRYPTION);
    }

    // Decryption allowed.
    _key_decrypt_count++;
    return true;
}


//----------------------------------------------------------------------------
// Encrypt one block of data.
//----------------------------------------------------------------------------

bool ts::BlockCipher::encrypt(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    return allowEncrypt() && encryptImpl(plain, plain_length, cipher, cipher_maxsize, cipher_length);
}


//----------------------------------------------------------------------------
// Decrypt one block of data.
//----------------------------------------------------------------------------

bool ts::BlockCipher::decrypt(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    return allowDecrypt() && decryptImpl(cipher, cipher_length, plain, plain_maxsize, plain_length);
}


//----------------------------------------------------------------------------
// Encrypt one block of data in place.
//----------------------------------------------------------------------------

bool ts::BlockCipher::encryptInPlace(void* data, size_t data_length, size_t* max_actual_length)
{
    return allowEncrypt() && encryptInPlaceImpl(data, data_length, max_actual_length);
}

bool ts::BlockCipher::encryptInPlaceImpl(void* data, size_t data_length, size_t* max_actual_length)
{
    const ByteBlock plain(data, data_length);
    const size_t cipher_max_size = max_actual_length != nullptr ? *max_actual_length : data_length;
    return encryptImpl(plain.data(), plain.size(), data, cipher_max_size, max_actual_length);
}


//----------------------------------------------------------------------------
// Decrypt one block of data in place.
//----------------------------------------------------------------------------

bool ts::BlockCipher::decryptInPlace(void* data, size_t data_length, size_t* max_actual_length)
{
    return allowDecrypt() && decryptInPlaceImpl(data, data_length, max_actual_length);
}

bool ts::BlockCipher::decryptInPlaceImpl(void* data, size_t data_length, size_t* max_actual_length)
{
    const ByteBlock cipher(data, data_length);
    const size_t plain_max_size = max_actual_length != nullptr ? *max_actual_length : data_length;
    return decryptImpl(cipher.data(), cipher.size(), data, plain_max_size, max_actual_length);
}
