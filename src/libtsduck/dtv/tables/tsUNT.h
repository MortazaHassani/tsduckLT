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
//!  Representation of an Update Notification Table (UNT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of an Update Notification Table (INT).
    //! @see ETSI TS 102 006, 9.4.1
    //! @ingroup table
    //!
    class TSDUCKDLL UNT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a compatibility descriptor.
        //! This structure is in fact one entry in the compatibilityDescriptor()
        //! structure as defined in ISO/IEC 13818-6 and ETSI TS 102 006.
        //!
        class TSDUCKDLL CompatibilityDescriptor
        {
        public:
            uint8_t        descriptorType;   //!< Type of descriptor.
            uint8_t        specifierType;    //!< Specified type, default is 1 (IEEE OUI).
            uint32_t       specifierData;    //!< 24 bits, specified data, must be an IEEE OUI as described in IEEE 802.
            uint16_t       model;            //!< Device model.
            uint16_t       version;          //!< Device version.
            DescriptorList subDescriptors;   //!< Device-specific descriptors, not real MPEG/DVB descriptors.

            //!
            //! Default constructor.
            //!
            explicit CompatibilityDescriptor();

            //!
            //! Copy constructor.
            //! @param [in] other Another instance to copy.
            //!
            CompatibilityDescriptor(const CompatibilityDescriptor& other);

            //!
            //! Assignment operator.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            CompatibilityDescriptor& operator=(const CompatibilityDescriptor& other) = default;
        };

        //!
        //! List of compatibility descriptors.
        //! This list is equivalent to the compatibilityDescriptor()
        //! structure as defined in ISO/IEC 13818-6 and ETSI TS 102 006.
        //!
        typedef std::list<CompatibilityDescriptor> CompatibilityDescriptorList;

        //!
        //! Description of a platform.
        //!
        class TSDUCKDLL Platform : public EntryBase
        {
        public:
            DescriptorList target_descs;       //!< Target descriptor loop, describes the target platform.
            DescriptorList operational_descs;  //!< Operational descriptor loop, describes the operations on the target platform.

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit Platform(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            Platform(const AbstractTable* table, const Platform& other);

            //!
            //! Assignment operator.
            //! The parent table remains unchanged.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            Platform& operator=(const Platform& other) = default;

        private:
            // Inaccessible operations.
            Platform() = delete;
            Platform(const Platform&) = delete;
        };

        //!
        //! List of platforms.
        //!
        typedef EntryWithDescriptorsList<Platform> PlatformList;

        //!
        //! Description of a set of devices.
        //!
        class TSDUCKDLL Devices : public EntryBase
        {
        public:
            CompatibilityDescriptorList compatibilityDescriptor;   //!< The entries of the compatibilityDescriptor.
            PlatformList                platforms;                 //!< The list of platforms.

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit Devices(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            Devices(const AbstractTable* table, const Devices& other);

            //!
            //! Assignment operator.
            //! The parent table remains unchanged.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            Devices& operator=(const Devices& other) = default;

        private:
            // Inaccessible operations.
            Devices() = delete;
            Devices(const Devices&) = delete;
        };

        //!
        //! List of devicess.
        //!
        typedef EntryWithDescriptorsList<Devices> DevicesList;

        // UNT public members:
        uint8_t        action_type;       //!< Action type.
        uint32_t       OUI;               //!< OUI, 24 bits.
        uint8_t        processing_order;  //!< Processing order code.
        DescriptorList descs;             //!< Common descriptor loop.
        DevicesList    devices;           //!< List of sets of devices.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        UNT(uint8_t version = 0, bool is_current = true);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        UNT(const UNT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        UNT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
