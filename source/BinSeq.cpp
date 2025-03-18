#include "BinSeq.h"
#include <iostream>
#include <cassert>

BinSeq::BinSeq(const std::string &fileName, uint64_t chunkInSizeBytes)
{
    reader = binseq_reader_open(fileName.c_str());
    readN = binseq_reader_num_records(reader);
    lenR1 = binseq_reader_slen(reader);
    lenR2 = binseq_reader_xlen(reader);
    isPE = lenR2 > 0;

    context = binseq_context_new();
    assert(context != NULL);
    record = binseq_record_new();
    assert(record != NULL);

    buffers = new char*[2];
    buffers[0] = new char[lenR1];
    if (isPE)
        buffers[1] = new char[lenR2];

    recStart = 0;
    recChunkN = chunkInSizeBytes / (isPE ? 2 : 1)  / (lenR1 + lenR2 + 20) - 10; //+20 for \n, read id, etc. -10 to be safe. /2 because this limitIObufferIn for both reads

    //char *fileNameC = fileName.c_str();
    //reader = binseq_reader_open("aaa");
    //struct BinseqReader *reader1 = binseq_reader_open(fileName.c_str());
    //struct BinseqReader *reader = binseq_reader_open("aaa");
    //*reader = open_mmap_reader(fileName);
    //auto reader = open_mmap_reader(fileName);
    //readN = reader->num_records();
    //lenR1 = reader->get_slen();
    //lenR2 = reader->get_xlen();
    //isPE = lenR2 > 0;

    std::cout << "BinSeq::BinSeq: readN=" << readN << ", lenR1=" << lenR1 << ", lenR2=" << lenR2 << ", recChunkN=" << recChunkN << std::endl;
}

bool BinSeq::loadRecord(uint irec, char** buff, std::array<uint, 3> &lens)
{
    if (!binseq_reader_get_record(reader, irec, record))
        return false;

    size_t len = binseq_record_decode_primary(record, context); // decode primary sequence
    // TODO: ask Noam if need to use lenR1 or s_len
    binseq_context_copy_primary(context, buff[0] + lens[0], lenR1); // copy primary sequence to buffer
    lens[0] += len;
    buff[0][lens[0]]='\n';
    lens[0] ++;

    if (isPE) {
        len = binseq_record_decode_extended(record, context); // decode auxiliary sequence
        binseq_context_copy_extended(context, buff[1] + lens[1], lenR2); // copy auxiliary sequence to buffer
        lens[1] += len;
        buff[1][lens[1]]='\n';
        lens[1] ++;
        assert(lens[0] == lens[1]);
    };

    return true;
}

BinSeq::~BinSeq()
{
    binseq_reader_close(reader);
    binseq_context_free(context);
    binseq_record_free(record);
    delete[] buffers[0];
    if (isPE)
        delete[] buffers[1];
    delete[] buffers;
}