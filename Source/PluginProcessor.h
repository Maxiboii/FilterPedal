/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Components.h"


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
    
    float delayAmount { 0 };
    
    bool lowCutBypassed { false }, highCutBypassed { false }, distortionBypassed { false }, delayBypassed { false };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using Gain = juce::dsp::Gain<float>;

using DistortionChain = juce::dsp::ProcessorChain<Gain, Distortion<float>, Gain>;

using DelayChain = juce::dsp::ProcessorChain<Delay<float>>;

using MonoChain = juce::dsp::ProcessorChain<CutFilter, CutFilter, DistortionChain, DelayChain>;

enum ChainPositions
{
    LowCut,
    HighCut,
    WaveshapingDistortion,
    DistortedDelay
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

template<typename ChainType, typename SettingsType>
void updateDelayValues(ChainType& chain, SettingsType chainSettings)
{
    chain.template setBypassed<0>(true);
    
    chain.template get<0>().setWetLevel(chainSettings.delayAmount);
    
    chain.template setBypassed<0>(false);
}

template<typename ChainType, typename SettingsType>
void muteDelay(ChainType& chain, SettingsType chainSettings)
{
    chain.template setBypassed<0>(true);

    chain.template get<0>().setWetLevel(0);
    
    chain.template setBypassed<0>(false);
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
    void updateDelay(const ChainSettings& chainSettings);
    
    void updateComponents();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPedalAudioProcessor)
};
