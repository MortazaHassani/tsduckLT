## TSDuck - The MPEG Transport Stream Toolkit

### Abstract

[TSDuck](https://tsduck.io/) is an extensible toolkit for MPEG transport streams.

TSDuck is used in digital television systems for test, monitoring, integration, debug, lab or demo.

In practice, TSDuck is used for:

- Transport stream acquisition or transmodulation, including DVB, ATSC, ISDB, ASI and IP multicast.
- Analyze transport streams, PSI/SI signalization, bitrates, timestamps.
- Monitor and report conditions on the stream (video and audio properties, bitrates, crypto-periods, signalization).
- On-the-fly transformation or injection of content and signalization.
- Modify, remove, rename, extract services.
- Work on live transport streams, DVB-S/C/T, ATSC, ISDB-T, ASI, UDP ("IP-TV"), HTTP, HLS, SRT, RIST or offline transport stream files.
- Use specialized hardware such as cheap DVB, ATSC or ISDB tuners (USB, PCI), professional Dektec devices, cheap HiDes modulators, VATek-based modulators (e.g. Suntechtv U3, USB).
- Re-route transport streams to other applications.
- Extract or inject Multi-Protocol Encapsulation (MPE) between TS and UDP/IP.
- Analyze and inject SCTE 35 splice information.
- Extract specific encapsulated data (Teletext, T2-MI).
- Emulate a CAS head-end using DVB SimulCrypt interfaces to and from ECMG or EMMG.
- And more...


Currently this repository is under development for learning purposes.
To run this on Linux/Ubuntu following commands to be executed.

su root

sudo apt install libsrt1.4-openssl

sudo apt --fix-broken install

sudo dpkg -i tsduck_3.32-2983.debian11_amd64.deb

sudo dpkg -i tsduck-dev_3.32-2983.debian11_amd64.deb

mkdir tsduck

cd tsduck

sudo apt install git

git clone https://github.com/MortazaHassani/tsduckLT.git

cd tsduck

scripts/install-prerequisites.sh

which tsp

tsp –version


ls /usr/bin/make

sudo apt-get update

sudo apt-get install -y make 

sudo apt-get install python

sudo apt-get install build-essential

sudo make install

sudo make installer

sudo make -j10

source scripts/setenv.sh

make test




### ---

After each edit run following on main tsduckLT folder.


