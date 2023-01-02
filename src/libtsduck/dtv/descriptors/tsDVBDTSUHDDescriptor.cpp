//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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

#include "tsDVBDTSUHDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DTS_UHD_descriptor"
#define MY_CLASS ts::DVBDTSUHDDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_DTS_UHD
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBDTSUHDDescriptor::DVBDTSUHDDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    DecoderProfileCode(0),
    FrameDurationCode(0),
    MaxPayloadCode(0),
    StreamIndex(0),
    codec_selector()
{
}

void ts::DVBDTSUHDDescriptor::clearContent()
{
    DecoderProfileCode = 0;
    FrameDurationCode = 0;
    MaxPayloadCode = 0;
    StreamIndex = 0;
    codec_selector.clear();
}

ts::DVBDTSUHDDescriptor::DVBDTSUHDDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBDTSUHDDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::DVBDTSUHDDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBDTSUHDDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(DecoderProfileCode, 6);
    buf.putBits(FrameDurationCode, 2);
    buf.putBits(MaxPayloadCode, 3);
    buf.putBits(0x00, 2); // must be b00 for DVB applications
    buf.putBits(StreamIndex, 3);
    buf.putBytes(codec_selector);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBDTSUHDDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(DecoderProfileCode, 6);
    buf.getBits(FrameDurationCode, 2);
    buf.getBits(MaxPayloadCode, 3);
    buf.skipBits(2); // must be b00 for DVB applications
    buf.getBits(StreamIndex, 3);
    codec_selector = buf.getBytes(buf.remainingReadBytes());
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBDTSUHDDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        uint8_t decoder_profile_code = buf.getBits<uint8_t>(6);
        disp << margin << "Decoder profile code: " << int(decoder_profile_code) << ", decoder profile: " << int(decoder_profile_code + 2) << std::endl;
        disp << margin << "Frame duration: " << DataName(MY_XML_NAME, u"FrameDurationCode", buf.getBits<uint8_t>(2), NamesFlags::VALUE|NamesFlags::DECIMAL);
        disp << ", max payload: " << DataName(MY_XML_NAME, u"MaxPayloadCode", buf.getBits<uint8_t>(3), NamesFlags::VALUE | NamesFlags::DECIMAL);
        buf.skipReservedBits(2, 0);
        disp << ", stream index: " << buf.getBits<uint16_t>(3) << std::endl;
        disp << margin << "Codec Selector: " << UString::Dump(buf.getBytes(), UString::SINGLE_LINE) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBDTSUHDDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"DecoderProfileCode", DecoderProfileCode);
    root->setIntAttribute(u"FrameDurationCode", FrameDurationCode);
    root->setIntAttribute(u"MaxPayloadCode", MaxPayloadCode);
    root->setIntAttribute(u"StreamIndex", StreamIndex);
    root->addHexaTextChild(u"codec_selector", codec_selector);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBDTSUHDDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return
        element->getIntAttribute(DecoderProfileCode, u"DecoderProfileCode", true, 0, 0, 127) &&
        element->getIntAttribute(FrameDurationCode, u"FrameDurationCode", true, 0, 0, 3) &&
        element->getIntAttribute(MaxPayloadCode, u"MaxPayloadCode", true, 0, 0, 7) &&
        element->getIntAttribute(StreamIndex, u"StreamIndex", true, 0, 0, 7) &&
        element->getHexaTextChild(codec_selector, u"codec_selector", false);
}
