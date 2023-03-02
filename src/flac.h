#pragma once

#include <vector>
#include <FLAC++/decoder.h>
#include <FLAC++/encoder.h>

class flac_encoder: public FLAC::Encoder::File {};
class flac_decoder: public FLAC::Decoder::File
{
    public:
        flac_decoder():
            FLAC::Decoder::File() {};

        [[nodiscard]] auto get() -> std::vector<FLAC__int32>&
            { return _buffer; }
    protected:
        auto write_callback(const FLAC__Frame* frame, const FLAC__int32* const buffer[]) -> FLAC__StreamDecoderWriteStatus override
        {
            if (_buffer.empty())
                _buffer.resize(frame->header.channels * frame->header.blocksize);

            for (auto sample = 0; sample < frame->header.blocksize; ++sample)
                for (auto channel = 0; channel < frame->header.channels; ++channel)
                    _buffer.push_back(buffer[channel][sample]);

            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }

        auto error_callback(FLAC__StreamDecoderErrorStatus status) -> void override
            { spdlog::error("flac decoder error: {}", FLAC__StreamDecoderErrorStatusString[status]); }
    private:
        std::vector<FLAC__int32> _buffer;
};