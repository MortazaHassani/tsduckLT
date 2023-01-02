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

#include "tsSystemManagementDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"system_management_descriptor"
#define MY_CLASS ts::SystemManagementDescriptor
#define MY_DID ts::DID_ISDB_SYSTEM_MGMT
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SystemManagementDescriptor::SystemManagementDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    broadcasting_flag(0),
    broadcasting_identifier(0),
    additional_broadcasting_identification(0),
    additional_identification_info()
{
}

void ts::SystemManagementDescriptor::clearContent()
{
    broadcasting_flag = 0;
    broadcasting_identifier = 0;
    additional_broadcasting_identification = 0;
    additional_identification_info.clear();
}

ts::SystemManagementDescriptor::SystemManagementDescriptor(DuckContext& duck, const Descriptor& desc) :
    SystemManagementDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SystemManagementDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(broadcasting_flag, 2);
    buf.putBits(broadcasting_identifier, 6);
    buf.putUInt8(additional_broadcasting_identification);
    buf.putBytes(additional_identification_info);
}

void ts::SystemManagementDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(broadcasting_flag, 2);
    buf.getBits(broadcasting_identifier, 6);
    additional_broadcasting_identification = buf.getUInt8();
    buf.getBytes(additional_identification_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SystemManagementDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Broadcasting flag: " << DataName(MY_XML_NAME, u"Broadcasting", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;
        disp << margin << "Broadcasting identifier: " << DataName(MY_XML_NAME, u"Identifier", buf.getBits<uint8_t>(6), NamesFlags::DECIMAL_FIRST) << std::endl;
        disp << margin << UString::Format(u"Additional broadcasting id: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp.displayPrivateData(u"Additional identification info", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SystemManagementDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"broadcasting_flag", broadcasting_flag);
    root->setIntAttribute(u"broadcasting_identifier", broadcasting_identifier, true);
    root->setIntAttribute(u"additional_broadcasting_identification", additional_broadcasting_identification, true);
    root->addHexaTextChild(u"additional_identification_info", additional_identification_info, true);
}

bool ts::SystemManagementDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(broadcasting_flag, u"broadcasting_flag", true, 0, 0, 3) &&
           element->getIntAttribute(broadcasting_identifier, u"broadcasting_identifier", true, 0, 0, 0x3F) &&
           element->getIntAttribute(additional_broadcasting_identification, u"additional_broadcasting_identification", true) &&
           element->getHexaTextChild(additional_identification_info, u"additional_identification_info", false, 0, 253);
}
