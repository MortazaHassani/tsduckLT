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

#include "tsSSUEventNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SSU_event_name_descriptor"
#define MY_CLASS ts::SSUEventNameDescriptor
#define MY_DID ts::DID_UNT_SSU_EVENT_NAME
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSUEventNameDescriptor::SSUEventNameDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ISO_639_language_code(),
    name(),
    text()
{
}

ts::SSUEventNameDescriptor::SSUEventNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSUEventNameDescriptor()
{
    deserialize(duck, desc);
}

void ts::SSUEventNameDescriptor::clearContent()
{
    ISO_639_language_code.clear();
    name.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSUEventNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(ISO_639_language_code);
    buf.putStringWithByteLength(name);
    buf.putStringWithByteLength(text);
}

void ts::SSUEventNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(ISO_639_language_code);
    buf.getStringWithByteLength(name);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUEventNameDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Event name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        disp << margin << "Event text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSUEventNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    root->addElement(u"name")->addText(name);
    root->addElement(u"text")->addText(text);
}

bool ts::SSUEventNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
           element->getTextChild(name, u"name") &&
           element->getTextChild(text, u"text");
}
