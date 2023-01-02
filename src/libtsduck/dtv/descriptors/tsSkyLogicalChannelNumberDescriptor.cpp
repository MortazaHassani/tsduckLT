//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2019-2023, Anthony Delannoy
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

#include "tsSkyLogicalChannelNumberDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"sky_logical_channel_number_descriptor"
#define MY_CLASS ts::SkyLogicalChannelNumberDescriptor
#define MY_DID ts::DID_LOGICAL_CHANNEL_SKY
#define MY_PDS ts::PDS_BSKYB
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SkyLogicalChannelNumberDescriptor::SkyLogicalChannelNumberDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    entries(),
    region_id(0)
{
}

void ts::SkyLogicalChannelNumberDescriptor::clearContent()
{
    entries.clear();
    region_id = 0;
}

ts::SkyLogicalChannelNumberDescriptor::SkyLogicalChannelNumberDescriptor(DuckContext& duck, const Descriptor& desc) :
    SkyLogicalChannelNumberDescriptor()
{
    deserialize(duck, desc);
}

ts::SkyLogicalChannelNumberDescriptor::Entry::Entry(uint16_t id_, uint8_t type_, uint16_t cid_, uint16_t lcn_, uint16_t skyid_):
    service_id(id_),
    service_type(type_),
    channel_id(cid_),
    lcn(lcn_),
    sky_id(skyid_)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SkyLogicalChannelNumberDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(region_id);
    for (const auto& it : entries) {
        buf.putUInt16(it.service_id);
        buf.putUInt8(it.service_type);
        buf.putUInt16(it.channel_id);
        buf.putUInt16(it.lcn);
        buf.putUInt16(it.sky_id);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SkyLogicalChannelNumberDescriptor::deserializePayload(PSIBuffer& buf)
{
    region_id = buf.getUInt16();
    while (buf.canRead()) {
        Entry e;
        e.service_id = buf.getUInt16();
        e.service_type = buf.getUInt8();
        e.channel_id = buf.getUInt16();
        e.lcn = buf.getUInt16();
        e.sky_id = buf.getUInt16();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SkyLogicalChannelNumberDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Region Id: %5d (0x%<X)", {buf.getUInt16()}) << std::endl;
        while (buf.canReadBytes(9)) {
            disp << margin << UString::Format(u"Service Id: %5d (0x%<X)", {buf.getUInt16()});
            disp << ", Service Type: " << names::ServiceType(buf.getUInt8(), NamesFlags::FIRST);
            disp << UString::Format(u", Channel number: %3d", {buf.getUInt16()});
            disp << UString::Format(u", Lcn: %5d",  {buf.getUInt16()});
            disp << UString::Format(u", Sky Id: %5d (0x%<X)", {buf.getUInt16()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SkyLogicalChannelNumberDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"region_id", region_id, true);

    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it.service_id, true);
        e->setIntAttribute(u"service_type", it.service_type, true);
        e->setIntAttribute(u"channel_id", it.channel_id, true);
        e->setIntAttribute(u"logical_channel_number", it.lcn, false);
        e->setIntAttribute(u"sky_id", it.sky_id, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SkyLogicalChannelNumberDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(region_id, u"region_id", true, 0, 0x0000, 0xFFFF) &&
        element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute(entry.service_type, u"service_type", true, 0, 0x00, 0xFF) &&
             children[i]->getIntAttribute(entry.channel_id, u"channel_id", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute(entry.lcn, u"logical_channel_number", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute(entry.sky_id, u"sky_id", true, 0, 0x0000, 0xFFFF);
        entries.push_back(entry);
    }
    return ok;
}
