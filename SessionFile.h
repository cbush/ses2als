#pragma once

#include <string>
#include <vector>

#include "json.hpp"

namespace CoolEdit
{

struct Block
{
    unsigned id;
    double left_volume;
    double right_volume;
    unsigned offset_samples;
    unsigned size_samples;
    unsigned wave_offset_samples;
    unsigned wave_id;
    unsigned track;
};

struct Wave
{
    unsigned id;
    std::string filename;
};

struct Track
{
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
    unsigned sample_rate;
    unsigned master_volume;
    std::string filename;
    Tempo tempo;
    std::vector<Track> tracks;
    std::vector<Wave> waves;
    std::vector<Block> blocks;
};

Session load_session(std::string const &path);

void to_json(nlohmann::json &j, Session const &);

} // namespace CoolEdit
