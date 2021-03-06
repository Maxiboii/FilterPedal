/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


auto orange = juce::Colour(225u, 134u, 1u);
auto blue = juce::Colour(0u, 220u, 255u);

void LookAndFeel::drawRotarySlider(juce::Graphics & g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider)
{
    using namespace juce;
    
    auto bounds = Rectangle<float>(x, y, width, height);
    
    auto enabled = slider.isEnabled();
    
    g.setColour(enabled ? Colour(102u, 102u, 102u) : Colours::darkgrey);
    g.fillEllipse(bounds);
    
    g.setColour(enabled ? orange : Colours::grey);
    g.drawEllipse(bounds, 1.5f);
    
    if( auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - width * 0.3);
        
        p.addRoundedRectangle(r, 2.f);
        
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
        
        g.fillPath(p);
        
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        
        g.setColour(enabled ? blue : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
    
    
}

void LookAndFeel::drawToggleButton(juce::Graphics &g,
                                   juce::ToggleButton &toggleButton,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    using namespace juce;
    
    if( auto* pb = dynamic_cast<PowerButton*>(&toggleButton) )
    {
        Path powerButton;
        
        auto bounds = toggleButton.getLocalBounds();
        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();
        
        float ang = 30.f;
        
        size -= 6;
        
        powerButton.addCentredArc(r.getCentreX(),
                                  r.getCentreY(),
                                  size * 0.5,
                                  size * 0.5,
                                  0.f,
                                  degreesToRadians(30.f),
                                  degreesToRadians(360.f - ang),
                                  true);
        
        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());
        
        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
        
        auto color = toggleButton.getToggleState() ? Colours::dimgrey : blue;
        
        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2);
    }
}
//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
//    g.setColour(Colours::red);
//    g.drawRect(getLocalBounds());
//    g.setColour(Colours::yellow);
//    g.drawRect(sliderBounds);
    
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);

    g.setColour(Colours::whitesmoke);
    for( int i = 0; i < nameLabels.size(); ++i )
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        Rectangle<float> r;
        auto str = nameLabels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(sliderBounds.getCentreX(), sliderBounds.getCentreY());
        r.setY(0);

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
    
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;
    
    g.setColour(Colours::whitesmoke);
    g.setFont(getTextHeight());
    
    auto numChoices = labels.size();
    for( int i = 0; i < numChoices; ++i )
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    size -= getTextHeight() * 2.5;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(getTextHeight() * 1.2);
    
    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if( auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param) )
        return choiceParam->getCurrentChoiceName();
    
    juce::String str;
    bool addK = false;
    
    if( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param) )
    {
        float val = getValue();
        auto paramName = floatParam->getName(100);
        if ( paramName == "Delay Dry" || paramName == "Delay Wet")
        {
            val *= 100;
        }
        
        if( val > 999.f )
        {
            val /= 1000.f; // 1001 / 1000 = 1.001
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    else
        {
            jassertfalse; // this shouldn't happen!
        }
    
    if( suffix.isNotEmpty() )
    {
        str << " ";
        if( addK )
            str << "k";
        str << suffix;
    }

    return str;
}
//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(FilterPedalAudioProcessor& p) :
audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->addListener(this);
    }

    //48000 / 2048 = 23hz

    updateChain();

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{

    if( parametersChanged.compareAndSetBool(false, true) )
    {
        //update the monochain
        updateChain();
        // signal a repaint
//        repaint();
    }

    repaint();
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
    updateDistortionGain(monoChain.get<ChainPositions::WaveshapingDistortion>(), chainSettings);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (Colours::lightslategrey);

    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysisArea();

    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
    auto& distortion = monoChain.get<ChainPositions::WaveshapingDistortion>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for( int i = 0; i < w; ++i )
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if ( !monoChain.isBypassed<ChainPositions::LowCut>() )
        {
            if (! lowcut.isBypassed<0>() )
                mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowcut.isBypassed<1>() )
                mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowcut.isBypassed<2>() )
                mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowcut.isBypassed<3>() )
                mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if ( !monoChain.isBypassed<ChainPositions::HighCut>() )
        {
            if (! highcut.isBypassed<0>() )
                mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highcut.isBypassed<1>() )
                mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highcut.isBypassed<2>() )
                mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highcut.isBypassed<3>() )
                mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }
    
    auto distortionPreGain {0.f};
    if ( !monoChain.isBypassed<ChainPositions::WaveshapingDistortion>() )
    {
        distortionPreGain = distortion.get<0>().getPreGain();
    }
    
    auto distortionPostGain {0.f};
    if ( !monoChain.isBypassed<ChainPositions::WaveshapingDistortion>() )
    {
        distortionPostGain = distortion.get<0>().getPostGain();
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -48.0, 48.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for( size_t i = 1; i < mags.size(); ++i )
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]) - distortionPreGain);
    }

    auto redValue = 0u + distortionPreGain * 4.3f + distortionPostGain * 1.f;
    auto greenValue = 255u - distortionPreGain * 3.9f - distortionPostGain * 0.7f;
    redValue = redValue < 0 ? 0u : redValue;
    greenValue = greenValue > 255 ? 255u : greenValue;
    auto responseCurveColour = Colour(redValue,
                                      greenValue,
                                      255u - distortionPreGain * 5.3f);
    g.setColour(responseCurveColour);
    g.strokePath(responseCurve, PathStrokeType(2.f));
    
    g.setColour(Colours::darkgrey);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 3.f, 3.f);
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

    Graphics g(background);

    Array<float> freqs
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    Array<float> xs;
    for( auto f : freqs )
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }

    g.setColour(Colour(60u, 60u, 65u));
    for( auto x : xs )
    {
        g.drawVerticalLine(x, top, bottom);
    }

    Array<float> gain
    {
        -48, -24, 0, 24, 48
    };

    for( auto gDb : gain)
    {
        auto y = jmap(gDb, -48.f, 48.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? orange : Colour(60u, 60u, 65u));
        g.drawHorizontalLine(y, left, right);
    }

    g.setColour(Colours::whitesmoke);
    const int fontHeight = 11;
    g.setFont(fontHeight);

    for( int i = 0; i < freqs.size(); ++i )
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if( addK )
            str << "k";
//        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    for( auto gDb : gain)
    {
        auto y = jmap(gDb, -48.f, 48.f, float(bottom), float(top));

        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? blue : Colours::lightgrey );

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(1);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//==============================================================================
FilterPedalAudioProcessorEditor::FilterPedalAudioProcessorEditor (FilterPedalAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),

lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
distortionPreGainSlider(*audioProcessor.apvts.getParameter("Distortion Amount"), "dB"),
distortionPostGainSlider(*audioProcessor.apvts.getParameter("Distortion PostGain"), "dB"),
delayDrySlider(*audioProcessor.apvts.getParameter("Delay Dry"), "%"),
delayWetSlider(*audioProcessor.apvts.getParameter("Delay Wet"), "%"),
delayFeedbackSlider(*audioProcessor.apvts.getParameter("Delay Feedback"), ""),
delayTimeLeftSlider(*audioProcessor.apvts.getParameter("Delay Time Left"), "s"),
delayTimeRightSlider(*audioProcessor.apvts.getParameter("Delay Time Right"), "s"),
delayLowCutSlider(*audioProcessor.apvts.getParameter("Delay LowCut"), "Hz"),
delayHighCutSlider(*audioProcessor.apvts.getParameter("Delay HighCut"), "Hz"),
delayDistortionPreGainSlider(*audioProcessor.apvts.getParameter("Delay Distortion"), ""),
delayDistortionPostGainSlider(*audioProcessor.apvts.getParameter("Delay PostGain"), ""),

responseCurveComponent(audioProcessor),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
distortionPreGainSliderAttachment(audioProcessor.apvts, "Distortion Amount", distortionPreGainSlider),
distortionPostGainSliderAttachment(audioProcessor.apvts, "Distortion PostGain", distortionPostGainSlider),
delayDrySliderAttachment(audioProcessor.apvts, "Delay Dry", delayDrySlider),
delayWetSliderAttachment(audioProcessor.apvts, "Delay Wet", delayWetSlider),
delayFeedbackSliderAttachment(audioProcessor.apvts, "Delay Feedback", delayFeedbackSlider),
delayTimeLeftSliderAttachment(audioProcessor.apvts, "Delay Time Left", delayTimeLeftSlider),
delayTimeRightSliderAttachment(audioProcessor.apvts, "Delay Time Right", delayTimeRightSlider),
delayLowCutSliderAttachment(audioProcessor.apvts, "Delay LowCut", delayLowCutSlider),
delayHighCutSliderAttachment(audioProcessor.apvts, "Delay HighCut", delayHighCutSlider),
delayDistortionPreGainSliderAttachment(audioProcessor.apvts, "Delay Distortion", delayDistortionPreGainSlider),
delayDistortionPostGainSliderAttachment(audioProcessor.apvts, "Delay PostGain", delayDistortionPostGainSlider),

lowcutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowcutBypassButton),
highcutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highcutBypassButton),
distortionBypassButtonAttachment(audioProcessor.apvts, "Distortion Bypassed", distortionBypassButton),
delayBypassButtonAttachment(audioProcessor.apvts, "Delay Bypassed", delayBypassButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    lowCutFreqSlider.labels.add({0.f, "20Hz"});
    lowCutFreqSlider.labels.add({1.f, "20kHz"});
    lowCutFreqSlider.nameLabels.add({0.f, "Freq"});
    
    highCutFreqSlider.labels.add({0.f, "20Hz"});
    highCutFreqSlider.labels.add({1.f, "20kHz"});
    highCutFreqSlider.nameLabels.add({0.f, "Freq"});
    
    lowCutSlopeSlider.labels.add({0.f, "12"});
    lowCutSlopeSlider.labels.add({1.f, "48"});
    lowCutSlopeSlider.nameLabels.add({0.f, "Slope"});
    
    highCutSlopeSlider.labels.add({0.f, "12"});
    highCutSlopeSlider.labels.add({1.f, "48"});
    highCutSlopeSlider.nameLabels.add({0.f, "Slope"});
    
    distortionPreGainSlider.labels.add({0.f, "0dB"});
    distortionPreGainSlider.labels.add({1.f, "48dB"});
    distortionPreGainSlider.nameLabels.add({0.f, "Amount"});
    
    distortionPostGainSlider.labels.add({0.f, "-48dB"});
    distortionPostGainSlider.labels.add({1.f, "48dB"});
    distortionPostGainSlider.nameLabels.add({0.f, "Post Gain"});
    
    delayDrySlider.labels.add({0.f, "0%"});
    delayDrySlider.labels.add({1.f, "100%"});
    delayDrySlider.nameLabels.add({0.f, "Dry"});

    delayWetSlider.labels.add({0.f, "0%"});
    delayWetSlider.labels.add({1.f, "100%"});
    delayWetSlider.nameLabels.add({0.f, "Wet"});

    delayFeedbackSlider.labels.add({0.f, "0"});
    delayFeedbackSlider.labels.add({1.f, "1"});
    delayFeedbackSlider.nameLabels.add({0.f, "Feedback"});
    
    delayTimeLeftSlider.labels.add({0.f, "0s"});
    delayTimeLeftSlider.labels.add({1.f, "3s"});
    delayTimeLeftSlider.nameLabels.add({0.f, "Time Left"});
    
    delayTimeRightSlider.labels.add({0.f, "0s"});
    delayTimeRightSlider.labels.add({1.f, "3s"});
    delayTimeRightSlider.nameLabels.add({0.f, "Time Right"});
    
    delayLowCutSlider.labels.add({0.f, "200Hz"});
    delayLowCutSlider.labels.add({1.f, "5kHz"});
    delayLowCutSlider.nameLabels.add({0.f, "LowCut"});
    
    delayHighCutSlider.labels.add({0.f, "3kHz"});
    delayHighCutSlider.labels.add({1.f, "10kHz"});
    delayHighCutSlider.nameLabels.add({0.f, "HighCut"});
    
    delayDistortionPreGainSlider.labels.add({0.f, "0dB"});
    delayDistortionPreGainSlider.labels.add({1.f, "48dB"});
    delayDistortionPreGainSlider.nameLabels.add({0.f, "Distortion"});
    
    delayDistortionPostGainSlider.labels.add({0.f, "-48dB"});
    delayDistortionPostGainSlider.labels.add({1.f, "48dB"});
    delayDistortionPostGainSlider.nameLabels.add({0.f, "Post Gain"});
    
    for( auto* comp: getComps() )
    {
        addAndMakeVisible(comp);
    }
    
    lowcutBypassButton.setLookAndFeel(&lnf);
    highcutBypassButton.setLookAndFeel(&lnf);
    distortionBypassButton.setLookAndFeel(&lnf);
    delayBypassButton.setLookAndFeel(&lnf);
    
    auto safePtr = juce::Component::SafePointer<FilterPedalAudioProcessorEditor>(this);
    lowcutBypassButton.onClick = [safePtr]()
    {
        if( auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->lowcutBypassButton.getToggleState();
            
            comp->lowCutFreqSlider.setEnabled( !bypassed );
            comp->lowCutSlopeSlider.setEnabled( !bypassed );
        }
    };
    
    highcutBypassButton.onClick = [safePtr]()
    {
        if( auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->highcutBypassButton.getToggleState();
            
            comp->highCutFreqSlider.setEnabled( !bypassed );
            comp->highCutSlopeSlider.setEnabled( !bypassed );
        }
    };
    
    distortionBypassButton.onClick = [safePtr]()
    {
        if( auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->distortionBypassButton.getToggleState();
            
            comp->distortionPreGainSlider.setEnabled( !bypassed );
            comp->distortionPostGainSlider.setEnabled( !bypassed );
        }
    };
    
    delayBypassButton.onClick = [safePtr]()
    {
        if( auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->delayBypassButton.getToggleState();

            comp->delayDrySlider.setEnabled( !bypassed );
            comp->delayWetSlider.setEnabled( !bypassed );
            comp->delayFeedbackSlider.setEnabled( !bypassed );
            comp->delayTimeLeftSlider.setEnabled( !bypassed );
            comp->delayTimeRightSlider.setEnabled( !bypassed );
            comp->delayLowCutSlider.setEnabled( !bypassed );
            comp->delayHighCutSlider.setEnabled( !bypassed );
            comp->delayDistortionPreGainSlider.setEnabled( !bypassed );
            comp->delayDistortionPostGainSlider.setEnabled( !bypassed );
        }
    };
    
    setSize (700, 500);
}

FilterPedalAudioProcessorEditor::~FilterPedalAudioProcessorEditor()
{
    lowcutBypassButton.setLookAndFeel(nullptr);
    highcutBypassButton.setLookAndFeel(nullptr);
    distortionBypassButton.setLookAndFeel(nullptr);
    delayBypassButton.setLookAndFeel(nullptr);
}

//==============================================================================
void FilterPedalAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(40u, 40u, 43u));
    
    auto width = getWidth();
    auto height = getHeight();
    
    g.setColour(Colours::darkgrey);
//    g.setOpacity(0.5f);
    g.drawVerticalLine(width * 0.4, height * 0.365, height * 0.935);
    g.drawVerticalLine(width * 0.6, height * 0.365, height * 0.935);
    
    drawComponentLabel("Low Cut", 0.1, g);
    drawComponentLabel("High Cut", 0.3, g);
    drawComponentLabel("Distortion", 0.5, g);
    drawComponentLabel("Delay", 0.8, g);
}

void FilterPedalAudioProcessorEditor::drawComponentLabel (std::string label, float x, juce::Graphics& g)
{
    using namespace juce;
    
    auto width = getWidth();
    auto height = getHeight();
    
    Rectangle<float> r;
    r.setLeft(width * x - 50);
    r.setRight(width * x + 50);
    r.setTop(height * 0.26);
    r.setBottom(height * 0.3);
    
    g.setColour(Colours::whitesmoke);
//    g.drawRect(r);
    g.setFont(20);
    g.drawFittedText(label, r.toNearestInt(), juce::Justification::centred, 1);
}

void FilterPedalAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    
    bounds.removeFromTop(5);
    
    float hRation = 25.f / 100.f; // JUCE_LIVE_CONSTANT(33) / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRation);
    
    responseCurveComponent.setBounds(responseArea);
    
    bounds.removeFromTop(25);
    
    auto initialBoundsWidth = bounds.getWidth();
    auto filterBounds = bounds.removeFromLeft(initialBoundsWidth * 0.4);
    auto distortionBounds = bounds.removeFromLeft(initialBoundsWidth * 0.2);
    auto delayBounds = bounds;
    
    auto buttonHeight = 25;
    auto delaySliderHeight = 100;
    
    auto lowCutArea = filterBounds.removeFromLeft(filterBounds.getWidth() * 0.5);
    auto highCutArea = filterBounds;
    
    auto lowCutButtonArea = lowCutArea.removeFromTop(buttonHeight);
    auto highCutButtonArea = highCutArea.removeFromTop(buttonHeight);
    auto distortionButtonArea = distortionBounds.removeFromTop(buttonHeight);
    auto delayBypassButtonArea = delayBounds.removeFromTop(buttonHeight);
    delayBounds.removeFromTop(1);
    auto initialdelayBoundsWidth = delayBounds.getWidth();
    auto delayColumn1 = delayBounds.removeFromLeft(initialdelayBoundsWidth * 0.3333);
    auto delayColumn2 = delayBounds.removeFromLeft(initialdelayBoundsWidth * 0.3333);
    auto delayColumn3 = delayBounds;
    
    lowcutBypassButton.setBounds(lowCutButtonArea.reduced(lowCutArea.getWidth() * 0.4, 0));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highcutBypassButton.setBounds(highCutButtonArea.reduced(highCutArea.getWidth() * 0.4, 0));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    distortionBypassButton.setBounds(distortionButtonArea.reduced(distortionBounds.getWidth() * 0.4, 0));
    distortionPreGainSlider.setBounds(distortionBounds.removeFromTop(distortionBounds.getHeight() * 0.5));
    distortionPostGainSlider.setBounds(distortionBounds);
    
    delayBypassButton.setBounds(delayBypassButtonArea.reduced(delayBypassButtonArea.getWidth() * 0.45, 0));
    delayDrySlider.setBounds(delayColumn1.removeFromTop(delaySliderHeight));
    delayWetSlider.setBounds(delayColumn1.removeFromTop(delaySliderHeight));
    delayFeedbackSlider.setBounds(delayColumn1.removeFromTop(delaySliderHeight));
    
    delayLowCutSlider.setBounds(delayColumn2.removeFromTop(delaySliderHeight));
    delayTimeLeftSlider.setBounds(delayColumn2.removeFromTop(delaySliderHeight));
    delayDistortionPreGainSlider.setBounds(delayColumn2.removeFromTop(delaySliderHeight));
    
    delayHighCutSlider.setBounds(delayColumn3.removeFromTop(delaySliderHeight));
    delayTimeRightSlider.setBounds(delayColumn3.removeFromTop(delaySliderHeight));
    delayDistortionPostGainSlider.setBounds(delayColumn3.removeFromTop(delaySliderHeight));
}

std::vector<juce::Component*> FilterPedalAudioProcessorEditor::getComps()
{
    return
    {
        &lowCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutFreqSlider,
        &highCutSlopeSlider,
        &distortionPreGainSlider,
        &distortionPostGainSlider,
        &responseCurveComponent,
        &delayDrySlider,
        &delayWetSlider,
        &delayFeedbackSlider,
        &delayTimeLeftSlider,
        &delayTimeRightSlider,
        &delayLowCutSlider,
        &delayHighCutSlider,
        &delayDistortionPreGainSlider,
        &delayDistortionPostGainSlider,
        
        &lowcutBypassButton,
        &highcutBypassButton,
        &distortionBypassButton,
        &delayBypassButton
    };
}
