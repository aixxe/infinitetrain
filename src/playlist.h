#pragma once

#include <deque>
#include <vector>
#include <random>

struct track_t
{
    std::string path;
    std::uint32_t start;
    std::uint32_t end;

    auto operator==(track_t const& other) const -> bool
        { return (path == other.path && start == other.start && end == other.end); };
};

class playlist
{
    public:
        explicit playlist(std::vector<track_t> songs):
            _songs(std::move(songs)) {}

        auto set_max_history(std::deque<track_t>::size_type max) -> void
            { _max_history = max; };

        auto reload(bool shuffle) -> void
        {
            static auto rd = std::random_device {};
            static auto gen = std::default_random_engine { rd() };

            _queue = _songs;

            if (shuffle)
                std::shuffle(_queue.begin(), _queue.end(), gen);
        }

        [[nodiscard]] auto next(bool shuffle = true) -> track_t
        {
            if (_queue.empty())
                reload(shuffle);

            auto const song = std::find_if(_queue.begin(), _queue.end(), [this] (auto const& track)
                { return std::find(_history.begin(), _history.end(), track) == _history.end(); });

            auto result = *song;

            if (_history.size() == _max_history)
                _history.pop_front();

            _history.push_back(result);
            _queue.erase(song);

            return result;
        }
    private:
        std::vector<track_t> _songs = {};
        std::vector<track_t> _queue = {};
        std::deque<track_t> _history = {};
        std::deque<track_t>::size_type _max_history = 1;
};