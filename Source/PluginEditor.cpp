/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    
    g.setColour(enabled ? Colour(97u, 18u, 167u) : Colours::darkgrey);
    g.fillEllipse(bounds);
    
    g.setColour(enabled ? Colour(255u, 154u, 1u) : Colours::grey);
    g.drawEllipse(bounds, 1.f);
    
    if( auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        
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
        
        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);
        
        g.setColour(enabled ? Colours::white : Colours::lightgrey);
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
                                  degreesToRadians(30),
                                  degreesToRadians(360.f - ang),
                                  true);
        
        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());
        
        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
        
        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
        
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
    
    g.setColour(Colour(0u, 172u, 1u));
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
    
    size -= getTextHeight() * 3;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(r.getHeight() * 0.2);
    
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
//    monoChain.setBypassed<ChainPositions::WaveshapingDistortion>(chainSettings.distortionBypassed);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
//    updateDistortionGain(monoChain.get<ChainPositions::WaveshapingDistortion>(), chainSettings);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysisArea();

    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
//    auto& peak = monoChain.get<ChainPositions::Peak>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for( int i = 0; i < w; ++i )
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

//        if (! monoChain.isBypassed<ChainPositions::Peak>() )
//            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

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

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for( size_t i = 1; i < mags.size(); ++i )
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
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

    g.setColour(Colours::dimgrey);
    for( auto x : xs )
    {
        g.drawVerticalLine(x, top, bottom);
    }

    Array<float> gain
    {
        -24, -12, 0, 12, 24
    };

    for( auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }

    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
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
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    for( auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey );

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
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
delayDrySlider(*audioProcessor.apvts.getParameter("Delay Dry"), ""),
delayWetSlider(*audioProcessor.apvts.getParameter("Delay Wet"), ""),
delayFeedbackSlider(*audioProcessor.apvts.getParameter("Delay Feedback"), ""),
delayTimeLeftSlider(*audioProcessor.apvts.getParameter("Delay Time Left"), "s"),
delayTimeRightSlider(*audioProcessor.apvts.getParameter("Delay Time Right"), "s"),
delayLowCutSlider(*audioProcessor.apvts.getParameter("Delay LowCut"), "Hz"),

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
    
    delayWetSlider.labels.add({0.f, "0"});
    delayWetSlider.labels.add({1.f, "100"});
    delayWetSlider.nameLabels.add({0.f, "Dry"});

    delayWetSlider.labels.add({0.f, "0"});
    delayWetSlider.labels.add({1.f, "100"});
    delayWetSlider.nameLabels.add({0.f, "Wet"});

    delayFeedbackSlider.labels.add({0.f, "0"});
    delayFeedbackSlider.labels.add({1.f, "100"});
    delayFeedbackSlider.nameLabels.add({0.f, "Feedback"});
    
    delayTimeLeftSlider.labels.add({0.f, "0"});
    delayTimeLeftSlider.labels.add({1.f, "3"});
    delayTimeLeftSlider.nameLabels.add({0.f, "Time Left"});
    
    delayTimeRightSlider.labels.add({0.f, "0"});
    delayTimeRightSlider.labels.add({1.f, "3"});
    delayTimeRightSlider.nameLabels.add({0.f, "Time Right"});
    
    delayLowCutSlider.labels.add({0.f, "1000"});
    delayLowCutSlider.labels.add({1.f, "3000"});
    delayLowCutSlider.nameLabels.add({0.f, "LowCut"});
    
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
        }
    };
    
    setSize (600, 480);
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
    g.fillAll (Colours::black);
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
    
    bounds.removeFromTop(5);
    
    auto filterBounds = bounds.removeFromLeft(bounds.getWidth() * 0.3);
    auto saturationBounds = bounds.removeFromLeft(bounds.getWidth() * 0.3);
    auto delayBounds = bounds.removeFromLeft(bounds.getWidth() * 0.3);
    auto delayBounds2 = bounds.removeFromLeft(bounds.getWidth() * 0.3);
    
    auto lowCutArea = filterBounds.removeFromLeft(filterBounds.getWidth() * 0.5);
    auto highCutArea = filterBounds;
    
    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    distortionBypassButton.setBounds(saturationBounds.removeFromTop(25));
    distortionPreGainSlider.setBounds(saturationBounds.removeFromTop(saturationBounds.getHeight() * 0.5));
    distortionPostGainSlider.setBounds(saturationBounds);
    
    delayBypassButton.setBounds(delayBounds.removeFromTop(25));
    delayDrySlider.setBounds(delayBounds.removeFromTop(70));
    delayWetSlider.setBounds(delayBounds.removeFromTop(70));
    delayFeedbackSlider.setBounds(delayBounds.removeFromTop(70));
    delayTimeLeftSlider.setBounds(delayBounds2.removeFromTop(75));
    delayTimeRightSlider.setBounds(delayBounds2.removeFromTop(75));
    //    delayMixSlider.setBounds(delayBounds.removeFromTop(delayBounds.getHeight() * JUCE_LIVE_CONSTANT(0.5)));
    //    delayFeedbackSlider.setBounds(delayBounds.removeFromTop(delayBounds.getHeight() * JUCE_LIVE_CONSTANT(0.5)));
    //    delayTimeLeftSlider.setBounds(delayBounds.removeFromTop(delayBounds.getHeight() * JUCE_LIVE_CONSTANT(0.5)));
    //    delayTimeRightSlider.setBounds(delayBounds.removeFromTop(delayBounds.getHeight() * JUCE_LIVE_CONSTANT(0.5)));
    
    delayLowCutSlider.setBounds(delayBounds2.removeFromTop(75));
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
        
        &lowcutBypassButton,
        &highcutBypassButton,
        &distortionBypassButton,
        &delayBypassButton
    };
}
