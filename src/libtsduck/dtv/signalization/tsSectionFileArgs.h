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
//!
//!  @file
//!  Command line arguments for section file processing.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSectionFile.h"
#include "tsReport.h"
#include "tsTime.h"
#include "tsEITOptions.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Command line arguments for section file processing.
    //! @ingroup cmd
    //!
    class TSDUCKDLL SectionFileArgs
    {
    public:
        //!
        //! Constructor.
        //!
        SectionFileArgs();

        // Public fields, by options.
        bool       pack_and_flush;   //!< Pack and flush incomplete tables before exiting.
        bool       eit_normalize;    //!< EIT normalization (ETSI TS 101 211).
        Time       eit_base_time;    //!< Last midnight reference for EIT normalization.
        EITOptions eit_options;      //!< EIT normalization options.

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        //!
        //! Process the content of a section file according to the selected options.
        //! @param [in,out] file Section file to manipulate.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on failure.
        //!
        bool processSectionFile(SectionFile& file, Report& report) const;
    };
}
