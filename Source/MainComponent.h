#pragma once

#include <JuceHeader.h>
#include "PuroEngine.h"

class MainComponent
    : public juce::AudioAppComponent
    , public juce::Button::Listener
    , private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void timerCallback() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

    void loadAudioFile(juce::File file);

    double getSampleRate();


private:

    juce::TextButton audioFileDialogButton;
    juce::AudioSampleBuffer audioFileBuffer;

    juce::Slider intervalSlider;
    juce::Slider durationSlider;
    juce::Slider panSlider;
    juce::Slider readposSlider;
    juce::Slider velocitySlider;

    juce::Slider intervalRandSlider;
    juce::Slider durationRandSlider;
    juce::Slider panRandSlider;
    juce::Slider readposRandSlider;
    juce::Slider velocityRandSlider;

    juce::Label activeGrainsLabel;

    PuroEngine puroEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
