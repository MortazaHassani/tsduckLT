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

#include "tsMultilingualServiceNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"multilingual_service_name_descriptor"
#define MY_CLASS ts::MultilingualServiceNameDescriptor
#define MY_DID ts::DID_MLINGUAL_SERVICE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MultilingualServiceNameDescriptor::MultilingualServiceNameDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
{
}

ts::MultilingualServiceNameDescriptor::MultilingualServiceNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultilingualServiceNameDescriptor()
{
    deserialize(duck, desc);
}

void ts::MultilingualServiceNameDescriptor::clearContent()
{
    entries.clear();
}

ts::MultilingualServiceNameDescriptor::Entry::Entry(const UString& lang_, const UString& prov_, const UString& name_) :
    language(lang_),
    service_provider_name(prov_),
    service_name(name_)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MultilingualServiceNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putLanguageCode(it.language);
        buf.putStringWithByteLength(it.service_provider_name);
        buf.putStringWithByteLength(it.service_name);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MultilingualServiceNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        buf.getLanguageCode(e.language);
        buf.getStringWithByteLength(e.service_provider_name);
        buf.getStringWithByteLength(e.service_name);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MultilingualServiceNameDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(4)) {
        disp << margin << "Language: " << buf.getLanguageCode();
        disp << ", provider: \"" << buf.getStringWithByteLength() << "\"";
        disp << ", service: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MultilingualServiceNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"language");
        e->setAttribute(u"code", it.language);
        e->setAttribute(u"service_provider_name", it.service_provider_name);
        e->setAttribute(u"service_name", it.service_name);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MultilingualServiceNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"language");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.language, u"code", true, u"", 3, 3) &&
             children[i]->getAttribute(entry.service_provider_name, u"service_provider_name", true) &&
             children[i]->getAttribute(entry.service_name, u"service_name", true);
        entries.push_back(entry);
    }
    return ok;
}
