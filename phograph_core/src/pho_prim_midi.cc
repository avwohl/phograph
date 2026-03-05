#include "pho_prim.h"
#include "plugins/MIDIPlugin.h"
#include <cstdlib>
#include <cstring>

namespace pho {

void register_midi_prims() {
    auto& r = PrimitiveRegistry::instance();

    // midi-init: -> boolean
    r.register_prim("midi-init", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(Value::boolean(midiInit()));
    });

    // midi-port-count: -> integer
    r.register_prim("midi-port-count", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(Value::integer(midiGetPortCount()));
    });

    // midi-port-name: index -> string
    r.register_prim("midi-port-name", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("midi-port-name: expected integer"));
        char* name = midiGetPortName((int)in[0].as_integer());
        if (!name) return PrimResult::fail_with(Value::error("midi-port-name: invalid port index"));
        std::string result(name);
        free(name);
        return PrimResult::success(Value::string(std::move(result)));
    });

    // midi-open: port-index -> handle (integer)
    r.register_prim("midi-open", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("midi-open: expected integer"));
        int handle = midiOpenPort((int)in[0].as_integer());
        if (handle < 0) return PrimResult::fail_with(Value::error("midi-open: failed to open port"));
        return PrimResult::success(Value::integer(handle));
    });

    // midi-close: handle -> boolean
    r.register_prim("midi-close", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("midi-close: expected integer"));
        midiClosePort((int)in[0].as_integer());
        return PrimResult::success(Value::boolean(true));
    });

    // midi-read: handle max-bytes -> data
    r.register_prim("midi-read", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("midi-read: expected handle and max-bytes"));
        int maxBytes = (int)in[1].as_integer();
        if (maxBytes <= 0 || maxBytes > 65536)
            return PrimResult::fail_with(Value::error("midi-read: max-bytes must be 1..65536"));
        std::vector<uint8_t> buf(maxBytes);
        int got = midiRead((int)in[0].as_integer(), buf.data(), maxBytes);
        buf.resize(got);
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(buf))));
    });

    // midi-write: handle data -> integer (bytes written)
    r.register_prim("midi-write", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_data())
            return PrimResult::fail_with(Value::error("midi-write: expected handle and data"));
        auto* d = in[1].as_data();
        int written = midiWrite((int)in[0].as_integer(), d->bytes().data(), (int)d->length());
        return PrimResult::success(Value::integer(written));
    });

    // midi-note-on: handle channel note velocity -> boolean
    r.register_prim("midi-note-on", 4, 1, [](const std::vector<Value>& in) -> PrimResult {
        for (int i = 0; i < 4; i++)
            if (!in[i].is_integer()) return PrimResult::fail_with(Value::error("midi-note-on: expected 4 integers"));
        int handle = (int)in[0].as_integer();
        uint8_t channel = (uint8_t)(in[1].as_integer() & 0x0F);
        uint8_t note = (uint8_t)(in[2].as_integer() & 0x7F);
        uint8_t velocity = (uint8_t)(in[3].as_integer() & 0x7F);
        return PrimResult::success(Value::boolean(midiSendShort(handle, 0x90 | channel, note, velocity)));
    });

    // midi-note-off: handle channel note velocity -> boolean
    r.register_prim("midi-note-off", 4, 1, [](const std::vector<Value>& in) -> PrimResult {
        for (int i = 0; i < 4; i++)
            if (!in[i].is_integer()) return PrimResult::fail_with(Value::error("midi-note-off: expected 4 integers"));
        int handle = (int)in[0].as_integer();
        uint8_t channel = (uint8_t)(in[1].as_integer() & 0x0F);
        uint8_t note = (uint8_t)(in[2].as_integer() & 0x7F);
        uint8_t velocity = (uint8_t)(in[3].as_integer() & 0x7F);
        return PrimResult::success(Value::boolean(midiSendShort(handle, 0x80 | channel, note, velocity)));
    });

    // midi-cc: handle channel controller value -> boolean
    r.register_prim("midi-cc", 4, 1, [](const std::vector<Value>& in) -> PrimResult {
        for (int i = 0; i < 4; i++)
            if (!in[i].is_integer()) return PrimResult::fail_with(Value::error("midi-cc: expected 4 integers"));
        int handle = (int)in[0].as_integer();
        uint8_t channel = (uint8_t)(in[1].as_integer() & 0x0F);
        uint8_t cc = (uint8_t)(in[2].as_integer() & 0x7F);
        uint8_t val = (uint8_t)(in[3].as_integer() & 0x7F);
        return PrimResult::success(Value::boolean(midiSendShort(handle, 0xB0 | channel, cc, val)));
    });

    // midi-program: handle channel program -> boolean
    r.register_prim("midi-program", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("midi-program: expected 3 integers"));
        int handle = (int)in[0].as_integer();
        uint8_t channel = (uint8_t)(in[1].as_integer() & 0x0F);
        uint8_t program = (uint8_t)(in[2].as_integer() & 0x7F);
        return PrimResult::success(Value::boolean(midiSendShort2(handle, 0xC0 | channel, program)));
    });

    // midi-clock: -> integer (microseconds)
    r.register_prim("midi-clock", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(Value::integer(midiGetClock()));
    });

    // midi-sysex: handle data -> boolean
    r.register_prim("midi-sysex", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_data())
            return PrimResult::fail_with(Value::error("midi-sysex: expected handle and data"));
        auto* d = in[1].as_data();
        return PrimResult::success(Value::boolean(
            midiSendSysEx((int)in[0].as_integer(), d->bytes().data(), (int)d->length())));
    });
}

} // namespace pho
