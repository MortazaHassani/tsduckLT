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
//
//  Transport stream processor shared library:
//  DVB SimulCrypt data injector using EMMG/PDG <=> MUX protocol.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSwitchableReport.h"
#include "tsSection.h"
#include "tsPacketizer.h"
#include "tsEMMGMUX.h"
#include "tstlvConnection.h"
#include "tsTCPServer.h"
#include "tsUDPReceiver.h"
#include "tsMessageQueue.h"
#include "tstlvMessageFactory.h"
#include "tsContinuityAnalyzer.h"
#include "tsThread.h"

#define DEFAULT_PROTOCOL_VERSION  2     // Default protocol version for EMMG/PDG <=> MUX.
#define DEFAULT_QUEUE_SIZE        1000  // Maximum number of TS packets in queue
#define SERVER_BACKLOG            1     // One connection at a time
#define SERVER_THREAD_STACK_SIZE  (128 * 1024)


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DataInjectPlugin: public ProcessorPlugin, private SectionProviderInterface
    {
        TS_NOBUILD_NOCOPY(DataInjectPlugin);
    public:
        // Implementation of plugin API
        DataInjectPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // TS packets or sections are passed from the server thread to the plugin thread using a message queue.
        typedef MessageQueue<TSPacket, Mutex> PacketQueue;
        typedef MessageQueue<Section, Mutex> SectionQueue;

        // Message queues enqueue smart pointers to the message type (MT = Multi-Thread).
        typedef PacketQueue::MessagePtr PacketPtrMT;
        typedef SectionQueue::MessagePtr SectionPtrMT;

        // TCP listener thread.
        class TCPListener : public Thread
        {
            TS_NOBUILD_NOCOPY(TCPListener);
        public:
            // Constructor.
            TCPListener(DataInjectPlugin* plugin);

            // Terminate the thread.
            void stop();

            // Invoked in the context of the server thread.
            virtual void main() override;

        private:
            DataInjectPlugin* const _plugin;
            SwitchableReport        _report;
            tlv::Connection<Mutex>  _client;
        };

        // UDP listener thread.
        class UDPListener : public Thread
        {
            TS_NOBUILD_NOCOPY(UDPListener);
        public:
            // Constructor.
            UDPListener(DataInjectPlugin* plugin);

            // Open the UDP socket.
            bool open();

            // Terminate the thread.
            void stop();

            // Invoked in the context of the server thread.
            virtual void main() override;

        private:
            DataInjectPlugin* const _plugin;
            SwitchableReport        _report;
            UDPReceiver             _client;
        };

        // Plugin private data
        PacketCounter      _pkt_current;          // Current TS packet index
        PacketCounter      _pkt_next_data;        // Next data insertion point
        PID                _data_pid;             // PID for data (constant after start)
        ContinuityAnalyzer _cc_fixer;             // To fix continuity counters in injected PID
        BitRate            _max_bitrate;          // Max data PID's bitrate (constant after start)
        bool               _unregulated;          // Insert data packet as soon as received.
        IPv4SocketAddress      _tcp_address;          // TCP port and optional local address.
        IPv4SocketAddress      _udp_address;          // UDP port and optional local address.
        bool               _reuse_port;           // Reuse port option.
        size_t             _sock_buf_size;        // Socket receive buffer size.
        TCPServer          _server;               // EMMG/PDG <=> MUX TCP server
        TCPListener        _tcp_listener;         // TCP listener thread.
        UDPListener        _udp_listener;         // UDP listener thread.
        PacketQueue        _packet_queue;         // Queue of incoming TS packets.
        SectionQueue       _section_queue;        // Queue of incoming sections.
        tlv::Logger        _logger;               // Message logger.
        volatile bool      _channel_established;  // Data channel open.
        volatile bool      _stream_established;   // Data stream open.
        volatile bool      _req_bitrate_changed;  // Requested bitrate has changed.
        // Start of protected area.
        Mutex              _mutex;                // Mutex for access to protected area
        uint32_t           _client_id;            // DVB SimilCrypt client id.
        uint16_t           _data_id;              // DVB SimilCrypt data id.
        bool               _section_mode;         // Datagrams are sections.
        Packetizer         _packetizer;           // Generate packets in the case of incoming sections.
        BitRate            _req_bitrate;          // Requested bitrate
        size_t             _lost_packets;         // Lost packets (queue full)

        // Reset all client session context information.
        void clearSession();

        // Process bandwidth request. Invoked in the server thread.
        bool processBandwidthRequest(const tlv::MessagePtr&, emmgmux::StreamBWAllocation&);

        // Process data provision. Invoked in the server thread.
        bool processDataProvision(const tlv::MessagePtr&);

        // Report packet/session loss. Invoked with _mutex held.
        void processPacketLoss(const UChar* type, bool enqueueSuccess);

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override { return false; }
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"datainject", ts::DataInjectPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DataInjectPlugin::DataInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"DVB SimulCrypt data injector using EMMG/PDG <=> MUX protocol", u"[options]"),
    _pkt_current(0),
    _pkt_next_data(0),
    _data_pid(PID_NULL),
    _cc_fixer(AllPIDs, tsp),
    _max_bitrate(0),
    _unregulated(false),
    _tcp_address(),
    _udp_address(),
    _reuse_port(false),
    _sock_buf_size(0),
    _server(),
    _tcp_listener(this),
    _udp_listener(this),
    _packet_queue(),
    _section_queue(),
    _logger(ts::Severity::Debug, tsp_),
    _channel_established(false),
    _stream_established(false),
    _req_bitrate_changed(false),
    _mutex(),
    _client_id(0),
    _data_id(0),
    _section_mode(false),
    _packetizer(duck, PID_NULL, this),
    _req_bitrate(0),
    _lost_packets(0)
{
    option<BitRate>(u"bitrate-max", 'b');
    help(u"bitrate-max",
         u"Specifies the maximum bitrate for the data PID in bits / second. "
         u"By default, the data PID bitrate is limited by the stuffing bitrate "
         u"(data insertion is performed by replacing stuffing packets).");

    option(u"buffer-size", 0, UNSIGNED);
    help(u"buffer-size",
         u"Specify the TCP and UDP socket receive buffer size (socket option).");

    option(u"emmg-mux-version", 'v', INTEGER, 0, 1, 1, 5);
    help(u"emmg-mux-version",
         u"Specifies the version of the EMMG/PDG <=> MUX DVB SimulCrypt protocol. "
         u"Valid values are 1 to 5. The default is " TS_USTRINGIFY(DEFAULT_PROTOCOL_VERSION) u".");

    option(u"log-data", 0, ts::Severity::Enums, 0, 1, true);
    help(u"log-data", u"level",
         u"Same as --log-protocol but applies to data_provision messages only. To "
         u"debug the session management without being flooded by data messages, use "
         u"--log-protocol=info --log-data=debug.");

    option(u"log-protocol", 0, ts::Severity::Enums, 0, 1, true);
    help(u"log-protocol", u"level",
         u"Log all EMMG/PDG <=> MUX protocol messages using the specified level. If "
         u"the option is not present, the messages are logged at debug level only. "
         u"If the option is present without value, the messages are logged at info "
         u"level. A level can be a numerical debug level or a name.");

    option(u"no-reuse-port");
    help(u"no-reuse-port",
         u"Disable the reuse port socket option. Do not use unless completely necessary.");

    option(u"pid", 'p', PIDVAL, 1, 1);
    help(u"pid",
         u"Specifies the PID for the data insertion. This option is mandatory.");

    option(u"queue-size", 'q', UINT32);
    help(u"queue-size",
         u"Specifies the maximum number of sections or TS packets in the internal "
         u"queue, ie. sections or packets which are received from the EMMG/PDG "
         u"client but not yet inserted into the TS. The default is " +
         UString::Decimal(DEFAULT_QUEUE_SIZE) + u".");

    option(u"reuse-port", 'r');
    help(u"reuse-port",
         u"Set the reuse port socket option. This is now enabled by default, the option "
         u"is present for legacy only.");

    option(u"server", 's', STRING, 1, 1);
    help(u"server", u"[address:]port",
         u"Specifies the local TCP port on which the plugin listens for an incoming "
         u"EMMG/PDG connection. This option is mandatory. "
         u"When present, the optional address shall specify a local IP address or "
         u"host name (by default, the plugin accepts connections on any local IP "
         u"interface). This plugin behaves as a MUX, ie. a TCP server, and accepts "
         u"only one EMMG/PDG connection at a time.");

    option(u"udp", 'u', STRING);
    help(u"udp", u"[address:]port",
         u"Specifies the local UDP port on which the plugin listens for data "
         u"provision messages (these messages can be sent using TCP or UDP). By "
         u"default, use the same port and optional local address as specified for "
         u"TCP using option --server.");

    option(u"unregulated");
    help(u"unregulated",
         u"Insert data packets immediately. Do not regulate insertion, do not limit "
         u"the data bitrate.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::start()
{
    // Command line options
    getValue(_max_bitrate, u"bitrate-max");
    getIntValue(_data_pid, u"pid");
    const size_t queue_size = intValue<size_t>(u"queue-size", DEFAULT_QUEUE_SIZE);
    _reuse_port = !present(u"no-reuse-port");
    getIntValue(_sock_buf_size, u"buffer-size");
    _unregulated = present(u"unregulated");

    // Set logging levels.
    const int log_protocol = present(u"log-protocol") ? intValue<int>(u"log-protocol", ts::Severity::Info) : ts::Severity::Debug;
    const int log_data = present(u"log-data") ? intValue<int>(u"log-data", ts::Severity::Info) : log_protocol;
    _logger.setDefaultSeverity(log_protocol);
    _logger.setSeverity(ts::emmgmux::Tags::data_provision, log_data);

    // Limit internal queues sizes.
    _packet_queue.setMaxMessages(queue_size);
    _section_queue.setMaxMessages(queue_size);

    // Specify which EMMG/PDG <=> MUX version to use.
    emmgmux::Protocol::Instance()->setVersion(intValue<tlv::VERSION>(u"emmg-mux-version", DEFAULT_PROTOCOL_VERSION));

    // Get TCP server address. The only mandatory part is the TCP port.
    if (!_tcp_address.resolve(value(u"server"), *tsp)) {
        return false;
    }
    if (!_tcp_address.hasPort()) {
        tsp->error(u"no TCP server port specified");
        return false;
    }

    // Get UDP server address. Same as TCP by default.
    _udp_address.clear();
    if (present(u"udp") && !_udp_address.resolve(value(u"udp"), *tsp)) {
        return false;
    }
    if (!_udp_address.hasAddress()) {
        _udp_address.setAddress(_tcp_address.address());
    }
    if (!_udp_address.hasPort()) {
        _udp_address.setPort(_tcp_address.port());
    }

    // Initialize the TCP server.
    if (!_server.open(*tsp)) {
        return false;
    }
    if (!_server.reusePort(_reuse_port, *tsp) || !_server.bind(_tcp_address, *tsp) || !_server.listen(SERVER_BACKLOG, *tsp)) {
        _server.close(*tsp);
        return false;
    }

    // Initialize the UDP receiver.
    if (!_udp_listener.open()) {
        _server.close(*tsp);
        return false;
    }

    // Clear client session.
    clearSession();
    tsp->verbose(u"initial bandwidth allocation is %'d", {_req_bitrate == 0 ? u"unlimited" : _req_bitrate.toString() + u" b/s"});

    // TS processing state
    _cc_fixer.reset();
    _cc_fixer.setGenerator(true);
    _pkt_current = 0;
    _pkt_next_data = 0;

    // Start the internal threads.
    _tcp_listener.start();
    _udp_listener.start();

    return true;
}


//----------------------------------------------------------------------------
// Reset all client session context information.
//----------------------------------------------------------------------------

void ts::DataInjectPlugin::clearSession()
{
    // Work on some protected data
    GuardMutex lock(_mutex);

    // No client session is established.
    _channel_established = false;
    _stream_established = false;

    // Reset queues.
    _packet_queue.clear();
    _section_queue.clear();
    _packetizer.reset();
    _lost_packets = 0;

    // Initial bandwidth allocation (zero means unlimited)
    _req_bitrate = _max_bitrate;
    _req_bitrate_changed = false;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::stop()
{
    // Stop the internal threads.
    _tcp_listener.stop();
    _udp_listener.stop();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DataInjectPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Count packets
    _pkt_current++;

    // Abort if data PID is already present in TS
    const PID pid = pkt.getPID();
    if (pid == _data_pid) {
        tsp->error(u"data PID conflict, specified %d (0x%X), now found as input PID, try another one", {pid, pid});
        return TSP_END;
    }

    // Data injection may occur only be replacing null packets
    if (pid == PID_NULL) {

        // Update data PID bitrate
        if (_req_bitrate_changed) {
            // Reinitialize insertion point when bitrate changes
            _pkt_next_data = _pkt_current;
            _req_bitrate_changed = false;
        }

        // Try to insert data
        if (_unregulated || _pkt_next_data <= _pkt_current) {
            // Time to insert data packet, if any is available immediately.
            GuardMutex lock(_mutex);

            // Get next packet to insert.
            bool got_packet = false;
            if (_section_mode) {
                // Section mode: Get a packet from the packetizer.
                got_packet = _packetizer.getNextPacket(pkt);
            }
            else {
                // Packet mode: Dequeue a packet immediately.
                PacketPtrMT pp;
                got_packet = _packet_queue.dequeue(pp, 0);
                if (got_packet) {
                    pkt = *pp;
                }
            }

            // Update new inserted packet.
            if (got_packet) {
                // Update PID and continuity counter.
                pkt.setPID(_data_pid);
                _cc_fixer.feedPacket(pkt);
                // Compute next insertion point if the data PID bitrate is specified.
                // Otherwise, try to update any null packet (unbounded bitrate).
                if (!_unregulated || _req_bitrate != 0) {
                    // TODO: refine this, works only for low injection bitrates.
                    _pkt_next_data += (tsp->bitrate() / _req_bitrate).toInt();
                }
            }
        }
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Implementation of SectionProviderInterface.
// Provide next section to packetize in section mode.
//----------------------------------------------------------------------------

void ts::DataInjectPlugin::provideSection(SectionCounter counter, SectionPtr& section)
{
    // Try to dequeue a section immediately.
    SectionPtrMT mt_section;
    if (_section_queue.dequeue(mt_section, 0)) {
        // A section was dequeue. Change the mutex of the safe pointer.
        section = mt_section.changeMutex<SectionPtr::MutexType>();
    }
    else {
        // No section available.
        section.clear();
    }
}


//----------------------------------------------------------------------------
// Process bandwidth request. Invoked in the server thread
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::processBandwidthRequest(const tlv::MessagePtr& request, emmgmux::StreamBWAllocation& response)
{
    // Interpret the message as a stream_BW_request.
    emmgmux::StreamBWRequest* m = dynamic_cast<emmgmux::StreamBWRequest*>(request.pointer());
    if (m == nullptr) {
        tsp->error(u"incorrect message, expected stream_BW_request");
        return false;
    }

    GuardMutex lock(_mutex);

    // Check that the stream is established.
    if (!_stream_established) {
        tsp->error(u"unexpected stream_BW_request, stream not setup");
        return false;
    }

    // Compute new bandwidth
    if (m->has_bandwidth) {
        BitRate requested = 1000 * BitRate(m->bandwidth); // protocol unit is kb/s
        _req_bitrate = _max_bitrate == 0 ? requested : std::min(requested, _max_bitrate);
        _req_bitrate_changed = true;
        tsp->verbose(u"requested bandwidth %'d b/s, allocated %'d b/s", {requested, _req_bitrate});
    }

    // Build the response
    response.channel_id = m->channel_id;
    response.stream_id = m->stream_id;
    response.client_id = m->client_id;
    response.has_bandwidth = _req_bitrate > 0;
    response.bandwidth = uint16_t(_req_bitrate.toInt() / 1000); // protocol unit is kb/s
    return true;
}


//----------------------------------------------------------------------------
// Process data provision. Invoked in the server threads.
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::processDataProvision(const tlv::MessagePtr& msg)
{
    // Interpret the message as a stream_BW_request.
    emmgmux::DataProvision* m = dynamic_cast<emmgmux::DataProvision*>(msg.pointer());
    if (m == nullptr) {
        tsp->error(u"incorrect message, expected data_provision");
        return false;
    }

    GuardMutex lock(_mutex);

    // Check that the stream is established.
    if (!_stream_established) {
        tsp->error(u"unexpected data_provision, stream not setup");
        return false;
    }

    // Check that the client and data id are expected.
    if (m->client_id != _client_id) {
        tsp->error(u"unexpected client id 0x%X in data_provision, expected 0x%X", {m->client_id, _client_id});
        return false;
    }
    if (m->data_id != _data_id) {
        tsp->error(u"unexpected data id 0x%X in data_provision, expected 0x%X", {m->data_id, _data_id});
        return false;
    }

    // Check that the stream is established.
    bool ok = true;
    if (_section_mode) {
        // Section mode, one section per datagram parameter, enqueue them.
        for (size_t i = 0; i < m->datagram.size(); ++i) {
            SectionPtrMT sp(new Section(m->datagram[i]));
            if (sp->isValid()) {
                processPacketLoss(u"sections", _section_queue.enqueue(sp, 0));
            }
            else {
                tsp->error(u"received an invalid section (%d bytes)", {m->datagram[i]->size()});
            }
        }
    }
    else {
        // Packet mode, locate packets and enqueue them
        for (size_t i = 0; i < m->datagram.size(); ++i) {
            const uint8_t* data = m->datagram[i]->data();
            size_t size = m->datagram[i]->size();
            while (size >= PKT_SIZE) {
                if (*data != SYNC_BYTE) {
                    tsp->error(u"invalid TS packet");
                }
                else {
                    PacketPtrMT p(new TSPacket());
                    p->copyFrom(data);
                    processPacketLoss(u"packets", _packet_queue.enqueue(p, 0));
                    data += PKT_SIZE;
                    size -= PKT_SIZE;
                }
            }
            if (size != 0) {
                tsp->error(u"extraneous %d bytes in datagram", {size});
            }
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Report packet/session loss. Invoked with _mutex held.
//----------------------------------------------------------------------------

void ts::DataInjectPlugin::processPacketLoss(const UChar* type, bool enqueueSuccess)
{
    if (!enqueueSuccess && _lost_packets++ == 0) {
        tsp->warning(u"internal queue overflow, losing %s, consider using --queue-size", {type});
    }
    else if (enqueueSuccess && _lost_packets != 0) {
        tsp->info(u"retransmitting after %'d lost %s", {_lost_packets, type});
        _lost_packets = 0;
    }
}


//----------------------------------------------------------------------------
// TCP listener thread.
//----------------------------------------------------------------------------

ts::DataInjectPlugin::TCPListener::TCPListener(DataInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _report(*plugin->tsp),
    _client(emmgmux::Protocol::Instance(), true, 3)
{
}

void ts::DataInjectPlugin::TCPListener::stop()
{
    // Switch off error messages from the network client.
    _report.setSwitch(false);

    // Close the server, then break client connection.
    // This will force the server thread to terminate.
    _plugin->_server.close(*_plugin->tsp);
    _client.disconnect(NULLREP);
    _client.close(NULLREP);

    // Wait for actual thread termination
    Thread::waitForTermination();
}

void ts::DataInjectPlugin::TCPListener::main()
{
    _plugin->tsp->debug(u"TCP server thread started");

    IPv4SocketAddress client_address;
    emmgmux::ChannelStatus channel_status;
    emmgmux::StreamStatus stream_status;

    // Loop on client acceptance (accept only one client at a time).
    while (_plugin->_server.accept(_client, client_address, _report)) {

        _report.verbose(u"incoming connection from %s", {client_address});

        // Start from a fresh client session context.
        _plugin->clearSession();

        // Connection state
        bool ok = true;
        tlv::MessagePtr msg;

        // Loop on message reception from the client
        while (ok && _client.receive(msg, _plugin->tsp, _plugin->_logger)) {

            // Message handling.
            // We do not send errors back to client, we just disconnect
            // (not too polite, but we don't care!)
            switch (msg->tag()) {

                case emmgmux::Tags::channel_setup: {
                    if (_plugin->_channel_established) {
                        _report.error(u"received channel_setup when channel is already setup");
                        ok = false;
                    }
                    else {
                        emmgmux::ChannelSetup* m = dynamic_cast<emmgmux::ChannelSetup*>(msg.pointer());
                        assert (m != nullptr);
                        // First, declare the channel as established.
                        {
                            GuardMutex lock(_plugin->_mutex);
                            _plugin->_client_id = m->client_id;
                            _plugin->_section_mode = !m->section_TSpkt_flag; // flag == 0 means section
                            _plugin->_channel_established = true;
                        }
                        // Build and send the channel_status
                        channel_status.channel_id = m->channel_id;
                        channel_status.client_id = m->client_id;
                        channel_status.section_TSpkt_flag = m->section_TSpkt_flag;
                        ok = _client.send(channel_status, _plugin->_logger);
                    }
                    break;
                }

                case emmgmux::Tags::channel_test: {
                    if (_plugin->_channel_established) {
                        // Automatic reply to channel_test
                        ok = _client.send(channel_status, _plugin->_logger);
                    }
                    else {
                        _report.error(u"unexpected channel_test, channel not setup");
                        ok = false;
                    }
                    break;
                }

                case emmgmux::Tags::channel_close: {
                    GuardMutex lock(_plugin->_mutex);
                    _plugin->_channel_established = false;
                    _plugin->_stream_established = false;
                    break;
                }

                case emmgmux::Tags::stream_setup: {
                    if (!_plugin->_channel_established) {
                        _report.error(u"unexpected stream_setup, channel not setup");
                        ok = false;
                    }
                    else if (_plugin->_stream_established) {
                        _report.error(u"received stream_setup when stream is already setup");
                        ok = false;
                    }
                    else {
                        emmgmux::StreamSetup* m = dynamic_cast<emmgmux::StreamSetup*>(msg.pointer());
                        assert(m != nullptr);
                        // First, declare the stream as established.
                        {
                            GuardMutex lock(_plugin->_mutex);
                            _plugin->_data_id = m->data_id;
                            _plugin->_stream_established = true;
                        }
                        // Build and send the stream_status
                        stream_status.channel_id = m->channel_id;
                        stream_status.stream_id = m->stream_id;
                        stream_status.client_id = m->client_id;
                        stream_status.data_id = m->data_id;
                        stream_status.data_type = m->data_type;
                        ok = _client.send(stream_status, _plugin->_logger);
                    }
                    break;
                }

                case emmgmux::Tags::stream_test: {
                    if (_plugin->_stream_established) {
                        // Automatic reply to stream_test
                        ok = _client.send(stream_status, _plugin->_logger);
                    }
                    else {
                        _report.error(u"unexpected stream_test, stream not setup");
                        ok = false;
                    }
                    break;
                }

                case emmgmux::Tags::stream_close_request: {
                    if (!_plugin->_stream_established) {
                        _report.error(u"unexpected stream_close_request, stream not setup");
                        ok = false;
                    }
                    else {
                        // First, declare the stream as closed.
                        {
                            GuardMutex lock(_plugin->_mutex);
                            _plugin->_stream_established = false;
                        }
                        // Send the stream_close_response
                        emmgmux::StreamCloseResponse resp;
                        emmgmux::StreamCloseRequest* m = dynamic_cast<emmgmux::StreamCloseRequest*>(msg.pointer());
                        assert (m != nullptr);
                        resp.channel_id = m->channel_id;
                        resp.stream_id = m->stream_id;
                        resp.client_id = m->client_id;
                        ok = _client.send(resp, _plugin->_logger);
                    }
                    break;
                }

                case emmgmux::Tags::stream_BW_request: {
                    emmgmux::StreamBWAllocation response;
                    ok = _plugin->processBandwidthRequest(msg, response) && _client.send(response, _plugin->_logger);
                    break;
                }

                case emmgmux::Tags::data_provision: {
                    ok = _plugin->processDataProvision(msg);
                    break;
                }

                default: {
                    break;
                }
            }
        }

        // Error while receiving messages during a client session, most likely a disconnection
        _client.disconnect(NULLREP);
        _client.close(NULLREP);
    }

    _plugin->tsp->debug(u"TCP server thread completed");
}


//----------------------------------------------------------------------------
// UDP listener thread.
//----------------------------------------------------------------------------

ts::DataInjectPlugin::UDPListener::UDPListener(DataInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _report(*plugin->tsp),
    _client(_report)
{
}

bool ts::DataInjectPlugin::UDPListener::open()
{
    _client.setParameters(_plugin->_udp_address, _plugin->_reuse_port, _plugin->_sock_buf_size);
    return _client.open(_report);
}

void ts::DataInjectPlugin::UDPListener::stop()
{
    // Switch off error messages from the network client.
    _report.setSwitch(false);

    // Close the UDP receiver.
    // This will force the server thread to terminate.
    _client.close(NULLREP);

    // Wait for actual thread termination
    Thread::waitForTermination();
}

void ts::DataInjectPlugin::UDPListener::main()
{
    _plugin->tsp->debug(u"UDP server thread started");

    uint8_t inbuf[65536];
    size_t insize = 0;
    IPv4SocketAddress sender;
    IPv4SocketAddress destination;

    // Loop on incoming messages.
    while (_client.receive(inbuf, sizeof(inbuf), insize, sender, destination, _plugin->tsp, _report)) {

        // Analyze the message
        tlv::MessageFactory mf(inbuf, insize, emmgmux::Protocol::Instance());
        const tlv::MessagePtr msg(mf.factory());

        if (mf.errorStatus() != tlv::OK || msg.isNull()) {
            _report.error(u"received invalid message from %s, %d bytes", {sender, insize});
        }
        else {
            // Log the message.
            _plugin->_logger.log(*msg, u"received UDP message from " + sender.toString());
            // The only accepted message is data_provision.
            _plugin->processDataProvision(msg);
        }
    }

    _plugin->tsp->debug(u"UDP server thread completed");
}
