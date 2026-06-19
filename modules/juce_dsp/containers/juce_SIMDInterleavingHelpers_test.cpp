/*
  ==============================================================================

   This file is part of the JUCE 9 preview.
   Copyright (c) Raw Material Software Limited

   You may use this code under the terms of the AGPLv3
   (see www.gnu.org/licenses).

   For the JUCE 9 preview this file cannot be licensed commercially.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce::dsp
{

#if JUCE_USE_SIMD

struct SIMDInterleavingHelpersTest : public UnitTest
{
    SIMDInterleavingHelpersTest()
        : UnitTest ("SIMDInterleavingHelpers", UnitTestCategories::dsp)
    {
    }

    void runTest() override
    {
        static constexpr auto registerSize = SIMDRegister<float>::size();

        testCase ("Interleave mono", [&]
        {
            const auto numSamples = 128;
            const auto iota = std::invoke ([&]
            {
                std::vector<float> result (numSamples);
                std::iota (result.begin(), result.end(), 1.0f);
                return result;
            });

            const std::vector<float> zeros (numSamples);

            const auto* floatData = iota.data();
            AudioBlock<const float> source { &floatData, 1, iota.size() };

            std::vector<SIMDRegister<float>> simd (numSamples);
            const auto simdData = simd.data();
            AudioBlock<SIMDRegister<float>> destination { &simdData, 1, simd.size() };

            SIMDInterleavingHelpers::interleave (source, destination, zeros);

            for (auto i = 0; i < numSamples; ++i)
            {
                for (size_t j = 0; j < registerSize; ++j)
                {
                    const auto expected = j < 1 ? source.getChannelPointer (j)[i] : 0.0f;
                    expectEquals ((float) destination.getChannelPointer (0)[i][j], expected);
                }
            }
        });

        testCase ("Interleave multi", [&]
        {
            auto random = getRandom();

            const auto numSamples = 128;
            const auto numChannels = 7;

            const auto noise = std::invoke ([&]
            {
                std::vector<std::vector<float>> result (numChannels);

                for (auto& channel : result)
                {
                    channel.resize (numSamples);

                    for (auto& sample : channel)
                    {
                        sample = random.nextFloat();
                    }
                }

                return result;
            });

            const std::vector<float> zeros (numSamples);

            std::vector<const float*> sourcePointers;

            for (auto& channel : noise)
                sourcePointers.push_back (channel.data());

            AudioBlock<const float> source { sourcePointers.data(), sourcePointers.size(), numSamples };

            auto simd = std::invoke ([&]
            {
                std::vector<std::vector<SIMDRegister<float>>> result ((numChannels + registerSize - 1) / registerSize);

                for (auto& channel : result)
                    channel.resize (numSamples);

                return result;
            });

            std::vector<SIMDRegister<float>*> destinationPointers;

            for (auto& channel : simd)
                destinationPointers.push_back (channel.data());

            AudioBlock<SIMDRegister<float>> destination { destinationPointers.data(), destinationPointers.size(), numSamples };

            SIMDInterleavingHelpers::interleave (source, destination, zeros);

            const auto numDestChannels = destination.getNumChannels() * registerSize;

            for (auto i = 0; i < numSamples; ++i)
            {
                for (size_t j = 0; j < numDestChannels; ++j)
                {
                    const auto expected = j < source.getNumChannels() ? source.getChannelPointer (j)[i] : 0.0f;
                    expectEquals ((float) destination.getChannelPointer (j / registerSize)[i][j % registerSize], expected);
                }
            }
        });

        testCase ("Deinterleave mono", [&]
        {
            const auto numSamples = 128;
            auto iota = std::invoke ([&]
            {
                std::vector<SIMDRegister<float>> result (numSamples);

                for (auto [index, value] : enumerate (result, 1))
                    value[0] = (float) index;

                return result;
            });

            std::vector<float> scratch (numSamples);

            auto* sourceData = iota.data();
            AudioBlock<SIMDRegister<float>> source { &sourceData, 1, iota.size() };

            std::vector<float> deinterleaved (numSamples);
            const auto destinationData = deinterleaved.data();
            AudioBlock<float> destination { &destinationData, 1, deinterleaved.size() };

            SIMDInterleavingHelpers::deinterleave (source, destination, scratch);

            for (auto i = 0; i < numSamples; ++i)
            {
                for (size_t j = 0; j < destination.getNumChannels(); ++j)
                {
                    expectEquals (destination.getChannelPointer (j)[i],
                                  (float) source.getChannelPointer (j / registerSize)[i][j % registerSize]);
                }
            }
        });

        testCase ("Deinterleave multi", [&]
        {
            auto random = getRandom();

            const auto numSamples = 128;
            const auto numChannels = 7;

            auto noise = std::invoke ([&]
            {
                std::vector<std::vector<SIMDRegister<float>>> result ((numChannels + registerSize - 1) / registerSize);

                for (auto& channel : result)
                {
                    channel.resize (numSamples);

                    for (auto& sample : channel)
                    {
                        for (size_t j = 0; j < registerSize; ++j)
                        {
                            sample[j] = random.nextFloat();
                        }
                    }
                }

                return result;
            });

            std::vector<float> scratch (numSamples);

            std::vector<SIMDRegister<float>*> sourcePointers;

            for (auto& channel : noise)
                sourcePointers.push_back (channel.data());

            AudioBlock<SIMDRegister<float>> source { sourcePointers.data(), sourcePointers.size(), numSamples };

            auto deinterleaved = std::invoke ([&]
            {
                std::vector<std::vector<float>> result (numChannels);

                for (auto& channel : result)
                    channel.resize (numSamples);

                return result;
            });

            std::vector<float*> destinationPointers;

            for (auto& channel : deinterleaved)
                destinationPointers.push_back (channel.data());

            AudioBlock<float> destination { destinationPointers.data(), destinationPointers.size(), numSamples };

            SIMDInterleavingHelpers::deinterleave (source, destination, scratch);

            for (auto i = 0; i < numSamples; ++i)
            {
                for (size_t j = 0; j < numChannels; ++j)
                {
                    expectEquals (destination.getChannelPointer (j)[i],
                                  (float) source.getChannelPointer (j / registerSize)[i][j % registerSize]);
                }
            }
        });
    }
};

static SIMDInterleavingHelpersTest simdInterleavingHelpersTest;

#endif

} // namespace juce::dsp
