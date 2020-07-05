#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : audioFileDialogButton("Load audio file")
    , intervalSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , durationSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , panSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , readposSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , velocitySlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)

    , intervalRandSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , durationRandSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , panRandSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , readposRandSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , velocityRandSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (0, 2);
    }

    addAndMakeVisible(audioFileDialogButton);
    audioFileDialogButton.addListener(this);

    addAndMakeVisible(activeGrainsLabel);
    startTimerHz(30);

    ////////////////////
    // Sliders
    ////////////////////
    addAndMakeVisible(intervalSlider);
    intervalSlider.setRange(0.1, 5000);
    intervalSlider.setSkewFactorFromMidPoint(1);
    intervalSlider.onValueChange = [this] {
        puroEngine.intervalParam.centre = (float)intervalSlider.getValue();
    };
    intervalSlider.setValue(1.0);

    addAndMakeVisible(durationSlider);
    durationSlider.setRange(1, 4000);
    durationSlider.onValueChange = [this] {
        puroEngine.durationParam.centre = (float)(durationSlider.getValue() / 1000.0f * this->getSampleRate());
    };
    durationSlider.setValue(300);

    addAndMakeVisible(panSlider);
    panSlider.setRange(-1, 1);
    panSlider.onValueChange = [this] {
        puroEngine.panningParam.centre = (float)panSlider.getValue();
    };
    panSlider.setValue(0);

    addAndMakeVisible(readposSlider);
    readposSlider.setRange(0, 1);
    readposSlider.onValueChange = [this] {
        puroEngine.readposParam.centre = (float)(readposSlider.getValue() * this->getSampleRate());
    };
    readposSlider.setValue(0);

    addAndMakeVisible(velocitySlider);
    velocitySlider.setRange(0.25, 4);
    velocitySlider.onValueChange = [this] {
        puroEngine.velocityParam.centre = (float)(velocitySlider.getValue());
    };
    velocitySlider.setValue(1);

    // Randomness

    addAndMakeVisible(intervalRandSlider);
    intervalRandSlider.setRange(0, 1);
    intervalRandSlider.onValueChange = [this] {
        puroEngine.intervalParam.deviation = (float)intervalRandSlider.getValue();
    };
    intervalRandSlider.setValue(0.0);

    addAndMakeVisible(durationRandSlider);
    durationRandSlider.setRange(0, 1000);
    durationRandSlider.onValueChange = [this] {
        puroEngine.durationParam.deviation = (float)(durationRandSlider.getValue() / 1000.0f * this->getSampleRate());
    };
    durationRandSlider.setValue(0);

    addAndMakeVisible(panRandSlider);
    panRandSlider.setRange(0, 2);
    panRandSlider.onValueChange = [this] {
        puroEngine.panningParam.deviation = (float)panRandSlider.getValue();
    };
    panRandSlider.setValue(0);

    addAndMakeVisible(readposRandSlider);
    readposRandSlider.setRange(0, 10);
    readposRandSlider.onValueChange = [this] {
        puroEngine.readposParam.deviation = (float)(readposRandSlider.getValue() * this->getSampleRate());
    };
    readposRandSlider.setValue(0);

    addAndMakeVisible(velocityRandSlider);
    velocityRandSlider.setRange(0, 1);
    velocityRandSlider.setSkewFactorFromMidPoint(0.1);
    velocityRandSlider.onValueChange = [this] {
        puroEngine.velocityParam.deviation = (float)velocityRandSlider.getValue();
    };
    velocityRandSlider.setValue(0.0);

}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

void MainComponent::timerCallback()
{
    activeGrainsLabel.setText(juce::String(puroEngine.pool.size()), juce::dontSendNotification);
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected);
    juce::ignoreUnused(sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    puroEngine.processBlock<puro::DynamicBuffer<float>> (*bufferToFill.buffer);
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

}

void MainComponent::resized()
{
    audioFileDialogButton.setBounds({10, 10, 200, 30});

    intervalSlider.setBounds(10, 50, 100, 100);
    durationSlider.setBounds(110, 50, 100, 100);
    panSlider.setBounds(210, 50, 100, 100);
    readposSlider.setBounds(310, 50, 100, 100);
    velocitySlider.setBounds(410, 50, 100, 100);

    intervalRandSlider.setBounds(10, 150, 100, 100);
    durationRandSlider.setBounds(110, 150, 100, 100);
    panRandSlider.setBounds(210, 150, 100, 100);
    readposRandSlider.setBounds(310, 150, 100, 100);
    velocityRandSlider.setBounds(410, 150, 100, 100);

    activeGrainsLabel.setBounds(10, getHeight()-40, 100, 30);
}

void MainComponent::buttonClicked(juce::Button* button)
{
    if (button == &audioFileDialogButton)
    {
        juce::FileChooser fileChooser ("Please select the moose you want to load...",
                           juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                           "*.wav");
 
        if (fileChooser.browseForFileToOpen())
        {
            juce::File file (fileChooser.getResult());
            loadAudioFile (file);
        }
    }

}

void MainComponent::loadAudioFile(juce::File file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor(file));
    if (reader != 0)
    { 
        audioFileBuffer = juce::AudioSampleBuffer(reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&audioFileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
    }


    for (int ch=0; ch<audioFileBuffer.getNumChannels(); ++ch)
        puroEngine.context.source.channelPtrs[ch] = audioFileBuffer.getWritePointer(ch);

    puroEngine.context.source.numSamples = audioFileBuffer.getNumSamples();
    puroEngine.context.source.numChannels = audioFileBuffer.getNumChannels();

    readposSlider.setRange(0, audioFileBuffer.getNumSamples() / getSampleRate());
    puroEngine.readposParam.maximum = audioFileBuffer.getNumSamples();
}

double MainComponent::getSampleRate()
{
    return deviceManager.getCurrentAudioDevice()->getCurrentSampleRate();
}
