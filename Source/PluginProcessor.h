/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


template <typename Type>
class Distortion
{
private:
    //==============================================================================
    enum
    {
        filterIndex,
        preGainIndex,
        waveshaperIndex,
        postGainIndex
    };

    using Filter = juce::dsp::IIR::Filter<Type>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<Type>;
    using ProcessorDuplicator = juce::dsp::ProcessorDuplicator<Filter, FilterCoefs>;
    using Gain = juce::dsp::Gain<Type>;
    using WaveShaper = juce::dsp::WaveShaper<Type>;
    using ProcessorChain = juce::dsp::ProcessorChain<ProcessorDuplicator, Gain, WaveShaper, Gain>;

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

        auto& preGain = processorChain->template get<preGainIndex>();
        preGain.setGainDecibels (0.0f);

        auto& postGain = processorChain->template get<postGainIndex>();
        postGain.setGainDecibels (0.0f);
    }
    
    template<typename ChainType, typename ChainSettings>
    void updateValues (ChainType& chain, ChainSettings settings)
    {
        
        
        
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        std::cout << settings.distortionAmount << std::endl;
        std::cout << settings.distortionGainInDecibels << std::endl;
        
        auto& preGain = processorChain->template get<preGainIndex>();
        preGain.setGainDecibels (settings.distortionAmount);

        auto& postGain = processorChain->template get<postGainIndex>();
        postGain.setGainDecibels (settings.distortionGainInDecibels);
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        auto& filter = processorChain->template get<filterIndex>();
        filter.state = FilterCoefs::makeFirstOrderHighPass (spec.sampleRate, 1000.0f);

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
    
    float distortionAmount { 0 }, distortionGainInDecibels { 0 };
    
    bool lowCutBypassed { false }, highCutBypassed { false }, distortionBypassed { false };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using MonoChain = juce::dsp::ProcessorChain<CutFilter, CutFilter, Distortion<float>>;

enum ChainPositions
{
    LowCut,
    HighCut,
//    Peak,
    Distortion_
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
    
//    void updatePeakFilter(const ChainSettings& chainSettings);
    
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    void updateDistortion(const ChainSettings& chainSettings);
    
    void updateFilters();
    
    juce::dsp::Oscillator<float> osc;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPedalAudioProcessor)
};
