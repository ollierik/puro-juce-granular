#pragma once

#ifdef JUCE_DEBUG
    #define PURO_DEBUG 1
#else
    #define PURO_DEBUG 0
#endif

#define MAX_NUM_CHANNELS 2
#define MAX_NUM_SRC_CHANNELS 2
#define MAX_BLOCK_SIZE 2048

#include "puro.hpp"

/** A grain is a single audio playback instance. */
struct Grain
{
    Grain(int offset, int length, int startIndex, float panning, double velocity, int sourceLength)
        : alignment({ offset, length })
        , readInc(velocity)
        , readPos(startIndex)
        , envelopeInc(puro::envelope_halfcos_get_increment<float>(length))
        , envelopePos(envelopeInc)
        , panCoeffs(puro::pan_create_stereo(panning))
    {
        std::tie(alignment, readPos) = puro::interp_avoid_out_of_bounds_reads<3>(alignment, readPos, readInc, sourceLength);
    }

    puro::relative_alignment alignment;

    const double readInc;
    double readPos;

    const float envelopeInc;
    float envelopePos;

    puro::PanCoeffs<float, 2> panCoeffs;
};

/** Context is passed to process_grain function, and may contain persistent data or memory for the algorithm.
    In this case, it holds temp buffers to do the audio processing on.
 */
struct Context
{
    puro::heap_block_pool<float, std::allocator<float>> context_pool;
    puro::dynamic_buffer<MAX_NUM_CHANNELS, float> temp;
    puro::buffer<1, float> envelope;
    
    Context() : temp(MAX_NUM_CHANNELS, MAX_BLOCK_SIZE, context_pool), envelope(MAX_BLOCK_SIZE, context_pool) {}
};

/** The actual beef of the processor, this function gets called for every grain in every block
    It receives a buffer dst that the grain should add its signal into.
 */
template <typename BufferType, typename ElementType, typename ContextType, typename SourceType>
bool process_grain(BufferType dst, ElementType& grain, ContextType& context, const SourceType source)
{
    // Crop the dst buffer to fit the grain's offset and length, i.e. if the grain starts in the middle of the block, or ends
    // before the end of the block. Update the dst buffer and grain.alignment.
    std::tie(dst, grain.alignment) = puro::alignment_advance_and_crop_buffer(dst, grain.alignment);
    
    // audio buffer uses the memory from context.temp, casting it into a buffer with compile-time constant number of channels, and truncating it to needed length
    auto audio = context.temp.template as_buffer<SourceType>().trunc(dst.length()); // clang "feature", requires the keyword "template" with the function call
    
    // read audio from source, cubic interpolation
    grain.readPos = puro::interp3_fill(audio, source, grain.readPos, grain.readInc);
    
    // truncate temp envelope buffer to fit the length of our audio, and fill
    auto envelope = context.envelope.trunc(audio.length());
    grain.envelopePos = puro::envelope_halfcos_fill(envelope, grain.envelopePos, grain.envelopeInc);

    puro::multiply(audio, envelope);

    // get the cropped output segment that we want to add our output to, apply panning and add the audio signal
    BufferType output = dst.trunc(envelope.length());
    puro::pan_apply_and_add(output, audio, grain.panCoeffs);

    // return true if should be removed
    return (grain.alignment.remaining <= 0) // grain has depleted
            || (output.length() != dst.length()); // we've run out of source material to read
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
        , sourceBuffer(0, 0)
    {
        pool.elements.reserve(4096);
    }

    /**
     A bootstrap function to call processGrains with correct number of source channels.
     This forces the compiler to compile an optimised version for each n of source channels. This removes potential branch misses down the line,
     since the actual inner loop (process_grain) runs with complete information about the number of channels it is processing.
     */
    void processBlock(juce::AudioBuffer<float>& writeBuffer)
    {
        // TODO potential bug if number of output channels is not 2
        errorif(writeBuffer.getNumChannels() < 2, "BUG: implement different write buffer sizes");
        
        auto dstBuffer = puro::buffer_from_juce_buffer<puro::buffer<2, float>> (writeBuffer);

        const int numChannels = sourceBuffer.num_channels();
        if (numChannels == 1)
        {
            processGrains (dstBuffer, sourceBuffer.as_buffer<puro::buffer<1, float>>());
        }
        else if (numChannels == 2)
        {
            processGrains (dstBuffer, sourceBuffer.as_buffer<puro::buffer<2, float>>());
        }
        else if (numChannels != 0)
        {
            errorif(true, "source buffer channel number not supported");
        }
        else
        {
            // if numChannels == 0, the audio source file has not been loaded, and we do nothing
        }
    }

    template <typename BufferType, typename SourceType>
    void processGrains(BufferType buffer, SourceType source)
    {
        const int blockSize = buffer.length();

        // iterate all grains, adding their output to audio buffer
        for (auto&& it : pool)
        {
            if (process_grain(buffer, it.get(), context, source))
            {
                pool.pop(it);
            }
        }

        // add new grains
        int n = buffer.length();
        while ((n = timer.advance(n)) > 0)
        {
            // for each tick, reset interval, get new values for the grain, and create grain
            
            const float interval = intervalParam.get();
            timer.interval = puro::math::round(durationParam.centre / interval);
            
            errorif(timer.interval < 0, "Well this is unexpected, timer shouldn't be let to do that");
            
            const int duration = durationParam.get();
            const float panning = panningParam.get();
            const int readpos = readposParam.get();
            const float velocity = velocityParam.get();

            // push to the pool of grains, get a handle in return
            auto it = pool.push(Grain(blockSize - n, duration, readpos, panning, velocity, sourceBuffer.length()));

            // immediately process the first block for the newly created grain, potentially also already removing it
            // if the pool is full, it rejects the push operation and returns invalid iterator, and the grain doesn't get added
            if (it.is_valid())
            {
                if (process_grain(buffer, it.get(), context, source))
                {
                    pool.pop(it);
                }
            }
        }
    }

    Context context;
    puro::Timer<int> timer;
    puro::AlignedPool<Grain> pool;
    puro::dynamic_buffer<MAX_NUM_SRC_CHANNELS, float> sourceBuffer;

    puro::Parameter<float, true> intervalParam;
    puro::Parameter<int, false> durationParam;
    puro::Parameter<float, false> panningParam;
    puro::Parameter<int, false> readposParam;
    puro::Parameter<float, true> velocityParam;
};
