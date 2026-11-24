#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout ECAirAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "midAir", 1 }, "Mid Air",
                 NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "highAir", 1 }, "High Air",
                 NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "trim", 1 }, "Trim",
                 NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "link", 1 }, "Link", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));

    return { std::make_move_iterator (p.begin()), std::make_move_iterator (p.end()) };
}

ECAirAudioProcessor::ECAirAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    midParam  = apvts.getRawParameterValue ("midAir");
    highParam = apvts.getRawParameterValue ("highAir");
    trimParam = apvts.getRawParameterValue ("trim");
}

bool ECAirAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void ECAirAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double fs2 = sampleRate * 2.0;

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) samplesPerBlock);
    oversampling->reset();
    setLatencySamples ((int) std::ceil (oversampling->getLatencyInSamples()));

    for (auto& c : chans) c.prepare (fs2);

    auto amt = [] (float v) { return std::pow (v / 100.0f, 1.5f); };
    midSmooth.reset (fs2, 0.03);   midSmooth.setCurrentAndTargetValue (amt (midParam->load()));
    highSmooth.reset (fs2, 0.03);  highSmooth.setCurrentAndTargetValue (amt (highParam->load()));
    lastTrim = juce::Decibels::decibelsToGain (trimParam->load());
}

void ECAirAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    auto amt = [] (float v) { return std::pow (v / 100.0f, 1.5f); };
    midSmooth.setTargetValue  (amt (midParam->load()));
    highSmooth.setTargetValue (amt (highParam->load()));

    juce::dsp::AudioBlock<float> block (buffer);
    auto up = oversampling->processSamplesUp (block);
    const int nUp = (int) up.getNumSamples();
    const int nCh = juce::jmin ((int) up.getNumChannels(), 2);

    for (int i = 0; i < nUp; ++i)
    {
        const float m = midSmooth.getNextValue();
        const float h = highSmooth.getNextValue();
        for (int ch = 0; ch < nCh; ++ch)
        {
            auto* d = up.getChannelPointer ((size_t) ch);
            d[i] = chans[(size_t) ch].process (d[i], m, h);
        }
    }

    oversampling->processSamplesDown (block);

    const float trim = juce::Decibels::decibelsToGain (trimParam->load());
    buffer.applyGainRamp (0, n, lastTrim, trim);
    lastTrim = trim;

    const float peak = buffer.getMagnitude (0, n);
    outLevel.store (juce::jmax (outLevel.load(), peak));
}

juce::AudioProcessorEditor* ECAirAudioProcessor::createEditor()
{
    return new ECAirAudioProcessorEditor (*this);
}

void ECAirAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void ECAirAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ECAirAudioProcessor();
}
