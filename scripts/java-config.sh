#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#  This script is used to locate java components on all Unix systems.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename ${BASH_SOURCE[0]})
error() { echo >&2 "$SCRIPT: $*"; exit 1; }
usage() { error "invalid option $*, try --help"; }

# Display help text
cmd_help() {
    cat >&2 <<EOF

Syntax: $SCRIPT [options]

  --bin     Directory for JDK executables
  --cflags  C++ pre-processor and compiler flags for JNI code
  --help    Display this help and exit
  --home    Jave home directory (typically for \$JAVA_HOME)
  --jar     JAR file manipulation command
  --java    Java execution command
  --javac   Java compiler command

EOF
    exit
}

# Locate Java home
cmd_home() {
    local system=$(uname -s)
    if [[ -n "$JAVA_HOME" && -x "$JAVA_HOME/bin/java" ]]; then
        # Explicit value
        echo "$JAVA_HOME"
    elif [[ $system == Darwin ]]; then
        # macOS
        javac=$(find -L /Library/Java/JavaVirtualMachines/openjdk.jdk -name javac -perm +444 2>/dev/null | tail -1)
        [[ -z "$javac" ]] && javac=$(find -L /Library/Java/JavaVirtualMachines -name javac -perm +444 2>/dev/null | tail -1)
        [[ -n "$javac" ]] && dirname $(dirname "$javac")
    elif [[ $system == FreeBSD  ]]; then
        # One or more version under /usr/local, use the last one.
        local jhome=
        for dir in /usr/local/*jdk*; do
            [[ -e $dir/bin/javac && -e $dir/include/jni.h ]] && jhome=$dir
        done
        [[ -n $jhome ]] && echo $jhome
    elif [[ -f /etc/gentoo-release ]]; then
        # Gentoo Linux does not use symbolic links into jdk for java and javac
        jdk=$(ls -d /etc/java-config*/current-system-vm 2>/dev/null | tail -1)
        [[ -n "$jdk" ]] && jdk=$(readlink -e -s "$jdk")
        [[ -n "$jdk" && -x "$jdk/bin/java" ]] && echo "$jdk"
    else
        # Linux
        cmd=$(which javac 2>/dev/null)
        [[ -z "$cmd" ]] && cmd=$(which java 2>/dev/null)
        [[ -n "$cmd" && -L "$cmd" ]] && cmd=$(readlink -e -s "$cmd")
        [[ -n "$cmd" && -x "$cmd" ]] && dirname $(dirname "$cmd")
    fi
}

# Process individual commands
cmd_bin() {
    home=$(cmd_home)
    [[ -n "$home" && -d "$home/bin" ]] && echo "$home/bin"
}

cmd_jar() {
    home=$(cmd_home)
    [[ -n "$home" && -x "$home/bin/jar" ]] && echo "$home/bin/jar"
}

cmd_java() {
    home=$(cmd_home)
    [[ -n "$home" && -x "$home/bin/java" ]] && echo "$home/bin/java"
}

cmd_javac() {
    home=$(cmd_home)
    [[ -n "$home" && -x "$home/bin/javac" ]] && echo "$home/bin/javac"
}

cmd_cflags() {
    home=$(cmd_home)
    if [[ -n "$home" && -d "$home/include" ]]; then
        # JNI headers exist
        echo "-I$home/include$(dirname $(ls $home/include/*/jni_md.h 2>/dev/null) | sed -e 's/^/ -I/')"
    else
        # Disable JNI code
        echo "-DTS_NO_JAVA=1"
    fi
}

if [ $# -eq 0 ]; then
    # No option, display everything.
    echo "home: $(cmd_home)"
    echo "bin: $(cmd_bin)"
    echo "java: $(cmd_java)"
    echo "javac: $(cmd_javac)"
    echo "jar: $(cmd_jar)"
    echo "cflags: $(cmd_cflags)"
else
    # Display options one by one.
    for arg in "$@"; do
        case "$arg" in
            --bin) cmd_bin ;;
            --cflags) cmd_cflags ;;
            --help) cmd_help ;;
            --home) cmd_home ;;
            --jar) cmd_jar ;;
            --java) cmd_java ;;
            --javac) cmd_javac ;;
            *) usage "$arg" ;;
        esac
        shift
    done
fi
