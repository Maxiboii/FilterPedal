/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


//==============================================================================
template <typename Type>
class DelayLine
{
public:
    void clear() noexcept
    {
        std::fill (rawData.begin(), rawData.end(), Type (0));
    }

    size_t size() const noexcept
    {
        return rawData.size();
    }

    void resize (size_t newValue)
    {
        rawData.resize (newValue);
        leastRecentIndex = 0;
    }

    Type back() const noexcept
    {
        return rawData[leastRecentIndex];
    }

    Type get (size_t delayInSamples) const noexcept
    {
        jassert (delayInSamples >= 0 && delayInSamples < size());
 
        return rawData[(leastRecentIndex + 1 + delayInSamples) % size()];   // [3]
    }

    /** Set the specified sample in the delay line */
    void set (size_t delayInSamples, Type newValue) noexcept
    {
        jassert (delayInSamples >= 0 && delayInSamples < size());
 
        rawData[(leastRecentIndex + 1 + delayInSamples) % size()] = newValue; // [4]
    }

    /** Adds a new value to the delay line, overwriting the least recently added sample */
    void push (Type valueToAdd) noexcept
    {
        rawData[leastRecentIndex] = valueToAdd;                                         // [1]
        leastRecentIndex = leastRecentIndex == 0 ? size() - 1 : leastRecentIndex - 1;   // [2]
    }

private:
    std::vector<Type> rawData;
    size_t leastRecentIndex = 0;
};

//==============================================================================
template <typename Type, size_t maxNumChannels = 2>
class Delay
{
public:
    //==============================================================================
    Delay()
    {
        setMaxDelayTime (2.0f);
        setDelayTime (0, 0.7f);
        setDelayTime (1, 0.5f);
        setWetLevel (0.8f);
        setFeedback (0.5f);
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        jassert (spec.numChannels <= maxNumChannels);
        sampleRate = (Type) spec.sampleRate;
        updateDelayLineSize();
        updateDelayTime();

        filterCoefs = juce::dsp::IIR::Coefficients<Type>::makeFirstOrderHighPass (sampleRate, Type (1e3)); // [2]

        for (auto& f : filters)
        {
            f.prepare (spec);
            f.coefficients = filterCoefs;
        }
    }

    //==============================================================================
    void reset() noexcept
    {
        for (auto& f : filters)
            f.reset();      // [5]
 
        for (auto& dline : delayLines)
            dline.clear();  // [6]
    }

    //==============================================================================
    size_t getNumChannels() const noexcept
    {
        return delayLines.size();
    }

    //==============================================================================
    void setMaxDelayTime (Type newValue)
    {
        jassert (newValue > Type (0));
        maxDelayTime = newValue;
        updateDelayLineSize(); // [1]
    }

    //==============================================================================
    void setFeedback (Type newValue) noexcept
    {
        jassert (newValue >= Type (0) && newValue <= Type (1));
        feedback = newValue;
    }

    //==============================================================================
    void setWetLevel (Type newValue) noexcept
    {
        jassert (newValue >= Type (0) && newValue <= Type (1));
        wetLevel = newValue;
    }

    //==============================================================================
    void setDelayTime (size_t channel, Type newValue)
    {
        if (channel >= getNumChannels())
        {
            jassertfalse;
            return;
        }
 
        jassert (newValue >= Type (0));
        delayTimes[channel] = newValue;
 
        updateDelayTime();  // [3]
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        auto& inputBlock  = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        auto numSamples  = outputBlock.getNumSamples();
        auto numChannels = outputBlock.getNumChannels();
     
        jassert (inputBlock.getNumSamples() == numSamples);
        jassert (inputBlock.getNumChannels() == numChannels);
     
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* input  = inputBlock .getChannelPointer (ch);
            auto* output = outputBlock.getChannelPointer (ch);
            auto& dline = delayLines[ch];
            auto delayTime = delayTimesSample[ch];
            auto& filter = filters[ch];
     
            for (size_t i = 0; i < numSamples; ++i)
            {
                auto delayedSample = filter.processSample (dline.get (delayTime));          // [1]
                auto inputSample = input[i];
                auto dlineInputSample = std::tanh (inputSample + feedback * delayedSample);
                dline.push (dlineInputSample);
                auto outputSample = inputSample + wetLevel * delayedSample;
                output[i] = outputSample;
            }
        }
    }

private:
    //==============================================================================
    std::array<DelayLine<Type>, maxNumChannels> delayLines;
    std::array<size_t, maxNumChannels> delayTimesSample;
    std::array<Type, maxNumChannels> delayTimes;
    Type feedback { Type (0) };
    Type wetLevel { Type (0) };

    std::array<juce::dsp::IIR::Filter<Type>, maxNumChannels> filters;
    typename juce::dsp::IIR::Coefficients<Type>::Ptr filterCoefs;

    Type sampleRate   { Type (44.1e3) };
    Type maxDelayTime { Type (2) };

    //==============================================================================
    void updateDelayLineSize()
    {
        auto delayLineSizeSamples = (size_t) std::ceil (maxDelayTime * sampleRate);
 
        for (auto& dline : delayLines)
            dline.resize (delayLineSizeSamples);    // [2]
    }

    //==============================================================================
    void updateDelayTime() noexcept
    {
        for (size_t ch = 0; ch < maxNumChannels; ++ch)
            delayTimesSample[ch] = (size_t) juce::roundToInt (delayTimes[ch] * sampleRate);
    }
};

template <typename Type>
class Distortion
{
private:
    //==============================================================================
    enum
    {
        waveshaperIndex,
    };
    using WaveShaper = juce::dsp::WaveShaper<Type>;
    using ProcessorChain = juce::dsp::ProcessorChain<WaveShaper>;

    std::unique_ptr<ProcessorChain> processorChain;
    
public:
    //==============================================================================
    Distortion()
    {
        processorChain = std::make_unique<ProcessorChain>();
        
        auto& waveshaper = processorChain->template get<waveshaperIndex>();
        waveshaper.functionToUse = [] (Type x)
                                   {
                                       return std::tanh (x);
                                   };
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        processorChain.get()->prepare (spec);
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        processorChain.get()->process (context);
    }

    //==============================================================================
    void reset() noexcept
    {
        processorChain.reset();
    }


};

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    float lowCutFreq { 0 }, highCutFreq { 0 };
    
    Slope lowCutSlope { Slope::Slope_12 }, highCutSlope { Slope::Slope_12 };
    
    float distortionPreGainInDecibels { 0 }, distortionPostGainInDecibels { 0 };
    
    bool lowCutBypassed { false }, highCutBypassed { false }, distortionBypassed { false };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using Gain = juce::dsp::Gain<float>;

using DistortionChain = juce::dsp::ProcessorChain<Gain, Distortion<float>, Gain>;

using MonoChain = juce::dsp::ProcessorChain<CutFilter, CutFilter, DistortionChain, Delay<float>>;

enum ChainPositions
{
    LowCut,
    HighCut,
    WaveshapingDistortion,
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);

template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& chain,
                     const CoefficientType& coefficients,
                     const Slope& slope)
{

    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);

    switch( slope )
    {

        case Slope_48:
        {
            update<3>(chain, coefficients);
        }
        case Slope_36:
        {
            update<2>(chain, coefficients);
        }
        case Slope_24:
        {
            update<1>(chain, coefficients);
        }
        case Slope_12:
        {
            update<0>(chain, coefficients);
        }
    }
}

template<typename ChainType, typename SettingsType>
void updateDistortionGain(ChainType& chain, SettingsType chainSettings)
{
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    
    chain.template get<0>().setGainDecibels(chainSettings.distortionPreGainInDecibels);
    chain.template get<2>().setGainDecibels(chainSettings.distortionPostGainInDecibels);
    
    chain.template setBypassed<0>(false);
    chain.template setBypassed<1>(false);
    chain.template setBypassed<2>(false);
}

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate )
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                       sampleRate,
                                                                                       2* (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate )
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                      sampleRate,
                                                                                      2* (chainSettings.highCutSlope + 1));
}

//==============================================================================
/**
*/
class FilterPedalAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    FilterPedalAudioProcessor();
    ~FilterPedalAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};
    
private:
    MonoChain leftChain, rightChain;
    
    std::unique_ptr<Distortion<float>> distortion;
    
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    void updateDistortion(const ChainSettings& chainSettings);
    
    void updateFilters();
    
    juce::dsp::Oscillator<float> osc;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPedalAudioProcessor)
};
