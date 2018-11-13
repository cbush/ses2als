#include "SessionFile.h"
#include "log.h"
#include "json.hpp"

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

namespace
{
#if LOG
auto &logger = std::cerr;
#else
std::ofstream logger;
#endif
} // namespace

using namespace CoolEdit;

/*

Keys to replace:

    Ableton
      __TEMPO__
      __TIME_SIGNATURE__
      __AUDIO_TRACKS__

    AudioTrack
      __ID__
      __PAN__
      __VOLUME__
      __MUTE__
      __AUDIO_CLIPS__

    AudioClip
      __TIME__
      __CURRENT_START__
      __CURRENT_END__
      __LOOP_START__
      __LOOP_END__
      __WARP_START_SEC_TIME__
      __WARP_START_BEAT_TIME__
      __WARP_END_SEC_TIME__
      __WARP_END_BEAT_TIME__
      __NAME__
      __COLOR_INDEX__
      __SAMPLE_FILE_NAME__
      __RELATIVE_PATH_ELEMENTS__

    RelativePathElement
      __DIR__

*/

std::string load_string(std::string const &path)
{
    std::ifstream in(path, std::ios::in|std::ios::ate);
    std::string result;
    result.reserve(in.tellg());
    in.seekg(0);
    result.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return result;
}

const std::string ABLETON_XML = load_string("templates/Ableton.xml");
const std::string AUDIO_CLIP_XML = load_string("templates/AudioClip.xml");
const std::string AUDIO_TRACK_XML = load_string("templates/AudioTrack.xml");
const std::string RELATIVE_PATH_ELEMENT_XML = load_string("templates/RelativePathElement.xml");

void replace(std::string &out, std::string const &key, std::string const &value)
{
    logger << "Replace " << key << '\n';
    auto pos = out.find(key);
    if (pos == std::string::npos)
    {
        throw exception("Key not found in string: %@", key);
    }
    out.replace(pos, key.length(), value);
}

template<typename T>
void replace(std::string &out, std::string const &key, T const &value)
{
    logger << "Replace T " << key << '\n';
    std::stringstream ss;
    ss << value;
    replace(out, key, ss.str());
}

void replace(std::string &out, std::string const &key, bool value)
{
    logger << "Replace bool " << key << '\n';
    replace(out, key, value ? "true" : "false");
}

double samples_to_seconds(Session const &session, unsigned samples)
{
    return samples / (double)session.sample_rate;
}

double seconds_to_beats(Session const &session, double seconds)
{
    return seconds * (session.tempo.beats_per_minute / 60.0);
}

std::string get_wave_filename(Session const &session, Block const &block)
{
    auto it = find_if(session.waves.begin(), session.waves.end(), [&](Wave const &wave)
    {
        return wave.id == block.wave_id;
    });
    if (it == session.waves.end())
    {
        throw exception("Invalid wave: %@ for block %@", block.wave_id, block.id);
    }
    auto &name = it->filename;
    return name.substr(name.rfind("\\") + 1); // Absolute windows path with backslashes. Just strip off the DIRNAME and hope the file will be located near the als project
}

double get_warp_sec(Session const &session, double seconds)
{
    auto beats = seconds_to_beats(session, seconds);
    return beats * session.tempo.beats_per_minute;
}

double get_warp_beat(Session const &session, double seconds)
{
    return get_warp_sec(session, seconds) * (session.tempo.beats_per_minute / 60.0);
}

std::string generate_audio_clips_xml(Session const &session, size_t track_index)
{
    std::string result;
    for (auto &block : session.blocks)
    {
        if (block.track != track_index)
        {
            continue;
        }

        auto xml = AUDIO_CLIP_XML;
        replace(xml, "__COLOR_INDEX__", 20);

        auto start_seconds = samples_to_seconds(session, block.offset_samples);
        auto start_beats = seconds_to_beats(session, start_seconds);
        auto duration_seconds = samples_to_seconds(session, block.size_samples);
        auto duration_beats = seconds_to_beats(session, duration_seconds);
        replace(xml, "__TIME__", start_beats);
        replace(xml, "__CURRENT_START__", start_beats);
        replace(xml, "__CURRENT_END__", start_beats + duration_beats);

        auto loop_start_sec = samples_to_seconds(session, block.wave_offset_samples);
        replace(xml, "__LOOP_START__", loop_start_sec);
        replace(xml, "__LOOP_END__", duration_beats);

        // This isn't really right, but at least Live will fix it when Warp is enabled manually
        replace(xml, "__WARP_START_SEC_TIME__", 0);
        replace(xml, "__WARP_START_BEAT_TIME__", 0);
        replace(xml, "__WARP_END_SEC_TIME__", 10000);
        replace(xml, "__WARP_END_BEAT_TIME__", 10000);

        auto filename = get_wave_filename(session, block);
        replace(xml, "__NAME__", filename);
        replace(xml, "__SAMPLE_FILE_NAME__", filename);
        replace(xml, "__RELATIVE_PATH_ELEMENTS__", "");
        result += xml;
    }
    return result;
}

std::string generate_audio_tracks_xml(Session const &session)
{
    std::string result;
    for (size_t i = 0; i < session.tracks.size(); ++i)
    {
        auto &track = session.tracks[i];
        auto xml = AUDIO_TRACK_XML;
        replace(xml, "__ID__", 8 + i);

        auto volume = (track.left_volume + track.right_volume) / 2.0;
        auto pan = (track.right_volume - track.left_volume) / std::max(volume, std::numeric_limits<double>::epsilon());
        replace(xml, "__VOLUME__", volume);
        replace(xml, "__PAN__", pan);
        replace(xml, "__MUTE__", track.mute);

        replace(xml, "__AUDIO_CLIPS__", generate_audio_clips_xml(session, i + 1));
        result += move(xml);
    }
    return result;
}

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() != 2)
    {
        std::cerr << "Usage: " << args[0] << " <path/to/sesfile>\n";
        return 1;
    }
    auto session = load_session(args[1]);
    auto ableton = ABLETON_XML;
    replace(ableton, "__TEMPO__", session.tempo.beats_per_minute);
    replace(ableton, "__TIME_SIGNATURE__", 197 + session.tempo.beats_per_bar);
    replace(ableton, "__AUDIO_TRACKS__", generate_audio_tracks_xml(session));
    std::cout << ableton << '\n';
}

