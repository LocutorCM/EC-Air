#pragma once
#include "PluginProcessor.h"

class AirLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
};

class ECAirAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ECAirAudioProcessorEditor (ECAirAudioProcessor&);
    ~ECAirAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, const juce::String& name, double defaultValue);

    ECAirAudioProcessor& proc;
    AirLookAndFeel laf;

    juce::Slider midKnob, highKnob, trimKnob;
    juce::ToggleButton linkButton, bypassButton;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> midA, highA, trimA;
    std::unique_ptr<BA> linkA, bypassA;

    bool syncing = false;
    float meterDb = -80.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ECAirAudioProcessorEditor)
};
