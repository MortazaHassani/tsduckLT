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
//!  Base class for ATSC Virtual Channel Table (VCT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsEnumeration.h"
#include "tsService.h"

namespace ts {
    //!
    //! Base class for ATSC Virtual Channel Table (VCT).
    //! Existing concrete subclasses are TVCT (terrestrial) and CVCT (cable).
    //! @see ATSC A/65, section 6.3.
    //! @ingroup table
    //!
    class TSDUCKDLL VCT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a virtual channel.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Channel : public EntryWithDescriptors
        {
        public:
            UString  short_name;             //!< Channel short name (up to 7 characters).
            uint16_t major_channel_number;   //!< 10 bits, major channel number.
            uint16_t minor_channel_number;   //!< 10 bits, minor channel number.
            uint8_t  modulation_mode;        //!< Modulation, can be anolog, terrestrial (VSB) or cable (QAM).
            uint32_t carrier_frequency;      //!< Should be a carrier frequency but specified as zero by ATSC (?).
            uint16_t channel_TSID;           //!< Transport stream id of the TS carrying the channel.
            uint16_t program_number;         //!< Program number (aka. service id) of the channel.
            uint8_t  ETM_location;           //!< 2 bits, location of Extended Text Message.
            bool     access_controlled;      //!< Under access control.
            bool     hidden;                 //!< Hidden service.
            bool     hide_guide;             //!< Hide associated program guide information.
            uint8_t  service_type;           //!< 6 bits, ATSC service type.
            uint16_t source_id;              //!< ATSC source id.
            // The following fields are valid only in the context of a CVCT.
            uint8_t  path_select;            //!< Either 0 (Path 1) or 1 (Path 2). Warning: CVCT only.
            bool     out_of_band;            //!< Out-of-band service. Warning: CVCT only.

            //!
            //! Constructor.
            //! @param [in] table Parent VCT.
            //!
            explicit Channel(const AbstractTable* table);

            //!
            //! Collect all informations about the service.
            //! @param [in,out] service A service description to update.
            //!
            void updateService(Service& service) const;

        private:
            // Inaccessible operations.
            Channel() = delete;
            Channel(const Channel&) = delete;
        };

        //!
        //! List of channels.
        //!
        typedef EntryWithDescriptorsList<Channel> ChannelList;

        // VCT public members:
        uint8_t        protocol_version;     //!< ATSC protocol version.
        uint16_t       transport_stream_id;  //!< Transport stream id.
        ChannelList    channels;             //!< List of channels which are described in this VCT.
        DescriptorList descs;                //!< Program-level descriptor list.

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        VCT(const VCT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        VCT& operator=(const VCT& other) = default;

        //!
        //! Search a service by id.
        //! @param [in] id Service id to search.
        //! @param [in] same_ts If true, only look for services in the same TS as the VCT.
        //! @return An iterator to the service if found, channels.end() if not found.
        //!
        ChannelList::const_iterator findService(uint16_t id, bool same_ts = true) const;

        //!
        //! Search a service by major.minor id.
        //! @param [in] major Major id to search.
        //! @param [in] minor Minor id to search.
        //! @param [in] same_ts If true, only look for services in the same TS as the VCT.
        //! @return An iterator to the service if found, channels.end() if not found.
        //!
        ChannelList::const_iterator findService(uint16_t major, uint16_t minor, bool same_ts = true) const;

        //!
        //! Search a service by name.
        //! @param [in] name Service name to search.
        //! @param [in] exact_match If true, the service name must be exactly
        //! identical to @a name. If it is false, the search is case-insensitive
        //! and blanks are ignored.
        //! @param [in] same_ts If true, only look for services in the same TS as the VCT.
        //! @return An iterator to the service if found, channels.end() if not found.
        //!
        ChannelList::const_iterator findService(const UString& name, bool exact_match = false, bool same_ts = true) const;

        //!
        //! Search a service by name or ATSC major.minor, using a ts::Service class.
        //! @param [in,out] service Service description. Use service name or major.minor to search.
        //! Set the service id if found.
        //! @param [in] exact_match If true, the service name must be exactly
        //! identical to the name in @a service. If it is false, the search is case-insensitive
        //! and blanks are ignored.
        //! @param [in] same_ts If true, only look for services in the same TS as the VCT.
        //! @return True if the service is found, false if not found.
        //!
        bool findService(Service& service, bool exact_match = false, bool same_ts = true) const;

        //!
        //! Collect all informations about all services in the VCT.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] services A list of service descriptions. Existing services
        //! are updated with the informations from the SDT. New entries are created for
        //! other services.
        //!
        void updateServices(DuckContext& duck, ServiceList& services) const;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] version_ Table version number.
        //! @param [in] is_current_ True if table is current, false if table is next.
        //!
        VCT(TID tid, const UChar* xml_name, Standards standards, uint8_t version_, bool is_current_);

        // Inherited methods
        virtual void clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Internal version of find by Service class.
        ChannelList::const_iterator findServiceInternal(Service& service, bool exact_match, bool same_ts) const;

        // XML values for modulation mode and service_type.
        static const Enumeration ModulationModeEnum;
        static const Enumeration ServiceTypeEnum;
    };
}
