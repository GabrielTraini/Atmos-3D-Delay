/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace std;

//==============================================================================
Atmos3DDelayAudioProcessor::Atmos3DDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::create7point1point2(), true)
                     #endif
                        ), parameters(*this, nullptr, "Parameter", createParameters()),
                           lowPassFilter(dsp::IIR::Coefficients<float>::makeLowPass(48000, 20000.0f, 0.8f)),
                           highPassFilter(dsp::IIR::Coefficients<float>::makeHighPass(48000, 15000.0f, 0.8f))

#endif
{
}

Atmos3DDelayAudioProcessor::~Atmos3DDelayAudioProcessor()
{
}

//==============================================================================
const juce::String Atmos3DDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Atmos3DDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Atmos3DDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Atmos3DDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Atmos3DDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Atmos3DDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Atmos3DDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Atmos3DDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Atmos3DDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void Atmos3DDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Atmos3DDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Reset Delay Buffer information
    float maxDelayTime = parameters.getParameterRange("delayTime").end;
    delayBufferChannels = getTotalNumInputChannels();
    delayBufferSamples = (int)(maxDelayTime * (float)sampleRate) + 1;
    delayBuffer0Channels = getTotalNumInputChannels();
    delayBuffer0Samples = (int)(maxDelayTime * (float)sampleRate) + 2;

    if (delayBufferSamples < 1) { delayBufferSamples = 1; }

    delayBuffer.setSize(delayBufferChannels, delayBufferSamples);
    delayBuffer0.setSize(delayBuffer0Channels, delayBuffer0Samples);
    delayBuffer.clear();
    delayBuffer0.clear();
    delayWritePosition = 0;

    //Pre-processing for LOW, HIGH, BAND PASS FILTERS
    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    lowPassFilter.prepare(spec);
    lowPassFilter.reset();
    highPassFilter.prepare(spec);
    highPassFilter.reset();
}

void Atmos3DDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Atmos3DDelayAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
        if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::create7point1point2()) { return true; }
        else { return false; }
#endif
}
#endif

void Atmos3DDelayAudioProcessor::updateLowpassFilter()
{
   auto lowpassFreq = parameters.getParameterAsValue("lowpass");
       float currentLowCutOff = lowpassFreq.getValue();
       *lowPassFilter.state = *dsp::IIR::Coefficients<float>::makeLowPass(lastSampleRate, currentLowCutOff, 0.8f);
}
void Atmos3DDelayAudioProcessor::updateHighpassFilter()
{
      auto highpassFreq = parameters.getParameterAsValue("highpass");
    float currentHighCutOff = highpassFreq.getValue();
    *highPassFilter.state = *dsp::IIR::Coefficients<float>::makeHighPass(lastSampleRate, currentHighCutOff, 0.8f);
}

void Atmos3DDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //========= Variables ===================================//
    ScopedNoDenormals noDenormals;

    auto dTime      = parameters.getRawParameterValue("delayTime");
    auto mix        = parameters.getRawParameterValue("mix");
    auto feedback   = parameters.getRawParameterValue("feedback");
    auto balance    = parameters.getRawParameterValue("balance");
    auto choice     = parameters.getRawParameterValue("delay_option");
    auto offset     = parameters.getRawParameterValue("offset");

    currentDelayTime    = (dTime->load()) * (float)getSampleRate();
    currentMix          = (mix->load());
    currentFeedback     = feedback->load();
    currentBalance      = balance->load();
    currentChoice       = choice->load();
    currentOffset       = (offset->load()) * (float)getSampleRate();

    int localWritePosition = delayWritePosition;

    //========== Processing =================================//
    
    // Gain control of input signal
    inputGainControl(buffer);

    // Perform DSP below
    if (currentChoice == 0)
        PingPongDelay(buffer, localWritePosition);
    else if (currentChoice == 1)
       SlapBackDelay(buffer, localWritePosition);
    else if (currentChoice==2)
       MidSideDelay(buffer, localWritePosition);

    lpFilter(buffer);
    hpFilter(buffer);

    // Gain control of output signal
    outputGainControl(buffer);
    
    // This is here to avoid people getting screaming feedback when they first compile a plugin
    for (auto i = getTotalNumOutputChannels(); i < getTotalNumOutputChannels(); ++i) { buffer.clear(i, 0, buffer.getNumSamples()); }
}

void Atmos3DDelayAudioProcessor::lpFilter(AudioBuffer<float>& inBuffer)
{
    AudioBlock <float> block(inBuffer);
    updateLowpassFilter();
    lowPassFilter.process(ProcessContextReplacing<float>(block));
}

void Atmos3DDelayAudioProcessor::hpFilter(AudioBuffer<float>& inBuffer)
{
    AudioBlock <float> block(inBuffer);
    updateHighpassFilter();
    highPassFilter.process(ProcessContextReplacing<float>(block));
}

void Atmos3DDelayAudioProcessor::inputGainControl(AudioBuffer<float>& buffer)
{
    auto gain = parameters.getRawParameterValue("inGain");
    float gainValue = gain->load();
    if (gainValue == startGain)
    {
        buffer.applyGain(gainValue);
    }
    else
    {
        buffer.applyGainRamp(0, buffer.getNumSamples(), startGain, gainValue);
        startGain = gainValue;
    }
}

void Atmos3DDelayAudioProcessor::outputGainControl(AudioBuffer<float>& buffer)
{
    auto gain = parameters.getRawParameterValue("outGain");
    float gainValue = gain->load();
    if (gainValue == finalGain)
    {
        buffer.applyGain(gainValue);
    }
    else
    {
        buffer.applyGainRamp(0, buffer.getNumSamples(), finalGain, gainValue);
        finalGain = gainValue;
    }
}

//==============================================================================
bool Atmos3DDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

void Atmos3DDelayAudioProcessor::MidSideDelay(AudioBuffer<float>& buffer, int localWritePosition)
{
    float* leftchannelData = buffer.getWritePointer(0);
    float* rightchannelData = buffer.getWritePointer(1);
    float* centerchannelData = buffer.getWritePointer(2);
    float* surroundleftchannelData = buffer.getWritePointer(6);
    float* surroundrightchannelData = buffer.getWritePointer(7);
    float* rearleftchannelData = buffer.getWritePointer(4);
    float* rearrightchannelData = buffer.getWritePointer(5);
    float* topleftchannelData = buffer.getWritePointer(8);
    float* toprightchannelData = buffer.getWritePointer(9);

    float* leftdelayData = delayBuffer.getWritePointer(0);
    float* rightdelayData = delayBuffer.getWritePointer(1);
    float* centerdelayData = delayBuffer.getWritePointer(2);
    float* surroundleftdelayData = delayBuffer.getWritePointer(6);
    float* surroundrightdelayData = delayBuffer.getWritePointer(7);
    float* rearleftdelayData = delayBuffer.getWritePointer(4);
    float* rearrightdelayData = delayBuffer.getWritePointer(5);
    float* topleftdelayData = delayBuffer.getWritePointer(8);
    float* toprightdelayData = delayBuffer.getWritePointer(9);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Input samples for each channel
        const float frontsampleInput = (1.0f - currentBalance) * leftchannelData[sample];
        const float rearsampleInput = currentBalance * rightchannelData[sample];
        const float centersampleInput = (leftchannelData[sample] + rightchannelData[sample]) / 2;

        // Output samples for each channel
        float leftsampleOutput = 0.0f;
        float rightsampleOutput = 0.0f;
        float centersampleOutput = 0.0f;
        float surroundleftsampleOutput = 0.0f;
        float surroundrightsampleOutput = 0.0f;
        float rearleftsampleOutput = 0.0f;
        float rearrightsampleOutput = 0.0f;
        float topleftsampleOutput = 0.0f;
        float toprightsampleOutput = 0.0f;

        // Obtain the position to read and write from and to the buffer
        float centerReadPosition = fmodf((float)localWritePosition - currentDelayTime + (float)delayBufferSamples, delayBufferSamples);
        float rightReadPosition = fmodf((float)localWritePosition - (currentDelayTime - currentOffset) + (float)delayBufferSamples, delayBufferSamples);
        float leftReadPosition = fmodf((float)localWritePosition - (currentDelayTime + currentOffset) + (float)delayBufferSamples, delayBufferSamples);


        int centerLocalReadPosition = floorf(centerReadPosition);
        int rightLocalReadPosition = floorf(rightReadPosition);
        int leftLocalReadPosition = floorf(leftReadPosition);

        if (centerLocalReadPosition != localWritePosition)
        {
            //================================PROCESSING DELAY==========================================//
            float centerFraction = centerReadPosition - (float)centerLocalReadPosition;
            float rightFraction = rightReadPosition - (float)rightLocalReadPosition;
            float leftFraction = leftReadPosition - (float)leftLocalReadPosition;

            float delayed1L = leftdelayData[(leftLocalReadPosition + 0)];
            float delayed1R = rightdelayData[(rightLocalReadPosition + 0)];
            float delayed1C = centerdelayData[(centerLocalReadPosition + 0)];

            float delayed2L = leftdelayData[(leftLocalReadPosition + 1) % delayBufferSamples];
            float delayed2R = rightdelayData[(rightLocalReadPosition + 1) % delayBufferSamples];
            float delayed2C = centerdelayData[(centerLocalReadPosition + 1) % delayBufferSamples];

            float delayed3SL = surroundleftdelayData[(leftLocalReadPosition + 0)];
            float delayed3SR = surroundrightdelayData[(rightLocalReadPosition + 0)];
            float delayed4SL = surroundleftdelayData[(leftLocalReadPosition + 1) % delayBufferSamples];
            float delayed4SR = surroundrightdelayData[(rightLocalReadPosition + 1) % delayBufferSamples];

            float delayed5RL = rearleftdelayData[(leftLocalReadPosition + 0)];
            float delayed5RR = rearrightdelayData[(rightLocalReadPosition + 0)];
            float delayed5TL = topleftdelayData[(centerLocalReadPosition + 0)];
            float delayed5TR = toprightdelayData[(centerLocalReadPosition + 0)];

            float delayed6RL = rearleftdelayData[(leftLocalReadPosition + 1) % delayBufferSamples];
            float delayed6RR = rearrightdelayData[(rightLocalReadPosition + 1) % delayBufferSamples];
            float delayed6TL = topleftdelayData[(centerLocalReadPosition + 1) % delayBufferSamples];
            float delayed6TR = toprightdelayData[(centerLocalReadPosition + 1) % delayBufferSamples];

            leftsampleOutput = delayed1L + leftFraction * (delayed2L - delayed1L);
            rightsampleOutput = delayed1R + rightFraction * (delayed2R - delayed1R);
            centersampleOutput = delayed1C + centerFraction * (delayed2C - delayed1C);
            surroundleftsampleOutput = delayed3SL + leftFraction * (delayed4SL - delayed3SL);
            surroundrightsampleOutput = delayed3SR + rightFraction * (delayed4SR - delayed3SR);
            rearleftsampleOutput = delayed5RL + leftFraction * (delayed6RL - delayed5RL);
            rearrightsampleOutput = delayed5RR + rightFraction * (delayed6RR - delayed5RR);
            topleftsampleOutput = delayed5TL + centerFraction * (delayed6TL - delayed5TL);
            toprightsampleOutput = delayed5TL + centerFraction * (delayed6TL - delayed5TL);

            //=========================MIX AND OUTPUT FOR CURRENT SAMPLE================================//
            leftchannelData[sample] = centersampleInput * (1 - currentMix) + currentMix * (leftsampleOutput - centersampleInput);
            rightchannelData[sample] = centersampleInput * (1 - currentMix) + currentMix * (rightsampleOutput - centersampleInput);
            centerchannelData[sample] = currentMix * (centersampleOutput);
            surroundleftchannelData[sample] = centersampleInput + currentMix * (surroundleftsampleOutput - centersampleInput);
            surroundrightchannelData[sample] = centersampleInput + currentMix * (surroundrightsampleOutput - centersampleInput);
            rearleftchannelData[sample] = centersampleInput + currentMix * (rearleftsampleOutput - centersampleInput);
            rearrightchannelData[sample] = centersampleInput + currentMix * (rearrightsampleOutput - centersampleInput);
            topleftchannelData[sample] = currentMix * (topleftsampleOutput);
            toprightchannelData[sample] = currentMix * (toprightsampleOutput);

            //leftdelayData[localWritePosition]           = centersampleInput       + rightsampleOutput         * currentFeedback;
            //rightdelayData[localWritePosition]          = centersampleInput      + leftsampleOutput          * currentFeedback;
            centerdelayData[localWritePosition] = centersampleInput;
            surroundleftdelayData[localWritePosition] = (centersampleInput) + surroundleftsampleOutput * currentFeedback;
            surroundrightdelayData[localWritePosition] = (centersampleInput) + surroundrightsampleOutput * currentFeedback;
            rearleftdelayData[localWritePosition] = (centersampleInput) + rearleftsampleOutput * currentFeedback;
            rearrightdelayData[localWritePosition] = (centersampleInput) + rearrightsampleOutput * currentFeedback;
            topleftdelayData[localWritePosition] = centersampleInput + toprightsampleOutput * currentFeedback;
            toprightdelayData[localWritePosition] = centersampleInput + topleftsampleOutput * currentFeedback;
        }

        if (++localWritePosition >= delayBufferSamples) { localWritePosition -= delayBufferSamples; }

    }

    delayWritePosition = localWritePosition;

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

void Atmos3DDelayAudioProcessor::PingPongDelay(AudioBuffer<float>& buffer, int localWritePosition)
{
    float* leftchannelData = buffer.getWritePointer(0);
    float* rightchannelData = buffer.getWritePointer(1);
    float* centerchannelData = buffer.getWritePointer(2);
    float* surroundleftchannelData = buffer.getWritePointer(6);   
    float* surroundrightchannelData = buffer.getWritePointer(7);  
    float* rearleftchannelData = buffer.getWritePointer(4);       
    float* rearrightchannelData = buffer.getWritePointer(5);     
    float* topleftchannelData = buffer.getWritePointer(8);
    float* toprightchannelData = buffer.getWritePointer(9);

    float* leftdelayData = delayBuffer.getWritePointer(0);
    float* rightdelayData = delayBuffer.getWritePointer(1);
    float* centerdelayData = delayBuffer.getWritePointer(2);
    float* surroundleftdelayData = delayBuffer.getWritePointer(6);  
    float* surroundrightdelayData = delayBuffer.getWritePointer(7);   
    float* rearleftdelayData = delayBuffer.getWritePointer(4);      
    float* rearrightdelayData = delayBuffer.getWritePointer(5);     
    float* topleftdelayData = delayBuffer.getWritePointer(8); 
    float* toprightdelayData = delayBuffer.getWritePointer(9);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
         //Input samples for each channel
        const float leftsampleInput = (1.0f - currentBalance) * leftchannelData[sample];
        const float rightsampleInput = currentBalance * rightchannelData[sample]; 
        const float centersampleInput = (leftchannelData[sample] + rightchannelData[sample]) / 2;

         //Output samples for each channel
        float leftsampleOutput = 0.0f;
        float rightsampleOutput = 0.0f;
        float centersampleOutput = 0.0f;
        float surroundleftsampleOutput = 0.0f;
        float surroundrightsampleOutput = 0.0f;
        float rearleftsampleOutput = 0.0f;
        float rearrightsampleOutput = 0.0f;
        float topleftsampleOutput = 0.0f;
        float toprightsampleOutput = 0.0f;

         //Obtain the position to read and write from and to the buffer
        float frontReadPosition = fmodf((float)localWritePosition - currentDelayTime + (float)delayBufferSamples, delayBufferSamples);
        float midReadPosition  = fmodf((float)localWritePosition - (currentDelayTime - currentOffset) + (float)delayBufferSamples, delayBufferSamples);
        float rearReadPosition = fmodf((float)localWritePosition - (currentDelayTime + currentOffset) + (float)delayBufferSamples, delayBufferSamples);


        int frontLocalReadPosition = floorf(frontReadPosition);
        int midLocalReadPosition = floorf(midReadPosition);
        int rearLocalReadPosition = floorf(rearReadPosition);

        if (frontLocalReadPosition != localWritePosition)
        {
            //================================PROCESSING DELAY==========================================//
            float frontFraction = frontReadPosition - (float)frontLocalReadPosition;
            float midFraction   = midReadPosition - (float)midLocalReadPosition;
            float rearFraction = rearReadPosition - (float)rearLocalReadPosition;

            float delayed1L = leftdelayData[(frontLocalReadPosition + 0)];
            float delayed1R = rightdelayData[(frontLocalReadPosition + 0)];
            float delayed1C = centerdelayData[(frontLocalReadPosition + 0)];

            float delayed2L = leftdelayData[(frontLocalReadPosition + 1) % delayBufferSamples];
            float delayed2R = rightdelayData[(frontLocalReadPosition + 1) % delayBufferSamples];
            float delayed2C = centerdelayData[(frontLocalReadPosition + 1) % delayBufferSamples];

            float delayed3SL = surroundleftdelayData[(midLocalReadPosition + 0)];
            float delayed3SR = surroundrightdelayData[(midLocalReadPosition + 0)];
            float delayed4SL = surroundleftdelayData[(midLocalReadPosition + 1) % delayBufferSamples];
            float delayed4SR = surroundrightdelayData[(midLocalReadPosition + 1) % delayBufferSamples];

            float delayed5RL = rearleftdelayData[(rearLocalReadPosition + 0)];
            float delayed5RR = rearrightdelayData[(rearLocalReadPosition + 0)];
            float delayed5TL = topleftdelayData[(frontLocalReadPosition + 0)];
            float delayed5TR = toprightdelayData[(frontLocalReadPosition + 0)];

            float delayed6RL = rearleftdelayData[(rearLocalReadPosition + 1) % delayBufferSamples];
            float delayed6RR = rearrightdelayData[(rearLocalReadPosition + 1) % delayBufferSamples];
            float delayed6TL = topleftdelayData[(frontLocalReadPosition + 1) % delayBufferSamples];
            float delayed6TR = toprightdelayData[(frontLocalReadPosition + 1) % delayBufferSamples];

            leftsampleOutput            = delayed1L     + frontFraction * (delayed2L - delayed1L);
            rightsampleOutput           = delayed1R     + frontFraction * (delayed2R - delayed1R);
            centersampleOutput          = delayed1C     + frontFraction * (delayed2C - delayed1C);
            surroundleftsampleOutput    = delayed3SL    + midFraction   * (delayed4SL - delayed3SL);
            surroundrightsampleOutput   = delayed3SR    + midFraction   * (delayed4SR - delayed3SR);
            rearleftsampleOutput        = delayed5RL    + rearFraction * (delayed6RL - delayed5RL);
            rearrightsampleOutput       = delayed5RR    + rearFraction * (delayed6RR - delayed5RR);
            topleftsampleOutput         = delayed5TL    + frontFraction * (delayed6TL - delayed5TL);
            toprightsampleOutput        = delayed5TL    + frontFraction * (delayed6TL - delayed5TL);

            //=========================MIX AND OUTPUT FOR CURRENT SAMPLE================================//
            leftchannelData[sample]             = leftsampleInput   *   (1 - currentMix)      + currentMix   * (leftsampleOutput- leftsampleInput);
            rightchannelData[sample]            = rightsampleInput  *   (1 - currentMix)      + currentMix   * (rightsampleOutput - rightsampleInput);
            centerchannelData[sample]           =                     currentMix   * (centersampleOutput);
            surroundleftchannelData[sample]     = leftsampleInput   + currentMix   * (surroundleftsampleOutput-leftsampleInput);
            surroundrightchannelData[sample]    = rightsampleInput  + currentMix   * (surroundrightsampleOutput - rightsampleInput);
            rearleftchannelData[sample]         = leftsampleInput   + currentMix   * (rearleftsampleOutput - leftsampleInput);
            rearrightchannelData[sample]        = rightsampleInput  + currentMix   * (rearrightsampleOutput - rightsampleInput);
            topleftchannelData[sample]          =                     currentMix   * (topleftsampleOutput);    
            toprightchannelData[sample]         =                     currentMix   * (toprightsampleOutput);

            leftdelayData[localWritePosition]           = leftsampleInput       + rightsampleOutput         * currentFeedback;
            rightdelayData[localWritePosition]          = rightsampleInput      + leftsampleOutput          * currentFeedback;
            centerdelayData[localWritePosition]         = centersampleInput;
            surroundleftdelayData[localWritePosition]   = (leftsampleInput)     + surroundrightsampleOutput * currentFeedback;
            surroundrightdelayData[localWritePosition]  = (rightsampleInput)    + surroundleftsampleOutput  * currentFeedback;
            rearleftdelayData[localWritePosition]       = (leftsampleInput)     + rearrightsampleOutput     * currentFeedback;
            rearrightdelayData[localWritePosition]      = (rightsampleInput)    + rearleftsampleOutput      * currentFeedback;
            topleftdelayData[localWritePosition]        = leftsampleInput       + topleftsampleOutput       * currentFeedback;
            toprightdelayData[localWritePosition]       = rightsampleInput      + toprightsampleOutput      * currentFeedback;
        }

        if (++localWritePosition >= delayBufferSamples) { localWritePosition -= delayBufferSamples; }

    }

    delayWritePosition = localWritePosition;

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

void Atmos3DDelayAudioProcessor::SlapBackDelay(AudioBuffer<float>& buffer, int localWritePosition)
{
    for (int channel = 0; channel < 10; ++channel)
    {
        float* inputData;
        if (channel != 3)
        {
            //Duplicate original stereo channels to multi-channel
            if (channel == 1 || channel == 4 || channel == 7 || channel == 8)
            {
                // Left Channel Data
                inputData = buffer.getWritePointer(0);
            }
            else if (channel == 0 || channel == 5 || channel == 6 || channel == 9)
            {
                // Right Channel Data
                inputData = buffer.getWritePointer(1);
            }
            else 
            {
                // Center Channel Data
                inputData = buffer.getWritePointer(0);
            }

            float* channelData = buffer.getWritePointer(channel);
            float* delayData = delayBuffer.getWritePointer(channel);
            localWritePosition = delayWritePosition;

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                const float in = (1/sqrt(2))*inputData[sample];
                float out = 0.0f;

                float readPosition = fmodf((float)localWritePosition - (currentDelayTime) + (float)delayBufferSamples, delayBufferSamples);
                int localReadPosition = floorf(readPosition);

                if (localReadPosition != localWritePosition)
                {
                    float fraction = readPosition - (float)localReadPosition;

                    float delayed1 = delayData[(localReadPosition + 0)];
                    float delayed2 = delayData[(localReadPosition + 1) % delayBufferSamples];
                    out = delayed1 + fraction* (delayed2 - delayed1);

                    channelData[sample] = in*(1-currentMix) + (currentMix * (out - in));
                    delayData[localWritePosition] = in + out * currentFeedback;
                }

                if (++localWritePosition >= delayBufferSamples)
                    localWritePosition -= delayBufferSamples;
            }
        }
    }

    delayWritePosition = localWritePosition;

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* Atmos3DDelayAudioProcessor::createEditor()
{
    return new Atmos3DDelayAudioProcessorEditor (*this);
}

//==============================================================================
void Atmos3DDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Atmos3DDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType())) { parameters.replaceState(ValueTree::fromXml(*xmlState)); }
    }
}

AudioProcessorValueTreeState::ParameterLayout Atmos3DDelayAudioProcessor::createParameters()
{

    // Parameter Vector
    vector<unique_ptr<RangedAudioParameter>> parameterVector;

    // Input Gain
    parameterVector.push_back(make_unique<AudioParameterFloat>("inGain",                "Input Gain",   0.0f, 2.0f, 1.0f));

    // Delay Time and Mix
    parameterVector.push_back(make_unique<AudioParameterFloat>("delayTime",             "Delay Time",   0.0f, 4.0f, 2.0f));

    parameterVector.push_back(make_unique<AudioParameterFloat>("mix",                   "Mix",          0.0f, 1.0f, 0.5f));
    parameterVector.push_back(make_unique<AudioParameterFloat>("feedback",              "Feedback",     0.0f, 0.9f, 0.5f));
    parameterVector.push_back(make_unique<AudioParameterFloat>("balance",               "Balance",      0.0f, 1.0f, 0.5f));
    parameterVector.push_back(make_unique<AudioParameterFloat>("offset",                "Offset",      -1.0f, 1.0f, 0.0f));

    // Low Pass, high pass
    parameterVector.push_back(make_unique<AudioParameterFloat>("lowpass",               "Low Pass",     1000.0f, 20000.0f, 5000.0f));
    parameterVector.push_back(make_unique<AudioParameterFloat>("highpass",              "High Pass",    50.0f, 15000.0f, 5000.0f));

    // Output Gain
    parameterVector.push_back(make_unique<AudioParameterFloat>("outGain",               "Output Gain",  0.0f, 2.0f, 1.0f));

    // Delay Options
    // StringArray for Options
    StringArray choices; choices.insert(1, "Ping-Pong"); choices.insert(2, "Normal"); choices.insert(3, "MidSide");
    parameterVector.push_back(make_unique<AudioParameterChoice>("delay_option", "Delay Options", choices, 1));

    return { parameterVector.begin(), parameterVector.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Atmos3DDelayAudioProcessor();
}
