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

#include "tsAreaBroadcastingInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"area_broadcasting_information_descriptor"
#define MY_CLASS ts::AreaBroadcastingInformationDescriptor
#define MY_DID ts::DID_ISDB_AREA_BCAST_INF
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AreaBroadcastingInformationDescriptor::AreaBroadcastingInformationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    stations()
{
}

void ts::AreaBroadcastingInformationDescriptor::clearContent()
{
    stations.clear();
}

ts::AreaBroadcastingInformationDescriptor::AreaBroadcastingInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    AreaBroadcastingInformationDescriptor()
{
    deserialize(duck, desc);
}

ts::AreaBroadcastingInformationDescriptor::Station::Station() :
    station_id(0),
    location_code(0),
    broadcast_signal_format(0),
    additional_station_info()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AreaBroadcastingInformationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(uint8_t(stations.size()));
    for (const auto& it : stations) {
        buf.putUInt24(it.station_id);
        buf.putUInt16(it.location_code);
        buf.putUInt8(it.broadcast_signal_format);
        buf.putUInt8(uint8_t(it.additional_station_info.size()));
        buf.putBytes(it.additional_station_info);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AreaBroadcastingInformationDescriptor::deserializePayload(PSIBuffer& buf)
{
    const size_t count = buf.getUInt8();
    for (size_t i = 0; i < count && buf.canRead(); ++i) {
        Station st;
        st.station_id = buf.getUInt24();
        st.location_code = buf.getUInt16();
        st.broadcast_signal_format = buf.getUInt8();
        const size_t len = buf.getUInt8();
        buf.getBytes(st.additional_station_info, len);
        stations.push_back(st);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AreaBroadcastingInformationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        size_t count = buf.getUInt8();
        while (count > 0 && buf.canReadBytes(7)) {
            disp << margin << UString::Format(u"- Station id: 0x%X (%<d)", {buf.getUInt24()}) << std::endl;
            disp << margin << UString::Format(u"  Location code: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << "  Broadcast signal format: " << DataName(MY_XML_NAME, u"BroadcastSignalFormat", buf.getUInt8(), NamesFlags::HEXA_FIRST) << std::endl;
            disp.displayPrivateData(u"Additional station info", buf, buf.getUInt8(), margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AreaBroadcastingInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : stations) {
        xml::Element* e = root->addElement(u"station");
        e->setIntAttribute(u"station_id", it.station_id, true);
        e->setIntAttribute(u"location_code", it.location_code, true);
        e->setIntAttribute(u"broadcast_signal_format", it.broadcast_signal_format, true);
        e->addHexaTextChild(u"additional_station_info", it.additional_station_info, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AreaBroadcastingInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xstation;
    bool ok = element->getChildren(xstation, u"station");

    for (auto it = xstation.begin(); ok && it != xstation.end(); ++it) {
        Station st;
        ok = (*it)->getIntAttribute(st.station_id, u"station_id", true, 0, 0, 0x00FFFFFF) &&
             (*it)->getIntAttribute(st.location_code, u"location_code", true) &&
             (*it)->getIntAttribute(st.broadcast_signal_format, u"broadcast_signal_format", true) &&
             (*it)->getHexaTextChild(st.additional_station_info, u"additional_station_info", false);
        stations.push_back(st);
    }
    return ok;
}
