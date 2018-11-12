#include "SessionFile.h"

#include "log.h"

#include "json.hpp"

#include <cstdint>
#include <fstream>

namespace CoolEdit
{

#if LOG
auto &logger = std::cout;
#else
std::ofstream logger;
#endif

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

#define PUT(os, object, property) \
    format(os, #property ": %@\n", object.property)

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
    char filename[256];
    BYTE unknown[44];
};

std::ostream &operator<<(std::ostream &os, Header const &header)
{
    PUT(os, header, sample_rate);
    PUT(os, header, samples_in_session);
    PUT(os, header, number_of_wave_blocks);
    PUT(os, header, bits_per_sample);
    PUT(os, header, channels);
    PUT(os, header, master_volume);
    PUT(os, header, master_volume_right);
    PUT(os, header, session_time_offset_samples);
    PUT(os, header, save_associated_files_separately);
    PUT(os, header, priv);
    PUT(os, header, filename);
    PUT(os, header, unknown);
    return os;
};

PACKED_STRUCT TempoBlock
{
    DOUBLE beats_per_minute;
    DWORD beats_per_bar;
    DWORD ticks_per_beat;
    DOUBLE beat_offset_ms;
    DOUBLE unknown;
};

std::ostream &operator<<(std::ostream &os, TempoBlock const &tempo)
{
    PUT(os, tempo, beats_per_minute);
    PUT(os, tempo, beats_per_bar);
    PUT(os, tempo, ticks_per_beat);
    PUT(os, tempo, beat_offset_ms);
    PUT(os, tempo, unknown);
    return os;
};

PACKED_STRUCT TrackBlock
{
    DOUBLE left_volume;
    DOUBLE right_volume;
    DWORD flags;
    char title[36];
    char unknown[40];
};

std::ostream &operator<<(std::ostream &os, TrackBlock const &block)
{
    PUT(os, block, left_volume);
    PUT(os, block, right_volume);
    PUT(os, block, flags);
    PUT(os, block, title);
    PUT(os, block, unknown);
    return os;
}

PACKED_STRUCT WaveBlockBlock
{
    DOUBLE left_volume;
    DOUBLE right_volume;
    DOUBLE unused1;
    DOUBLE unused2;
    DWORD offset_samples;
    DWORD size_samples;
    DWORD id;
    DWORD flags;
    DWORD wave_id;
    DWORD track_id;
    DWORD parent_group;
    DWORD unused;
    DWORD wave_offset;
    DWORD punch_generation;
    DWORD previous_punch;
    DWORD next_punch;
    DWORD original_index;
    DWORD unknown;
};

std::ostream &operator<<(std::ostream &os, WaveBlockBlock const &block)
{
    PUT(os, block, left_volume);
    PUT(os, block, right_volume);
    PUT(os, block, unused1);
    PUT(os, block, unused2);
    PUT(os, block, offset_samples);
    PUT(os, block, size_samples);
    PUT(os, block, id);
    PUT(os, block, flags);
    PUT(os, block, wave_id);
    PUT(os, block, track_id);
    PUT(os, block, parent_group);
    PUT(os, block, unused);
    PUT(os, block, wave_offset);
    PUT(os, block, punch_generation);
    PUT(os, block, previous_punch);
    PUT(os, block, next_punch);
    PUT(os, block, original_index);
    PUT(os, block, unknown);
    return os;
}

struct WaveListEntryBlock
{
    DWORD id;
    DWORD nineteen;
    std::string filename;
    DWORD unused[2];
};

std::ostream &operator<<(std::ostream &os, WaveListEntryBlock const &block)
{
    PUT(os, block, id);
    PUT(os, block, nineteen);
    PUT(os, block, filename);
    PUT(os, block, unused);
    return os;
}

std::string read_block_header(std::istream &in)
{
    char header[4];
    CHECKED_READ(header, in);
    return {header, 4};
}

bool read(WaveListEntryBlock &block, std::istream &in, size_t length)
{
    read(block.id, in);
    read(block.nineteen, in);
    length -= sizeof(DWORD) * 4 + 1;
    block.filename.resize(length);
    in.read(&block.filename[0], length);
    in.get();
    read(block.unused, in);
    return true;
}

Header read_header(std::istream &in)
{
    Header header{};
    CHECKED_READ(header, in);
    logger << header << '\n';
    return header;
}

std::string get_clean_string(const char *s, size_t count)
{
    std::string string(s, count);
    string.erase(find(string.begin(), string.end(), '\0'), string.end());
    return string;
}

std::string read_block(Session &session, std::istream &in)
{
    auto header = read_block_header(in);
    logger << "Block header: " << header << '\n';
    
    if (header == "LIST")
    {
        header = read_block_header(in);
        logger << "Block header 2: " << header << '\n';
        EXPECT_EQ("FILE", header);
    }

    DWORD length{};
    read(length, in);
    logger << "Block length: " << length << '\n';

    auto previous_tellg = (int)in.tellg();

    if (header == "hdr ")
    {
        auto block = read_header(in);
        session.sample_rate = block.sample_rate;
        session.master_volume = block.master_volume;
        session.filename = get_clean_string(block.filename, sizeof(block.filename));
    }
    else if (header == "tmpo")
    {
        TempoBlock tempo{};
        CHECKED_READ(tempo, in);
        logger << tempo << '\n';
        session.tempo.beats_per_minute = tempo.beats_per_minute;
        session.tempo.beats_per_bar = (unsigned)tempo.beats_per_bar;
        session.tempo.ticks_per_beat = (unsigned)tempo.ticks_per_beat;
    }
    else if (header == "trks")
    {
        DWORD count{};
        read(count, in);
        logger << "Track count: " << count << '\n';
        for (auto i = 0; i < count; ++i)
        {
            TrackBlock block;
            CHECKED_READ(block, in);
            logger << block << '\n';
            Track track{};
            track.left_volume = block.left_volume;
            track.right_volume = block.right_volume;
            track.title = get_clean_string(block.title, sizeof(block.title));
            track.mute = block.flags & 1;
            session.tracks.push_back(std::move(track));
        }
    }
    else if (header == "FILE")
    {
        while (read_block(session, in) == "wav ");
        return header;
    }
    else if (header == "wav ")
    {
        WaveListEntryBlock block;
        read(block, in, length);
        logger << block << '\n';
        Wave wave{};
        wave.id = block.id;
        wave.filename = block.filename;
        session.waves.push_back(std::move(wave));
    }
    else if (header == "blk ")
    {
        DWORD count{};
        read(count, in);
        logger << "Block count: " << count << '\n';
        for (auto i = 0; i < count; ++i)
        {
            WaveBlockBlock block;
            CHECKED_READ(block, in);
            logger << block << '\n';
            Block wave;
            wave.id = block.id;
            wave.left_volume = block.left_volume;
            wave.right_volume = block.right_volume;
            wave.offset_samples = block.offset_samples;
            wave.size_samples = block.size_samples;
            wave.wave_id = block.wave_id;
            wave.track_id = block.track_id;
        }
    }
    else
    {
        in.seekg(length, std::ios::cur);
    }
    EXPECT_EQ(previous_tellg + length, (int)in.tellg());
    return header;
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
    logger << "File length: " << length << '\n';
    
    EXPECT_EQ(actual_file_length, length + 8 + 4); // COOLNESS + length

    while (file.tellg() != actual_file_length)
    {
        read_block(session, file);
    }

    return session;
}

void to_json(nlohmann::json &out, Block const &in)
{
    out = {
        {"id", in.id},
        {"left_volume", in.left_volume},
        {"right_volume", in.right_volume},
        {"offset_samples", in.offset_samples},
        {"size_samples", in.size_samples},
        {"wave_id", in.wave_id},
        {"track_id", in.track_id},
    };
}

void to_json(nlohmann::json &out, Wave const &in)
{
    out = {
        {"id", in.id},
        {"filename", in.filename},
    };
}

void to_json(nlohmann::json &out, Track const &in)
{
    out = {
        {"left_volume", in.left_volume},
        {"right_volume", in.right_volume},
        {"title", in.title},
        {"mute", in.mute},
    };
}

void to_json(nlohmann::json &out, Tempo const &in)
{
    out = {
        {"beats_per_minute", in.beats_per_minute},
        {"beats_per_bar", in.beats_per_bar},
        {"ticks_per_beat", in.ticks_per_beat}
    };
}

void to_json(nlohmann::json &out, Session const &in)
{
    out = {
        {"sample_rate", in.sample_rate},
        {"master_volume", in.master_volume},
        {"filename", in.filename},
        {"tempo", in.tempo},
        {"tracks", in.tracks},
        {"waves", in.waves},
        {"blocks", in.blocks}
    };
}

} // namespace CoolEdit
