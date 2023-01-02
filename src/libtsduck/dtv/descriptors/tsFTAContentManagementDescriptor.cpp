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

#include "tsFTAContentManagementDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"FTA_content_management_descriptor"
#define MY_CLASS ts::FTAContentManagementDescriptor
#define MY_DID ts::DID_FTA_CONTENT_MGMT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::FTAContentManagementDescriptor::FTAContentManagementDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    user_defined(false),
    do_not_scramble(false),
    control_remote_access_over_internet(0),
    do_not_apply_revocation(false)
{
}

void ts::FTAContentManagementDescriptor::clearContent()
{
    user_defined = false;
    do_not_scramble = false;
    control_remote_access_over_internet = 0;
    do_not_apply_revocation = false;
}

ts::FTAContentManagementDescriptor::FTAContentManagementDescriptor(DuckContext& duck, const Descriptor& desc) :
    FTAContentManagementDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::FTAContentManagementDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(user_defined);
    buf.putBits(0xFF, 3);
    buf.putBit(do_not_scramble);
    buf.putBits(control_remote_access_over_internet, 2);
    buf.putBit(do_not_apply_revocation);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::FTAContentManagementDescriptor::deserializePayload(PSIBuffer& buf)
{
    user_defined = buf.getBool();
    buf.skipBits(3);
    do_not_scramble = buf.getBool();
    buf.getBits(control_remote_access_over_internet, 2);
    do_not_apply_revocation = buf.getBool();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::FTAContentManagementDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"User-defined: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(3);
        disp << margin << UString::Format(u"Do not scramble: %s", {buf.getBool()}) << std::endl;
        disp << margin << "Access over Internet: " << DataName(MY_XML_NAME, u"RemoteAccessInternet", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;
        disp << margin << UString::Format(u"Do not apply revocation: %s", {buf.getBool()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::FTAContentManagementDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"user_defined", user_defined);
    root->setBoolAttribute(u"do_not_scramble", do_not_scramble);
    root->setIntAttribute(u"control_remote_access_over_internet", control_remote_access_over_internet);
    root->setBoolAttribute(u"do_not_apply_revocation", do_not_apply_revocation);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::FTAContentManagementDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(user_defined, u"user_defined", true) &&
           element->getBoolAttribute(do_not_scramble, u"do_not_scramble", true) &&
           element->getIntAttribute(control_remote_access_over_internet, u"control_remote_access_over_internet", true, 0, 0, 3) &&
           element->getBoolAttribute(do_not_apply_revocation, u"do_not_apply_revocation", true);
}
