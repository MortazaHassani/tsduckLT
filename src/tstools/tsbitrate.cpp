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
//  Evaluate the bitrate of a transport stream based on PCR values.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTSFile.h"
#include "tsPCRAnalyzer.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        uint32_t           min_pcr;        // Min # of PCR per PID
        uint16_t           min_pid;        // Min # of PID
        ts::UString        pcr_name;       // Time stamp type name
        bool               use_dts;        // Use DTS instead of PCR
        bool               all;            // All packets analysis
        bool               full;           // Full analysis
        bool               value_only;     // Output value only
        bool               ignore_errors;  // Ignore TS errors
        ts::UString        infile;         // Input file name
        ts::TSPacketFormat format;         // Input file format.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Evaluate the bitrate of a transport stream", u"[options] [filename]"),
    min_pcr(0),
    min_pid(0),
    pcr_name(),
    use_dts(false),
    all(false),
    full(false),
    value_only(false),
    ignore_errors(false),
    infile(),
    format(ts::TSPacketFormat::AUTODETECT)
{
    ts::DefineTSPacketFormatInputOption(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"MPEG capture file (standard input if omitted).");

    option(u"all", 'a');
    help(u"all",
         u"Analyze all packets in the input file. By default, stop analysis when "
         u"enough PCR information has been collected.");

    option(u"dts", 'd');
    help(u"dts",
         u"Use DTS (Decoding Time Stamps) from video PID's instead of PCR "
         u"(Program Clock Reference) from the transport layer");

    option(u"full", 'f');
    help(u"full",
         u"Full analysis. The file is entirely analyzed (as with --all) and the "
         u"final report includes a complete per PID bitrate analysis.");

    option(u"ignore-errors", 'i');
    help(u"ignore-errors",
         u"Ignore transport stream errors such as discontinuities. When errors are "
         u"not ignored (the default), the bitrate of the original stream (before corruptions) "
         u"is evaluated. When errors are ignored, the bitrate of the received stream is "
         u"evaluated, missing packets being considered as non-existent.");

    option(u"min-pcr", 0, POSITIVE);
    help(u"min-pcr", u"p analysis when that number of PCR are read from the required minimum number of PID (default: 64).");

    option(u"min-pid", 0, INTEGER, 0, 1, 1, ts::PID_MAX);
    help(u"min-pid", u"Minimum number of PID's to get PCR from (default: 1).");

    option(u"value-only", 'v');
    help(u"value-only",
         u"Display only the bitrate value, in bits/seconds, based on "
         u"188-byte packets. Useful to reuse the value in scripts.");

    analyze(argc, argv);

    getValue(infile, u"");
    full = present(u"full");
    all = full || present(u"all");
    value_only = present(u"value-only");
    getIntValue(min_pcr, u"min-pcr", 64);
    getIntValue(min_pid, u"min-pid", 1);
    use_dts = present(u"dts");
    pcr_name = use_dts ? u"DTS" : u"PCR";
    ignore_errors = present(u"ignore-errors");
    format = ts::LoadTSPacketFormatInputOption(*this);

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Decode command line options.
    Options opt(argc, argv);

    // Configure the PCR analyzer.
    ts::PCRAnalyzer zer(opt.min_pid, opt.min_pcr);
    zer.setIgnoreErrors(opt.ignore_errors);
    if (opt.use_dts) {
        zer.resetAndUseDTS(opt.min_pid, opt.min_pcr);
    }

    // Open the TS file.
    ts::TSFile file;
    if (!file.openRead(opt.infile, 1, 0, opt, opt.format)) {
        return EXIT_FAILURE;
    }

    // Read all packets in the file and pass them to the PCR analyzer.
    ts::TSPacket pkt;
    while (file.readPackets(&pkt, nullptr, 1, opt) > 0 && (!zer.feedPacket(pkt) || opt.all)) {}
    file.close(opt);

    // Display results.
    ts::PCRAnalyzer::Status status;
    zer.getStatus(status);

    if (!status.bitrate_valid) {
        opt.error(u"cannot compute transport bitrate, insufficient %s", {opt.pcr_name});
        if (!opt.full) {
            return EXIT_FAILURE;
        }
    }

    if (opt.value_only) {
        std::cout << status.bitrate_188 << std::endl;
        return EXIT_SUCCESS;
    }

    if (opt.full) {
        std::cout << std::endl
                  << "Transport Stream" << std::endl
                  << "----------------" << std::endl;
        if (!opt.infile.empty()) {
            std::cout << "File           : " << opt.infile << std::endl;
        }
        std::cout << "TS packets     : " << ts::UString::Decimal(status.packet_count) << std::endl
                  << opt.pcr_name << "            : " << ts::UString::Decimal(status.pcr_count) << std::endl
                  << "PIDs with " << opt.pcr_name << "  : " << ts::UString::Decimal(status.pcr_pids) << std::endl;
    }

    std::cout << "TS bitrate" << (opt.full ? "     " : "") << ": "
              << ts::UString::Decimal(status.bitrate_188.toInt()) << " b/s (188-byte), "
              << ts::UString::Decimal(status.bitrate_204.toInt()) << " b/s (204-byte)"
              << std::endl;

    if (opt.full) {
        std::cout << std::endl
                  << "PID              TS Packets  Bitrate (188-byte)  Bitrate (204-byte)" << std::endl
                  << "-------------  ------------  ------------------  ------------------" << std::endl;
        for (ts::PID pid = 0; pid < ts::PID_MAX; pid++) {
            ts::PacketCounter pcount = zer.packetCount (pid);
            if (pcount > 0) {
                std::cout << ts::UString::Format(u"%4d (0x%04X)  %12'd  %14'd b/s  %14'd b/s", {pid, pid, pcount, zer.bitrate188(pid), zer.bitrate204(pid)})
                          << std::endl;
            }
        }
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}
