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

#include "tsAVCHRDParameters.h"


//----------------------------------------------------------------------------
// Constructor from a binary area
//----------------------------------------------------------------------------

ts::AVCHRDParameters::AVCHRDParameters(const uint8_t* data, size_t size) :
    SuperClass(),
    cpb_cnt_minus1(0),
    bit_rate_scale(0),
    cpb_size_scale(0),
    bit_rate_value_minus1(),
    cpb_size_value_minus1(),
    cbr_flag(),
    initial_cpb_removal_delay_length_minus1(0),
    cpb_removal_delay_length_minus1(0),
    dpb_output_delay_length_minus1(0),
    time_offset_length(0)
{
    parse(data, size);
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AVCHRDParameters::clear()
{
    SuperClass::clear();
    cpb_cnt_minus1 = 0;
    bit_rate_scale = 0;
    cpb_size_scale = 0;
    bit_rate_value_minus1.clear();
    cpb_size_value_minus1.clear();
    cbr_flag.clear();
    initial_cpb_removal_delay_length_minus1 = 0;
    cpb_removal_delay_length_minus1 = 0;
    dpb_output_delay_length_minus1 = 0;
    time_offset_length = 0;
}


//----------------------------------------------------------------------------
// Parse a memory area.
//----------------------------------------------------------------------------

bool ts::AVCHRDParameters::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    return SuperClass::parse(data, size, params);
}

bool ts::AVCHRDParameters::parse(AVCParser& parser, std::initializer_list<uint32_t>)
{
    clear();

    valid =
        parser.ue(cpb_cnt_minus1) &&
        parser.u(bit_rate_scale, 4) &&
        parser.u(cpb_size_scale, 4);

    for (uint32_t i = 0; valid && i <= cpb_cnt_minus1; i++) {
        uint32_t x_bit_rate_value_minus1;
        uint32_t x_cpb_size_value_minus1;
        uint8_t  x_cbr_flag;
        valid = valid &&
            parser.ue(x_bit_rate_value_minus1) &&
            parser.ue(x_cpb_size_value_minus1) &&
            parser.u(x_cbr_flag, 1);
        if (valid) {
            bit_rate_value_minus1.push_back(x_bit_rate_value_minus1);
            cpb_size_value_minus1.push_back(x_cpb_size_value_minus1);
            cbr_flag.push_back(x_cbr_flag);
        }
    }

    valid = valid &&
        parser.u(initial_cpb_removal_delay_length_minus1, 5) &&
        parser.u(cpb_removal_delay_length_minus1, 5) &&
        parser.u(dpb_output_delay_length_minus1, 5) &&
        parser.u(time_offset_length, 5);

    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::AVCHRDParameters::display(std::ostream& out, const UString& margin, int level) const
{
    if (valid) {
#define DISP(n) disp(out, margin, u ## #n, n)
        DISP(cpb_cnt_minus1);
        DISP(bit_rate_scale);
        DISP(cpb_size_scale);
        DISP(bit_rate_value_minus1);
        DISP(cpb_size_value_minus1);
        DISP(cbr_flag);
        DISP(initial_cpb_removal_delay_length_minus1);
        DISP(cpb_removal_delay_length_minus1);
        DISP(dpb_output_delay_length_minus1);
        DISP(time_offset_length);
#undef DISP
    }
    return out;
}
