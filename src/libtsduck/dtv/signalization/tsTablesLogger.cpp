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

#include "tsTablesLogger.h"
#include "tsTablesLoggerFilterRepository.h"
#include "tsTablesDisplay.h"
#include "tsBinaryTable.h"
#include "tsSectionFile.h"
#include "tsArgs.h"
#include "tsFileUtils.h"
#include "tsDuckContext.h"
#include "tsSimulCryptDate.h"
#include "tsDuckProtocol.h"
#include "tsxmlElement.h"
#include "tsjsonArray.h"
#include "tsjsonObject.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::TablesLogger::DEFAULT_LOG_SIZE;
#endif


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TablesLogger::TablesLogger(TablesDisplay& display) :
    TableHandlerInterface(),
    SectionHandlerInterface(),
    _use_text(false),
    _use_xml(false),
    _use_json(false),
    _use_binary(false),
    _use_udp(false),
    _text_destination(),
    _xml_destination(),
    _json_destination(),
    _bin_destination(),
    _udp_destination(),
    _bin_multi_files(false),
    _bin_stdout(false),
    _flush(false),
    _rewrite_xml(false),
    _rewrite_json(false),
    _rewrite_binary(false),
    _log_xml_line(false),
    _log_json_line(false),
    _log_hexa_line(false),
    _log_xml_prefix(),
    _log_json_prefix(),
    _log_hexa_prefix(),
    _udp_local(),
    _udp_ttl(0),
    _udp_raw(false),
    _all_sections(false),
    _all_once(false),
    _invalid_sections(false),
    _invalid_only(false),
    _invalid_versions(false),
    _max_tables(0),
    _time_stamp(false),
    _packet_index(false),
    _logger(false),
    _log_size(DEFAULT_LOG_SIZE),
    _no_duplicate(false),
    _pack_all_sections(false),
    _pack_and_flush(false),
    _fill_eit(false),
    _use_current(true),
    _use_next(false),
    _xml_tweaks(),
    _initial_pids(),
    _xml_options(),
    _display(display),
    _duck(_display.duck()),
    _report(_duck.report()),
    _table_handler(nullptr),
    _section_handler(nullptr),
    _abort(false),
    _exit(false),
    _table_count(0),
    _packet_count(0),
    _demux(_duck),
    _cas_mapper(_duck),
    _xml_doc(_report),
    _x2j_conv(_report),
    _json_doc(_report),
    _bin_file(),
    _sock(false, _report),
    _short_sections(),
    _last_sections(),
    _sections_once(),
    _section_filters()
{
    // Create an instance of each registered section filter.
    TablesLoggerFilterRepository::Instance()->createFilters(_section_filters);
    _report.debug(u"TablesLogger has %s section filters", {_section_filters.size()});
}

ts::TablesLogger::~TablesLogger()
{
    close();
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesLogger::defineArgs(Args& args)
{
    // Define XML options.
    _xml_tweaks.defineArgs(args);

    // Define options from all section filters.
    for (const auto& it : _section_filters) {
        it->defineFilterOptions(args);
    }

    args.option(u"all-once");
    args.help(u"all-once",
              u"Same as --all-sections but collect each section only once per combination of "
              u"PID, table id, table id extension, section number and version.");

    args.option(u"all-sections", 'a');
    args.help(u"all-sections",
              u"Display/save all sections, as they appear in the stream. "
              u"By default, collect complete tables, with all sections of the tables grouped "
              u"and ordered and collect each version of a table only once. "
              u"Note that this mode is incompatible with XML or JSON output since valid XML "
              u"or JSON structures may contain complete tables only.");

    args.option(u"binary-output", 'b', Args::FILENAME);
    args.help(u"binary-output", u"filename",
              u"Save sections in the specified binary output file. "
              u"If empty or '-', the binary sections are written to the standard output. "
              u"See also option -m, --multiple-files.");

    args.option(u"fill-eit");
    args.help(u"fill-eit",
              u"Before exiting, add missing empty sections in EIT's and flush them. "
              u"This can be useful with segmented EIT schedule where empty sections "
              u"at end of segments are usually not transmitted.");

    args.option(u"flush", 'f');
    args.help(u"flush", u"Flush output after each display.");

    args.option(u"exclude-current");
    args.help(u"exclude-current",
              u"Exclude short sections and long sections with \"current\" indicator. "
              u"This is rarely necessary. See also --include-next.");

    args.option(u"include-next");
    args.help(u"include-next",
              u"Include long sections with \"next\" indicator. By default, they are excluded.");

    args.option(u"invalid-sections");
    args.help(u"invalid-sections",
              u"Display and dump invalid sections. These sections are normally dropped "
              u"because they are truncated, incomplete, corrupted, have an invalid CRC32, etc. "
              u"Because these sections are invalid, they cannot be formatted as normal sections. "
              u"Instead, a binary and text dump is displayed.");

    args.option(u"invalid-versions");
    args.help(u"invalid-versions",
              u"Track invalid version numbers in sections. "
              u"Per MPEG rules, the version number of a section with long header shall be updated each time the content of the section is updated. "
              u"With this option, the content of the sections is tracked to detect modified sections without version updates. "
              u"These events are considered as errors.");

    args.option(u"ip-udp", 'i', Args::STRING);
    args.help(u"ip-udp", u"address:port",
              u"Send binary tables over UDP/IP to the specified destination. "
              u"The 'address' specifies an IP address which can be either unicast "
              u"or multicast. It can be also a host name that translates to an IP "
              u"address. The 'port' specifies the destination UDP port.");

    args.option(u"local-udp", 0, Args::STRING);
    args.help(u"local-udp", u"address",
              u"With --ip-udp, when the destination is a multicast address, specify "
              u"the IP address of the outgoing local interface. It can be also a host "
              u"name that translates to a local address.");

    args.option(u"log", 0);
    args.help(u"log", u"Display a short one-line log of each table instead of full table display.");

    args.option(u"log-size", 0, Args::UNSIGNED);
    args.help(u"log-size",
              u"With option --log, specify how many bytes are displayed at the "
              u"beginning of the table payload (the header is not displayed). "
              u"The default is 8 bytes.");

    args.option(u"log-xml-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"log-xml-line", u"'prefix'",
              u"Log each table as one single XML line in the message logger instead of an output file. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the XML text to locate the appropriate line in the logs.");

    args.option(u"log-json-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"log-json-line", u"'prefix'",
              u"Log each table as one single JSON line in the message logger instead of an output file. "
              u"The table is formatted as XML and automated XML-to-JSON conversion is applied. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the JSON text to locate the appropriate line in the logs.");

    args.option(u"log-hexa-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"log-hexa-line", u"'prefix'",
              u"Log each binary table or section (with --all-sections) as one single hexadecimal "
              u"line in the message logger instead of an output binary file. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the hexadecimal text to locate the appropriate line in the logs.");

    args.option(u"max-tables", 'x', Args::POSITIVE);
    args.help(u"max-tables", u"Maximum number of tables to dump. Stop logging tables when this limit is reached.");

    args.option(u"multiple-files", 'm');
    args.help(u"multiple-files",
              u"Create multiple binary output files, one per section. "
              u"A binary output file name must be specified (option -b or --binary-output). "
              u"Assuming that the specified file name has the form 'base.ext', "
              u"each file is created with the name 'base_pXXXX_tXX.ext' for short sections and "
              u"'base_pXXXX_tXX_eXXXX_vXX_sXX.ext' for long sections, where the XX specify the hexadecimal "
              u"values of the PID, TID (table id), TIDext (table id extension), version and section index.");

    args.option(u"no-duplicate");
    args.help(u"no-duplicate",
              u"Do not report consecutive identical tables with a short section in the "
              u"same PID. This can be useful for ECM's. This is the way to display new "
              u"ECM's only. By default, tables with long sections are reported only when "
              u"a new version is detected but tables with a short section are all reported.");

    args.option(u"no-encapsulation");
    args.help(u"no-encapsulation",
              u"With --ip-udp, send the tables as raw binary messages in UDP packets. "
              u"By default, the tables are formatted into TLV messages.");

    args.option(u"only-invalid-sections");
    args.help(u"only-invalid-sections",
              u"Same as --invalid-sections but do not display valid tables and sections.");

    args.option(u"output-file", 'o', Args::FILENAME);
    args.help(u"output-file", u"filename",
              u"Save the tables or sections in human-readable text format in the specified "
              u"file. By default, when no output option is specified, text is produced on "
              u"the standard output. If you need text formatting on the standard output in "
              u"addition to other output like binary files or UPD/IP, explicitly specify "
              u"this option with \"-\" as output file name.\n\n"
              u"By default, the tables are interpreted and formatted as text on the standard "
              u"output. Several destinations can be specified at the same time: human-readable "
              u"text output, binary output, UDP/IP messages.");

    args.option(u"pack-all-sections");
    args.help(u"pack-all-sections",
              u"Same as --all-sections but also modify each long section so that it becomes a "
              u"valid complete table. Its section_number and last_section_number are forced "
              u"to zero. Use with care because this may create inconsistent tables. This "
              u"option can be useful with tables with sparse sections such as EIT's to save "
              u"them in XML format (as an alternative, see also --fill-eit).");

    args.option(u"pack-and-flush");
    args.help(u"pack-and-flush",
              u"Before exiting, pack incomplete tables, ignoring missing sections, and flush "
              u"them. Use with care because this may create inconsistent tables. Unlike option "
              u"--pack-all-sections, --pack-and-flush does not force --all-sections because it "
              u"only applies to the last incomplete tables before exiting.");

    args.option(u"packet-index");
    args.help(u"packet-index",
              u"Display the index of the first and last TS packet of each displayed "
              u"section or table.");

    args.option(u"rewrite-binary");
    args.help(u"rewrite-binary",
              u"With --binary-output, rewrite the same file with each table. "
              u"The specified file always contains one single table, the latest one.");

    args.option(u"rewrite-xml");
    args.help(u"rewrite-xml",
              u"With --xml-output, rewrite the same file with each table. "
              u"The specified file always contains one single table, the latest one.");

    args.option(u"rewrite-json");
    args.help(u"rewrite-json",
              u"With --json-output, rewrite the same file with each table. "
              u"The specified file always contains one single table, the latest one.");

    args.option(u"text-output", 0, Args::FILENAME);
    args.help(u"text-output", u"filename", u"A synonym for --output-file.");

    args.option(u"time-stamp");
    args.help(u"time-stamp", u"Display a time stamp (current local time) with each table.");

    args.option(u"ttl", 0, Args::POSITIVE);
    args.help(u"ttl",
              u"With --ip-udp, specifies the TTL (Time-To-Live) socket option. "
              u"The actual option is either \"Unicast TTL\" or \"Multicast TTL\", "
              u"depending on the destination address. Remember that the default "
              u"Multicast TTL is 1 on most systems.");

    args.option(u"xml-output", 0,  Args::FILENAME);
    args.help(u"xml-output", u"filename",
              u"Save the tables in XML format in the specified file. "
              u"To output the XML text on the standard output, explicitly specify this option with \"-\" as output file name.");

    args.option(u"json-output", 0,  Args::FILENAME);
    args.help(u"json-output", u"filename",
              u"Save the tables in JSON format in the specified file. "
              u"The tables are initially formatted as XML and automated XML-to-JSON conversion is applied. "
              u"To output the JSON text on the standard output, explicitly specify this option with \"-\" as output file name.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TablesLogger::loadArgs(DuckContext& duck, Args& args)
{
    // Type of output, text is the default when no other logging option is specified.
    _use_xml = args.present(u"xml-output");
    _use_json = args.present(u"json-output");
    _use_binary = args.present(u"binary-output");
    _use_udp = args.present(u"ip-udp");
    _log_xml_line = args.present(u"log-xml-line");
    _log_json_line = args.present(u"log-json-line");
    _log_hexa_line = args.present(u"log-hexa-line");
    _use_text = args.present(u"output-file") ||
                args.present(u"text-output") ||
                (!_use_xml && !_use_json && !_use_binary && !_use_udp &&
                 !_log_xml_line && !_log_json_line && !_log_hexa_line &&
                 _table_handler == nullptr && _section_handler == nullptr);

    // --output-file and --text-output are synonyms.
    if (args.present(u"output-file") && args.present(u"text-output")) {
        args.error(u"--output-file and --text-output are synonyms, do not use both");
    }

    // Output destinations.
    args.getValue(_xml_destination, u"xml-output");
    args.getValue(_json_destination, u"json-output");
    args.getValue(_bin_destination, u"binary-output");
    args.getValue(_udp_destination, u"ip-udp");
    args.getValue(_text_destination, u"output-file", args.value(u"text-output").c_str());

    _bin_stdout = _use_binary && (_bin_destination.empty() || _bin_destination == u"-");
    _bin_multi_files = !_bin_stdout && args.present(u"multiple-files");
    _rewrite_binary = !_bin_stdout && args.present(u"rewrite-binary");
    _rewrite_xml = args.present(u"rewrite-xml");
    _rewrite_json = args.present(u"rewrite-json");
    args.getValue(_log_xml_prefix, u"log-xml-line");
    args.getValue(_log_json_prefix, u"log-json-line");
    args.getValue(_log_hexa_prefix, u"log-hexa-line");
    _flush = args.present(u"flush");
    _udp_local = args.value(u"local-udp");
    args.getIntValue(_udp_ttl, u"ttl", 0);
    _pack_all_sections = args.present(u"pack-all-sections");
    _pack_and_flush = args.present(u"pack-and-flush");
    _fill_eit = args.present(u"fill-eit");
    _all_once = args.present(u"all-once");
    _all_sections = _all_once || _pack_all_sections || args.present(u"all-sections");
    _invalid_only = args.present(u"only-invalid-sections");
    _invalid_sections = _invalid_only || args.present(u"invalid-sections");
    _invalid_versions = args.present(u"invalid-versions");
    args.getIntValue(_max_tables, u"max-tables", 0);
    _time_stamp = args.present(u"time-stamp");
    _packet_index = args.present(u"packet-index");
    _logger = args.present(u"log");
    args.getIntValue(_log_size, u"log-size", DEFAULT_LOG_SIZE);
    _no_duplicate = args.present(u"no-duplicate");
    _udp_raw = args.present(u"no-encapsulation");
    _use_current = !args.present(u"exclude-current");
    _use_next = args.present(u"include-next");

    // Check consistency of options.
    if (_rewrite_binary && _bin_multi_files) {
        args.error(u"options --rewrite-binary and --multiple-files are incompatible");
        return false;
    }
    if ((_use_xml || _use_json || _log_xml_line || _log_json_line) && (_all_sections && !_pack_all_sections)) {
        args.error(u"filtering sections (--all-sections or --all-once) is incompatible with XML or JSON output");
        return false;
    }

    // Load options from all section filters.
    _initial_pids.reset();
    for (const auto& it : _section_filters) {
        PIDSet pids;
        if (!it->loadFilterOptions(_duck, args, pids)) {
            return false;
        }
        _initial_pids |= pids;
    }

    // XML options.
    _xml_options.setPID = true;
    _xml_options.setLocalTime = _time_stamp;
    _xml_options.setPackets = _packet_index;
    return _xml_tweaks.loadArgs(duck, args);
}


//----------------------------------------------------------------------------
// Open files, start operations.
//----------------------------------------------------------------------------

bool ts::TablesLogger::open()
{
    // Reinitialize working data.
    _abort = _exit = false;
    _table_count = 0;
    _packet_count = 0;
    _demux.reset();
    _cas_mapper.reset();
    _xml_doc.clear();
    _json_doc.close();
    _short_sections.clear();
    _last_sections.clear();
    _sections_once.clear();

    if (_bin_file.is_open()) {
        _bin_file.close();
    }
    if (_sock.isOpen()) {
        _sock.close(_report);
    }

    // Set PID's to filter.
    _demux.setPIDFilter(_initial_pids);

    // Set either a table or section handler, depending on --all-sections
    _demux.setTableHandler(_all_sections ? nullptr : this);
    _demux.setSectionHandler(_all_sections ? this : nullptr);
    _demux.setInvalidSectionHandler(_invalid_sections ? this : nullptr);

    // Type of sections to get.
    _demux.setCurrentNext(_use_current, _use_next);
    _cas_mapper.setCurrentNext(_use_current, _use_next);

    // Track invalid section versions.
    _demux.trackInvalidSectionVersions(_invalid_versions);
    _cas_mapper.trackInvalidSectionVersions(_invalid_versions);

    // Log TS error at verbose level.
    _demux.setTransportErrorLogLevel(Severity::Verbose);

    // Load the XML model for tables if we need to convert to JSON.
    if ((_use_json || _log_json_line) && !SectionFile::LoadModel(_x2j_conv)) {
        return false;
    }

    // Open/create the text output.
    if (_use_text && !_duck.setOutput(_text_destination)) {
        _abort = true;
        return false;
    }

    // Set XML options in document and converter.
    _xml_doc.setTweaks(_xml_tweaks);
    _x2j_conv.setTweaks(_xml_tweaks);

    // Open/create the XML output.
    if (_use_xml && !_rewrite_xml && _xml_doc.open(u"tsduck", u"", _xml_destination, std::cout) == nullptr) {
        _abort = true;
        return false;
    }

    // Open/create the JSON output.
    if (_use_json && !_rewrite_json) {
        json::ValuePtr root;
        if (_xml_tweaks.x2jIncludeRoot) {
            root = new json::Object;
            root->add(u"#name", u"tsduck");
            root->add(u"#nodes", json::ValuePtr(new json::Array));
        }
        if (!_json_doc.open(root, _json_destination, std::cout)) {
            _abort = true;
            return false;
        }
    }

    // Open/create the binary output.
    if (_use_binary && !_bin_multi_files && !_rewrite_binary && !createBinaryFile(_bin_destination)) {
        _abort = true;
        return false;
    }

    // Initialize UDP output.
    if (_use_udp) {
        // Create UDP socket.
        _abort =
            !_sock.open(_report) ||
            !_sock.setDefaultDestination(_udp_destination, _report) ||
            (!_udp_local.empty() && !_sock.setOutgoingMulticast(_udp_local, _report)) ||
            (_udp_ttl > 0 && !_sock.setTTL(_udp_ttl, _report));
        if (_abort) {
            _sock.close(_report);
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Close all operations, flush tables if required, close files and sockets.
//----------------------------------------------------------------------------

void ts::TablesLogger::close()
{
    if (!_exit) {

        // Pack sections in incomplete tables if required.
        if (_pack_and_flush) {
            _demux.packAndFlushSections();
        }
        if (_fill_eit) {
            _demux.fillAndFlushEITs();
        }

        // Close files and documents.
        _xml_doc.close();
        _json_doc.close();
        if (_bin_file.is_open()) {
            _bin_file.close();
        }
        if (_sock.isOpen()) {
            _sock.close(_report);
        }

        // Now completed.
        _exit = true;
    }
}


//----------------------------------------------------------------------------
// The following method feeds the logger with a TS packet.
//----------------------------------------------------------------------------

void ts::TablesLogger::feedPacket(const TSPacket& pkt)
{
    if (!completed()) {
        _demux.feedPacket(pkt);
        _cas_mapper.feedPacket(pkt);
        _packet_count++;
    }
}


//----------------------------------------------------------------------------
// Detect and track duplicate section by PID.
//----------------------------------------------------------------------------

bool ts::TablesLogger::isDuplicate(PID pid, const Section& section, std::map<PID,ByteBlock> TablesLogger::* tracker)
{
    // Get a SHA-1 for the section.
    const ByteBlock hash(section.hash());
    ByteBlock& last((this->*tracker)[pid]);
    if (last.empty() || last != hash) {
        // Not the same section, keep the hash for next time.
        last = hash;
        return false;
    }
    else {
        // Same section (same hash) as previously.
        return true;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void ts::TablesLogger::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    // Give up if completed.
    if (completed()) {
        return;
    }

    assert(table.sectionCount() > 0);
    const PID pid = table.sourcePID();
    const uint16_t cas = _cas_mapper.casId(table.sourcePID());

    // Ignore table if not to be filtered. Keep the table if at least one section shall be kept.
    bool keep = false;
    for (size_t i = 0; !keep && i < table.sectionCount(); ++i) {
        keep = isFiltered(*table.sectionAt(i), cas);
    }
    if (!keep) {
        return;
    }

    // Ignore duplicate tables with a short section.
    if (_no_duplicate && table.isShortSection() && isDuplicate(pid, *table.sectionAt(0), &TablesLogger::_short_sections)) {
        // Same section as previously, ignore it.
        return;
    }

    // Filtering done, now save table in various formats.

    // Save table in text format.
    if (_use_text && !_invalid_only) {
        preDisplay(table.firstTSPacketIndex(), table.lastTSPacketIndex());
        if (_logger) {
            // Short log message
            logSection(*table.sectionAt(0));
        }
        else {
            // Full table formatting
            _display.displayTable(table, u"", _cas_mapper.casId(pid));
            _display << std::endl;
        }
        postDisplay();
    }

    // Save table in XML format.
    if (_use_xml) {
        if (_rewrite_xml) {
            // Build and save a new document each time.
            xml::Document doc(_report);
            doc.initialize(u"tsduck");
            table.toXML(_duck, doc.rootElement(), _xml_options);
            doc.save(_xml_destination, 2);
        }
        else {
            // Just add the table in the running doc.
            // Convert the table into an XML structure, print and delete the XML table.
            table.toXML(_duck, _xml_doc.rootElement(), _xml_options);
            _xml_doc.flush();
        }
    }

    // Save table in JSON format.
    if (_use_json) {
        // First, build an XML document with the table.
        xml::Document doc(_report);
        doc.initialize(u"tsduck");
        table.toXML(_duck, doc.rootElement(), _xml_options);
        if (_rewrite_json) {
            // Convert to JSON and save a new document each time.
            _x2j_conv.convertToJSON(doc)->save(_json_destination, 2, true, _report);
        }
        else {
            // Convert to JSON. Force "tsduck" root to appear so that the path to the first table is always the same.
            // Query the first (and only) converted table and add it to the running document.
            _json_doc.add(_x2j_conv.convertToJSON(doc, true)->query(u"#nodes[0]"));
        }
    }

    // Save table in binary format.
    if (_use_binary) {
        // In case of rewrite for each table, create a new file.
        if (_rewrite_binary && !createBinaryFile(_bin_destination)) {
            return;
        }
        // Save each section in binary format
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            saveBinarySection(*table.sectionAt(i));
        }
        if (_rewrite_binary && _bin_file.is_open()) {
            _bin_file.close();
        }
    }

    // Log table as a one-liner XML and/or JSON.
    if (_log_xml_line || _log_json_line) {
        logXMLJSON(table);
    }

    // Log table as a one-liner hexadecimal.
    if (_log_hexa_line) {
        UString line;
        // Concatenate all sections in hexa.
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            line.append(UString::Dump(table.sectionAt(i)->content(), table.sectionAt(i)->size(), UString::COMPACT));
        }
        _report.info(_log_hexa_prefix + line);
    }

    // Send binary table in UDP message.
    if (_use_udp) {
        sendUDP(table);
    }

    // Notify table, either at once or section by section.
    if (_table_handler != nullptr) {
        _table_handler->handleTable(demux, table);
    }
    else if (_section_handler != nullptr) {
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            _section_handler->handleSection(demux, *table.sectionAt(i));
        }
    }

    // Check max table count
    _table_count++;
    if (_max_tables > 0 && _table_count >= _max_tables) {
        _exit = true;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Only used with option --all-sections
//----------------------------------------------------------------------------

void ts::TablesLogger::handleSection(SectionDemux& demux, const Section& sect)
{
    const PID pid = sect.sourcePID();
    const uint16_t cas = _cas_mapper.casId(sect.sourcePID());

    // With option --all-once, track duplicate PID/TID/TDIext/secnum/version.
    if (_all_once) {
        // Pack PID/TID/TDIext/secnum/version into one single 64-bit integer.
        const uint64_t id =
            (uint64_t(pid) << 40) |
            (uint64_t(sect.tableId()) << 32) |
            (uint64_t(sect.tableIdExtension()) << 16) |
            (uint64_t(sect.sectionNumber()) << 8) |
            uint64_t(sect.version());
        if (_sections_once.count(id) != 0) {
            // Already found this one, give up.
            return;
        }
        else {
            // Remember this combination.
            _sections_once.insert(id);
        }
    }

    // With option --pack-all-sections, force the processing of a complete table.
    if (_pack_all_sections) {
        BinaryTable table;
        table.addSection(new Section(sect, ShareMode::SHARE));
        table.packSections();
        if (table.isValid()) {
            handleTable(demux, table);
        }
        return;
    }

    // Give up if completed.
    if (completed()) {
        return;
    }

    // Ignore section if not to be filtered
    if (!isFiltered(sect, cas)) {
        return;
    }

    // Ignore duplicate sections.
    if (_no_duplicate && isDuplicate(pid, sect, &TablesLogger::_last_sections)) {
        // Same section (same hash) as previously, ignore it.
        return;
    }

    // Filtering done, now save data.
    // Note that no XML can be produced since valid XML structures contain complete tables only.

    if (_use_text && !_invalid_only) {
        preDisplay(sect.firstTSPacketIndex(), sect.lastTSPacketIndex());
        if (_logger) {
            // Short log message
            logSection(sect);
        }
        else {
            // Full section formatting.
            _display.displaySection(sect, u"", _cas_mapper.casId(pid));
            _display << std::endl;
        }
        postDisplay();
    }

    if (_use_binary) {
        // In case of rewrite for each section, create a new file.
        if (_rewrite_binary && !createBinaryFile(_bin_destination)) {
            return;
        }
        saveBinarySection(sect);
        if (_rewrite_binary && _bin_file.is_open()) {
            _bin_file.close();
        }
    }

    if (_log_hexa_line) {
        // Log section as a one-liner hexadecimal.
        _report.info(_log_hexa_prefix + UString::Dump(sect.content(), sect.size(), UString::COMPACT));
    }

    if (_use_udp) {
        sendUDP(sect);
    }

    if (_section_handler != nullptr) {
        _section_handler->handleSection(demux, sect);
    }

    // Check max table count (actually count sections with --all-sections)
    _table_count++;
    if (_max_tables > 0 && _table_count >= _max_tables) {
        _exit = true;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Only used with option --invalid-sections
//----------------------------------------------------------------------------

void ts::TablesLogger::handleInvalidSection(SectionDemux& demux, const DemuxedData& ddata)
{
    const uint8_t* const data = ddata.content();
    const size_t size = ddata.size();

    // Try to determine the reason for the invalid section.
    const size_t sec_size = Section::SectionSize(data, size);
    const bool is_long = Section::StartLongSection(data, size);
    UString reason;
    if (sec_size > 0 && sec_size != size) {
        reason.format(u"invalid section size: %d, data size: %d", {sec_size, size});
    }
    else if (is_long && sec_size > 4 && CRC32(data, sec_size - 4) != GetUInt32(data + sec_size)) {
        reason = u"invalid CRC32, corrupted section";
    }
    else if (is_long && data[6] > data[7]) {
        reason.format(u"invalid section number: %d, last section: %d", {data[6], data[7]});
    }

    preDisplay(ddata.firstTSPacketIndex(), ddata.lastTSPacketIndex());
    if (_logger) {
        // Short log message
        logInvalid(ddata, reason);
    }
    else {
        _display.displayInvalidSection(ddata, reason, u"", _cas_mapper.casId(ddata.sourcePID()));
        _display << std::endl;
    }
    postDisplay();
}


//----------------------------------------------------------------------------
// Log XML or JSON one-liners.
//----------------------------------------------------------------------------

void ts::TablesLogger::logXMLJSON(const BinaryTable& table)
{
    // Build an XML document.
    xml::Document doc;
    doc.initialize(u"tsduck");
    xml::Element* elem = table.toXML(_duck, doc.rootElement(), _xml_options);
    if (elem == nullptr) {
        // Error serializing the table, error message already printed.
        return;
    }

    // Initialize a text formatter for one-liner.
    TextFormatter text(_report);
    text.setString();
    text.setEndOfLineMode(TextFormatter::EndOfLineMode::SPACING);

    // Log the XML line.
    if (_log_xml_line) {
        doc.print(text);
        _report.info(_log_xml_prefix + text.toString());
    }

    // Log the JSON line.
    if (_log_json_line) {

        // Convert the XML document into JSON.
        // Force "tsduck" root to appear so that the path to the first table is always the same.
        const json::ValuePtr root(_x2j_conv.convertToJSON(doc, true));

        // Reset the text formatter if already used for XML.
        if (_log_xml_line) {
            text.setString();
        }

        // Query the first (and only) converted table and log it as one line.
        root->query(u"#nodes[0]").print(text);
        _report.info(_log_json_prefix + text.toString());
    }
}


//----------------------------------------------------------------------------
// Send UDP table and section.
//----------------------------------------------------------------------------

void ts::TablesLogger::sendUDP(const ts::BinaryTable& table)
{
    ByteBlockPtr bin(new ByteBlock);

    // Minimize allocation by reserving over size
    bin->reserve(table.totalSize() + 32 + 4 * table.sectionCount());

    if (_udp_raw) {
        // Add raw content of each section the message
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            const Section& sect(*table.sectionAt(i));
            bin->append(sect.content(), sect.size());
        }
    }
    else {
        // Build a TLV message.
        duck::LogTable msg;
        msg.pid = table.sourcePID();
        msg.timestamp = SimulCryptDate(Time::CurrentLocalTime());
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            msg.sections.push_back(table.sectionAt(i));
        }
        tlv::Serializer serial(bin);
        msg.serialize(serial);
    }

    // Send TLV message over UDP
    _sock.send(bin->data(), bin->size(), _report);
}

void ts::TablesLogger::sendUDP(const ts::Section& section)
{
    if (_udp_raw) {
        // Send raw content of section as one single UDP message
        _sock.send(section.content(), section.size(), _report);
    }
    else {
        // Build a TLV message.
        duck::LogSection msg;
        msg.pid = section.sourcePID();
        msg.timestamp = SimulCryptDate(Time::CurrentLocalTime());
        msg.section = new Section(section, ShareMode::SHARE);

        // Serialize the message.
        ByteBlockPtr bin(new ByteBlock);
        tlv::Serializer serial(bin);
        msg.serialize(serial);

        // Send TLV message over UDP
        _sock.send(bin->data(), bin->size(), _report);
    }
}


//----------------------------------------------------------------------------
// Static routine to analyze UDP messages as sent with option --ip-udp.
//----------------------------------------------------------------------------

bool ts::TablesLogger::AnalyzeUDPMessage(const uint8_t* data, size_t size, bool no_encapsulation, SectionPtrVector& sections, Time& timestamp)
{
    // Clear output parameters.
    sections.clear();
    timestamp = Time::Epoch;

    // Filter invalid parameters.
    if (data == nullptr) {
        return false;
    }

    Variable<SimulCryptDate> scDate;
    Variable<PID> pid;

    if (no_encapsulation) {

        // Raw sections in UDP packets.
        // Loop on sections in the packet.
        while (size > 0) {
            const size_t sect_size = Section::SectionSize(data, size);
            assert(sect_size <= size);
            if (sect_size == 0) {
                return false;
            }
            const SectionPtr section(new Section(data, sect_size, ts::PID_NULL, ts::CRC32::CHECK));
            if (!section->isValid()) {
                return false;
            }
            sections.push_back(section);
            data += sect_size;
            size -= sect_size;
        }
    }
    else {
        // TLV messages in UDP packets. Decode the message.
        tlv::MessageFactory mf(data, size, duck::Protocol::Instance());
        tlv::MessagePtr msg(mf.factory());

        // We expected only two possible messages:
        const duck::LogSection* logSection = dynamic_cast<const duck::LogSection*>(msg.pointer());
        const duck::LogTable* logTable = dynamic_cast<const duck::LogTable*>(msg.pointer());

        if (logSection != nullptr) {
            scDate = logSection->timestamp;
            pid = logSection->pid;
            if (logSection->section.isNull() || !logSection->section->isValid()) {
                return false;
            }
            else {
                sections.push_back(logSection->section);
            }
        }
        else if (logTable != nullptr) {
            scDate = logTable->timestamp;
            pid = logTable->pid;
            sections = logTable->sections;
        }
        else {
            return false;
        }
    }

    // Set the PID in all sections.
    if (pid.set()) {
        for (auto& it : sections) {
            if (!it.isNull()) {
                it->setSourcePID(pid.value());
            }
        }
    }

    // Interpret the timestamp.
    if (scDate.set()) {
        try {
            timestamp = Time(scDate.value());
        }
        catch (...) {
            timestamp = Time::Epoch;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Create a binary file. On error, set _abort and return false.
//----------------------------------------------------------------------------

bool ts::TablesLogger::createBinaryFile(const ts::UString& name)
{
    if (_bin_stdout) {
        // Make sure that the standard output is in binary mode.
        return SetBinaryModeStdout(_report);
    }
    else {
        _report.verbose(u"creating %s", {name});
        _bin_file.open(name.toUTF8().c_str(), std::ios::out | std::ios::binary);
        if (_bin_file) {
            return true;
        }
        else {
            _report.error(u"error creating %s", {name});
            _abort = true;
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Save a section in a binary file
//----------------------------------------------------------------------------

void ts::TablesLogger::saveBinarySection(const Section& sect)
{
    // Create individual file for this section if required.
    if (_bin_multi_files) {
        // Build a unique file name for this section
        UString outname(PathPrefix(_bin_destination));
        outname.format(u"_p%04X_t%02X", {sect.sourcePID(), sect.tableId()});
        if (sect.isLongSection()) {
            outname.format(u"_e%04X_v%02X_s%02X", {sect.tableIdExtension(), sect.version(), sect.sectionNumber()});
        }
        outname += PathSuffix(_bin_destination);
        // Create the output file
        if (!createBinaryFile(outname)) {
            return;
        }
    }

    // Write the section to the file
    const bool success = _bin_stdout ? bool(sect.write(std::cout, _report)) : bool(sect.write(_bin_file, _report));
    _abort = _abort || !success;

    // Close individual files
    if (_bin_multi_files && _bin_file.is_open()) {
        _bin_file.close();
    }
}


//----------------------------------------------------------------------------
// Log a table (option --log)
//----------------------------------------------------------------------------

ts::UString ts::TablesLogger::logHeader(const DemuxedData& data)
{
    UString header;
    if (_time_stamp) {
        header.format(u"%s: ", {Time::CurrentLocalTime()});
    }
    if (_packet_index) {
        header.format(u"Packet %'d to %'d, ", {data.firstTSPacketIndex(), data.lastTSPacketIndex()});
    }
    header.format(u"PID 0x%X", {data.sourcePID()});
    return header;
}

void ts::TablesLogger::logSection(const Section& sect)
{
    UString header(logHeader(sect));
    header.format(u", TID 0x%X", {sect.tableId()});
    if (sect.isLongSection()) {
        header.format(u", TIDext 0x%X, V%d, Sec %d/%d", {sect.tableIdExtension(), sect.version(), sect.sectionNumber(), sect.lastSectionNumber()});
    }
    header.append(u": ");
    _display.logSectionData(sect, header, _log_size, _cas_mapper.casId(sect.sourcePID()));
}

void ts::TablesLogger::logInvalid(const DemuxedData& data, const UString& reason)
{
    // Number of bytes to log:
    const size_t size = _log_size == 0 ? data.size() : std::min(_log_size, data.size());

    _display << logHeader(data) << ", invalid section";
    if (!reason.empty()) {
        _display << " (" << reason << ")";
    }
    _display << ": " << UString::Dump(data.content(), size, UString::SINGLE_LINE);
    if (data.size() > size) {
        _display << " ...";
    }
    _display << std::endl;
}


//----------------------------------------------------------------------------
// Check if a specific section must be filtered
//----------------------------------------------------------------------------

bool ts::TablesLogger::isFiltered(const Section& sect, uint16_t cas)
{
    // By default, keep the section.
    bool status = true;

    // Call all section filters. Keep the section if all filters agree.
    // Make sure to call all filters, even after one returned false to collect additional PID's.
    for (const auto& it : _section_filters) {
        PIDSet pids;
        if (!it->filterSection(_duck, sect, cas, pids)) {
            status = false;
        }
        _demux.addPIDs(pids);
    }
    return status;
}


//----------------------------------------------------------------------------
// Display header information, before a table
//----------------------------------------------------------------------------

void ts::TablesLogger::preDisplay(PacketCounter first, PacketCounter last)
{
    std::ostream& strm(_duck.out());

    // Initial spacing
    if (_table_count == 0 && !_logger) {
        strm << std::endl;
    }

    // Display time stamp if required
    if ((_time_stamp || _packet_index) && !_logger) {
        strm << "* ";
        if (_time_stamp) {
            strm << "At " << Time::CurrentLocalTime();
        }
        if (_packet_index && _time_stamp) {
            strm << ", ";
        }
        if (_packet_index) {
            strm << UString::Format(u"First TS packet: %'d, last: %'d", {first, last});
        }
        strm << std::endl;
    }
}


//----------------------------------------------------------------------------
//  Post-display action
//----------------------------------------------------------------------------

void ts::TablesLogger::postDisplay()
{
    // Flush output file if required
    if (_flush) {
        _duck.flush();
    }
}


//----------------------------------------------------------------------------
// Report the demux errors (if any)
//----------------------------------------------------------------------------

void ts::TablesLogger::reportDemuxErrors(std::ostream& strm)
{
    if (_demux.hasErrors()) {
        SectionDemux::Status status(_demux);
        strm << "* PSI/SI analysis errors:" << std::endl;
        status.display(strm, 4, true);
    }
}

void ts::TablesLogger::reportDemuxErrors(Report& report, int level)
{
    if (_demux.hasErrors()) {
        SectionDemux::Status status(_demux);
        status.display(report, level, UString(), true);
    }
}
