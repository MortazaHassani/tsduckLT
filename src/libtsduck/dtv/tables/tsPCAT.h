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
//!  Representation of an ISDB Partial Content Announcement Table (PCAT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ISDB Partial Content Announcement Table (PCAT).
    //! @see ARIB STD-B10, Part 2, 5.2.12
    //! @ingroup table
    //!
    class TSDUCKDLL PCAT : public AbstractLongTable
    {
    public:
        //!
        //! Schedule entry.
        //!
        class TSDUCKDLL Schedule
        {
        public:
            Schedule();         //!< Constructor
            Time   start_time;  //!< Event start_time in UTC (or JST in Japan).
            Second duration;    //!< Event duration in seconds.
        };

        //!
        //! List of schedule entries.
        //!
        typedef std::list<Schedule> ScheduleList;

        //!
        //! Content version entry.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL ContentVersion : public EntryWithDescriptors
        {
        public:
            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            ContentVersion(const AbstractTable* table);

            uint16_t     content_version;        //!< Content version.
            uint16_t     content_minor_version;  //!< Content minor version.
            uint8_t      version_indicator;      //!< 2 bits, how to use the version.
            ScheduleList schedules;              //!< List of schedules.

        private:
            // Inaccessible operations.
            ContentVersion() = delete;
            ContentVersion(const ContentVersion&) = delete;
        };

        //!
        //! List of content versions.
        //!
        typedef EntryWithDescriptorsList<ContentVersion> ContentVersionList;

        // PCAT public members:
        uint16_t           service_id;           //!< Service id.
        uint16_t           transport_stream_id;  //!< Transport stream id.
        uint16_t           original_network_id;  //!< Original network id.
        uint32_t           content_id;           //!< Content id.
        ContentVersionList versions;             //!< List of content versions.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        PCAT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        PCAT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        PCAT(const PCAT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        PCAT& operator=(const PCAT& other) = default;

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
