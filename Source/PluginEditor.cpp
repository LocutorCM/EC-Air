#include "PluginEditor.h"
using namespace juce;

static const Colour kCyan (0xff3ee6d6);

// ---------------------------------------------------------------- Look & Feel
void AirLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                       float startA, float endA, Slider& s)
{
    auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float d = jmin (area.getWidth(), area.getHeight());
    auto b = area.withSizeKeepingCentre (d, d);
    const auto c = b.getCentre();
    const float r = d * 0.5f;

    Path disc;
    disc.addEllipse (b);
    DropShadow (Colour (0x66647c88), (int) (d * 0.14f), { (int) (d * 0.05f), (int) (d * 0.06f) }).drawForPath (g, disc);
    DropShadow (Colours::white.withAlpha (0.95f), (int) (d * 0.12f), { -(int) (d * 0.04f), -(int) (d * 0.04f) }).drawForPath (g, disc);

    g.setGradientFill (ColourGradient (Colour (0xfffbfdfd), b.getX(), b.getY(),
                                       Colour (0xffdde8eb), b.getRight(), b.getBottom(), false));
    g.fillEllipse (b);

    const float ang = startA + pos * (endA - startA);
    const float ar = r - 3.0f;

    Path track;
    track.addCentredArc (c.x, c.y, ar, ar, 0.0f, startA, endA, true);
    g.setColour (Colour (0x22000000));
    g.strokePath (track, PathStrokeType (2.0f));

    if (pos > 0.002f)
    {
        Path val;
        val.addCentredArc (c.x, c.y, ar, ar, 0.0f, startA, ang, true);
        for (int i = 4; i >= 1; --i)   // brillo (glow)
        {
            g.setColour (kCyan.withAlpha (0.07f * (float) (5 - i)));
            g.strokePath (val, PathStrokeType (3.0f + (float) i * 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (kCyan);
        g.strokePath (val, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    const Point<float> dir (std::sin (ang), -std::cos (ang));
    g.setColour (Colour (0xff6f7f86));
    g.drawLine (Line<float> (c + dir * (ar * 0.80f), c + dir * (ar * 0.97f)), 2.0f);

    g.setColour (Colour (0xff5d6c73));
    g.setFont (d * (d > 100.0f ? 0.22f : 0.30f));
    g.drawText (s.getTextFromValue (s.getValue()),
                b.withSizeKeepingCentre (d * 0.7f, d * 0.3f).translated (0.0f, d > 100.0f ? -d * 0.05f : 0.0f),
                Justification::centred);
    if (d > 100.0f)
    {
        g.setFont (d * 0.075f);
        g.setColour (Colour (0xff8a999f));
        g.drawText (s.getName(), b.withSizeKeepingCentre (d * 0.7f, d * 0.12f).translated (0.0f, d * 0.17f),
                    Justification::centred);
    }
}

void AirLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& bt, bool, bool down)
{
    auto b = bt.getLocalBounds().toFloat().reduced (3.0f);
    const bool on = bt.getToggleState();
    const bool isLink = bt.getName() == "link";

    Path shape;
    if (isLink) shape.addRoundedRectangle (b, b.getHeight() * 0.5f);
    else        shape.addEllipse (b);

    DropShadow (Colour (0x55647c88), 8, { 3, 4 }).drawForPath (g, shape);
    DropShadow (Colours::white.withAlpha (0.9f), 7, { -3, -3 }).drawForPath (g, shape);
    g.setColour (down ? Colour (0xffe3ecee) : Colour (0xfff2f7f8));
    g.fillPath (shape);

    const auto c = b.getCentre();
    if (isLink)
    {
        g.setColour (on ? kCyan : Colour (0xff7d8d94));
        const float rr = b.getHeight() * 0.28f;
        g.drawEllipse (c.x - rr * 1.5f, c.y - rr, rr * 2.0f, rr * 2.0f, 1.8f);
        g.drawEllipse (c.x - rr * 0.5f, c.y - rr, rr * 2.0f, rr * 2.0f, 1.8f);
    }
    else
    {
        g.setColour (on ? Colour (0xffff7a59) : Colour (0xff7d8d94));   // naranja = bypass activo
        const float rr = b.getWidth() * 0.22f;
        Path p;
        p.addCentredArc (c.x, c.y + 1.0f, rr, rr, 0.0f, 0.65f, MathConstants<float>::twoPi - 0.65f, true);
        p.startNewSubPath (c.x, c.y - rr - 2.0f);
        p.lineTo (c.x, c.y);
        g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
}

// ---------------------------------------------------------------- Editor
ECAirAudioProcessorEditor::ECAirAudioProcessorEditor (ECAirAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (640, 480);

    setupKnob (midKnob,  "MID AIR",  50.0);
    setupKnob (highKnob, "HIGH AIR", 50.0);
    setupKnob (trimKnob, "TRIM",     0.0);

    midKnob.textFromValueFunction  = [] (double v) { return String ((int) std::round (v)); };
    highKnob.textFromValueFunction = midKnob.textFromValueFunction;
    trimKnob.textFromValueFunction = midKnob.textFromValueFunction;

    linkButton.setName ("link");
    bypassButton.setName ("bypass");
    addAndMakeVisible (linkButton);
    addAndMakeVisible (bypassButton);

    midA    = std::make_unique<SA> (proc.apvts, "midAir",  midKnob);
    highA   = std::make_unique<SA> (proc.apvts, "highAir", highKnob);
    trimA   = std::make_unique<SA> (proc.apvts, "trim",    trimKnob);
    linkA   = std::make_unique<BA> (proc.apvts, "link",    linkButton);
    bypassA = std::make_unique<BA> (proc.apvts, "bypass",  bypassButton);

    // LINK: mueve ambas perillas a la vez
    auto follow = [this] (Slider& from, Slider& to)
    {
        if (linkButton.getToggleState() && ! syncing)
        {
            syncing = true;
            to.setValue (from.getValue(), sendNotificationSync);
            syncing = false;
        }
    };
    midKnob.onValueChange  = [this, follow] { follow (midKnob, highKnob); };
    highKnob.onValueChange = [this, follow] { follow (highKnob, midKnob); };
    linkButton.onClick = [this]
    {
        if (linkButton.getToggleState())
            highKnob.setValue (midKnob.getValue(), sendNotificationSync);
    };

    startTimerHz (30);
}

ECAirAudioProcessorEditor::~ECAirAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ECAirAudioProcessorEditor::setupKnob (Slider& s, const String& name, double def)
{
    s.setName (name);
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    s.setDoubleClickReturnValue (true, def);
    addAndMakeVisible (s);
}

void ECAirAudioProcessorEditor::resized()
{
    midKnob.setBounds   (85, 245, 170, 170);
    highKnob.setBounds  (385, 245, 170, 170);
    linkButton.setBounds (292, 318, 56, 30);
    bypassButton.setBounds (40, 392, 48, 48);
    trimKnob.setBounds  (546, 388, 54, 54);
}

void ECAirAudioProcessorEditor::timerCallback()
{
    const float lv = proc.outLevel.exchange (0.0f);
    const float db = Decibels::gainToDecibels (lv, -80.0f);
    meterDb = jmax (db, meterDb - 1.5f);
    repaint (0, 34, getWidth(), 192);
}

void ECAirAudioProcessorEditor::paint (Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight();

    g.setGradientFill (ColourGradient (Colour (0xffe6f0f2), 0, 0, Colour (0xffcddfe3), 0, H, false));
    g.fillAll();

    // barra superior
    g.setColour (Colour (0xff6f8088));
    g.setFont (13.0f);
    g.drawText ("EC AIR", 14, 6, 120, 22, Justification::centredLeft);

    // pantalla oscura
    Rectangle<float> disp (6.0f, 34.0f, W - 12.0f, 192.0f);
    g.setColour (Colour (0xff06080d));
    g.fillRoundedRectangle (disp, 10.0f);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (disp.toNearestInt());
        g.setGradientFill (ColourGradient (Colour (0x7030a090), disp.getX() + 40, disp.getBottom() - 10,
                                           Colours::transparentBlack, disp.getX() + 220, disp.getBottom() - 10, true));
        g.fillRect (disp);
        g.setGradientFill (ColourGradient (Colour (0x7030d0a0), disp.getRight() - 40, disp.getBottom() - 10,
                                           Colours::transparentBlack, disp.getRight() - 220, disp.getBottom() - 10, true));
        g.fillRect (disp);
    }

    g.setColour (Colour (0xffc9d6da));
    g.setFont (22.0f);
    g.drawText ("E C   A I R", Rectangle<int> (0, 52, (int) W, 30), Justification::centred);

    // medidor en arco
    const float cx = W * 0.5f, cy = 300.0f, R = 180.0f, a0 = -0.7f, a1 = 0.7f;
    Path track;
    track.addCentredArc (cx, cy, R, R, 0.0f, a0, a1, true);
    g.setColour (Colour (0xff1c2430));
    g.strokePath (track, PathStrokeType (8.0f));

    const float norm = jlimit (0.0f, 1.0f, jmap (meterDb, -60.0f, 0.0f, 0.0f, 1.0f));
    if (norm > 0.003f)
    {
        Path val;
        val.addCentredArc (cx, cy, R, R, 0.0f, a0, a0 + norm * (a1 - a0), true);
        g.setGradientFill (ColourGradient (Colour (0xff35d0ff), cx - R, cy, Colour (0xffff3b8d), cx + R, cy, false));
        g.strokePath (val, PathStrokeType (8.0f));
    }

    g.setColour (Colour (0x66ffffff));
    for (int i = 0; i <= 40; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 40.0f;
        const float r1 = R + 10.0f, r2 = R + (i % 5 == 0 ? 17.0f : 14.0f);
        g.drawLine (cx + r1 * std::sin (a), cy - r1 * std::cos (a), cx + r2 * std::sin (a), cy - r2 * std::cos (a), 1.0f);
    }
    g.setFont (11.0f);
    g.setColour (Colour (0xff8896a0));
    for (int db : { -48, -24, -12, -6, -3, 0 })
    {
        const float a = a0 + (a1 - a0) * (float) (db + 60) / 60.0f;
        const float rr = R - 24.0f;
        g.drawText (String (db), Rectangle<float> (40, 16).withCentre ({ cx + rr * std::sin (a), cy - rr * std::cos (a) }),
                    Justification::centred);
    }

    // escalas de perillas y etiquetas
    g.setFont (11.0f);
    g.setColour (Colour (0xff7d8d94));
    for (auto* k : { &midKnob, &highKnob })
    {
        auto bb = k->getBounds();
        g.drawText ("0",   bb.getX() + 6,  bb.getBottom() - 14, 40, 14, Justification::centredLeft);
        g.drawText ("100", bb.getRight() - 46, bb.getBottom() - 14, 40, 14, Justification::centredRight);
    }
    g.drawText ("BYPASS", bypassButton.getX() - 10, bypassButton.getY() - 16, 68, 14, Justification::centred);
    g.drawText ("TRIM",   trimKnob.getX() - 7, trimKnob.getY() - 14, 68, 14, Justification::centred);
}
