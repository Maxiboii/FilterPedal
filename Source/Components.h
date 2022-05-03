//
//  Components.h
//  FilterPedal
//
//  Created by Maksim Chichkan on 4/29/22.
//

#ifndef Components_h
#define Components_h


#endif /* Components_h */


template <typename Type>
class Distortion
{
private:
    //==============================================================================
    enum
    {
        preGainIndex,
        waveshaperIndex,
        postGainIndex,
    };
    using WaveShaper = juce::dsp::WaveShaper<Type>;
    using Gain = juce::dsp::Gain<float>;
    using ProcessorChain = juce::dsp::ProcessorChain<Gain, WaveShaper, Gain>;

    std::unique_ptr<ProcessorChain> processorChain;
    
public:
    //==============================================================================
    Type preGainAmount { Type (0) };
    Type postGainAmount { Type (0) };
    
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
    template <typename SampleType>
    SampleType processSample (const SampleType& sample) noexcept
    {
        auto& preGain = processorChain->template get<preGainIndex>();
        auto& waveshaper = processorChain->template get<waveshaperIndex>();
        auto& postGain = processorChain->template get<postGainIndex>();
        
        auto processedSample = preGain.processSample (sample);
        processedSample = waveshaper.processSample (processedSample);
        processedSample = postGain.processSample (processedSample);
//
        return processedSample;
    }

    //==============================================================================
    void reset() noexcept
    {
        processorChain.reset();
    }
    
    //==============================================================================
    template <typename AmountType>
    void setPreGain (const AmountType& amount) noexcept
    {
        auto& preGain = processorChain->template get<preGainIndex>();
        preGain.setGainDecibels(amount);
    }
    
    //==============================================================================
    template <typename AmountType>
    void setPostGain (const AmountType& amount) noexcept
    {
        auto& postGain = processorChain->template get<postGainIndex>();
        postGain.setGainDecibels(amount);
    }
    
    //==============================================================================
    Type getPreGain () noexcept
    {
        return preGainAmount;
    }
    


};

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
template <typename Type, size_t maxNumChannels = 1>
class Delay
{
public:
    //==============================================================================
    Delay()
    {
        setMaxDelayTime (3.1f);
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        jassert (spec.numChannels <= maxNumChannels);
        sampleRate = (Type) spec.sampleRate;
        updateDelayLineSize();
        updateDelayTime();

        lowCutCoefficients = juce::dsp::IIR::Coefficients<Type>::makeFirstOrderHighPass (sampleRate, lowCutFreq);
        highCutCoefficients = juce::dsp::IIR::Coefficients<Type>::makeFirstOrderLowPass (sampleRate, highCutFreq);

        for (auto& f : lowCutFilters)
        {
            f.prepare (spec);
            f.coefficients = lowCutCoefficients;
        }
        
        for (auto& f : lowCutFilters)
        {
            f.prepare (spec);
            f.coefficients = highCutCoefficients;
        }
    }

    //==============================================================================
    void reset() noexcept
    {
        for (auto& f : lowCutFilters)
            f.reset();      // [5]
        for (auto& f : highCutFilters)
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
    void setLowCutFreq (Type newValue) noexcept
    {
        jassert (newValue >= Type (20) && newValue <= Type (20000));
        lowCutFreq = newValue;
    }
    
    //==============================================================================
    void setHighCutFreq (Type newValue) noexcept
    {
        jassert (newValue >= Type (20) && newValue <= Type (20000));
        highCutFreq = newValue;
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
    void setDryLevel (Type newValue) noexcept
    {
        jassert (newValue >= Type (0) && newValue <= Type (1));
        dryLevel = newValue;
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
    void setDistortionPreGainAmount (Type newValue) noexcept
    {
        jassert (newValue >= Type (0) && newValue <= Type (100));
        distortionPreGainAmount = newValue;
    }
    
    //==============================================================================
    void setDistortionPostGainAmount (Type newValue) noexcept
    {
        jassert (newValue >= Type (-100) && newValue <= Type (100));
        distortionPostGainAmount = newValue;
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
            auto& lowCutFilter = lowCutFilters[ch];
            auto& highCutFilter = highCutFilters[ch];
            auto& distortion = distortions[ch];
            
            distortion.setPreGain(distortionPreGainAmount);
            distortion.setPostGain(distortionPostGainAmount);
            
            lowCutCoefficients = juce::dsp::IIR::Coefficients<Type>::makeFirstOrderHighPass (sampleRate, lowCutFreq);
            highCutCoefficients = juce::dsp::IIR::Coefficients<Type>::makeFirstOrderLowPass (sampleRate, highCutFreq);
            lowCutFilter.coefficients = lowCutCoefficients;
            highCutFilter.coefficients = highCutCoefficients;
     
            for (size_t i = 0; i < numSamples; ++i)
            {
                auto delayedSample = lowCutFilter.processSample (dline.get (delayTime));
                delayedSample = highCutFilter.processSample (delayedSample);
                auto inputSample = input[i];
                auto dlineInputSample = std::tanh (inputSample + feedback * delayedSample);
                dline.push (dlineInputSample);
                
                auto drySample = inputSample * dryLevel;
                auto wetSample = wetLevel * delayedSample;
                auto distortedWetSample = distortion.processSample(wetSample);
                auto outputSample = drySample + distortedWetSample;
                output[i] = outputSample;
            }
        }
    }

private:
    //==============================================================================
    std::array<DelayLine<Type>, maxNumChannels> delayLines;
    std::array<size_t, maxNumChannels> delayTimesSample;
    std::array<Type, maxNumChannels> delayTimes;
    Type lowCutFreq { Type (500) };
    Type highCutFreq { Type (3000) };
    Type feedback { Type (0) };
    Type dryLevel { Type (0) };
    Type wetLevel { Type (0) };
    Type distortionPreGainAmount { Type (0) };
    Type distortionPostGainAmount { Type (0) };

    std::array<juce::dsp::IIR::Filter<Type>, maxNumChannels> lowCutFilters, highCutFilters;
    typename juce::dsp::IIR::Coefficients<Type>::Ptr lowCutCoefficients, highCutCoefficients;
    
    std::array<Distortion<float>, maxNumChannels> distortions;

    Type sampleRate   { Type (44.1e3) };
    Type maxDelayTime { Type (3) };

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
