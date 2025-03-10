#ifndef CODE_BinSeq
#define CODE_BinSeq

//#include 
#include "binseq/cxxbridge/binseq-cxx/src/lib.rs.h"
#include "binseq/cxxbridge/rust/cxx.h"

class BinSeq
{
    public:
        rust::cxxbridge1::Box<BinseqReaderWrapper> *reader;
        uint64_t readN, lenR1, lenR2;
        bool isPE;
        BinSeq(const std::string &fileName);
};

#endif
