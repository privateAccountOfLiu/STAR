#ifndef CODE_BinSeq
#define CODE_BinSeq

//#include 
//#include "binseq/cxxbridge/binseq-cxx/src/lib.rs.h"
//#include "binseq/cxxbridge/rust/cxx.h"
extern "C" {
    #include "binseq/binseq.h"
}

#include "IncludeDefine.h"
#include <string>
#include <array>
#include <atomic>

class BinSeq
{
    public:
        //rust::cxxbridge1::Box<BinseqReaderWrapper> *reader;
        struct BinseqReader *reader;
        struct BinseqContext *context;
        struct BinseqRecord *record;

        char **buffers;

        uint readN, lenR1, lenR2;
        bool isPE;
        std::atomic<uint> recStart; //current starting position in the binseq file
        uint recChunkN;

        BinSeq(const std::string &fileName, uint64_t chunkInSizeBytes);
        ~BinSeq();

        bool loadRecord(uint irec, char** buff, std::array<uint, 3> &lens);


};

#endif
