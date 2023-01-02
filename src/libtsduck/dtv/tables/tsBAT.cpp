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

#include "tsBAT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"BAT"
#define MY_CLASS ts::BAT
#define MY_TID ts::TID_BAT
#define MY_PID ts::PID_BAT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::BAT::BAT(uint8_t vers, bool cur, uint16_t id) :
    AbstractTransportListTable(MY_TID, MY_XML_NAME, MY_STD, id, vers, cur),
    bouquet_id(_tid_ext)
{
}

ts::BAT::BAT(DuckContext& duck, const BinaryTable& table) :
    AbstractTransportListTable(duck, MY_TID, MY_XML_NAME, MY_STD, table),
    bouquet_id(_tid_ext)
{
}

ts::BAT::BAT(const BAT& other) :
    AbstractTransportListTable(other),
    bouquet_id(_tid_ext)
{
}

ts::BAT& ts::BAT::operator=(const BAT& other)
{
    if (&other != this) {
        // Assign super class but leave uint16_t& bouquet_id unchanged.
        AbstractTransportListTable::operator=(other);
    }
    return *this;
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::BAT::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the BAT section is limited to 1024 bytes in ETSI EN 300 468.
    return MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// A static method to display a BAT section.
//----------------------------------------------------------------------------

void ts::BAT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    // Display bouquet information
    disp << margin << UString::Format(u"Bouquet Id: %d (0x%<X)", {section.tableIdExtension()}) << std::endl;
    disp.displayDescriptorListWithLength(section, buf, margin, u"Bouquet information:");

    // Transport stream loop
    buf.skipReservedBits(4);
    buf.pushReadSizeFromLength(12); // transport_stream_loop_length
    while (buf.canReadBytes(6)) {
        const uint16_t tsid = buf.getUInt16();
        const uint16_t nwid = buf.getUInt16();
        disp << margin << UString::Format(u"Transport Stream Id: %d (0x%<X), Original Network Id: %d (0x%<X)", {tsid, nwid}) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin);
    }
    buf.popState(); // transport_stream_loop_length
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::BAT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"bouquet_id", bouquet_id, true);
    descs.toXML(duck, root);

    for (auto& tp : transports) {
        xml::Element* e = root->addElement(u"transport_stream");
        e->setIntAttribute(u"transport_stream_id", tp.first.transport_stream_id, true);
        e->setIntAttribute(u"original_network_id", tp.first.original_network_id, true);
        if (tp.second.preferred_section >= 0) {
            e->setIntAttribute(u"preferred_section", tp.second.preferred_section, false);
        }
        tp.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::BAT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(bouquet_id, u"bouquet_id", true, 0, 0x0000, 0xFFFF) &&
        descs.fromXML(duck, children, element, u"transport_stream");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        TransportStreamId ts;
        ok = children[index]->getIntAttribute(ts.transport_stream_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntAttribute(ts.original_network_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
             transports[ts].descs.fromXML(duck, children[index]);
        if (ok && children[index]->hasAttribute(u"preferred_section")) {
            ok = children[index]->getIntAttribute(transports[ts].preferred_section, u"preferred_section", true, 0, 0, 255);
        }
        else {
            transports[ts].preferred_section = -1;
        }
    }
    return ok;
}
