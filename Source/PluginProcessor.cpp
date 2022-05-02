/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FilterPedalAudioProcessor::FilterPedalAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

FilterPedalAudioProcessor::~FilterPedalAudioProcessor()
{
}

//==============================================================================
const juce::String FilterPedalAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FilterPedalAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FilterPedalAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FilterPedalAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FilterPedalAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FilterPedalAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FilterPedalAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FilterPedalAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FilterPedalAudioProcessor::getProgramName (int index)
{
    return {};
}

void FilterPedalAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FilterPedalAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec;
    
    spec.maximumBlockSize = samplesPerBlock;
    
    spec.numChannels = 1;
    
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    updateComponents();
}

void FilterPedalAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FilterPedalAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FilterPedalAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    updateComponents();

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool FilterPedalAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FilterPedalAudioProcessor::createEditor()
{
    return new FilterPedalAudioProcessorEditor (*this);
//    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FilterPedalAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void FilterPedalAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if( tree.isValid() )
    {
        apvts.replaceState(tree);
        updateComponents();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    settings.distortionPreGainInDecibels = apvts.getRawParameterValue("Distortion Amount")->load();
    settings.distortionPostGainInDecibels = apvts.getRawParameterValue("Distortion PostGain")->load();
    settings.delayDry = apvts.getRawParameterValue("Delay Dry")->load();
    settings.delayWet = apvts.getRawParameterValue("Delay Wet")->load();
    settings.delayFeedback = apvts.getRawParameterValue("Delay Feedback")->load();
    settings.delayTimeLeft = apvts.getRawParameterValue("Delay Time Left")->load();
    settings.delayTimeRight = apvts.getRawParameterValue("Delay Time Right")->load();
    settings.delayLowCutFreq = apvts.getRawParameterValue("Delay LowCut")->load();
    settings.delayHighCutFreq = apvts.getRawParameterValue("Delay HighCut")->load();
    settings.delayDistortionPreGain = apvts.getRawParameterValue("Delay Distortion")->load();
    settings.delayDistortionPostGain = apvts.getRawParameterValue("Delay PostGain")->load();
    
    settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed")->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed")->load() > 0.5f;
    settings.distortionBypassed = apvts.getRawParameterValue("Distortion Bypassed")->load() > 0.5f;
    settings.delayBypassed = apvts.getRawParameterValue("Delay Bypassed")->load() > 0.5f;

    return settings;
}

void updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    *old = *replacements;
}

void FilterPedalAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings)
{
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    
    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void FilterPedalAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    
    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void FilterPedalAudioProcessor::updateDistortion(const ChainSettings &chainSettings)
{
    auto& leftDistortion = leftChain.get<ChainPositions::WaveshapingDistortion>();
    auto& rightDistortion = rightChain.get<ChainPositions::WaveshapingDistortion>();

    leftChain.setBypassed<ChainPositions::WaveshapingDistortion>(chainSettings.distortionBypassed);
    rightChain.setBypassed<ChainPositions::WaveshapingDistortion>(chainSettings.distortionBypassed);

    updateDistortionGain(leftDistortion, chainSettings);
    updateDistortionGain(rightDistortion, chainSettings);
}

void FilterPedalAudioProcessor::updateDelay(const ChainSettings &chainSettings)
{
    auto& leftDelay = leftChain.get<ChainPositions::DistortedDelay>();
    auto& rightDelay = rightChain.get<ChainPositions::DistortedDelay>();
    
    leftChain.setBypassed<ChainPositions::DistortedDelay>(chainSettings.delayBypassed);
    rightChain.setBypassed<ChainPositions::DistortedDelay>(chainSettings.delayBypassed);
    
    if (chainSettings.delayBypassed == 1)
    {
        muteDelay(leftDelay, chainSettings);
        muteDelay(rightDelay, chainSettings);
    }
    else
    {
        updateDelayValues(leftDelay, chainSettings, 0);
        updateDelayValues(rightDelay, chainSettings, 1);
    }
}

void FilterPedalAudioProcessor::updateComponents()
{
    auto chainSettings = getChainSettings(apvts);
    
    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
    updateDistortion(chainSettings);
    updateDelay(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout FilterPedalAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                           20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                           20000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Distortion Amount",
                                                           "Distortion Amount",
                                                           juce::NormalisableRange<float>(0.f, 48.f, 0.1f, 1.f),
                                                           0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Distortion PostGain",
                                                           "Distortion PostGain",
                                                           juce::NormalisableRange<float>(-48.f, 48.f, 0.1f, 1.f),
                                                           0.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay Dry",
                                                           "Delay Dry",
                                                           juce::NormalisableRange<float>(0.f, 1.f, 0.01f, 1.f),
                                                           1.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay Wet",
                                                           "Delay Wet",
                                                           juce::NormalisableRange<float>(0.f, 1.f, 0.01f, 1.f),
                                                           0.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay Feedback",
                                                           "Delay Feedback",
                                                           juce::NormalisableRange<float>(0.f, 0.99f, 0.01f, 1.f),
                                                           0.3f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay Time Left",
                                                           "Delay Time Left",
                                                           juce::NormalisableRange<float>(0.f, 3.f, 0.01f, 1.f),
                                                           0.3f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay Time Right",
                                                           "Delay Time Right",
                                                           juce::NormalisableRange<float>(0.f, 3.f, 0.01f, 1.f),
                                                           0.3f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay LowCut",
                                                           "Delay LowCut",
                                                           juce::NormalisableRange<float>(200.f, 5000.f, 1.0f, 1.f),
                                                           500.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay HighCut",
                                                           "Delay HighCut",
                                                           juce::NormalisableRange<float>(3000.f, 10000.f, 1.0f, 1.f),
                                                           5000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay Distortion",
                                                           "Delay Distortion",
                                                           juce::NormalisableRange<float>(0.f, 48.f, 0.1f, 1.f),
                                                           0.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay PostGain",
                                                           "Delay PostGain",
                                                           juce::NormalisableRange<float>(-48.f, 48.f, 0.1f, 1.f),
                                                           0.f));

    
    juce::StringArray stringArray;
    for( int i = 0; i < 4; ++i )
    {
        juce::String str;
        str << (12 + i*12);
        str << " db/Oct";
        stringArray.add(str);
    }
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));
    
    layout.add(std::make_unique<juce::AudioParameterBool>("LowCut Bypassed", "LowCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("HighCut Bypassed", "HighCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Distortion Bypassed", "Distortion Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Delay Bypassed", "Delay Bypassed", false));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FilterPedalAudioProcessor();
}
