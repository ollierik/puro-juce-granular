#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : audioFileDialogButton("Load audio file")
    , intervalSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , durationSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , panningSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , readposSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , velocitySlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)

    , intervalRandSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , durationRandSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , panningRandSlider(juce::Slider::SliderStyle::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
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
        const float interval = puroEngine.intervalParam.get();
        puroEngine.timer.interval = puro::math::round(puroEngine.durationParam.centre / interval);
    };
    intervalSlider.setValue(1.0);

    addAndMakeVisible(durationSlider);
    durationSlider.setRange(1, 4000);
    durationSlider.onValueChange = [this] {
        puroEngine.durationParam.centre = (float)(durationSlider.getValue() / 1000.0f * this->getSampleRate());
    };
    durationSlider.setValue(300);

    addAndMakeVisible(panningSlider);
    panningSlider.setRange(-1, 1);
    panningSlider.onValueChange = [this] {
        puroEngine.panningParam.centre = (float)panningSlider.getValue();
    };
    panningSlider.setValue(0);

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

    addAndMakeVisible(panningRandSlider);
    panningRandSlider.setRange(0, 2);
    panningRandSlider.onValueChange = [this] {
        puroEngine.panningParam.deviation = (float)panningRandSlider.getValue();
    };
    panningRandSlider.setValue(0);

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
    
    ////////////////////
    // Labels
    ////////////////////
    addAndMakeVisible(intervalLabel);
    addAndMakeVisible(durationLabel);
    addAndMakeVisible(panningLabel);
    addAndMakeVisible(readposLabel);
    addAndMakeVisible(velocityLabel);
    addAndMakeVisible(randomLabel);

    intervalLabel.setText("interval", juce::dontSendNotification);
    durationLabel.setText("duration", juce::dontSendNotification);
    panningLabel.setText("panning", juce::dontSendNotification);
    readposLabel.setText("readpos", juce::dontSendNotification);
    velocityLabel.setText("velocity", juce::dontSendNotification);
    randomLabel.setText("random", juce::dontSendNotification);
    
    intervalLabel.setJustificationType(juce::Justification::centred);
    durationLabel.setJustificationType(juce::Justification::centred);
    panningLabel.setJustificationType(juce::Justification::centred);
    readposLabel.setJustificationType(juce::Justification::centred);
    velocityLabel.setJustificationType(juce::Justification::centred);
    randomLabel.setJustificationType(juce::Justification::centred);
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
    puroEngine.processBlock (*bufferToFill.buffer);
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
    
    intervalLabel.setBounds(100, 50, 100, 30);
    durationLabel.setBounds(200, 50, 100, 30);
    panningLabel.setBounds(300, 50, 100, 30);
    readposLabel.setBounds(400, 50, 100, 30);
    velocityLabel.setBounds(500, 50, 100, 30);

    intervalSlider.setBounds(100, 100, 100, 100);
    durationSlider.setBounds(200, 100, 100, 100);
    panningSlider.setBounds(300, 100, 100, 100);
    readposSlider.setBounds(400, 100, 100, 100);
    velocitySlider.setBounds(500, 100, 100, 100);
    
    randomLabel.setBounds(0, 230, 100, 30);

    intervalRandSlider.setBounds(100, 200, 100, 100);
    durationRandSlider.setBounds(200, 200, 100, 100);
    panningRandSlider.setBounds(300, 200, 100, 100);
    readposRandSlider.setBounds(400, 200, 100, 100);
    velocityRandSlider.setBounds(500, 200, 100, 100);

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
    
    puroEngine.sourceBuffer = puro::dynamic_buffer<MAX_NUM_SRC_CHANNELS, float> (audioFileBuffer.getNumChannels()
                                                                                 , audioFileBuffer.getNumSamples()
                                                                                 , audioFileBuffer.getArrayOfWritePointers());

    readposSlider.setRange(0, audioFileBuffer.getNumSamples() / getSampleRate());
    puroEngine.readposParam.maximum = audioFileBuffer.getNumSamples();
}

double MainComponent::getSampleRate()
{
    return deviceManager.getCurrentAudioDevice()->getCurrentSampleRate();
}
