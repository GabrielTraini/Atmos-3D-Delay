/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;
using namespace std;
using namespace dsp;

//==============================================================================
/**
*/
class Atmos3DDelayAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    Atmos3DDelayAudioProcessor();
    ~Atmos3DDelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    //void updateFilter();
    void updateLowpassFilter();
    void updateHighpassFilter();

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void lpFilter(AudioBuffer<float>& buffer);
    void hpFilter(AudioBuffer<float>& buffer);

    // Functions of Input and Output Gain
    void inputGainControl(AudioBuffer<float>& buffer);
    void outputGainControl(AudioBuffer<float>& buffer);

    //Functions for Delay Processing
    void MidSideDelay(AudioBuffer<float>& buffer, int localWritePosition);
    void PingPongDelay(AudioBuffer<float>& buffer, int localWritePosition);
    void SlapBackDelay(AudioBuffer<float>& buffer, int localWritePosition);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    

    AudioProcessorValueTreeState    parameters;
    
private:

    //User Variables
    float                       currentDelayTime, currentMix, currentFeedback, currentBalance, currentChoice, currentOffset;

    // Variables
    float                       startGain, finalGain, lastSampleRate{48000};
    int                         delayBufferSamples, delayBufferChannels, delayWritePosition;
    int                         delayBuffer0Channels, delayBuffer0Samples;
    AudioSampleBuffer           delayBuffer;
    AudioSampleBuffer           delayBuffer0;

    ProcessorDuplicator<IIR::Filter <float>, IIR::Coefficients <float>> lowPassFilter;
    ProcessorDuplicator<IIR::Filter <float>, IIR::Coefficients <float>> highPassFilter;

    // Functions
    AudioProcessorValueTreeState::ParameterLayout createParameters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Atmos3DDelayAudioProcessor)
};
