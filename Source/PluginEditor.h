/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics&,
                           int x, int y, int width, int height,
                           float SliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override;
    
    void drawToggleButton (juce::Graphics &g,
                           juce::ToggleButton & toggleButton,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
    juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                 juce::Slider::TextEntryBoxPosition::NoTextBox),
    param(&rap),
    suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }
    
    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }
    
    struct LabelPos
    {
        float pos;
        juce::String label;
    };
    
    juce::Array<LabelPos> labels;
    juce::Array<LabelPos> nameLabels;
    
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
private:
    LookAndFeel lnf;
    
    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct ResponseCurveComponent: juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    ResponseCurveComponent(FilterPedalAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged (int parameterIndex, float newValue) override;

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override { }

    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    FilterPedalAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged { false };

    MonoChain monoChain;

    void updateChain();

    juce::Image background;

    juce::Rectangle<int> getRenderArea();

    juce::Rectangle<int> getAnalysisArea();
};

//==============================================================================
struct PowerButton : juce::ToggleButton { };
/**
*/
class FilterPedalAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FilterPedalAudioProcessorEditor (FilterPedalAudioProcessor&);
    ~FilterPedalAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FilterPedalAudioProcessor& audioProcessor;
    
    RotarySliderWithLabels lowCutFreqSlider,
                           lowCutSlopeSlider,
                           highCutFreqSlider,
                           highCutSlopeSlider,
                           distortionPreGainSlider,
                           distortionPostGainSlider,
                           delayDrySlider,
                           delayWetSlider,
                           delayFeedbackSlider,
                           delayTimeLeftSlider,
                           delayTimeRightSlider,
                           delayLowCutSlider;
    
    ResponseCurveComponent responseCurveComponent;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment lowCutFreqSliderAttachment,
               lowCutSlopeSliderAttachment,
               highCutFreqSliderAttachment,
               highCutSlopeSliderAttachment,
               distortionPreGainSliderAttachment,
               distortionPostGainSliderAttachment,
               delayDrySliderAttachment,
               delayWetSliderAttachment,
               delayFeedbackSliderAttachment,
               delayTimeLeftSliderAttachment,
               delayTimeRightSliderAttachment,
               delayLowCutSliderAttachment;
    
    PowerButton lowcutBypassButton, highcutBypassButton, distortionBypassButton, delayBypassButton;
    
    using ButtonAttachment = APVTS::ButtonAttachment;
    ButtonAttachment lowcutBypassButtonAttachment,
                     highcutBypassButtonAttachment,
                     distortionBypassButtonAttachment,
                     delayBypassButtonAttachment;
    
    std::vector<juce::Component*> getComps();
    
    LookAndFeel lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPedalAudioProcessorEditor)
};
