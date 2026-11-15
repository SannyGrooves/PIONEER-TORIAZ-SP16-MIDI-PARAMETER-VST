#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <map>
#include <vector>

namespace ids
{
    inline constexpr const char* channel   = "channel";
    inline constexpr const char* chord     = "chord";
    inline constexpr const char* transpose = "transpose";
    inline constexpr const char* bend      = "bend";
    inline constexpr const char* arpOn     = "arpOn";
    inline constexpr const char* arpMode   = "arpMode";
    inline constexpr const char* arpRate   = "arpRate";
    inline constexpr const char* arpOct    = "arpOct";
    inline constexpr const char* arpGate   = "arpGate";

    static const juce::StringArray chordNames { "Off", "Major", "Minor", "Dim", "Aug", "Sus2",
                                                "Sus4", "Maj7", "Min7", "Dom7", "Power" };
    static const juce::StringArray arpModeNames { "Up", "Down", "Up/Down", "Random", "As Played" };
    static const juce::StringArray arpRateNames { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
}

class SP16Processor : public juce::AudioProcessor
{
public:
    SP16Processor();
    ~SP16Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Called from the editor's PANIC button (message thread). The audio thread releases
        every sounding note and sends All Notes Off on all 16 channels. */
    void requestPanic() { panicRequested.store (true); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    struct Active
    {
        int channel = 1;
        std::vector<int> notes;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float getParam (const char* id) const;
    void handleInput (const juce::MidiMessage& m, int pos, juce::MidiBuffer& out);
    void tickArp (int pos, juce::MidiBuffer& out);
    void releaseDirectFor (int inputNote, juce::MidiBuffer& out, int pos);
    void releaseArpNote (juce::MidiBuffer& out, int pos);
    void releaseAll (juce::MidiBuffer& out, int pos);
    int buildChord (int note, int* dest) const;
    void buildSequence();

    juce::MidiBuffer outBuffer;
    std::vector<int> held;
    std::map<int, Active> directActive;
    std::vector<int> seq;

    juce::uint8 lastVelocity = 100;
    int arpNote = -1;
    int arpChannel = 1;
    int gateLeft = 0;
    double samplesToNext = 0.0;
    double stepLen = 1.0;
    juce::int64 stepIndex = 0;
    juce::Random rng;

    int lastChannel = 0;
    int lastBend = -1;
    bool lastArpOn = false;
    bool wasPlaying = false;
    std::atomic<bool> flushRequested { false };
    std::atomic<bool> panicRequested { false };

    // Parameter snapshot, refreshed once per block.
    int outChannel = 1;
    int chordType = 0;
    int transposeSemis = 0;
    int arpModeIdx = 0;
    int arpOctaves = 1;
    bool arpEnabled = false;
    float arpGateAmount = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SP16Processor)
};
