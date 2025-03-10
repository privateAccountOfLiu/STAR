#include "BinSeq.h"
#include <iostream>

BinSeq::BinSeq(const std::string &fileName)
{
    *reader = open_mmap_reader(fileName);
    //auto reader = open_mmap_reader(fileName);
    //readN = reader->num_records();
    //lenR1 = reader->get_slen();
    //lenR2 = reader->get_xlen();
    //isPE = lenR2 > 0;

    std::cout << "BinSeq::BinSeq: readN=" << readN << ", lenR1=" << lenR1 << ", lenR2=" << lenR2 << std::endl;
}
