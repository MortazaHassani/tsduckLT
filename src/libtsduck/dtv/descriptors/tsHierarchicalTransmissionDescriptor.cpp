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

#include "tsHierarchicalTransmissionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"hierarchical_transmission_descriptor"
#define MY_CLASS ts::HierarchicalTransmissionDescriptor
#define MY_DID ts::DID_ISDB_HIERARCH_TRANS
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HierarchicalTransmissionDescriptor::HierarchicalTransmissionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    high_quality(false),
    reference_PID(PID_NULL)
{
}

void ts::HierarchicalTransmissionDescriptor::clearContent()
{
    high_quality = false;
    reference_PID = PID_NULL;
}

ts::HierarchicalTransmissionDescriptor::HierarchicalTransmissionDescriptor(DuckContext& duck, const Descriptor& desc) :
    HierarchicalTransmissionDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HierarchicalTransmissionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 7);
    buf.putBit(high_quality);
    buf.putPID(reference_PID);
}

void ts::HierarchicalTransmissionDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(7);
    high_quality = buf.getBool();
    reference_PID = buf.getPID();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HierarchicalTransmissionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        buf.skipBits(7);
        disp << margin << "Quality level: " << (buf.getBool() ? u"high" : u"low") << std::endl;
        disp << margin << UString::Format(u"Reference PID: 0x%X (%<d)", {buf.getPID()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HierarchicalTransmissionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"high_quality", high_quality);
    root->setIntAttribute(u"reference_PID", reference_PID, true);
}

bool ts::HierarchicalTransmissionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(high_quality, u"high_quality", true) &&
           element->getIntAttribute<PID>(reference_PID, u"reference_PID", true, 0, 0, 0x1FFF);
}
