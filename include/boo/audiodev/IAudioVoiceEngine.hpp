#ifndef BOO_IAUDIOVOICEENGINE_HPP
#define BOO_IAUDIOVOICEENGINE_HPP

#include "IAudioVoice.hpp"
#include "IAudioSubmix.hpp"
#include "IMIDIPort.hpp"
#include <memory>
#include <vector>

namespace boo
{

/** Mixing and sample-rate-conversion system. Allocates voices and mixes them
 *  before sending the final samples to an OS-supplied audio-queue */
struct IAudioVoiceEngine
{
    virtual ~IAudioVoiceEngine() = default;

    /** Client calls this to request allocation of new mixer-voice.
     *  Returns empty unique_ptr if necessary resources aren't available.
     *  ChannelLayout automatically reduces to maximum-supported layout by HW.
     *
     *  Client must be prepared to supply audio frames via the callback when this is called;
     *  the backing audio-buffers are primed with initial data for low-latency playback start
     */
    virtual std::unique_ptr<IAudioVoice> allocateNewMonoVoice(double sampleRate,
                                                              IAudioVoiceCallback* cb,
                                                              bool dynamicPitch=false)=0;

    /** Same as allocateNewMonoVoice, but source audio is stereo-interleaved */
    virtual std::unique_ptr<IAudioVoice> allocateNewStereoVoice(double sampleRate,
                                                                IAudioVoiceCallback* cb,
                                                                bool dynamicPitch=false)=0;

    /** Client calls this to allocate a Submix for gathering audio together for effects processing */
    virtual std::unique_ptr<IAudioSubmix> allocateNewSubmix(bool mainOut, IAudioSubmixCallback* cb, int busId)=0;

    /** Client may optionally register a 200-virtual-updates each second callback for the stream */
    virtual void register5MsCallback(std::function<void(double dt)>&& callback)=0;

    /** Client may use this to determine current speaker-setup */
    virtual AudioChannelSet getAvailableSet()=0;

    /** Ensure backing platform buffer is filled as much as possible with mixed samples */
    virtual void pumpAndMixVoices()=0;

    /** Set total volume of engine */
    virtual void setVolume(float vol)=0;

    /** Get list of MIDI devices found on system */
    virtual std::vector<std::pair<std::string, std::string>> enumerateMIDIDevices() const=0;

    /** Create ad-hoc MIDI in port and register with system */
    virtual std::unique_ptr<IMIDIIn> newVirtualMIDIIn(ReceiveFunctor&& receiver)=0;

    /** Create ad-hoc MIDI out port and register with system */
    virtual std::unique_ptr<IMIDIOut> newVirtualMIDIOut()=0;

    /** Create ad-hoc MIDI in/out port and register with system */
    virtual std::unique_ptr<IMIDIInOut> newVirtualMIDIInOut(ReceiveFunctor&& receiver)=0;

    /** Open named MIDI in port, name format depends on OS */
    virtual std::unique_ptr<IMIDIIn> newRealMIDIIn(const char* name, ReceiveFunctor&& receiver)=0;

    /** Open named MIDI out port, name format depends on OS */
    virtual std::unique_ptr<IMIDIOut> newRealMIDIOut(const char* name)=0;

    /** Open named MIDI in/out port, name format depends on OS */
    virtual std::unique_ptr<IMIDIInOut> newRealMIDIInOut(const char* name, ReceiveFunctor&& receiver)=0;

    /** If this returns true, MIDI callbacks are assumed to be *not* thread-safe; need protection via mutex */
    virtual bool useMIDILock() const=0;

    /** Get canonical count of frames for each 5ms output block */
    virtual size_t get5MsFrames() const=0;
};

/** Construct host platform's voice engine */
std::unique_ptr<IAudioVoiceEngine> NewAudioVoiceEngine();

/** Construct WAV-rendering voice engine */
std::unique_ptr<IAudioVoiceEngine> NewWAVAudioVoiceEngine(const char* path, double sampleRate);
#if _WIN32
std::unique_ptr<IAudioVoiceEngine> NewWAVAudioVoiceEngine(const wchar_t* path, double sampleRate);
#endif

}

#endif // BOO_IAUDIOVOICEENGINE_HPP
