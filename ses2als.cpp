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
auto &logger = std::cout;
#else
std::ofstream logger;
#endif

}

using namespace CoolEdit;

/*

Keys to replace:

    Ableton
      __TEMPO__
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
    return name.substr(name.rfind("\\") + 1); // Absolute windows path with backslashes
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

        auto start = samples_to_seconds(session, block.offset_samples);
        auto duration = samples_to_seconds(session, block.size_samples);
        auto end = start + duration;
        replace(xml, "__TIME__", start);
        replace(xml, "__CURRENT_START__", start);
        replace(xml, "__CURRENT_END__", 1000);
        replace(xml, "__COLOR_INDEX__", 20);

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
    replace(ableton, "__TEMPO__", 60);
    replace(ableton, "__AUDIO_TRACKS__", generate_audio_tracks_xml(session));
    std::cout << ableton << '\n';
}
