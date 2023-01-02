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

#include "tsCDT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"CDT"
#define MY_CLASS ts::CDT
#define MY_TID ts::TID_CDT
#define MY_PID ts::PID_CDT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::CDT::CDT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    download_data_id(0),
    original_network_id(0),
    data_type(0),
    descs(this),
    data_module()
{
}

ts::CDT::CDT(const CDT& other) :
    AbstractLongTable(other),
    download_data_id(other.download_data_id),
    original_network_id(other.original_network_id),
    data_type(other.data_type),
    descs(this, other.descs),
    data_module(other.data_module)
{
}

ts::CDT::CDT(DuckContext& duck, const BinaryTable& table) :
    CDT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::CDT::tableIdExtension() const
{
    return download_data_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::CDT::clearContent()
{
    download_data_id = 0;
    original_network_id = 0;
    data_type = 0;
    descs.clear();
    data_module.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CDT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    download_data_id = section.tableIdExtension();
    original_network_id = buf.getUInt16();
    data_type = buf.getUInt8();
    buf.getDescriptorListWithLength(descs);
    buf.getBytesAppend(data_module);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CDT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt16(original_network_id);
    buf.putUInt8(data_type);
    buf.pushState();

    // Loop on new sections until all descriptors and data bytes are gone.
    size_t desc_index = 0;
    size_t data_index = 0;
    while (table.sectionCount() == 0 || desc_index < descs.size() || data_index < data_module.size()) {
        desc_index = buf.putPartialDescriptorListWithLength(descs, desc_index);
        data_index += buf.putBytes(data_module, data_index, std::min(data_module.size() - data_index, buf.remainingWriteBytes()));
        addOneSection(table, buf);
    }
}


//----------------------------------------------------------------------------
// A static method to display a CDT section.
//----------------------------------------------------------------------------

void ts::CDT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Download data id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;
    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Data type: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin, u"Common descriptors:");
        if (buf.canRead()) {
            disp.displayPrivateData(u"Data module", buf, NPOS, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CDT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"download_data_id", download_data_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setIntAttribute(u"data_type", data_type, true);
    descs.toXML(duck, root);
    root->addHexaTextChild(u"data_module", data_module, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CDT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xdata;
    return element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
           element->getBoolAttribute(is_current, u"current", false, true) &&
           element->getIntAttribute(download_data_id, u"download_data_id", true) &&
           element->getIntAttribute(original_network_id, u"original_network_id", true) &&
           element->getIntAttribute(data_type, u"data_type", true) &&
           descs.fromXML(duck, xdata, element, u"data_module") &&
           element->getHexaTextChild(data_module, u"data_module");
}
