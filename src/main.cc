#include <args.hxx>
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "flac.h"
#include "playlist.h"

struct options_t
{
    std::string input;
    std::string output;
    std::size_t iterations;
    bool regular;
};

auto process_arguments(int argc, char* argv[]) -> options_t
{
    auto parser = args::ArgumentParser("train train");

    parser.Prog("infinitetrain");

    parser.helpParams.useValueNameOnce = true;
    parser.helpParams.proglineShowFlags = true;
    parser.helpParams.proglinePreferShortFlags = true;
    parser.helpParams.showCommandFullHelp = false;
    parser.helpParams.showCommandChildren = false;

    auto input = args::Positional<std::string>(parser, "input", "path to track list yml file", "", args::Options::Required);
    auto output = args::Positional<std::string>(parser, "output", "path to output file (- for stdout)", "", args::Options::Required);

    auto iterations = args::ValueFlag<std::size_t>(parser, "count", "amount of tracks to play before exiting", {'n', "iterations"}, 0);
    auto regular = args::Flag(parser, "regular", "disable randomization of track order", {'r', "regular"}, false);

    auto verbose = args::Flag(parser, "verbose", "enable debugging messages", {'v', "verbose"});
    auto help = args::HelpFlag(parser, "help", "display help text and exit", {'h', "help"});

    try
    {
        parser.ParseCLI(argc, argv);

        if (args::get(iterations) == 0 && args::get(output) != "-")
            throw std::logic_error("Iteration count must be set when using file output.");
    }
    catch (const args::Help&)
        { std::cerr << parser.Help() << std::endl; std::exit(0); }
    catch (const std::exception& e)
        { std::cerr << parser.Help(); spdlog::error("{}", e.what()); std::exit(1); }

    spdlog::set_level(args::get(verbose) ? spdlog::level::debug: spdlog::level::info);

    return {
        .input = args::get(input),
        .output = args::get(output),
        .iterations = args::get(iterations),
        .regular = args::get(regular),
    };
}

int main(int argc, char* argv[])
{
    // use stderr for logging, so we can use stdout for the audio
    spdlog::set_default_logger(spdlog::stderr_color_mt("stderr"));

    // parse command line arguments
    auto args = process_arguments(argc, argv);

    // parse track list from yml
    auto root = YAML::LoadFile(args.input);
    auto tracks = std::vector<track_t> {};

    for (auto const& item: root["tracks"])
    {
        auto track = track_t {
            .path  = item["file"].as<std::string>(),
            .start = item["lead in"].as<std::uint32_t>(),
            .end   = item["lead out"].as<std::uint32_t>(),
        };

        spdlog::debug("parsed track with lead in @ {}, lead out @ {}", track.start, track.end);
        tracks.push_back(track);
    }

    // initialise the playlist
    auto queue = playlist(tracks);
    queue.set_max_history(tracks.size() / 2);

    // initialise the encoder
    auto encoder = flac_encoder();
    auto encoder_configured = std::once_flag {};

    // we move the previous track in here when we start merging samples
    // this lets us merge the remaining samples while we start playing the next track
    auto previous = std::unique_ptr<flac_decoder>();
    auto iterations = 0;

    while (true)
    {
        auto track = queue.next(!args.regular);
        auto decoder = std::make_unique<flac_decoder>();

        if (args.iterations > 0)
            spdlog::info("now playing: {} ({}/{})", track.path, iterations + 1, args.iterations);
        else
            spdlog::info("now playing: {}", track.path);

        decoder->init(track.path);
        decoder->process_until_end_of_stream();

        spdlog::debug("read {} samples", decoder->get_total_samples());

        auto const channels = decoder->get_channels();

        std::call_once(encoder_configured, [&]()
        {
            encoder.set_verify(false);
            encoder.set_compression_level(0);
            encoder.set_channels(channels);
            encoder.set_bits_per_sample(decoder->get_bits_per_sample());
            encoder.set_sample_rate(decoder->get_sample_rate());

            if (args.output == "-")
                encoder.init(stdout);
            else
                encoder.init(args.output);
        });

        auto data = reinterpret_cast<const FLAC__int32*>(decoder->get().data());

        if (previous == nullptr)
        {
            // this is the first track, so just write everything until we reach the lead out
            spdlog::debug("encoding from begin to sample {}", track.end);
            encoder.process_interleaved(data, track.end);
        }
        else
        {
            // this isn't the first track anymore, so offset the data pointer to the first sample
            data += (track.start * channels);
            spdlog::debug("encoding from sample {} to sample {}", track.start, track.end);

            // we still have some samples remaining from the previous track
            auto prev_data = reinterpret_cast<const FLAC__int32*>(previous->get().data());
            auto const prev_size = previous->get().size();
            auto const prev_count = (prev_size / channels);
            spdlog::debug("merging {} samples from previous track", prev_count);

            auto mixed = std::vector<FLAC__int32>(prev_size);

            for (auto i = 0; i < prev_count; ++i)
            {
                // mix the samples
                for (auto channel = 0; channel < channels; ++channel)
                {
                    auto const index = (i * channels + channel);
                    auto const sum = (prev_data[index] + data[index]);

                    mixed[index] = std::clamp(sum, -32768, 32767);
                }
            }

            // write the mixed samples
            encoder.process_interleaved(mixed.data(), mixed.size() / channels);

            // write the remaining samples
            spdlog::debug("writing remaining samples of current track");
            encoder.process_interleaved(data + (prev_size), track.end - track.start - prev_count);
        }

        // remove samples we've already written, so we only have the remaining samples that need to be mixed
        decoder->get().erase(decoder->get().begin(), decoder->get().begin() + (track.end * channels));
        spdlog::debug("erasing {} copied samples, {} remaining", track.end, (decoder->get().size() / channels));

        iterations++;

        if (iterations == args.iterations)
        {
            // if this is the last track we should just write the remaining samples
            spdlog::debug("writing remaining samples of last track");
            encoder.process_interleaved(decoder->get().data(), (decoder->get().size() / channels));
            break;
        }

        // move this track into `previous`
        previous = std::move(decoder);

        // check if whatever we're writing to is still open
        if (std::fflush(stdout) == EOF)
        {
            spdlog::debug("stdout reached eof, exiting...");
            break;
        }
    }

    // finish up
    encoder.finish();

	return EXIT_SUCCESS;
}