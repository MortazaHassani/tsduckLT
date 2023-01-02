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
//!  Representation of a generic linkage_descriptor.
//!  Specialized classes exist, depending on the linkage type.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Linkage type values (in linkage_descriptor)
    //!
    enum : uint8_t {
        LINKAGE_INFO            = 0x01, //!< Information service
        LINKAGE_EPG             = 0x02, //!< EPG service
        LINKAGE_CA_REPLACE      = 0x03, //!< CA replacement service
        LINKAGE_TS_NIT_BAT      = 0x04, //!< TS containing complet network/bouquet SI
        LINKAGE_SERVICE_REPLACE = 0x05, //!< Service replacement service
        LINKAGE_DATA_BROADCAST  = 0x06, //!< Data broadcast service
        LINKAGE_RCS_MAP         = 0x07, //!< RCS map
        LINKAGE_HAND_OVER       = 0x08, //!< Mobile hand-over
        LINKAGE_SSU             = 0x09, //!< System software update service
        LINKAGE_SSU_TABLE       = 0x0A, //!< TS containing SSU BAT or NIT
        LINKAGE_IP_NOTIFY       = 0x0B, //!< IP/MAC notification service
        LINKAGE_INT_BAT_NIT     = 0x0C, //!< TS containing INT BAT or NIT
        LINKAGE_EVENT           = 0x0D, //!< Event linkage
        LINKAGE_EXT_EVENT_MIN   = 0x0E, //!< Extented event linkage, first value
        LINKAGE_EXT_EVENT_MAX   = 0x1F, //!< Extented event linkage, last value
    };

    //!
    //! Representation of a generic linkage_descriptor.
    //! Specialized classes exist, depending on the linkage type.
    //! @see ETSI EN 300 468, 6.2.19.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL LinkageDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Definition of mobile_hand-over_info when linkage_type == LINKAGE_HAND_OVER
        //!
        class TSDUCKDLL MobileHandoverInfo
        {
        public:
            MobileHandoverInfo();         //!< Default constructor.
            void clear();                 //!< Clear object content.

            uint8_t  handover_type;       //!< Hand-over type, 4 bits.
            uint8_t  origin_type;         //!< Origin type, 0 = NIT, 1 = SDT.
            uint16_t network_id;          //!< Network when handover_type == 0x01, 0x02, 0x03.
            uint16_t initial_service_id;  //!< Initial service when origin_type == 0x00
        };

        //!
        //! Definition of event_linkage_info when linkage_type == LINKAGE_EVENT
        //!
        class TSDUCKDLL EventLinkageInfo
        {
        public:
            EventLinkageInfo();         //!< Default constructor.
            void clear();               //!< Clear object content.

            uint16_t target_event_id;   //!< Target event.
            bool     target_listed;     //!< Service is listed in SDT.
            bool     event_simulcast;   //!< Target and source event are simulcast.
        };

        //!
        //! Definition of extended_event_linkage_info when linkage_type in LINKAGE_EXT_EVENT_MIN .. LINKAGE_EXT_EVENT_MAX
        //!
        class TSDUCKDLL ExtendedEventLinkageInfo
        {
        public:
            ExtendedEventLinkageInfo();                      //!< Default constructor.
            void clear();                                    //!< Clear object content.

            uint16_t           target_event_id;              //!< Target event.
            bool               target_listed;                //!< Service is listed in SDT.
            bool               event_simulcast;              //!< Target and source event are simulcast.
            uint8_t            link_type;                    //!< Link type, 2 bits.
            uint8_t            target_id_type;               //!< Target type, 2 bits.
            uint16_t           user_defined_id;              //!< User-defined id when target_id_type == 3
            uint16_t           target_transport_stream_id;   //!< Target TS when target_id_type == 1
            Variable<uint16_t> target_original_network_id;   //!< Optional target original network.
            Variable<uint16_t> target_service_id;            //!< Optional target service.
        };

        //!
        //! List of extended event info.
        //!
        typedef std::list<ExtendedEventLinkageInfo> ExtendedEventLinkageList;

        // LinkageDescriptor public members:
        uint16_t                 ts_id;                        //!< Transport stream id.
        uint16_t                 onetw_id;                     //!< Original network id.
        uint16_t                 service_id;                   //!< Service id.
        uint8_t                  linkage_type;                 //!< Linkage type, LINKAGE_* constants, eg ts::LINKAGE_INFO.
        MobileHandoverInfo       mobile_handover_info;         //!< mobile_hand-over_info when linkage_type == LINKAGE_HAND_OVER.
        EventLinkageInfo         event_linkage_info;           //!< event_linkage_info when linkage_type == LINKAGE_EVENT.
        ExtendedEventLinkageList extended_event_linkage_info;  //!< extended_event_linkage_info when linkage_type in LINKAGE_EXT_EVENT_MIN .. LINKAGE_EXT_EVENT_MAX.
        ByteBlock                private_data;                 //!< Private data, depends on linkage type.

        //!
        //! Default constructor
        //! @param [in] ts Transport stream id.
        //! @param [in] onetw Original network id.
        //! @param [in] service Service id.
        //! @param [in] ltype Linkage type.
        //!
        LinkageDescriptor(uint16_t ts = 0, uint16_t onetw = 0, uint16_t service = 0, uint8_t ltype = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        LinkageDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

        // The specific cases of linkage_descriptor reuse buildXML().
        friend class SSULinkageDescriptor;

    private:
        // Display linkage private data of various types.
        // Fields data and size are updated.
        static void DisplayPrivateMobileHandover(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
        static void DisplayPrivateSSU(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
        static void DisplayPrivateTableSSU(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
        static void DisplayPrivateINT(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
        static void DisplayPrivateDeferredINT(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
    };
}
