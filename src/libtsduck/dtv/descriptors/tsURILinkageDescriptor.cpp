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

#include "tsURILinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"URI_linkage_descriptor"
#define MY_CLASS ts::URILinkageDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_URI_LINKAGE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::URILinkageDescriptor::URILinkageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    uri_linkage_type(0),
    uri(),
    min_polling_interval(0),
    private_data()
{
}

void ts::URILinkageDescriptor::clearContent()
{
    uri_linkage_type = 0;
    uri.clear();
    min_polling_interval = 0;
    private_data.clear();
}

ts::URILinkageDescriptor::URILinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    URILinkageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::URILinkageDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(uri_linkage_type);
    buf.putStringWithByteLength(uri);
    if (uri_linkage_type == 0x00 || uri_linkage_type == 0x01) {
        buf.putUInt16(min_polling_interval);
    }
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::deserializePayload(PSIBuffer& buf)
{
    uri_linkage_type = buf.getUInt8();
    buf.getStringWithByteLength(uri);
    if (uri_linkage_type == 0x00 || uri_linkage_type == 0x01) {
        min_polling_interval = buf.getUInt16();
    }
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const uint8_t type = buf.getUInt8();
        disp << margin << "URI linkage type: " << DataName(MY_XML_NAME, u"LinkageType", type, NamesFlags::HEXA_FIRST) << std::endl;
        disp << margin << "URI: " << buf.getStringWithByteLength() << std::endl;
        if ((type == 0x00 || type == 0x01) && buf.canReadBytes(2)) {
            const int interval = buf.getUInt16();
            disp << margin << UString::Format(u"Min polling interval: %d (%d seconds)", {interval, 2 * interval}) << std::endl;
        }
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"uri_linkage_type", uri_linkage_type, true);
    root->setAttribute(u"uri", uri);
    if (uri_linkage_type == 0x00 || uri_linkage_type == 0x01) {
        root->setIntAttribute(u"min_polling_interval", min_polling_interval);
    }
    if (!private_data.empty()) {
        root->addHexaTextChild(u"private_data", private_data);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::URILinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(uri_linkage_type, u"uri_linkage_type", true) &&
           element->getAttribute(uri, u"uri", true) &&
           element->getIntAttribute(min_polling_interval, u"min_polling_interval", uri_linkage_type <= 1) &&
           element->getHexaTextChild(private_data, u"private_data", false);
}
