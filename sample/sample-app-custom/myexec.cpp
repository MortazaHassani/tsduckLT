#include "tsduck.h"
#include <fstream>

TS_MAIN(MainCode);

class MPEG_TS_LearnData : public ts::Object
{
public:
    MPEG_TS_LearnData(const ts::UString& s = ts::UString()) : message(s) {}
    ts::UString message;
};

class MPEG_TS_LearnPlugin: public ts::ProcessorPlugin
{
public:
    MPEG_TS_LearnPlugin(ts::TSP*);
    virtual bool getOptions() override;
    virtual bool start() override;
    virtual bool stop() override;
    virtual Status processPacket(ts::TSPacket&, ts::TSPacketMetadata&) override;

private:
    ts::PID _pid;   
    ts::PacketCounter _count; 
    ts::PacketCounter pcr_count;

};
TS_REGISTER_PROCESSOR_PLUGIN(u"MPEG_TS_Learn", MPEG_TS_LearnPlugin);

MPEG_TS_LearnPlugin::MPEG_TS_LearnPlugin(ts::TSP* t) :
    ts::ProcessorPlugin(t, u"Count TS packets in one PID", u"[options]"),
    _pid(ts::PID_NULL),
    _count(0)
{
    option(u"pid", 'p', PIDVAL);
}

bool MPEG_TS_LearnPlugin::getOptions()
{
    _pid = intValue<ts::PID>(u"pid", ts::PID_NULL);
    return true;
}

bool MPEG_TS_LearnPlugin::start()
{
    0;
    return true;
}

bool MPEG_TS_LearnPlugin::stop()
{
    std::string bit_rate;

    std::ifstream inFile;
    inFile.open("my_file.txt");
    inFile >> bit_rate;
    inFile.close();
   
    //printf("PID: 0x%X (%d),\npackets(pkt): %'d,\nPCR packets: %'d,\nbitrate(b/s): %'s\n", _pid, _pid, _count, pcr_count, bit_rate.c_str());
    printf("=================================================\n");
    printf("    \tPID 0x%X\t:\t\t%d\n    ||\tpackets(pkt): \t\t%'d\n     \tPCR packets: \t\t%'d\n    ||\tbitrate(b/s):\t\t%'s\n", _pid, _pid, _count, pcr_count, bit_rate.c_str());    
    printf("=================================================\n");

    return true;
}

ts::ProcessorPlugin::Status MPEG_TS_LearnPlugin::processPacket(ts::TSPacket& pkt, ts::TSPacketMetadata& metadata)
{
    if (pkt.getPID() != _pid) {
        _count++;
        
        if(pkt.hasPCR()){
          pcr_count++;}

    }
    return TSP_OK;
}

int MainCode(int argc, char* argv[])
{
   
    ts::AsyncReport report(ts::Severity::Error);
    ts::TSProcessorArgs opt;
   
    opt.input = {u"http", {u"https://tsduck.io/streams/italy-sardinia-dttv/mux1rai.ts"}};

    opt.plugins = {
        {u"pattern", {u"--pid", u"4", u"DEADBEEF"}},
        {u"MPEG_TS_Learn", {u"--pid", u"5"}},
        {u"continuity", {}},
    };

    opt.output = {u"file", {u"output.ts"}};
  
    ts::TSProcessor tsproc(report);
    if (!tsproc.start(opt)) {
        return EXIT_FAILURE;
    }

    tsproc.waitForTermination();
    return EXIT_SUCCESS;
}
