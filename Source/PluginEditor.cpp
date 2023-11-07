/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace std;

//==============================================================================
Atmos3DDelayAudioProcessorEditor::Atmos3DDelayAudioProcessorEditor(Atmos3DDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    buildElements();


    setSize(1000, 500);

    startTimerHz(60);
}

Atmos3DDelayAudioProcessorEditor::~Atmos3DDelayAudioProcessorEditor()
{
}

//==============================================================================
void Atmos3DDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    Image background = ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);
    g.drawImageAt(background, 0, 0);
    
    //Draw the semi-transparent rectangle around components
    const Rectangle<float> area(10, 20, 960, 460);
    g.setColour(Colours::ghostwhite);
    g.drawRoundedRectangle(area, 5.0f, 3.0f);

    //Draw background for rectangle
    g.setColour(Colours::black);
    g.setOpacity(0.5f);
    g.fillRoundedRectangle(area, 5.0f);

    //Draw text labels for each component
    g.setColour(Colours::white);
    g.setFont(18.0f);

    // Input Gain
    g.drawText("Input Gain",    0.03, 430,    200, 50, Justification::centred, false);

    //// Delay Time and Mix and Feedback
    g.drawText("Delay Time",        130, 435,    200, 50, Justification::centred, false);
    g.drawText("Mix",               260, 435,    200, 50, Justification::centred, false);
    g.drawText("Feedback",          390, 435,    200, 50, Justification::centred, false);
    g.drawText("Balance",           150, 265,   200, 50, Justification::centred, false);
    g.drawText("Offset",            360, 265,   200, 50, Justification::centred, false);

    //// Low Pass
    g.drawText("High Cut",          720, 430,    200, 50, Justification::centred, false);
    g.drawText("LOW PASS FILTER",   720, 265, 200, 50, Justification::centred, false);

    //// High Pass
    g.drawText("Low Cut",           720, 230 , 200, 50, Justification::centred, false);
    g.drawText("HIGH PASS FILTER",  720, 65, 200, 50, Justification::centred, false);


    //// Output Gain
    g.drawText("Output Gain",       550, 430, 200, 50, Justification::centred, false);

    // Title of PlugIn
    g.setFont(35.0f);
    g.drawText("Atmos 3D-Delay",    125, 20, 1160, 75, Justification::centred, false);

    //if (delayOptions.getSelectedId() == 1)
    //{
    //    balanceSlider.setVisible(true);
    //    offsetKnob.setVisible(true);
    //}
    //else if (delayOptions.getSelectedId() == 2)
    //{
    //    balanceSlider.setVisible(false);
    //    offsetKnob.setVisible(false);
    //}
}

void Atmos3DDelayAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any subcomponents in your editor.
    // List of Options
    delayOptions.setBounds      (50, 50, 400, 50);

    // Input Gain Slider
    inputGainSlider.setBounds   (15,    160,    150, 275);

    // Delay Time Knob and Mix Knob
    delayTimeKnob.setBounds     (170, 320, 120, 120);
    mixKnob.setBounds           (430, 320, 120, 120);
    feedbackSlider.setBounds    (300, 320, 120, 120);
    balanceSlider.setBounds     (200, 150, 120, 120);
    offsetKnob.setBounds        (400, 150, 120, 120);

    // Low Pass Knob
    lowCutSlider.setBounds      (750,  100, 140, 140);
    highCutSlider.setBounds     (750, 300, 140, 140);

    // Output Gain Slider
    outputGainSlider.setBounds  (580, 160, 150, 275);

}

void Atmos3DDelayAudioProcessorEditor::timerCallback()
{

}

void Atmos3DDelayAudioProcessorEditor::buildElements()
{


    delayOptVal = make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "delay_option", delayOptions);
    delayOptions.setEditableText(false);
    delayOptions.addItem("Ping-Pong", 1);
    delayOptions.addItem("Normal", 2);
    delayOptions.setSelectedId(1);
    addAndMakeVisible(&delayOptions);

    //Building the Input Gain
    inputGainVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "inGain", inputGainSlider);
    inputGainSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    inputGainSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    inputGainSlider.setRange(0.0f, 2.0f);
    addAndMakeVisible(&inputGainSlider);

    //Building the Delay Time Knob
    delayTimeVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "delayTime", delayTimeKnob);
    delayTimeKnob.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    delayTimeKnob.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    delayTimeKnob.setTextValueSuffix(" s");
    delayTimeKnob.setRange(0.0f, 4.0f);
    addAndMakeVisible(&delayTimeKnob);

    //Building the Offset Time Knob
    offsetVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "offset", offsetKnob);
    offsetKnob.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    offsetKnob.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    offsetKnob.setTextValueSuffix(" s");
    offsetKnob.setRange(-0.5f, 0.5f);
    //addChildComponent(&offsetKnob);
    addAndMakeVisible(&offsetKnob);

    //Building the Mix Knob
    mixVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "mix", mixKnob);
    mixKnob.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    mixKnob.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    mixKnob.setRange(0.0f, 1.0f); mixKnob.setTextValueSuffix(" %");
    addAndMakeVisible(&mixKnob);

    //Building the Feedback Knob
    feedbackVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "feedback", feedbackSlider);
    feedbackSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    feedbackSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    feedbackSlider.setRange(0.0f, 1.0f);
    addAndMakeVisible(&feedbackSlider);

    //Building the Balance Knob
    balanceVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "balance", balanceSlider);
    balanceSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalDrag);
    balanceSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    balanceSlider.setRange(0.0f, 1.0f);
    //addChildComponent(&balanceSlider);
    addAndMakeVisible(&balanceSlider);

    //Building the Low Pass Slider
    lowpassVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "lowpass", highCutSlider);
    highCutSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    highCutSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    highCutSlider.setRange(1000.0f, 20000.0f); highCutSlider.setTextValueSuffix(" Hz");
    addAndMakeVisible(&highCutSlider);

    //Building the High Pass Slider
    highpassVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "highpass", lowCutSlider);
    lowCutSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    lowCutSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    lowCutSlider.setRange(50.0f, 20000.0f); lowCutSlider.setTextValueSuffix(" Hz");
    addAndMakeVisible(&lowCutSlider);

    //Building the Output Gain
    outputGainVal = make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "outGain", outputGainSlider);
    outputGainSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    outputGainSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 20);
    outputGainSlider.setRange(0.0f, 2.0f);
    addAndMakeVisible(&outputGainSlider);
}
