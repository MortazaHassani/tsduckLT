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

#include "tsS2SatelliteDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"S2_satellite_delivery_system_descriptor"
#define MY_CLASS ts::S2SatelliteDeliverySystemDescriptor
#define MY_DID ts::DID_S2_SAT_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::S2SatelliteDeliverySystemDescriptor::S2SatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_S2, MY_XML_NAME),
    backwards_compatibility_indicator(false),
    TS_GS_mode(0),
    scrambling_sequence_index(),
    input_stream_identifier(),
    timeslice_number()
{
}

ts::S2SatelliteDeliverySystemDescriptor::S2SatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    S2SatelliteDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::S2SatelliteDeliverySystemDescriptor::clearContent()
{
    backwards_compatibility_indicator = false;
    TS_GS_mode = 0;
    scrambling_sequence_index.clear();
    input_stream_identifier.clear();
    timeslice_number.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::S2SatelliteDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(scrambling_sequence_index.set());
    buf.putBit(input_stream_identifier.set());
    buf.putBit(backwards_compatibility_indicator);
    buf.putBit(!timeslice_number.set());
    buf.putBits(0xFF, 2);
    buf.putBits(TS_GS_mode, 2);
    if (scrambling_sequence_index.set()) {
        buf.putBits(0xFF, 6);
        buf.putBits(scrambling_sequence_index.value(), 18);
    }
    if (input_stream_identifier.set()) {
        buf.putUInt8(input_stream_identifier.value());
    }
    if (timeslice_number.set()) {
        buf.putUInt8(timeslice_number.value());
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::S2SatelliteDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    const bool scrambling_sequence_selector = buf.getBool();
    const bool multiple_input_stream_flag = buf.getBool();
    backwards_compatibility_indicator = buf.getBool();
    const bool not_timeslice_flag = buf.getBool();
    buf.skipReservedBits(2);
    buf.getBits(TS_GS_mode, 2);

    if (scrambling_sequence_selector) {
        buf.skipReservedBits(6);
        buf.getBits(scrambling_sequence_index, 18);
    }
    if (multiple_input_stream_flag) {
        input_stream_identifier = buf.getUInt8();
    }
    if (!not_timeslice_flag) {
        timeslice_number = buf.getUInt8();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::S2SatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const bool scrambling_sequence_selector = buf.getBool();
        const bool multiple_input_stream_flag = buf.getBool();
        disp << margin << UString::Format(u"Backward compatibility: %s", {buf.getBool()}) << std::endl;
        const bool not_timeslice_flag = buf.getBool();
        buf.skipReservedBits(2);
        disp << margin << "TS/GS mode: " << DataName(MY_XML_NAME, u"TSGSS2Mode", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;

        if (scrambling_sequence_selector && buf.canReadBytes(3)) {
            buf.skipReservedBits(6);
            disp << margin << UString::Format(u"Scrambling sequence index: 0x%05X", {buf.getBits<uint32_t>(18)}) << std::endl;
        }
        if (multiple_input_stream_flag && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"Input stream identifier: 0x%X", {buf.getUInt8()}) << std::endl;
        }
        if (!not_timeslice_flag && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"Time slice number: 0x%X", {buf.getUInt8()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::S2SatelliteDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    if (backwards_compatibility_indicator) {
        root->setBoolAttribute(u"backwards_compatibility", backwards_compatibility_indicator);
    }
    if (TS_GS_mode != 3) {
        root->setIntAttribute(u"TS_GS_mode", TS_GS_mode);
    }
    root->setOptionalIntAttribute(u"scrambling_sequence_index", scrambling_sequence_index, true);
    root->setOptionalIntAttribute(u"input_stream_identifier", input_stream_identifier, true);
    root->setOptionalIntAttribute(u"timeslice_number", timeslice_number, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::S2SatelliteDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(backwards_compatibility_indicator, u"backwards_compatibility", false, false) &&
           element->getIntAttribute(TS_GS_mode, u"TS_GS_mode", false, 3, 0, 3) &&
           element->getOptionalIntAttribute(scrambling_sequence_index, u"scrambling_sequence_index", 0x00000000, 0x0003FFFF) &&
           element->getOptionalIntAttribute(input_stream_identifier, u"input_stream_identifier") &&
           element->getOptionalIntAttribute(timeslice_number, u"timeslice_number");
}
