

#ifndef __NUDF_CRC_HPP__
#define __NUDF_CRC_HPP__



namespace NX {

namespace utility {

unsigned long calculate_crc32(const unsigned char* pb, unsigned long cb);
unsigned __int64 calculate_crc64(const unsigned char* pb, unsigned long cb);

}

}


#endif