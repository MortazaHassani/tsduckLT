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

#include "tsDVBAC4Descriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DVB_AC4_descriptor"
#define MY_XML_NAME_LEGACY u"AC4_descriptor"
#define MY_CLASS ts::DVBAC4Descriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_AC4
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBAC4Descriptor::DVBAC4Descriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0, MY_XML_NAME_LEGACY),
    ac4_dialog_enhancement_enabled(),
    ac4_channel_mode(),
    ac4_dsi_toc(),
    additional_info()
{
}

void ts::DVBAC4Descriptor::clearContent()
{
    ac4_dialog_enhancement_enabled.clear();
    ac4_channel_mode.clear();
    ac4_dsi_toc.clear();
    additional_info.clear();
}

ts::DVBAC4Descriptor::DVBAC4Descriptor(DuckContext& duck, const Descriptor& desc) :
    DVBAC4Descriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::DVBAC4Descriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(ac4_dialog_enhancement_enabled.set() && ac4_channel_mode.set());
    buf.putBit(!ac4_dsi_toc.empty());
    buf.putBits(0, 6); // reserved bits are zero here
    if (ac4_dialog_enhancement_enabled.set() && ac4_channel_mode.set()) {
        buf.putBit(ac4_dialog_enhancement_enabled.value());
        buf.putBits(ac4_channel_mode.value(), 2);
        buf.putBits(0, 5); // reserved bits are zero here
    }
    if (!ac4_dsi_toc.empty()) {
        buf.putUInt8(uint8_t(ac4_dsi_toc.size()));
        buf.putBytes(ac4_dsi_toc);
    }
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::deserializePayload(PSIBuffer& buf)
{
    const bool ac4_config_flag = buf.getBool();
    const bool ac4_toc_flag = buf.getBool();
    buf.skipBits(6);
    if (ac4_config_flag) {
        ac4_dialog_enhancement_enabled = buf.getBool();
        buf.getBits(ac4_channel_mode, 2);
        buf.skipBits(5);
    }
    if (ac4_toc_flag) {
        const size_t len = buf.getUInt8();
        buf.getBytes(ac4_dsi_toc, len);
    }
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const bool ac4_config_flag = buf.getBool();
        const bool ac4_toc_flag = buf.getBool();
        buf.skipBits(6);
        if (ac4_config_flag && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"Dialog enhancement enabled: %d", {buf.getBool()});
            disp << ", channel mode: " << DataName(MY_XML_NAME, u"ChannelMode", buf.getBits<uint8_t>(2), NamesFlags::FIRST) << std::endl;
            buf.skipBits(5);
        }
        if (ac4_toc_flag && buf.canReadBytes(1)) {
            disp.displayPrivateData(u"AC-4 TOC (in DSI)", buf, buf.getUInt8(), margin);
        }
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBAC4Descriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setOptionalBoolAttribute(u"ac4_dialog_enhancement_enabled", ac4_dialog_enhancement_enabled);
    root->setOptionalIntAttribute(u"ac4_channel_mode", ac4_channel_mode);
    if (!ac4_dsi_toc.empty()) {
        root->addHexaTextChild(u"ac4_dsi_toc", ac4_dsi_toc);
    }
    if (!additional_info.empty()) {
        root->addHexaTextChild(u"additional_info", additional_info);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBAC4Descriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getOptionalBoolAttribute(ac4_dialog_enhancement_enabled, u"ac4_dialog_enhancement_enabled") &&
           element->getOptionalIntAttribute(ac4_channel_mode, u"ac4_channel_mode", 0, 3) &&
           element->getHexaTextChild(ac4_dsi_toc, u"ac4_dsi_toc", false, 0, MAX_DESCRIPTOR_SIZE - 6) &&
           element->getHexaTextChild(additional_info, u"additional_info", false, 0, MAX_DESCRIPTOR_SIZE - 6 - ac4_dsi_toc.size());
}
