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
//!  Forward declarations for MPEG PSI/SI types.
//!  Useful to avoid interdependencies of header files.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSafePtr.h"
#include "tsPSI.h"

namespace ts {

    class AbstractDescriptor;
    class AbstractTable;
    class BinaryTable;
    class Descriptor;
    class DescriptorList;
    class PSIBuffer;
    class Section;
    class TablesDisplay;

    //!
    //! Safe pointer for AbstractDescriptor (not thread-safe).
    //!
    typedef SafePtr<AbstractDescriptor,NullMutex> AbstractDescriptorPtr;

    //!
    //! Vector of AbstractDescriptor pointers
    //!
    typedef std::vector<AbstractDescriptorPtr> AbstractDescriptorPtrVector;

    //!
    //! Safe pointer for AbstractTable (not thread-safe)
    //!
    typedef SafePtr<AbstractTable,NullMutex> AbstractTablePtr;

    //!
    //! Safe pointer for Section (not thread-safe).
    //!
    typedef SafePtr<Section, NullMutex> SectionPtr;

    //!
    //! Vector of Section pointers.
    //!
    typedef std::vector<SectionPtr> SectionPtrVector;

    //!
    //! Vector of BinaryTable pointers
    //!
    typedef std::vector<AbstractTablePtr> AbstractTablePtrVector;

    //!
    //! Safe pointer for BinaryTable (not thread-safe)
    //!
    typedef SafePtr<BinaryTable,NullMutex> BinaryTablePtr;

    //!
    //! Vector of BinaryTable pointers
    //!
    typedef std::vector<BinaryTablePtr> BinaryTablePtrVector;

    //!
    //! Safe pointer for Descriptor (not thread-safe)
    //!
    typedef SafePtr<Descriptor, NullMutex> DescriptorPtr;

    //!
    //! Vector of Descriptor pointers
    //! Use class DescriptorList for advanced features.
    //! @see DescriptorList
    //!
    typedef std::vector<DescriptorPtr> DescriptorPtrVector;

    //!
    //! Profile of a function to display a section.
    //! Each subclass of AbstractTable should provide a static function named
    //! @e DisplaySection which displays a section of its table-id.
    //!
    //! @param [in,out] display Display engine.
    //! @param [in] section The section to display.
    //! @param [in,out] payload A read-only PSIBuffer over the section payload.
    //! Everything that was not read from the buffer will be displayed by the
    //! caller as "extraneous data". Consequently, the table subclasses do
    //! not have to worry about those extraneous data.
    //! @param [in] margin Left margin content.
    //!
    typedef void (*DisplaySectionFunction)(TablesDisplay& display, const Section& section, PSIBuffer& payload, const UString& margin);

    //!
    //! Profile of a function to display a brief overview ("log") of a section on one line.
    //! A subclass of AbstractTable may provide a static function for this.
    //!
    //! @param [in] section The section to log.
    //! @param [in] max_bytes Maximum number of bytes to log from the section. 0 means unlimited.
    //! @return A one-line brief summary of the table.
    //!
    typedef UString (*LogSectionFunction)(const Section& section, size_t max_bytes);

    //!
    //! Profile of a function to display a descriptor.
    //! Each subclass of AbstractDescriptor should provide a static function named
    //! @e DisplayDescriptor which displays a descriptor of its type.
    //!
    //! @param [in,out] display Display engine.
    //! @param [in,out] payload A read-only PSIBuffer over the descriptor payload.
    //! For "extended descriptors", the buffer starts after the "extension tag".
    //! Everything that was not read from the buffer will be displayed by the
    //! caller as "extraneous data". Consequently, the descriptor subclasses do
    //! not have to worry about those extraneous data.
    //! @param [in] margin Left margin content.
    //! @param [in] did Descriptor id.
    //! @param [in] tid Table id of table containing the descriptors.
    //! This is optional. Used by some descriptors the interpretation of which may
    //! vary depending on the table that they are in.
    //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
    //!
    typedef void (*DisplayDescriptorFunction)(TablesDisplay& display, PSIBuffer& payload, const UString& margin, DID did, TID tid, PDS pds);

    //!
    //! Profile of a function to display the private part of a CA_descriptor.
    //!
    //! @param [in,out] display Display engine.
    //! @param [in,out] private_part A read-only PSIBuffer over the private part of a CA_descriptor.
    //! @param [in] margin Left margin content.
    //! @param [in] tid Table id of table containing the descriptors (typically CAT or PMT).
    //!
    typedef void (*DisplayCADescriptorFunction)(TablesDisplay& display, PSIBuffer& private_part, const UString& margin, TID tid);
}

//!
//! @hideinitializer
//! Define a DisplaySection static function.
//!
#define DeclareDisplaySection()                                  \
    /** A static method to display a section.                 */ \
    /** @param [in,out] display Display engine.               */ \
    /** @param [in] section The section to display.           */ \
    /** @param [in,out] payload A PSIBuffer over the payload. */ \
    /** @param [in] margin Left margin content.               */ \
    static void DisplaySection(ts::TablesDisplay& display, const ts::Section& section, ts::PSIBuffer& payload, const ts::UString& margin)

//!
//! @hideinitializer
//! Define a LogSection static function.
//!
#define DeclareLogSection()                                      \
    /** A static method to log a section.                     */ \
    /** @param [in] section The section to log.               */ \
    /** @param [in] max_bytes Maximum number of bytes to log. */ \
    static ts::UString LogSection(const ts::Section& section, size_t max_bytes)

//!
//! @hideinitializer
//! Define a DisplayDescriptor static function.
//!
#define DeclareDisplayDescriptor()                                       \
    /** Static method to display a descriptor.                        */ \
    /** @param [in,out] display Display engine.                       */ \
    /** @param [in,out] payload A PSIBuffer over the payload.         */ \
    /** @param [in] margin Left margin content.                       */ \
    /** @param [in] did Descriptor id.                                */ \
    /** @param [in] tid Table id of table containing the descriptors. */ \
    /** @param [in] pds Private Data Specifier.                       */ \
    static void DisplayDescriptor(ts::TablesDisplay& display, ts::PSIBuffer& payload, const ts::UString& margin, ts::DID did, ts::TID tid, ts::PDS pds)
