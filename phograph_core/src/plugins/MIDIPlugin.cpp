/*
 * MIDIPlugin.cpp - CoreMIDI wrapper
 *
 * Provides MIDI input/output on macOS via CoreMIDI.
 * Input uses a per-port ring buffer filled by the MIDIReadProc callback.
 *
 * Derived from iospharo MIDIPlugin (source-level sharing).
 */

#include "MIDIPlugin.h"

#ifdef __APPLE__
#include <CoreMIDI/CoreMIDI.h>
#include <mach/mach_time.h>
#include <cstdlib>
#include <cstring>
#include <mutex>

// =====================================================================
// CoreMIDI state
// =====================================================================

static MIDIClientRef gClient = 0;
static bool gInitialized = false;

static constexpr int kMaxPorts = 32;
static constexpr int kInputRingSize = 4096;

struct OpenPort {
    bool active;
    bool isInput;
    int endpointIndex;
    MIDIPortRef port;
    MIDIEndpointRef endpoint;
    uint8_t ringBuf[kInputRingSize];
    int ringHead;
    int ringTail;
    std::mutex ringMutex;
};

static OpenPort gPorts[kMaxPorts];

// =====================================================================
// Helpers
// =====================================================================

static int numDestinations() { return (int)MIDIGetNumberOfDestinations(); }
static int numSources() { return (int)MIDIGetNumberOfSources(); }

static int ringAvail(OpenPort* p) {
    return (p->ringHead - p->ringTail + kInputRingSize) % kInputRingSize;
}

static void ringPut(OpenPort* p, const uint8_t* data, int count) {
    for (int i = 0; i < count; i++) {
        int next = (p->ringHead + 1) % kInputRingSize;
        if (next == p->ringTail) break;
        p->ringBuf[p->ringHead] = data[i];
        p->ringHead = next;
    }
}

static int ringGet(OpenPort* p, uint8_t* buf, int maxCount) {
    int avail = ringAvail(p);
    if (maxCount > avail) maxCount = avail;
    for (int i = 0; i < maxCount; i++) {
        buf[i] = p->ringBuf[p->ringTail];
        p->ringTail = (p->ringTail + 1) % kInputRingSize;
    }
    return maxCount;
}

static void midiReadProc(const MIDIPacketList* pktList, void* readProcRefCon, void* /*srcConnRefCon*/) {
    OpenPort* p = (OpenPort*)readProcRefCon;
    if (!p || !p->active) return;

    std::lock_guard<std::mutex> lock(p->ringMutex);
    const MIDIPacket* pkt = &pktList->packet[0];
    for (UInt32 i = 0; i < pktList->numPackets; i++) {
        ringPut(p, pkt->data, (int)pkt->length);
        pkt = MIDIPacketNext(pkt);
    }
}

// =====================================================================
// Mach time
// =====================================================================

static mach_timebase_info_data_t gTimebase = {0, 0};

static int64_t machTimeToMicros(uint64_t machTime) {
    if (gTimebase.denom == 0) mach_timebase_info(&gTimebase);
    return (int64_t)(machTime * gTimebase.numer / gTimebase.denom / 1000);
}

// =====================================================================
// Public API
// =====================================================================

bool midiInit(void) {
    if (gInitialized) return true;
    OSStatus status = MIDIClientCreate(CFSTR("Phograph"), nullptr, nullptr, &gClient);
    if (status != noErr) return false;
    memset(gPorts, 0, sizeof(gPorts));
    gInitialized = true;
    return true;
}

int midiGetPortCount(void) {
    if (!midiInit()) return 0;
    return numDestinations() + numSources();
}

char* midiGetPortName(int portIndex) {
    if (!midiInit()) return nullptr;
    int nDest = numDestinations();
    MIDIEndpointRef ep;
    if (portIndex < nDest) {
        ep = MIDIGetDestination(portIndex);
    } else {
        int srcIdx = portIndex - nDest;
        if (srcIdx >= numSources()) return nullptr;
        ep = MIDIGetSource(srcIdx);
    }
    CFStringRef name = nullptr;
    MIDIObjectGetStringProperty(ep, kMIDIPropertyName, &name);
    if (!name) return strdup("(unknown)");
    char buf[256];
    if (!CFStringGetCString(name, buf, sizeof(buf), kCFStringEncodingUTF8)) {
        CFRelease(name);
        return strdup("(unknown)");
    }
    CFRelease(name);
    return strdup(buf);
}

int midiOpenPort(int portIndex) {
    if (!midiInit()) return -1;
    int slot = -1;
    for (int i = 0; i < kMaxPorts; i++) {
        if (!gPorts[i].active) { slot = i; break; }
    }
    if (slot < 0) return -1;

    int nDest = numDestinations();
    OpenPort* p = &gPorts[slot];
    memset(p, 0, sizeof(OpenPort));

    if (portIndex < nDest) {
        p->isInput = false;
        p->endpoint = MIDIGetDestination(portIndex);
        p->endpointIndex = portIndex;
        CFStringRef name = CFStringCreateWithFormat(nullptr, nullptr, CFSTR("PhoOut%d"), slot);
        OSStatus status = MIDIOutputPortCreate(gClient, name, &p->port);
        CFRelease(name);
        if (status != noErr) return -1;
    } else {
        int srcIdx = portIndex - nDest;
        if (srcIdx >= numSources()) return -1;
        p->isInput = true;
        p->endpoint = MIDIGetSource(srcIdx);
        p->endpointIndex = srcIdx;
        CFStringRef name = CFStringCreateWithFormat(nullptr, nullptr, CFSTR("PhoIn%d"), slot);
        OSStatus status = MIDIInputPortCreate(gClient, name, midiReadProc, p, &p->port);
        CFRelease(name);
        if (status != noErr) return -1;
        MIDIPortConnectSource(p->port, p->endpoint, nullptr);
    }

    p->active = true;
    return slot;
}

void midiClosePort(int handle) {
    if (handle < 0 || handle >= kMaxPorts) return;
    OpenPort* p = &gPorts[handle];
    if (!p->active) return;
    if (p->isInput) MIDIPortDisconnectSource(p->port, p->endpoint);
    MIDIPortDispose(p->port);
    p->active = false;
}

int midiRead(int handle, uint8_t* buf, int bufSize) {
    if (handle < 0 || handle >= kMaxPorts) return 0;
    OpenPort* p = &gPorts[handle];
    if (!p->active || !p->isInput) return 0;
    std::lock_guard<std::mutex> lock(p->ringMutex);
    return ringGet(p, buf, bufSize);
}

int midiWrite(int handle, const uint8_t* data, int count) {
    if (handle < 0 || handle >= kMaxPorts) return -1;
    OpenPort* p = &gPorts[handle];
    if (!p->active || p->isInput) return -1;
    uint8_t pktBuf[512];
    MIDIPacketList* pktList = (MIDIPacketList*)pktBuf;
    MIDIPacket* pkt = MIDIPacketListInit(pktList);
    pkt = MIDIPacketListAdd(pktList, sizeof(pktBuf), pkt, 0, count, data);
    if (!pkt) return -1;
    OSStatus status = MIDISend(p->port, p->endpoint, pktList);
    return (status == noErr) ? count : -1;
}

int64_t midiGetClock(void) {
    return machTimeToMicros(mach_absolute_time());
}

bool midiSendShort(int handle, uint8_t status, uint8_t data1, uint8_t data2) {
    uint8_t msg[3] = {status, data1, data2};
    return midiWrite(handle, msg, 3) == 3;
}

bool midiSendShort2(int handle, uint8_t status, uint8_t data1) {
    uint8_t msg[2] = {status, data1};
    return midiWrite(handle, msg, 2) == 2;
}

bool midiSendSysEx(int handle, const uint8_t* data, int count) {
    if (handle < 0 || handle >= kMaxPorts) return false;
    OpenPort* p = &gPorts[handle];
    if (!p->active || p->isInput) return false;
    MIDISysexSendRequest req;
    memset(&req, 0, sizeof(req));
    req.destination = p->endpoint;
    req.data = data;
    req.bytesToSend = (UInt32)count;
    OSStatus status = MIDISendSysex(&req);
    return status == noErr;
}

#else
// Non-Apple stubs
bool midiInit(void) { return false; }
int midiGetPortCount(void) { return 0; }
char* midiGetPortName(int) { return nullptr; }
int midiOpenPort(int) { return -1; }
void midiClosePort(int) {}
int midiRead(int, uint8_t*, int) { return 0; }
int midiWrite(int, const uint8_t*, int) { return -1; }
int64_t midiGetClock(void) { return 0; }
bool midiSendShort(int, uint8_t, uint8_t, uint8_t) { return false; }
bool midiSendShort2(int, uint8_t, uint8_t) { return false; }
bool midiSendSysEx(int, const uint8_t*, int) { return false; }
#endif
