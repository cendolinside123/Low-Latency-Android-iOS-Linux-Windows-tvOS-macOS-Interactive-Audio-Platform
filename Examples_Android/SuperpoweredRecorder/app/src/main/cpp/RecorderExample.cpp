#include <jni.h>
#include <string>
#include <android/log.h>
#include <OpenSource/SuperpoweredAndroidAudioIO.h>
#include <Superpowered.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredRecorder.h>
#include <unistd.h>

static SuperpoweredAndroidAudioIO *audioIO;
static Superpowered::Recorder *recorder;

// This is called periodically by the audio I/O.
static bool audioProcessing (
        void * __unused clientdata, // custom pointer
        short int *audio,           // buffer of interleaved samples
        int numberOfFrames,         // number of frames to process
        int __unused samplerate     // current sample rate in Hz
) {
    float floatBuffer[numberOfFrames * 2];
    Superpowered::ShortIntToFloat(audio, floatBuffer, (unsigned int)numberOfFrames);
    recorder->recordInterleaved(floatBuffer, (unsigned int)numberOfFrames);
    return true;
}

// StartAudio - Start audio engine.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_StartAudio (
        JNIEnv *env,
        jobject  __unused obj,
        jint samplerate,
        jint buffersize,
        jint destinationfd // file descriptor of the destination file
) {
    Superpowered::Initialize(
            "ExampleLicenseKey-WillExpire-OnNextUpdate",
            false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
            false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
            false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
            false, // enableAudioEffects (using any SuperpoweredFX class)
            false, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
            false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
            false  // enableNetworking (using Superpowered::httpRequest)
    );

    // Initialize the recorder.
    recorder = new Superpowered::Recorder(NULL);

    // Start a new recording.
    recorder->preparefd(
            destinationfd,            // destination file descriptor
            0,                        // not used
            (unsigned int)samplerate, // sample rate in Hz
            true,                     // apply fade in/fade out
            1                         // minimum length of the recording in seconds
            );


    // Initialize audio engine with audio callback function.
    audioIO = new SuperpoweredAndroidAudioIO (
            samplerate,      // native sampe rate
            buffersize,      // native buffer size
            true,            // enableInput
            false,           // enableOutput
            audioProcessing, // process callback function
            NULL             // clientData
    );
}

// StopAudio - Stop audio engine and free audio buffer.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_StopRecording(JNIEnv * __unused env, jobject __unused obj) {
    recorder->stop();
    delete audioIO;

    // Wait until the recorder finished writing everything to disk.
    // It's better to do this asynchronously, but we're just blocking (sleeping) now.
    while (!recorder->isFinished()) usleep(100000);

    __android_log_print(ANDROID_LOG_DEBUG, "Recorder", "Finished recording.");
    delete recorder;
}

// onBackground - Put audio processing to sleep if no audio is playing.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_onBackground(JNIEnv * __unused env, jobject __unused obj) {
    audioIO->onBackground();
}

// onForeground - Resume audio processing.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_onForeground(JNIEnv * __unused env, jobject __unused obj) {
    audioIO->onForeground();
}
