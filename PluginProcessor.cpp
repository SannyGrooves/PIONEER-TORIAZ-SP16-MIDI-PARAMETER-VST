#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

namespace
{
    constexpr int numChordTypes = 11;

    // Same order as ids::chordNames.
    constexpr int chordSizes[numChordTypes] = { 1, 3, 3, 3, 3, 3, 3, 4, 4, 4, 3 };
    constexpr int chordIntervals[numChordTypes][4] = {
        { 0, 0, 0, 0 },   // Off
        { 0, 4, 7, 0 },   // Major
        { 0, 3, 7, 0 },   // Minor
        { 0, 3, 6, 0 },   // Dim
        { 0, 4, 8, 0 },   // Aug
        { 0, 2, 7, 0 },   // Sus2
        { 0, 5, 7, 0 },   // Sus4
        { 0, 4, 7, 11 },  // Maj7
        { 0, 3, 7, 10 },  // Min7
        { 0, 4, 7, 10 },  // Dom7
        { 0, 7, 12, 0 }   // Power
    };

    // Same order as ids::arpRateNames. Quarter note = 1.0 beat.
    constexpr double rateBeats[6] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };

    constexpr int centreBend = 8192;
}

//==============================================================================
SP16Processor::SP16Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout())
{
    held.reserve (128);
    seq.reserve (1024);
}

juce::AudioProcessorValueTreeState::ParameterLayout SP16Processor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ids::channel, 1 },
                                                           "MIDI Channel", 1, 16, 1));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ids::chord, 1 },
                                                              "Chord", ids::chordNames, 0));

    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ids::transpose, 1 },
                                                           "Pitch Shift", -24, 24, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ids::bend, 1 },
                                                             "Pitch Bend",
                                                             juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f),
                                                             0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ids::arpOn, 1 },
                                                            "Arp On", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ids::arpMode, 1 },
                                                              "Arp Mode", ids::arpModeNames, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ids::arpRate, 1 },
                                                              "Arp Rate", ids::arpRateNames, 3));

    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ids::arpOct, 1 },
                                                           "Arp Octaves", 1, 4, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ids::arpGate, 1 },
                                                             "Arp Gate",
                                                             juce::NormalisableRange<float> (0.05f, 1.0f, 0.01f),
                                                             0.5f));

    return layout;
}

float SP16Processor::getParam (const char* id) const
{
    const float v = *apvts.getRawParameterValue (id);
    return v;
}

//==============================================================================
void SP16Processor::prepareToPlay (double, int)
{
    samplesToNext = 0.0;
    stepIndex = 0;
    lastBend = -1;
    flushRequested.store (true); // release anything left over from a previous session
}

void SP16Processor::releaseResources()
{
    flushRequested.store (true);
}

void SP16Processor::reset()
{
    flushRequested.store (true);
}

bool SP16Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::stereo() || mainOut.isDisabled();
}

//==============================================================================
int SP16Processor::buildChord (int note, int* dest) const
{
    const int type = juce::jlimit (0, numChordTypes - 1, chordType);
    int count = 0;

    for (int k = 0; k < chordSizes[type]; ++k)
    {
        const int n = note + transposeSemis + chordIntervals[type][k];

        if (n >= 0 && n <= 127)
            dest[count++] = n;
    }

    return count;
}

void SP16Processor::buildSequence()
{
    seq.clear();

    int tmp[4];

    for (int o = 0; o < arpOctaves; ++o)
    {
        for (int heldNote : held)
        {
            const int count = buildChord (heldNote, tmp);

            for (int k = 0; k < count; ++k)
            {
                const int n = tmp[k] + 12 * o;

                if (n <= 127)
                    seq.push_back (n);
            }
        }
    }

    if (arpModeIdx != 4) // anything except "As Played"
    {
        std::sort (seq.begin(), seq.end());
        seq.erase (std::unique (seq.begin(), seq.end()), seq.end());
    }
}

//==============================================================================
void SP16Processor::releaseDirectFor (int inputNote, juce::MidiBuffer& out, int pos)
{
    const auto it = directActive.find (inputNote);

    if (it == directActive.end())
        return;

    for (int n : it->second.notes)
        out.addEvent (juce::MidiMessage::noteOff (it->second.channel, n), pos);

    directActive.erase (it);
}

void SP16Processor::releaseArpNote (juce::MidiBuffer& out, int pos)
{
    if (arpNote >= 0)
        out.addEvent (juce::MidiMessage::noteOff (arpChannel, arpNote), pos);

    arpNote = -1;
    gateLeft = 0;
}

void SP16Processor::releaseAll (juce::MidiBuffer& out, int pos)
{
    for (const auto& entry : directActive)
        for (int n : entry.second.notes)
            out.addEvent (juce::MidiMessage::noteOff (entry.second.channel, n), pos);

    directActive.clear();
    releaseArpNote (out, pos);
}

//==============================================================================
void SP16Processor::handleInput (const juce::MidiMessage& m, int pos, juce::MidiBuffer& out)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        lastVelocity = m.getVelocity();

        held.erase (std::remove (held.begin(), held.end(), note), held.end());
        held.push_back (note);

        if (! arpEnabled)
        {
            releaseDirectFor (note, out, pos);

            Active a;
            a.channel = outChannel;

            int tmp[4];
            const int count = buildChord (note, tmp);

            for (int k = 0; k < count; ++k)
            {
                a.notes.push_back (tmp[k]);
                out.addEvent (juce::MidiMessage::noteOn (a.channel, tmp[k], lastVelocity), pos);
            }

            if (! a.notes.empty())
                directActive[note] = std::move (a);
        }
        else if (held.size() == 1)
        {
            samplesToNext = 0.0;
            stepIndex = 0;
        }
    }
    else if (m.isNoteOff())
    {
        const int note = m.getNoteNumber();
        held.erase (std::remove (held.begin(), held.end(), note), held.end());

        if (! arpEnabled)
            releaseDirectFor (note, out, pos);
        else if (held.empty())
            releaseArpNote (out, pos);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        held.clear();
        releaseAll (out, pos);
    }
    else if (m.getChannel() > 0)
    {
        auto copy = m;
        copy.setChannel (outChannel);
        out.addEvent (copy, pos);
    }
}

void SP16Processor::tickArp (int pos, juce::MidiBuffer& out)
{
    if (arpNote >= 0)
    {
        if (--gateLeft <= 0)
            releaseArpNote (out, pos);
    }

    if (held.empty())
    {
        samplesToNext = 0.0;
        return;
    }

    samplesToNext -= 1.0;

    if (samplesToNext > 0.0)
        return;

    releaseArpNote (out, pos);
    buildSequence();

    if (seq.empty())
    {
        samplesToNext += stepLen;
        return;
    }

    const int n = (int) seq.size();
    int idx = 0;

    switch (arpModeIdx)
    {
        case 0: // Up
            idx = (int) (stepIndex % n);
            break;

        case 1: // Down
            idx = n - 1 - (int) (stepIndex % n);
            break;

        case 2: // Up/Down
        {
            if (n == 1)
            {
                idx = 0;
            }
            else
            {
                const int period = 2 * n - 2;
                const int p = (int) (stepIndex % period);
                idx = (p < n) ? p : period - p;
            }
            break;
        }

        case 3: // Random
            idx = rng.nextInt (n);
            break;

        default: // As Played
            idx = (int) (stepIndex % n);
            break;
    }

    ++stepIndex;

    arpNote = seq[(size_t) idx];
    arpChannel = outChannel;
    out.addEvent (juce::MidiMessage::noteOn (arpChannel, arpNote, lastVelocity), pos);

    gateLeft = juce::jmax (1, (int) (stepLen * (double) arpGateAmount));
    samplesToNext += stepLen;
}

//==============================================================================
void SP16Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    const int numSamples = buffer.getNumSamples();

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    juce::MidiBuffer& out = outBuffer;
    out.clear();

    // Parameter snapshot for this block.
    outChannel     = juce::jlimit (1, 16, juce::roundToInt (getParam (ids::channel)));
    chordType      = juce::jlimit (0, numChordTypes - 1, juce::roundToInt (getParam (ids::chord)));
    transposeSemis = juce::roundToInt (getParam (ids::transpose));
    arpEnabled     = getParam (ids::arpOn) > 0.5f;
    arpModeIdx     = juce::jlimit (0, 4, juce::roundToInt (getParam (ids::arpMode)));
    arpOctaves     = juce::jlimit (1, 4, juce::roundToInt (getParam (ids::arpOct)));
    arpGateAmount  = juce::jlimit (0.05f, 1.0f, getParam (ids::arpGate));
    const int rateIdx = juce::jlimit (0, 5, juce::roundToInt (getParam (ids::arpRate)));
    const float bendParam = juce::jlimit (-1.0f, 1.0f, getParam (ids::bend));

    // Requests from the message thread / host.
    if (panicRequested.exchange (false))
    {
        held.clear();
        releaseAll (out, 0);

        for (int c = 1; c <= 16; ++c)
            out.addEvent (juce::MidiMessage::allNotesOff (c), 0);

        flushRequested.store (false);
    }
    else if (flushRequested.exchange (false))
    {
        held.clear();
        releaseAll (out, 0);
    }

    // Transport and tempo.
    bool playing = false;
    double bpm = 120.0;

    if (auto* ph = getPlayHead())
    {
        if (const auto info = ph->getPosition())
        {
            playing = info->getIsPlaying();

            if (const auto hostBpm = info->getBpm())
                bpm = *hostBpm;
        }
    }

    if (bpm < 1.0)
        bpm = 120.0;

    if (wasPlaying && ! playing)
    {
        // Transport stopped: never leave notes hanging.
        held.clear();
        releaseAll (out, 0);
    }

    wasPlaying = playing;

    double sr = getSampleRate();

    if (sr <= 0.0)
        sr = 44100.0;

    stepLen = juce::jmax (1.0, 60.0 / bpm * sr * rateBeats[rateIdx]);

    // Channel change.
    if (outChannel != lastChannel)
    {
        if (lastChannel != 0)
        {
            releaseAll (out, 0);

            if (lastBend >= 0 && lastBend != centreBend)
                out.addEvent (juce::MidiMessage::pitchWheel (lastChannel, centreBend), 0);
        }

        lastBend = -1; // force re-send on the new channel
        lastChannel = outChannel;
    }

    // Arp toggle.
    if (arpEnabled != lastArpOn)
    {
        releaseAll (out, 0);
        samplesToNext = 0.0;
        stepIndex = 0;
        lastArpOn = arpEnabled;
    }

    // Pitch bend.
    const int bendValue = juce::jlimit (0, 16383, juce::roundToInt ((bendParam + 1.0f) * 0.5f * 16383.0f));

    if (bendValue != lastBend)
    {
        out.addEvent (juce::MidiMessage::pitchWheel (outChannel, bendValue), 0);
        lastBend = bendValue;
    }

    // Per-sample processing.
    auto it = midi.begin();
    const auto end = midi.end();

    for (int i = 0; i < numSamples; ++i)
    {
        while (it != end && (*it).samplePosition <= i)
        {
            handleInput ((*it).getMessage(), i, out);
            ++it;
        }

        if (arpEnabled)
            tickArp (i, out);
    }

    // Anything positioned past the end of the block is handled at the last sample.
    while (it != end)
    {
        handleInput ((*it).getMessage(), juce::jmax (0, numSamples - 1), out);
        ++it;
    }

    midi.swapWith (out);
}

//==============================================================================
juce::AudioProcessorEditor* SP16Processor::createEditor()
{
    return new SP16Editor (*this);
}

void SP16Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SP16Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SP16Processor();
}
