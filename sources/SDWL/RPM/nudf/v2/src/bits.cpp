

#include <Windows.h>
#include <assert.h>

#include <nudf\bits.hpp>

using namespace NX;


static const unsigned long bits_mask_map[32] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000
};

bool NX::check_bit(unsigned long value, unsigned long bit)
{
    assert(bit < 32);
    return (0 != (value & bits_mask_map[bit]));
}

void NX::set_bit(unsigned long& value, unsigned long bit)
{
    assert(bit < 32);
    value |= bits_mask_map[bit];
}

void NX::clear_bit(unsigned long& value, unsigned long bit)
{
    assert(bit < 32);
    value &= (~bits_mask_map[bit]);
}
