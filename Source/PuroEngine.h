#pragma once

#ifdef JUCE_DEBUG
    #define PURO_DEBUG 1
#else
    #define PURO_DEBUG 0
#endif

#include "puro.hpp"

struct Grain
{
    Grain(int offset, int length, int startIndex, float panning, double velocity, int sourceLength)
        : alignment({ offset, length })
        , readInc(velocity)
        , readPos(startIndex)
        , envelopeInc(puro::envelope_hann_get_increment<float>(length))
        , envelopePos(envelopeInc)
        , panCoeffs(puro::pan_create_stereo(panning))
    {
        std::tie(alignment, readPos) = puro::interp3_avoid_out_of_bounds_reads(alignment, readPos, readInc, sourceLength);
    }

    puro::RelativeAlignment alignment;

    const double readInc;
    double readPos;

    const float envelopeInc;
    float envelopePos;

    puro::PanCoeffs<float, 2> panCoeffs;
};

struct Context
{
    std::vector<float> vec1;
    std::vector<float> vec2;
    puro::DynamicBuffer<float> source;
};

template <typename BufferType, typename ElementType, typename ContextType, typename SourceBufferType>
bool process_grain(BufferType dst, ElementType& grain, ContextType& context, SourceBufferType source)
{
    using MonoBufferType = puro::Buffer<typename BufferType::value_type, 1>;

    std::tie(dst, grain.alignment) = puro::alignment_advance_and_crop_buffer(dst, grain.alignment);

    if (dst.isInvalid())
        return true;

    auto audio = puro::buffer_wrap_vector<SourceBufferType> (context.vec1, dst.length());
    grain.readPos = puro::interp3_fill(audio, source, grain.readPos, grain.readInc);

    MonoBufferType envelope = puro::buffer_wrap_vector<MonoBufferType> (context.vec2, audio.length());
    grain.envelopePos = puro::envelope_halfcos_fill(envelope, grain.envelopePos, grain.envelopeInc);

    puro::buffer_multiply(audio, envelope);

    BufferType output = puro::buffer_trim_length(dst, audio.length());

    puro::content_pan_apply_and_add(output, audio, grain.panCoeffs);

    return (grain.alignment.remaining <= 0) || (output.length() != dst.length());
}

class PuroEngine
{
public:

    PuroEngine()
        : timer(0)
        , intervalParam(1.0f, 0.0f, 0.1f, 5000.0f)
        , durationParam(100.0f, 100.0f, 0, 44100*10)
        , panningParam(0.0f, 0.0f, -1.0f, 1.0f)
        , readposParam(44100.0f, 0.0f, 0, 88200)
        , velocityParam(1.0f, 0.0f, 0.25f, 4.0f)
    {
        pool.elements.reserve(4096);
    }

    template <typename SourceBufferType>
    void processBlock(juce::AudioBuffer<float>& writeBuffer)
    {
        puro::Buffer<float, 2> dstBuffer (writeBuffer.getNumSamples());

        // TODO potential bug if number of output channels is not 2
        errorif(writeBuffer.getNumChannels() < 2, "BUG: implement different write buffer sizes");

        for (int ch=0; ch < dstBuffer.getNumChannels(); ++ch)
            dstBuffer.channelPtrs[ch] = writeBuffer.getWritePointer(ch);

        const int numChannels = context.source.getNumChannels();
        if (numChannels == 1)
        {
            using CallSourceBufferType = puro::Buffer<typename SourceBufferType::value_type, 1>;
            CallSourceBufferType srcBuffer = puro::buffer_convert_to_type<CallSourceBufferType, SourceBufferType> (context.source);
            processGrains(dstBuffer, srcBuffer);
        }
        else if (numChannels == 2)
        {
            using CallSourceBufferType = puro::Buffer<typename SourceBufferType::value_type, 2>;
            CallSourceBufferType srcBuffer = puro::buffer_convert_to_type<CallSourceBufferType, SourceBufferType> (context.source);
            processGrains(dstBuffer, srcBuffer);
        }
        else if (numChannels != 0)
        {
            errorif(true, "source buffer channel number not supported");
        }
    }

    template <typename BufferType, typename SourceBufferType>
    void processGrains(BufferType buffer, SourceBufferType sourceBuffer)
    {
        const int blockSize = buffer.length();

        for (auto&& it : pool)
        {
            if (process_grain(buffer, it.get(), context, sourceBuffer))
            {
                pool.pop(it);
            }
        }

        int n = buffer.length();
        while ((n = timer.advance(n)) > 0)
        {
            const float interval = intervalParam.get();
            timer.interval = math::round(durationParam.centre / interval);
            if (timer.interval < 0)
                errorif(true, "");
            const int duration = durationParam.get();
            const float panning = panningParam.get();
            const int readpos = readposParam.get();
            const float velocity = velocityParam.get();

            auto it = pool.push(Grain(blockSize - n, duration, readpos, panning, velocity, sourceBuffer.length()));

            if (it.isValid())
            {
                if (process_grain(buffer, it.get(), context, sourceBuffer))
                    pool.pop(it);
            }
        }
    }

    puro::Parameter<float, true> intervalParam;
    puro::Parameter<int, false> durationParam;
    puro::Parameter<float, false> panningParam;
    puro::Parameter<int, false> readposParam;
    puro::Parameter<float, true> velocityParam;

    Context context;
    puro::Timer<int> timer;
    puro::AlignedPool<Grain> pool;
};
