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

#include "tsContentAvailabilityDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"content_availability_descriptor"
#define MY_CLASS ts::ContentAvailabilityDescriptor
#define MY_DID ts::DID_ISDB_CONTENT_AVAIL
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ContentAvailabilityDescriptor::ContentAvailabilityDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    copy_restriction_mode(false),
    image_constraint_token(false),
    retention_mode(false),
    retention_state(0),
    encryption_mode(false),
    reserved_future_use()
{
}

void ts::ContentAvailabilityDescriptor::clearContent()
{
    copy_restriction_mode = false;
    image_constraint_token = false;
    retention_mode = false;
    retention_state = 0;
    encryption_mode = false;
    reserved_future_use.clear();
}

ts::ContentAvailabilityDescriptor::ContentAvailabilityDescriptor(DuckContext& duck, const Descriptor& desc) :
    ContentAvailabilityDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ContentAvailabilityDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(1);
    buf.putBit(copy_restriction_mode);
    buf.putBit(image_constraint_token);
    buf.putBit(retention_mode);
    buf.putBits(retention_state, 3);
    buf.putBit(encryption_mode);
    buf.putBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ContentAvailabilityDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(1);
    copy_restriction_mode = buf.getBool();
    image_constraint_token = buf.getBool();
    retention_mode = buf.getBool();
    buf.getBits(retention_state, 3);
    encryption_mode = buf.getBool();
    buf.getBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ContentAvailabilityDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(1);
        disp << margin << UString::Format(u"Copy restriction mode: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"Image constraint toke: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"Retention mode: %s", {buf.getBool()}) << std::endl;
        disp << margin << "Retention state: " << DataName(MY_XML_NAME, u"RetentionState", buf.getBits<uint8_t>(3), NamesFlags::DECIMAL_FIRST) << std::endl;
        disp << margin << UString::Format(u"Encryption mode: %s", {buf.getBool()}) << std::endl;
        disp.displayPrivateData(u"Reserved future use", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ContentAvailabilityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"copy_restriction_mode", copy_restriction_mode);
    root->setBoolAttribute(u"image_constraint_token", image_constraint_token);
    root->setBoolAttribute(u"retention_mode", retention_mode);
    root->setIntAttribute(u"retention_state", retention_state);
    root->setBoolAttribute(u"encryption_mode", encryption_mode);
    root->addHexaTextChild(u"reserved_future_use", reserved_future_use, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ContentAvailabilityDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(copy_restriction_mode, u"copy_restriction_mode", true) &&
           element->getBoolAttribute(image_constraint_token, u"image_constraint_token", true) &&
           element->getBoolAttribute(retention_mode, u"retention_mode", true) &&
           element->getIntAttribute(retention_state, u"retention_state", true, 0, 0, 7) &&
           element->getBoolAttribute(encryption_mode, u"encryption_mode", true) &&
           element->getHexaTextChild(reserved_future_use, u"reserved_future_use", false, 0, 253);
}
