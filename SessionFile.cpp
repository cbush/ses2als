#include "SessionFile.h"

#include "log.h"

#include <cstdint>
#include <fstream>

namespace CoolEdit
{

using BYTE = uint8_t;
using WORD = uint16_t;
using SIGNED_WORD = int16_t;
using DWORD = uint32_t;
using BOOL = uint32_t;
using DOUBLE = double;

template <typename T>
bool read(T &out, std::istream &in)
{
    uint8_t *bytes = reinterpret_cast<uint8_t *>(&out);
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        if (!in.good())
        {
            return false;
        }
        bytes[i] = in.get();
    }
    return true;
}

#define CHECKED_READ(OUT, IN)                               \
    if (!read(OUT, IN))                                     \
    {                                                       \
        throw exception("failed to read value '" #OUT "'"); \
    }

#define EXPECT_EQ(A, B)                                                                                        \
    {                                                                                                          \
        const auto &a(A);                                                                                      \
        const auto &b(B);                                                                                      \
        if (a != b)                                                                                            \
        {                                                                                                      \
            loge("Verification failed: expected " #A " == " #B ", got %@ and %@", a, b);            \
            throw exception("Verification failed: expected " #A " == " #B ", got %@ and %@", a, b); \
        }                                                                                                      \
    }

#define PACKED_STRUCT struct __attribute__((packed))

PACKED_STRUCT Header
{
    DWORD length;
    DWORD sample_rate;
    DWORD samples_in_session;
    DWORD number_of_wave_blocks;
    WORD bits_per_sample;
    WORD channels;
    DOUBLE master_volume;
    DOUBLE master_volume_right;
    DWORD session_time_offset_samples;
    BOOL save_associated_files_separately;
    BOOL priv;
    BYTE filename[256];
};

Header read_header(std::istream &in)
{
    Header header{};
    CHECKED_READ(header, in);
    EXPECT_EQ(sizeof(Header), header.length);
    return header;
}

std::string read_block_header(std::istream &in)
{
    BYTE header[4];
    CHECKED_READ(header, in);
    return {(char *)header, 4};
}

void read_block(Session &session, std::istream &in)
{
    auto header = read_block_header(in);
    std::cout << "Block header: " << header << '\n';
    
    if (header == "LIST")
    {
        header = read_block_header(in);
        std::cout << "Block header 2: " << header << '\n';
    }

    DWORD length{};
    read(length, in);
    
    std::cout << "Block length: " << length << '\n';
    in.seekg(length, std::ios::cur);
}

const uint64_t COOLNESS = 0x5353454e4c4f4f43;

Session load_session(std::string const &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::in | std::ios::ate);

    if (!file.good())
    {
        throw exception("failed to load file: %@", path);
    }
    
    DWORD actual_file_length = static_cast<DWORD>(file.tellg());
    file.seekg(0);

    Session session{};
    
    uint64_t coolness{};
    read(coolness, file);
    EXPECT_EQ(COOLNESS, coolness); // COOLNESS

    DWORD length{};
    read(length, file);
    std::cout << "File length: " << length << '\n';
    
    EXPECT_EQ(actual_file_length, length + 8 + 4); // COOLNESS + length

    while (file.tellg() != actual_file_length)
    {
        read_block(session, file);
    }

    return session;
}

std::ostream &operator<<(std::ostream &os, Session const &session)
{
    return os;
}

} // namespace CoolEdit
