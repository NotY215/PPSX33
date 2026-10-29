#pragma once
// WORK: Host audio device — 48 kHz stream open, buffer loop, cellAudio bridge entry.
// Implementation: audio_device.cpp (migrated from runtime/audio_save.cpp).
namespace ppsx33::audio {
void audio_device_init();
void audio_device_shutdown();
}
