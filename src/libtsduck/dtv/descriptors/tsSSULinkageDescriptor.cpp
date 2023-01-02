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

#include "tsSSULinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

// This is not a fully registered descriptor. This is just a specific case of linkage_descriptor.
// It has no specific XML representation. It cannot be converted from XML because it has no specific
// syntax. It can be converted to XML, as a <linkage_descriptor>.

#define MY_XML_NAME u"linkage_descriptor"
#define MY_DID ts::DID_LINKAGE
#define MY_STD ts::Standards::DVB


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSULinkageDescriptor::SSULinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    entries(),
    private_data()
{
}

ts::SSULinkageDescriptor::SSULinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint32_t oui) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    entries(),
    private_data()
{
    entries.push_back(Entry(oui));
}

ts::SSULinkageDescriptor::SSULinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSULinkageDescriptor()
{
    deserialize(duck, desc);
}

void ts::SSULinkageDescriptor::clearContent()
{
    ts_id = 0;
    onetw_id = 0;
    service_id = 0;
    entries.clear();
    private_data.clear();
}

ts::SSULinkageDescriptor::SSULinkageDescriptor(DuckContext& duck, const ts::LinkageDescriptor& desc) :
    SSULinkageDescriptor()
{
    if (!desc.isValid() || desc.linkage_type != LINKAGE_SSU) {
        invalidate();
    }
    else {
        // Convert using serialization / deserialization.
        Descriptor bin;
        desc.serialize(duck, bin);
        deserialize(duck, bin);
    }
}

ts::SSULinkageDescriptor::Entry::Entry(uint32_t oui_) :
    oui(oui_),
    selector()
{
}


//----------------------------------------------------------------------------
// Convert to a linkage_descriptor.
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::toLinkageDescriptor(DuckContext& duck, ts::LinkageDescriptor& desc) const
{
    if (isValid()) {
        // Convert using serialization / deserialization.
        Descriptor bin;
        serialize(duck, bin);
        desc.deserialize(duck, bin);
    }
    else {
        desc.invalidate();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(ts_id);
    buf.putUInt16(onetw_id);
    buf.putUInt16(service_id);
    buf.putUInt8(LINKAGE_SSU);
    buf.pushWriteSequenceWithLeadingLength(8); // OUI_data_length
    for (const auto& it : entries) {
        buf.putUInt24(it.oui);
        buf.putUInt8(uint8_t(it.selector.size()));
        buf.putBytes(it.selector);
    }
    buf.popState(); // update OUI_data_length
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::deserializePayload(PSIBuffer& buf)
{
    ts_id = buf.getUInt16();
    onetw_id = buf.getUInt16();
    service_id = buf.getUInt16();

    if (buf.getUInt8() != LINKAGE_SSU) {
        // Not an SSU linkage_descriptor.
        buf.setUserError();
    }

    buf.pushReadSizeFromLength(8); // OUI_data_length
    while (buf.canRead()) {
        Entry entry(buf.getUInt24());
        const size_t len = buf.getUInt8();
        buf.getBytes(entry.selector, len);
        entries.push_back(entry);
    }
    buf.popState(); // end of OUI_data_length
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::buildXML(DuckContext& duck, xml::Element* parent) const
{
    // There is no specific representation of this descriptor.
    // Convert to a linkage_descriptor.
    LinkageDescriptor desc;
    toLinkageDescriptor(duck, desc);
    desc.buildXML(duck, parent);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SSULinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    // There is no specific representation of this descriptor.
    // We cannot be called since there is no registration in the XML factory.
    element->report().error(u"Internal error, there is no XML representation for SSULinkageDescriptor");
    return false;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    LinkageDescriptor::DisplayDescriptor(disp, buf, margin, did, tid, pds);
}
