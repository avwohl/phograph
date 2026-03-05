/*
 * MIDIPlugin.h - CoreMIDI wrapper
 *
 * Convention: ports 0..N-1 are output (destinations),
 *             N..N+M-1 are input (sources).
 *
 * Derived from iospharo MIDIPlugin (source-level sharing).
 */

#ifndef PHO_MIDI_PLUGIN_H
#define PHO_MIDI_PLUGIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool midiInit(void);
int midiGetPortCount(void);
char* midiGetPortName(int portIndex);
int midiOpenPort(int portIndex);
void midiClosePort(int handle);
int midiRead(int handle, uint8_t* buf, int bufSize);
int midiWrite(int handle, const uint8_t* data, int count);
int64_t midiGetClock(void);
bool midiSendShort(int handle, uint8_t status, uint8_t data1, uint8_t data2);
bool midiSendShort2(int handle, uint8_t status, uint8_t data1);
bool midiSendSysEx(int handle, const uint8_t* data, int count);

#ifdef __cplusplus
}
#endif

#endif
