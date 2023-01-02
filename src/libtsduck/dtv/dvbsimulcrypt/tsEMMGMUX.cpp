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

#include "tsEMMGMUX.h"
#include "tstlvMessageFactory.h"
#include "tsNamesFile.h"

// Define protocol singleton instance
TS_DEFINE_SINGLETON(ts::emmgmux::Protocol);


//----------------------------------------------------------------------------
// Protocol name.
//----------------------------------------------------------------------------

#define PROTOCOL_NAME u"EMMG/PDG<=>MUX"

ts::UString ts::emmgmux::Protocol::name() const
{
    return PROTOCOL_NAME;
}


//----------------------------------------------------------------------------
// Protocol Constructor: Define the syntax of the protocol
//----------------------------------------------------------------------------

ts::emmgmux::Protocol::Protocol() :
    tlv::Protocol(ts::emmgmux::CURRENT_VERSION)
{
    // Define the syntax of all commands:
    // add(cmd_tag, param_tag, min_size, max_size, min_count, max_count)

    add(Tags::channel_setup, Tags::client_id, 4, 4, 1, 1);
    add(Tags::channel_setup, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::channel_setup, Tags::section_TSpkt_flag, 1, 1, 1, 1);

    add(Tags::channel_test, Tags::client_id, 4, 4, 1, 1);
    add(Tags::channel_test, Tags::data_channel_id, 2, 2, 1, 1);

    add(Tags::channel_status, Tags::client_id, 4, 4, 1, 1);
    add(Tags::channel_status, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::channel_status, Tags::section_TSpkt_flag, 1, 1, 1, 1);

    add(Tags::channel_close, Tags::client_id, 4, 4, 1, 1);
    add(Tags::channel_close, Tags::data_channel_id, 2, 2, 1, 1);

    add(Tags::channel_error, Tags::client_id, 4, 4, 1, 1);
    add(Tags::channel_error, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::channel_error, Tags::error_status, 2, 2, 1, 0xFFFF);
    add(Tags::channel_error, Tags::error_information, 2, 2, 0, 0xFFFF);

    add(Tags::stream_setup, Tags::client_id, 4, 4, 1, 1);
    add(Tags::stream_setup, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::stream_setup, Tags::data_stream_id, 2, 2, 1, 1);
    add(Tags::stream_setup, Tags::data_id, 2, 2, 1, 1);
    add(Tags::stream_setup, Tags::data_type, 1, 1, 1, 1);

    add(Tags::stream_test, Tags::client_id, 4, 4, 1, 1);
    add(Tags::stream_test, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::stream_test, Tags::data_stream_id, 2, 2, 1, 1);

    add(Tags::stream_status, Tags::client_id, 4, 4, 1, 1);
    add(Tags::stream_status, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::stream_status, Tags::data_stream_id, 2, 2, 1, 1);
    add(Tags::stream_status, Tags::data_id, 2, 2, 1, 1);
    add(Tags::stream_status, Tags::data_type, 1, 1, 1, 1);

    add(Tags::stream_close_request, Tags::client_id, 4, 4, 1, 1);
    add(Tags::stream_close_request, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::stream_close_request, Tags::data_stream_id, 2, 2, 1, 1);

    add(Tags::stream_close_response, Tags::client_id, 4, 4, 1, 1);
    add(Tags::stream_close_response, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::stream_close_response, Tags::data_stream_id, 2, 2, 1, 1);

    add(Tags::stream_error, Tags::client_id, 4, 4, 1, 1);
    add(Tags::stream_error, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::stream_error, Tags::data_stream_id, 2, 2, 1, 1);
    add(Tags::stream_error, Tags::error_status, 2, 2, 1, 0xFFFF);
    add(Tags::stream_error, Tags::error_information, 2, 2, 0, 0xFFFF);

    add(Tags::stream_BW_request, Tags::client_id, 4, 4, 1, 1);
    add(Tags::stream_BW_request, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::stream_BW_request, Tags::data_stream_id, 2, 2, 1, 1);
    add(Tags::stream_BW_request, Tags::bandwidth, 2, 2, 0, 1);

    add(Tags::stream_BW_allocation, Tags::client_id, 4, 4, 1, 1);
    add(Tags::stream_BW_allocation, Tags::data_channel_id, 2, 2, 1, 1);
    add(Tags::stream_BW_allocation, Tags::data_stream_id, 2, 2, 1, 1);
    add(Tags::stream_BW_allocation, Tags::bandwidth, 2, 2, 0, 1);

    add(Tags::data_provision, Tags::client_id, 4, 4, 1, 1);
    add(Tags::data_provision, Tags::data_channel_id, 2, 2, 0, 1);
    add(Tags::data_provision, Tags::data_stream_id, 2, 2, 0, 1);
    add(Tags::data_provision, Tags::data_id, 2, 2, 1, 1);
    add(Tags::data_provision, Tags::datagram, 0, 0xFFFF, 1, 0xFFFF);
}


//----------------------------------------------------------------------------
// Message factory for the protocol
//----------------------------------------------------------------------------

void ts::emmgmux::Protocol::factory(const tlv::MessageFactory& fact, tlv::MessagePtr& msg) const
{
    switch (fact.commandTag()) {
        case Tags::channel_setup:
            msg = new ChannelSetup(fact);
            break;
        case Tags::channel_test:
            msg = new ChannelTest(fact);
            break;
        case Tags::channel_status:
            msg = new ChannelStatus(fact);
            break;
        case Tags::channel_close:
            msg = new ChannelClose(fact);
            break;
        case Tags::channel_error:
            msg = new ChannelError(fact);
            break;
        case Tags::stream_setup:
            msg = new StreamSetup(fact);
            break;
        case Tags::stream_test:
            msg = new StreamTest(fact);
            break;
        case Tags::stream_status:
            msg = new StreamStatus(fact);
            break;
        case Tags::stream_close_request:
            msg = new StreamCloseRequest(fact);
            break;
        case Tags::stream_close_response:
            msg = new StreamCloseResponse(fact);
            break;
        case Tags::stream_error:
            msg = new StreamError(fact);
            break;
        case Tags::stream_BW_request:
            msg = new StreamBWRequest(fact);
            break;
        case Tags::stream_BW_allocation:
            msg = new StreamBWAllocation(fact);
            break;
        case Tags::data_provision:
            msg = new DataProvision(fact);
            break;
        default:
            throw tlv::DeserializationInternalError(UString::Format(PROTOCOL_NAME u" message 0x%X unimplemented", {fact.commandTag()}));
    }
}


//----------------------------------------------------------------------------
// Return a message for a given protocol error status.
//----------------------------------------------------------------------------

ts::UString ts::emmgmux::Errors::Name(uint16_t status)
{
    return NameFromDTV(u"EmmgPdgMuxErrors", status, NamesFlags::HEXA_FIRST);
}


//----------------------------------------------------------------------------
// Create an error response message for a faulty incoming message.
//----------------------------------------------------------------------------

void ts::emmgmux::Protocol::buildErrorResponse(const tlv::MessageFactory& fact, tlv::MessagePtr& msg) const
{
    // Create a channel_error message
    SafePtr<ChannelError> errmsg(new ChannelError);

    // Try to get an data_channel_id from the incoming message.
    try {
        errmsg->channel_id = fact.get<uint16_t>(Tags::data_channel_id);
    }
    catch (const tlv::DeserializationInternalError&) {
        errmsg->channel_id = 0;
    }

    // Convert general TLV error code into EMMG/PDG <=> MUX error_status
    uint16_t status;
    switch (fact.errorStatus()) {
        case tlv::OK: // should not happen
        case tlv::InvalidMessage:
            status = Errors::inv_message;
            break;
        case tlv::UnsupportedVersion:
            status = Errors::inv_proto_version;
            break;
        case tlv::UnknownCommandTag:
            status = Errors::inv_message_type;
            break;
        case tlv::UnknownParameterTag:
            status = Errors::inv_param_type;
            break;
        case tlv::InvalidParameterLength:
            status = Errors::inv_param_length;
            break;
        case tlv::InvalidParameterCount:
        case tlv::MissingParameter:
            status = Errors::missing_param;
            break;
        default:
            status = Errors::unknown_error;
            break;
    }

    // Copy error_status and error_information into response
    errmsg->error_status.push_back(status);
    errmsg->error_information.push_back(fact.errorInformation());

    // Transfer ownership of safe ptr
    msg = errmsg.release();
}


//----------------------------------------------------------------------------
// channel_setup
//----------------------------------------------------------------------------

ts::emmgmux::ChannelSetup::ChannelSetup() :
    ChannelMessage(emmgmux::Protocol::Instance()->version(), Tags::channel_setup),
    client_id(0),
    section_TSpkt_flag(false)
{
}

ts::emmgmux::ChannelSetup::ChannelSetup(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::data_channel_id),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    section_TSpkt_flag(fact.get<bool>(Tags::section_TSpkt_flag))
{
}

void ts::emmgmux::ChannelSetup::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id,    channel_id);
    fact.put(Tags::client_id,          client_id);
    fact.put(Tags::section_TSpkt_flag, section_TSpkt_flag);
}

ts::UString ts::emmgmux::ChannelSetup::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_setup (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpDecimal(indent, u"section_TSpkt_flag", section_TSpkt_flag ? 1 : 0);
}


//----------------------------------------------------------------------------
// channel_test
//----------------------------------------------------------------------------

ts::emmgmux::ChannelTest::ChannelTest() :
    ChannelMessage(emmgmux::Protocol::Instance()->version(), Tags::channel_test),
    client_id(0)
{
}

ts::emmgmux::ChannelTest::ChannelTest(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::data_channel_id),
    client_id(fact.get<uint32_t>(Tags::client_id))
{
}

void ts::emmgmux::ChannelTest::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::client_id,       client_id);
}

ts::UString ts::emmgmux::ChannelTest::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_test (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id);
}


//----------------------------------------------------------------------------
// channel_status
//----------------------------------------------------------------------------

ts::emmgmux::ChannelStatus::ChannelStatus() :
    ChannelMessage(emmgmux::Protocol::Instance()->version(), Tags::channel_status),
    client_id(0),
    section_TSpkt_flag(false)
{
}

ts::emmgmux::ChannelStatus::ChannelStatus(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::data_channel_id),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    section_TSpkt_flag(fact.get<bool>(Tags::section_TSpkt_flag))
{
}

void ts::emmgmux::ChannelStatus::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::client_id, client_id);
    fact.put(Tags::section_TSpkt_flag, section_TSpkt_flag);
}

ts::UString ts::emmgmux::ChannelStatus::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_status (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpDecimal(indent, u"section_TSpkt_flag", section_TSpkt_flag ? 1 : 0);
}


//----------------------------------------------------------------------------
// channel_close
//----------------------------------------------------------------------------

ts::emmgmux::ChannelClose::ChannelClose() :
    ChannelMessage(emmgmux::Protocol::Instance()->version(), Tags::channel_close),
    client_id(0)
{
}

ts::emmgmux::ChannelClose::ChannelClose(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::data_channel_id),
    client_id(fact.get<uint32_t>(Tags::client_id))
{
}

void ts::emmgmux::ChannelClose::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::client_id, client_id);
}

ts::UString ts::emmgmux::ChannelClose::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_close (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id);
}


//----------------------------------------------------------------------------
// channel_error
//----------------------------------------------------------------------------

ts::emmgmux::ChannelError::ChannelError() :
    ChannelMessage(emmgmux::Protocol::Instance()->version(), Tags::channel_error),
    client_id(0),
    error_status(),
    error_information()
{
}

ts::emmgmux::ChannelError::ChannelError(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::data_channel_id),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    error_status(),
    error_information()
{
    fact.get(Tags::error_status, error_status);
    fact.get(Tags::error_information, error_information);
}

void ts::emmgmux::ChannelError::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::client_id, client_id);
    fact.put(Tags::error_status, error_status);
    fact.put(Tags::error_information, error_information);
}

ts::UString ts::emmgmux::ChannelError::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_error (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpVector(indent, u"error_status", error_status, Errors::Name) +
        dumpVector(indent, u"error_information", error_information);
}


//----------------------------------------------------------------------------
// stream_setup
//----------------------------------------------------------------------------

ts::emmgmux::StreamSetup::StreamSetup() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::stream_setup),
    client_id(0),
    data_id(0),
    data_type(0)
{
}

ts::emmgmux::StreamSetup::StreamSetup(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::data_channel_id, Tags::data_stream_id),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    data_id(fact.get<uint16_t>(Tags::data_id)),
    data_type(fact.get<uint8_t>(Tags::data_type))
{
}

void ts::emmgmux::StreamSetup::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
    fact.put(Tags::data_id, data_id);
    fact.put(Tags::data_type, data_type);
}

ts::UString ts::emmgmux::StreamSetup::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_setup (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpHexa(indent, u"data_stream_id", stream_id) +
        dumpHexa(indent, u"data_id", data_id) +
        dumpHexa(indent, u"data_type", data_type);
}


//----------------------------------------------------------------------------
// stream_test
//----------------------------------------------------------------------------

ts::emmgmux::StreamTest::StreamTest() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::stream_test),
    client_id(0)
{
}

ts::emmgmux::StreamTest::StreamTest(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::data_channel_id, Tags::data_stream_id),
    client_id(fact.get<uint32_t>(Tags::client_id))
{
}

void ts::emmgmux::StreamTest::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
}

ts::UString ts::emmgmux::StreamTest::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_test (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump (indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpHexa(indent, u"data_stream_id", stream_id);
}


//----------------------------------------------------------------------------
// stream_status
//----------------------------------------------------------------------------

ts::emmgmux::StreamStatus::StreamStatus() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::stream_status),
    client_id(0),
    data_id(0),
    data_type(0)
{
}

ts::emmgmux::StreamStatus::StreamStatus(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::data_channel_id, Tags::data_stream_id),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    data_id(fact.get<uint16_t>(Tags::data_id)),
    data_type(fact.get<uint8_t>(Tags::data_type))
{
}

void ts::emmgmux::StreamStatus::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
    fact.put(Tags::data_id, data_id);
    fact.put(Tags::data_type, data_type);
}

ts::UString ts::emmgmux::StreamStatus::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_status (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpHexa(indent, u"data_stream_id", stream_id) +
        dumpHexa(indent, u"data_id", data_id) +
        dumpHexa(indent, u"data_type", data_type);
}


//----------------------------------------------------------------------------
// stream_close_request
//----------------------------------------------------------------------------

ts::emmgmux::StreamCloseRequest::StreamCloseRequest() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::stream_close_request),
    client_id(0)
{
}

ts::emmgmux::StreamCloseRequest::StreamCloseRequest(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::data_channel_id, Tags::data_stream_id),
    client_id(fact.get<uint32_t>(Tags::client_id))
{
}

void ts::emmgmux::StreamCloseRequest::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
}

ts::UString ts::emmgmux::StreamCloseRequest::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_close_request (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpHexa(indent, u"data_stream_id", stream_id);
}


//----------------------------------------------------------------------------
// stream_close_response
//----------------------------------------------------------------------------

ts::emmgmux::StreamCloseResponse::StreamCloseResponse() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::stream_close_response),
    client_id(0)
{
}

ts::emmgmux::StreamCloseResponse::StreamCloseResponse(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::data_channel_id, Tags::data_stream_id),
    client_id(fact.get<uint32_t>(Tags::client_id))
{
}

void ts::emmgmux::StreamCloseResponse::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
}

ts::UString ts::emmgmux::StreamCloseResponse::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_close_response (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpHexa(indent, u"data_stream_id", stream_id);
}


//----------------------------------------------------------------------------
// stream_error
//----------------------------------------------------------------------------

ts::emmgmux::StreamError::StreamError() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::stream_error),
    client_id(0),
    error_status(),
    error_information()
{
}

ts::emmgmux::StreamError::StreamError(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::data_channel_id, Tags::data_stream_id),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    error_status(),
    error_information()
{
    fact.get(Tags::error_status, error_status);
    fact.get(Tags::error_information, error_information);
}

void ts::emmgmux::StreamError::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
    fact.put(Tags::error_status, error_status);
    fact.put(Tags::error_information, error_information);
}

ts::UString ts::emmgmux::StreamError::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_error (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpHexa(indent, u"data_stream_id", stream_id) +
        dumpVector(indent, u"error_status", error_status, Errors::Name) +
        dumpVector(indent, u"error_information", error_information);
}


//----------------------------------------------------------------------------
// stream_BW_request
//----------------------------------------------------------------------------

ts::emmgmux::StreamBWRequest::StreamBWRequest() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::stream_BW_request),
    client_id(0),
    has_bandwidth(false),
    bandwidth(0)
{
}

ts::emmgmux::StreamBWRequest::StreamBWRequest(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::data_channel_id, Tags::data_stream_id),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    has_bandwidth(1 == fact.count(Tags::bandwidth)),
    bandwidth(!has_bandwidth ? 0 : fact.get<int16_t>(Tags::bandwidth))
{
}

void ts::emmgmux::StreamBWRequest::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
    if (has_bandwidth) {
        fact.put(Tags::bandwidth, bandwidth);
    }
}

ts::UString ts::emmgmux::StreamBWRequest::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_BW_request (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpHexa(indent, u"data_stream_id", stream_id) +
        dumpOptionalDecimal(indent, u"bandwidth", has_bandwidth, bandwidth);
}


//----------------------------------------------------------------------------
// stream_BW_allocation
//----------------------------------------------------------------------------

ts::emmgmux::StreamBWAllocation::StreamBWAllocation() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::stream_BW_allocation),
    client_id(0),
    has_bandwidth(false),
    bandwidth(0)
{
}

ts::emmgmux::StreamBWAllocation::StreamBWAllocation(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::data_channel_id, Tags::data_stream_id),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    has_bandwidth(1 == fact.count(Tags::bandwidth)),
    bandwidth(!has_bandwidth ? 0 : fact.get<int16_t>(Tags::bandwidth))
{
}

void ts::emmgmux::StreamBWAllocation::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
    if (has_bandwidth) {
        fact.put(Tags::bandwidth, bandwidth);
    }
}

ts::UString ts::emmgmux::StreamBWAllocation::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_BW_allocation (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"client_id", client_id) +
        dumpHexa(indent, u"data_channel_id", channel_id) +
        dumpHexa(indent, u"data_stream_id", stream_id) +
        dumpOptionalDecimal(indent, u"bandwidth", has_bandwidth, bandwidth);
}


//----------------------------------------------------------------------------
// data_provision
//----------------------------------------------------------------------------

ts::emmgmux::DataProvision::DataProvision() :
    StreamMessage(emmgmux::Protocol::Instance()->version(), Tags::data_provision),
    client_id(0),
    data_id(0),
    datagram()
{
}

ts::emmgmux::DataProvision::DataProvision(const tlv::MessageFactory& fact) :
    StreamMessage(fact.protocolVersion(),
                  fact.commandTag(),
                  fact.count(Tags::data_channel_id) == 0 ? 0xFFFF : fact.get<uint16_t>(Tags::data_channel_id),
                  fact.count(Tags::data_stream_id) == 0 ? 0xFFFF : fact.get<uint16_t>(Tags::data_stream_id)),
    client_id(fact.get<uint32_t>(Tags::client_id)),
    data_id(fact.get<uint16_t>(Tags::data_id)),
    datagram()
{
    std::vector <tlv::MessageFactory::Parameter> params;
    fact.get(Tags::datagram, params);
    datagram.resize(params.size());
    for (size_t i = 0; i < params.size(); ++i) {
        datagram[i] = new ByteBlock(params[i].addr, params[i].length);
    }
}

void ts::emmgmux::DataProvision::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::data_channel_id, channel_id);
    fact.put(Tags::data_stream_id, stream_id);
    fact.put(Tags::client_id, client_id);
    fact.put(Tags::data_id, data_id);
    for (size_t i = 0; i < datagram.size(); ++i) {
        if (!datagram[i].isNull()) {
            fact.put(Tags::datagram, *(datagram[i]));
        }
    }
}

ts::UString ts::emmgmux::DataProvision::dump(size_t indent) const
{
    UString value(UString::Format(u"%*sdata_provision (" PROTOCOL_NAME u")\n", {indent, u""}));
    value += tlv::Message::dump(indent);
    value += dumpHexa(indent, u"client_id", client_id);
    value += dumpHexa(indent, u"data_channel_id", channel_id);
    value += dumpHexa(indent, u"data_stream_id", stream_id);
    value += dumpHexa(indent, u"data_id", data_id);
    for (size_t i = 0; i < datagram.size(); ++i) {
        value += dumpOptional(indent, u"datagram", true, *(datagram[i]), UString::HEXA);
    }
    return value;
}
