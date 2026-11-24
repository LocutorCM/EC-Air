#pragma once
#include <JuceHeader.h>

class ECAirAudioProcessor : public juce::AudioProcessor
{
public:
    ECAirAudioProcessor();
    ~ECAirAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EC Air"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorParameter* getBypassParameter() const override
    {
        return apvts.getParameter ("bypass");
    }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> outLevel { 0.0f };   // pico de salida para el medidor

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Procesado por canal (se ejecuta a 2x de sobremuestreo para evitar aliasing)
    struct Chan
    {
        using F = juce::dsp::IIR::Filter<float>;
        using C = juce::dsp::IIR::Coefficients<float>;
        F midHP, midLP, midHarmHP, highHP, highSatHP, highHarmHP;

        void prepare (double fs)
        {
            midHP.coefficients       = C::makeHighPass (fs, 3000.0f, 0.707f);
            midLP.coefficients       = C::makeLowPass  (fs, 9000.0f, 0.707f);
            midHarmHP.coefficients   = C::makeHighPass (fs, 9000.0f, 0.707f);
            highHP.coefficients      = C::makeHighPass (fs, 10000.0f, 0.707f);
            highSatHP.coefficients   = C::makeHighPass (fs, 6000.0f, 0.707f);
            highHarmHP.coefficients  = C::makeHighPass (fs, 12000.0f, 0.707f);
            midHP.reset(); midLP.reset(); midHarmHP.reset();
            highHP.reset(); highSatHP.reset(); highHarmHP.reset();
        }

        float process (float x, float mAmt, float hAmt)
        {
            // MID AIR: banda 3-9 kHz + armónicos generados por saturación suave
            const float mid  = midLP.processSample (midHP.processSample (x));
            const float sat  = (std::tanh (4.0f * mid + 0.2f) - std::tanh (0.2f)) * 0.25f;
            const float mHar = midHarmHP.processSample (sat);

            // HIGH AIR: shelf >10 kHz + armónicos de la zona 6 kHz+ filtrados a 12 kHz+
            const float hi   = highHP.processSample (x);
            const float sIn  = highSatHP.processSample (x);
            const float hHar = highHarmHP.processSample (std::tanh (3.0f * sIn) * (1.0f / 3.0f));

            return x + mAmt * (0.6f * mid + 4.0f * mHar)
                     + hAmt * (1.5f * hi  + 4.0f * hHar);
        }
    };

    std::array<Chan, 2> chans;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::SmoothedValue<float> midSmooth, highSmooth;
    float lastTrim = 1.0f;

    std::atomic<float>* midParam  = nullptr;
    std::atomic<float>* highParam = nullptr;
    std::atomic<float>* trimParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ECAirAudioProcessor)
};
