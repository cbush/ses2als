#pragma once

#include <string>
#include <vector>

namespace CoolEdit
{

struct Block
{
    unsigned id;
    double left_volume;
    double right_volume;
    unsigned offset_samples;
    unsigned size_samples;
    unsigned wave_id;
    unsigned track_id;
};

struct Wave
{
    unsigned id;
    std::string filename;
};

struct Track
{
    unsigned id;
    double left_volume;
    double right_volume;
    std::string title;
    bool mute;
};

struct Tempo
{
    double beats_per_minute;
    unsigned beats_per_bar;
    unsigned ticks_per_beat;
};

struct Session
{
    Tempo tempo;
    std::vector<Track> tracks;
    std::vector<Wave> waves;
    std::vector<Block> blocks;
};

std::ostream &operator<<(std::ostream &os, Block const &);
std::ostream &operator<<(std::ostream &os, Session const &);
std::ostream &operator<<(std::ostream &os, Tempo const &);
std::ostream &operator<<(std::ostream &os, Track const &);
std::ostream &operator<<(std::ostream &os, Wave const &);

Session load_session(std::string const &path);

} // namespace CoolEdit
