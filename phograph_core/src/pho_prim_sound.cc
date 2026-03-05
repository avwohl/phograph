#include "pho_prim.h"
#include "plugins/SoundPlugin.h"
#include <cmath>
#include <vector>
#include <cstring>

namespace pho {

// Generate 16-bit PCM samples for basic waveforms
static std::vector<int16_t> generate_tone(const char* type, double freq, double duration, int sampleRate) {
    int numSamples = (int)(duration * sampleRate);
    std::vector<int16_t> samples(numSamples);
    for (int i = 0; i < numSamples; i++) {
        double t = (double)i / sampleRate;
        double value = 0.0;
        if (strcmp(type, "sine") == 0) {
            value = sin(2.0 * M_PI * freq * t);
        } else if (strcmp(type, "square") == 0) {
            value = sin(2.0 * M_PI * freq * t) >= 0.0 ? 1.0 : -1.0;
        } else if (strcmp(type, "sawtooth") == 0) {
            double phase = fmod(freq * t, 1.0);
            value = 2.0 * phase - 1.0;
        }
        samples[i] = (int16_t)(value * 32000.0); // slightly below max to avoid clipping
    }
    return samples;
}

void register_sound_prims() {
    auto& r = PrimitiveRegistry::instance();

    // sound-init: sample-rate stereo? -> boolean
    r.register_prim("sound-init", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_boolean())
            return PrimResult::fail_with(Value::error("sound-init: expected integer sample-rate and boolean stereo"));
        bool ok = soundInit((int)in[0].as_integer(), in[1].as_boolean());
        return PrimResult::success(Value::boolean(ok));
    });

    // sound-stop: -> boolean
    r.register_prim("sound-stop", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        soundStop();
        return PrimResult::success(Value::boolean(true));
    });

    // sound-play: data -> integer (bytes written)
    r.register_prim("sound-play", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_data()) return PrimResult::fail_with(Value::error("sound-play: expected data"));
        auto* d = in[0].as_data();
        int written = soundPlaySamples(d->bytes().data(), 0, (int)d->length());
        return PrimResult::success(Value::integer(written));
    });

    // sound-volume: -> real
    r.register_prim("sound-volume", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(Value::real(soundGetVolume()));
    });

    // sound-set-volume: volume -> boolean
    r.register_prim("sound-set-volume", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric()) return PrimResult::fail_with(Value::error("sound-set-volume: expected number"));
        soundSetVolume((float)in[0].as_number());
        return PrimResult::success(Value::boolean(true));
    });

    // tone-sine: freq duration sample-rate -> data
    r.register_prim("tone-sine", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric() || !in[1].is_numeric() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("tone-sine: expected freq, duration, sample-rate"));
        auto samples = generate_tone("sine", in[0].as_number(), in[1].as_number(), (int)in[2].as_integer());
        std::vector<uint8_t> bytes(samples.size() * 2);
        memcpy(bytes.data(), samples.data(), bytes.size());
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    // tone-square: freq duration sample-rate -> data
    r.register_prim("tone-square", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric() || !in[1].is_numeric() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("tone-square: expected freq, duration, sample-rate"));
        auto samples = generate_tone("square", in[0].as_number(), in[1].as_number(), (int)in[2].as_integer());
        std::vector<uint8_t> bytes(samples.size() * 2);
        memcpy(bytes.data(), samples.data(), bytes.size());
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    // tone-sawtooth: freq duration sample-rate -> data
    r.register_prim("tone-sawtooth", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric() || !in[1].is_numeric() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("tone-sawtooth: expected freq, duration, sample-rate"));
        auto samples = generate_tone("sawtooth", in[0].as_number(), in[1].as_number(), (int)in[2].as_integer());
        std::vector<uint8_t> bytes(samples.size() * 2);
        memcpy(bytes.data(), samples.data(), bytes.size());
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    // tone-silence: duration sample-rate -> data
    r.register_prim("tone-silence", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("tone-silence: expected duration, sample-rate"));
        int numSamples = (int)(in[0].as_number() * in[1].as_integer());
        std::vector<uint8_t> bytes(numSamples * 2, 0);
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    // samples-mix: data-a data-b -> data
    r.register_prim("samples-mix", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_data() || !in[1].is_data())
            return PrimResult::fail_with(Value::error("samples-mix: expected two data values"));
        auto* a = in[0].as_data();
        auto* b = in[1].as_data();
        size_t len = std::max(a->length(), b->length());
        std::vector<uint8_t> result(len, 0);
        const int16_t* sa = (const int16_t*)a->bytes().data();
        const int16_t* sb = (const int16_t*)b->bytes().data();
        int16_t* out = (int16_t*)result.data();
        size_t nSamples = len / 2;
        size_t nA = a->length() / 2;
        size_t nB = b->length() / 2;
        for (size_t i = 0; i < nSamples; i++) {
            int32_t mixed = 0;
            if (i < nA) mixed += sa[i];
            if (i < nB) mixed += sb[i];
            if (mixed > 32767) mixed = 32767;
            if (mixed < -32768) mixed = -32768;
            out[i] = (int16_t)mixed;
        }
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(result))));
    });

    // samples-concat: data-a data-b -> data
    r.register_prim("samples-concat", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_data() || !in[1].is_data())
            return PrimResult::fail_with(Value::error("samples-concat: expected two data values"));
        auto* a = in[0].as_data();
        auto* b = in[1].as_data();
        std::vector<uint8_t> result;
        result.reserve(a->length() + b->length());
        result.insert(result.end(), a->bytes().begin(), a->bytes().end());
        result.insert(result.end(), b->bytes().begin(), b->bytes().end());
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(result))));
    });

    // samples-gain: data gain -> data
    r.register_prim("samples-gain", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_data() || !in[1].is_numeric())
            return PrimResult::fail_with(Value::error("samples-gain: expected data and number"));
        auto* d = in[0].as_data();
        double gain = in[1].as_number();
        std::vector<uint8_t> result(d->length());
        const int16_t* src = (const int16_t*)d->bytes().data();
        int16_t* dst = (int16_t*)result.data();
        size_t n = d->length() / 2;
        for (size_t i = 0; i < n; i++) {
            int32_t val = (int32_t)(src[i] * gain);
            if (val > 32767) val = 32767;
            if (val < -32768) val = -32768;
            dst[i] = (int16_t)val;
        }
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(result))));
    });

    // sound-is-running: -> boolean
    r.register_prim("sound-is-running", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(Value::boolean(soundIsRunning()));
    });
}

} // namespace pho
