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

#include "tsSysInfo.h"
#include "tsSysUtils.h"
#include "tsMemory.h"

#if defined(TS_MAC)
    #include "tsMacPList.h"
#endif

#if defined(TS_UNIX)
    #include "tsSysCtl.h"
#endif

// Define singleton instance
TS_DEFINE_SINGLETON(ts::SysInfo);


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::SysInfo::SysInfo() :
#if defined(TS_LINUX)
    _isLinux(true),
#else
    _isLinux(false),
#endif
    _isFedora(false),
    _isRedHat(false),
    _isUbuntu(false),
    _isDebian(false),
    _isRaspbian(false),
#if defined(TS_MAC)
    _isMacOS(true),
#else
    _isMacOS(false),
#endif
#if defined(TS_FREEBSD)
    _isFreeBSD(true),
#else
    _isFreeBSD(false),
#endif
#if defined(TS_WINDOWS)
    _isWindows(true),
#else
    _isWindows(false),
#endif
#if defined(TS_I386)
    _isIntel32(true),
#else
    _isIntel32(false),
#endif
#if defined(TS_X86_64)
    _isIntel64(true),
#else
    _isIntel64(false),
#endif
#if defined(TS_ARM)
    _isArm32(true),
#else
    _isArm32(false),
#endif
#if defined(TS_ARM64)
    _isArm64(true),
#else
    _isArm64(false),
#endif
    _systemVersion(),
    _systemName(),
    _hostName(),
#if defined(TS_I386)
    _cpuName(u"Intel x86"),
#elif defined(TS_X86_64)
    _cpuName(u"Intel x86-64"),
#elif defined(TS_ARM64)
    _cpuName(u"Arm-64"),
#elif defined(TS_ARM)
    _cpuName(u"Arm-32"),
#elif defined(TS_MIPS)
    _cpuName(u"MIPS"),
#elif defined(TS_SPARC)
    _cpuName(u"SPARC"),
#elif defined(TS_ALPHA)
    _cpuName(u"Alpha"),
#elif defined(TS_POWERPC64)
    _cpuName(u"PowerPC-64"),
#elif defined(TS_POWERPC)
    _cpuName(u"PowerPC"),
#else
    _cpuName(u"unknown CPU"),
#endif
    _memoryPageSize(0)
{
    //
    // Get operating system name and version.
    //
#if defined(TS_LINUX)

    // On Linux, the actual system shall be determined dynamically.
    UStringList lines;
    Environment env;
    if (UString::Load(lines, u"/etc/fedora-release") && !lines.empty()) {
        _isFedora = true;
        _systemName = u"Fedora";
        _systemVersion = lines.front();
    }
    else if (UString::Load(lines, u"/etc/redhat-release") && !lines.empty()) {
        _isRedHat = true;
        _systemName = u"Red Hat Entreprise Linux";
        _systemVersion = lines.front();
    }
    else if (LoadEnvironment(env, u"/etc/lsb-release")) {
        _systemName = env[u"DISTRIB_ID"];
        _systemVersion = env[u"DISTRIB_DESCRIPTION"];
        if (_systemVersion.empty()) {
            _systemVersion = env[u"DISTRIB_RELEASE"];
        }
        _isUbuntu = _systemName.similar(u"Ubuntu");
        _isDebian = _systemName.similar(u"Debian");
        _isRaspbian = _systemName.similar(u"Raspbian");
    }
    if (_systemName.empty() && UString::Load(lines, u"/etc/debian_version") && !lines.empty()) {
        _systemName = u"Debian";
        _systemVersion = u"Debian " + lines.front();
    }
    if (_systemName.empty()) {
        _systemName = u"Linux";
    }

#elif defined(TS_MAC)

    // Get system version.
    MacPList sysList(u"/System/Library/CoreServices/SystemVersion.plist");
    const UString sysName(sysList[u"ProductName"]);
    const UString sysVersion(sysList[u"ProductVersion"]);
    if (!sysName.empty() && !sysVersion.empty()) {
        _systemName = sysName;
        _systemVersion = sysName + u" " + sysVersion;
    }
    else {
        _systemName = u"macOS";
    }

    // Get kernel version.
    const UString osrelease(SysCtrlString({CTL_KERN, KERN_OSRELEASE}));
    if (!osrelease.empty()) {
        if (!_systemVersion.empty()) {
            _systemVersion += u", ";
        }
        _systemVersion += u"Darwin " + osrelease;
    }

#elif defined(TS_FREEBSD)

    _systemName = SysCtrlString({CTL_KERN, KERN_OSTYPE});
    if (_systemName.empty()) {
        _systemName = u"FreeBSD";
    }

    _systemVersion = SysCtrlString({CTL_KERN, KERN_VERSION});
    if (_systemVersion.empty()) {
        _systemVersion = SysCtrlString({CTL_KERN, KERN_OSRELEASE});
    }

#elif defined(TS_WINDOWS)

    _systemName = u"Windows";

    TS_PUSH_WARNING()
    TS_MSC_NOWARNING(4996) // warning C4996: 'GetVersionExW': was declared deprecated

    // System version.
    ::OSVERSIONINFOW info;
    TS_ZERO(info);
    info.dwOSVersionInfoSize = sizeof(info);
    if (::GetVersionExW(&info)) {
        _systemVersion = UString::Format(u"Windows %d.%d Build %d %s", {info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber, UString(info.szCSDVersion)});
        _systemVersion.trim();
    }

    TS_POP_WARNING()

    // Detect 32-bit application on 64-bit system.
    ::BOOL wow64 = 0;
    if (::IsWow64Process(::GetCurrentProcess(), &wow64) && wow64) {
        // 32-bit application on 64-bit system => set system characteristics, not application.
        _isIntel32 = false;
        _isIntel64 = true;
    }

#endif

    // System version defaults to system name.
    if (_systemVersion.empty()) {
        _systemVersion = _systemName;
    }

    //
    // Get host name.
    //
#if defined(TS_WINDOWS)

    // Window implementation.
    std::array<::WCHAR, 1024> name;
    ::DWORD length = ::DWORD(name.size());
    if (::GetComputerNameW(name.data(), &length)) {
        _hostName.assign(name, length);
    }

#else

    // POSIX implementation.
    char name[1024];
    if (::gethostname(name, sizeof(name)) == 0) {
        name[sizeof(name) - 1] = '\0';
        _hostName.assignFromUTF8(name);
    }

#endif

    //
    // Get system memory page size
    //
#if defined(TS_WINDOWS)

    ::SYSTEM_INFO sysinfo;
    ::GetSystemInfo(&sysinfo);
    _memoryPageSize = size_t(sysinfo.dwPageSize);

#else

    // POSIX implementation.
    const long pageSize = ::sysconf(_SC_PAGESIZE);
    if (pageSize > 0) {
        _memoryPageSize = size_t(pageSize);
    }

#endif
}
