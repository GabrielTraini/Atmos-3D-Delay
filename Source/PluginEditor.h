/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

using namespace juce;
using namespace std;

//==============================================================================
/**
*/
class Atmos3DDelayAudioProcessorEditor : public juce::AudioProcessorEditor, public Timer
{
public:
    Atmos3DDelayAudioProcessorEditor(Atmos3DDelayAudioProcessor&);
    ~Atmos3DDelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void buildElements();

    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> inputGainVal;        // Attachment for Input Gain

    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> delayTimeVal;        // Attachment for Delay Time
    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mixVal;              // Attachment for Delay Mix Value
    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> feedbackVal;         // Attachment for Feedback Value
    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> balanceVal;         // Attachment for Balance Value
    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> offsetVal;

    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> lowpassVal;          // Attachment for Low Pass Value
    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> highpassVal;          // Attachment for Low Pass Value

    unique_ptr<AudioProcessorValueTreeState::SliderAttachment> outputGainVal;       // Attachment for Output Gain
    unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> delayOptVal;          // Attachment for Delay Option Value

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Atmos3DDelayAudioProcessor& audioProcessor;

    Slider      inputGainSlider;        // Slider for Input Gain

    Slider      delayTimeKnob;          // Knob for Delay Time
    Slider      mixKnob;                // Knob for Delay Mix
    Slider      feedbackSlider;         // Slider for Feedback
    Slider      balanceSlider;         // Slider for Balance
    Slider      offsetKnob;             // Slider for Offset

    Slider      lowCutSlider;           // Slider for Low Cut
    Slider      highCutSlider;          // Slider for High Cut

    Slider      outputGainSlider;       // Slider for Output Gain

    ComboBox    delayOptions;       // Options of Delay

    Label       balanceText;
    Label       offsetText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Atmos3DDelayAudioProcessorEditor)
};
