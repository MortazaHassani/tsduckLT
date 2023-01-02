//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsjsonOutputArgs.h"
#include "tsArgs.h"
#include "tsjsonValue.h"
#include "tsjsonRunningDocument.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::json::OutputArgs::OutputArgs() :
    _json_opt(false),
    _json_line(false),
    _json_tcp(false),
    _json_tcp_keep(false),
    _json_udp(false),
    _line_prefix(),
    _tcp_destination(),
    _udp_destination(),
    _udp_local(),
    _udp_ttl(0),
    _sock_buffer_size(0),
    _udp_sock(),
    _tcp_sock()
{
}

ts::json::OutputArgs::~OutputArgs()
{
    if (_tcp_sock.isOpen()) {
        _tcp_sock.closeWriter(NULLREP);
        _tcp_sock.disconnect(NULLREP);
        _tcp_sock.close(NULLREP);
    }
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::json::OutputArgs::defineArgs(Args& args, bool use_short_opt, const UString& help)
{
    args.option(u"json", use_short_opt ? 'j' : 0);
    args.help(u"json", help.empty() ? u"Report in JSON output format (useful for automatic analysis)." : help);

    args.option(u"json-buffer-size", 0, Args::UNSIGNED);
    args.help(u"json-buffer-size",
              u"With --json-tcp or --json-udp, specify the network socket send buffer size.");

    args.option(u"json-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"json-line", u"'prefix'",
              u"Same as --json but report the JSON text as one single line in the message logger instead of the output file. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the JSON text to locate the appropriate line in the logs.");

    args.option(u"json-tcp", 0, Args::STRING);
    args.help(u"json-tcp", u"address:port",
              u"Same as --json but report the JSON text as one single line in a TCP connection instead of the output file. "
              u"The 'address' specifies an IP address or a host name that translates to an IP address. "
              u"The 'port' specifies the destination TCP port. "
              u"By default, a new TCP connection is established each time a JSON message is produced. "
              u"Be aware that a complete TCP connection cycle may introduce some latency in the processing. "
              u"If latency is an issue, consider using --json-udp.");

    args.option(u"json-tcp-keep");
    args.help(u"json-tcp-keep",
              u"With --json-tcp, keep the TCP connection open for all JSON messages. "
              u"By default, a new TCP connection is established each time a JSON message is produced.");

    args.option(u"json-udp", 0, Args::STRING);
    args.help(u"json-udp", u"address:port",
              u"Same as --json but report the JSON text as one single line in a UDP datagram instead of the output file. "
              u"The 'address' specifies an IP address which can be either unicast or multicast. "
              u"It can be also a host name that translates to an IP address. "
              u"The 'port' specifies the destination UDP port. "
              u"Be aware that the size of UDP datagrams is limited by design to 64 kB. "
              u"If larger JSON contents are expected, consider using --json-tcp.");

    args.option(u"json-udp-local", 0, Args::STRING);
    args.help(u"json-udp-local", u"address",
              u"With --json-udp, when the destination is a multicast address, specify "
              u"the IP address of the outgoing local interface. It can be also a host "
              u"name that translates to a local address.");

    args.option(u"json-udp-ttl", 0, Args::POSITIVE);
    args.help(u"json-udp-ttl",
              u"With --json-udp, specifies the TTL (Time-To-Live) socket option. "
              u"The actual option is either \"Unicast TTL\" or \"Multicast TTL\", "
              u"depending on the destination address. Remember that the default "
              u"Multicast TTL is 1 on most systems.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::json::OutputArgs::loadArgs(DuckContext& duck, Args& args)
{
    bool ok = true;
    _json_opt = args.present(u"json");
    _json_line = args.present(u"json-line");
    _json_tcp = args.present(u"json-tcp");
    _json_tcp_keep = args.present(u"json-tcp-keep");
    _json_udp = args.present(u"json-udp");
    args.getValue(_line_prefix, u"json-line");
    args.getIntValue(_udp_ttl, u"json-udp-ttl");
    args.getIntValue(_sock_buffer_size, u"json-buffer-size");
    _udp_destination.clear();
    _udp_local.clear();
    if (_json_tcp) {
        ok = _tcp_destination.resolve(args.value(u"json-tcp"), args);
    }
    if (_json_udp) {
        ok = _udp_destination.resolve(args.value(u"json-udp"), args);
    }
    if (args.present(u"json-udp-local")) {
        ok = _udp_local.resolve(args.value(u"json-udp-local"), args) && ok;
    }

    // Force reinit of UDP and TCP session in case the arguments are reloaded.
    udpClose(args);
    tcpDisconnect(true, args);

    return ok;
}


//----------------------------------------------------------------------------
// Open/close the UDP socket.
//----------------------------------------------------------------------------

bool ts::json::OutputArgs::udpOpen(Report& rep)
{
    if (_udp_sock.isOpen()) {
        return true;
    }
    else if (!_udp_sock.open(rep)) {
        return false;
    }
    else if (_udp_sock.setDefaultDestination(_udp_destination, rep) &&
             (_sock_buffer_size == 0 || _udp_sock.setSendBufferSize(_sock_buffer_size, rep)) &&
             (!_udp_local.hasAddress() || _udp_sock.setOutgoingMulticast(_udp_local, rep)) &&
             (_udp_ttl <= 0 || _udp_sock.setTTL(_udp_ttl, rep)))
    {
        return true;
    }
    else {
        _udp_sock.close(rep);
        return false;
    }
}

bool ts::json::OutputArgs::udpClose(Report& rep)
{
    return !_udp_sock.isOpen() || _udp_sock.close(rep);
}


//----------------------------------------------------------------------------
// Connect/disconnect the TCP session.
//----------------------------------------------------------------------------

bool ts::json::OutputArgs::tcpConnect(Report& rep)
{
    if (_tcp_sock.isOpen()) {
        return true;
    }
    else if (!_tcp_sock.open(rep)) {
        return false;
    }
    else if ((_sock_buffer_size == 0 || _tcp_sock.setSendBufferSize(_sock_buffer_size, rep)) &&
             _tcp_sock.bind(IPv4SocketAddress::AnySocketAddress, rep) &&
             _tcp_sock.connect(_tcp_destination, rep))
    {
        return true;
    }
    else {
        _tcp_sock.close(rep);
        return false;
    }
}

bool ts::json::OutputArgs::tcpDisconnect(bool force, Report& rep)
{
    bool ok = true;
    if (_tcp_sock.isOpen() && (force || !_json_tcp_keep)) {
        ok = _tcp_sock.closeWriter(rep) && _tcp_sock.disconnect(rep);
        ok = _tcp_sock.close(rep) && ok;
    }
    return ok;
}


//----------------------------------------------------------------------------
// Issue a JSON report, except --json file.
//----------------------------------------------------------------------------

bool ts::json::OutputArgs::reportOthers(const json::Value& root, Report& rep)
{
    bool udp_ok = true;
    bool tcp_ok = true;

    if (_json_line || _json_tcp || _json_udp) {

        // Generate one JSON line.
        TextFormatter text(rep);
        text.setString();
        text.setEndOfLineMode(TextFormatter::EndOfLineMode::SPACING);
        root.print(text);
        UString line;
        text.getString(line);

        // When sent over the network, use a UTF-8 string.
        std::string line8;
        if (_json_tcp || _json_udp) {
            line.toUTF8(line8);
        }

        // Report in logger.
        if (_json_line) {
            rep.info(_line_prefix + line);
        }

        // Report through UDP. Open socket the first time.
        if (_json_udp) {
            udp_ok = udpOpen(rep) && _udp_sock.send(line8.data(), line8.size(), rep);
        }

        // Report through TCP. Connect to TCP server the first time (--json-tcp-keep) or every time.
        if (_json_tcp) {
            tcp_ok = tcpConnect(rep);
            if (tcp_ok) {
                tcp_ok = _tcp_sock.sendLine(line8, rep);
                // In case of send error, retry opening the socket once.
                // This is useful when the session is kept open and the server disconnected since last time.
                if (!tcp_ok) {
                    tcpDisconnect(true, rep);
                    tcp_ok = tcpConnect(rep) && _tcp_sock.sendLine(line8, rep);
                }
                // Disconnect on error or when the connection shall not be kept open.
                tcpDisconnect(!tcp_ok, rep);
            }
        }
    }

    return udp_ok && tcp_ok;
}


//----------------------------------------------------------------------------
// Issue a JSON report according to options.
//----------------------------------------------------------------------------

bool ts::json::OutputArgs::report(const json::Value& root, std::ostream& stm, Report& rep)
{
    // Process file output.
    if (_json_opt) {
        TextFormatter text(rep);
        text.setStream(stm);
        root.print(text);
        text << ts::endl;
        text.close();
    }

    // Other output forms.
    return reportOthers(root, rep);
}

bool ts::json::OutputArgs::report(const json::Value& root, json::RunningDocument& doc, Report& rep)
{
    // Process file output.
    if (_json_opt) {
        doc.add(root);
    }

    // Other output forms.
    return reportOthers(root, rep);
}
