

#ifndef __DH_GENERATOR_H__
#define __DH_GENERATOR_H__


#include <vector>

bool dh_generate_y(_In_ const std::vector<unsigned char>& p, _In_ const std::vector<unsigned char>& g, _In_ const std::vector<unsigned char>& x, _Out_ std::vector<unsigned char>& y);


#endif