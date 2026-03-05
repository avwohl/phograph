#include "pho_prim.h"
#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <random>

#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#include <Security/SecRandom.h>
#endif

namespace pho {

static std::string to_hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; i++)
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)data[i];
    return oss.str();
}

static std::string base64_encode(const uint8_t* data, size_t len) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = ((uint32_t)data[i]) << 16;
        if (i + 1 < len) n |= ((uint32_t)data[i + 1]) << 8;
        if (i + 2 < len) n |= (uint32_t)data[i + 2];
        result += table[(n >> 18) & 0x3F];
        result += table[(n >> 12) & 0x3F];
        result += (i + 1 < len) ? table[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? table[n & 0x3F] : '=';
    }
    return result;
}

static std::vector<uint8_t> base64_decode(const std::string& encoded) {
    static const int table[128] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    std::vector<uint8_t> result;
    uint32_t buf = 0;
    int bits = 0;
    for (char c : encoded) {
        if (c == '=' || c == '\n' || c == '\r') continue;
        if ((unsigned char)c >= 128 || table[(unsigned char)c] < 0) continue;
        buf = (buf << 6) | table[(unsigned char)c];
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            result.push_back((buf >> bits) & 0xFF);
        }
    }
    return result;
}

void register_crypto_prims() {
    auto& r = PrimitiveRegistry::instance();

    // sha256: data -> hex string
    r.register_prim("sha256", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("sha256: expected string"));
#ifdef __APPLE__
        uint8_t digest[CC_SHA256_DIGEST_LENGTH];
        CC_SHA256(in[0].as_string()->c_str(), (CC_LONG)in[0].as_string()->length(), digest);
        return PrimResult::success(Value::string(to_hex(digest, CC_SHA256_DIGEST_LENGTH)));
#else
        return PrimResult::fail_with(Value::error("sha256: not available on this platform"));
#endif
    });

    // sha1: data -> hex string
    r.register_prim("sha1", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("sha1: expected string"));
#ifdef __APPLE__
        uint8_t digest[CC_SHA1_DIGEST_LENGTH];
        CC_SHA1(in[0].as_string()->c_str(), (CC_LONG)in[0].as_string()->length(), digest);
        return PrimResult::success(Value::string(to_hex(digest, CC_SHA1_DIGEST_LENGTH)));
#else
        return PrimResult::fail_with(Value::error("sha1: not available on this platform"));
#endif
    });

    // md5: data -> hex string
    r.register_prim("md5", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("md5: expected string"));
#ifdef __APPLE__
        uint8_t digest[CC_MD5_DIGEST_LENGTH];
        CC_MD5(in[0].as_string()->c_str(), (CC_LONG)in[0].as_string()->length(), digest);
        return PrimResult::success(Value::string(to_hex(digest, CC_MD5_DIGEST_LENGTH)));
#else
        return PrimResult::fail_with(Value::error("md5: not available on this platform"));
#endif
    });

    // hmac-sha256: key message -> hex string
    r.register_prim("hmac-sha256", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string())
            return PrimResult::fail_with(Value::error("hmac-sha256: expected two strings"));
#ifdef __APPLE__
        uint8_t digest[CC_SHA256_DIGEST_LENGTH];
        CCHmac(kCCHmacAlgSHA256,
               in[0].as_string()->c_str(), in[0].as_string()->length(),
               in[1].as_string()->c_str(), in[1].as_string()->length(),
               digest);
        return PrimResult::success(Value::string(to_hex(digest, CC_SHA256_DIGEST_LENGTH)));
#else
        return PrimResult::fail_with(Value::error("hmac-sha256: not available"));
#endif
    });

    // hmac-sha1: key message -> hex string
    r.register_prim("hmac-sha1", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string())
            return PrimResult::fail_with(Value::error("hmac-sha1: expected two strings"));
#ifdef __APPLE__
        uint8_t digest[CC_SHA1_DIGEST_LENGTH];
        CCHmac(kCCHmacAlgSHA1,
               in[0].as_string()->c_str(), in[0].as_string()->length(),
               in[1].as_string()->c_str(), in[1].as_string()->length(),
               digest);
        return PrimResult::success(Value::string(to_hex(digest, CC_SHA1_DIGEST_LENGTH)));
#else
        return PrimResult::fail_with(Value::error("hmac-sha1: not available"));
#endif
    });

    // random-bytes: count -> data
    r.register_prim("random-bytes", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("random-bytes: expected integer"));
        int64_t count = in[0].as_integer();
        if (count < 0 || count > 65536) return PrimResult::fail_with(Value::error("random-bytes: count 0..65536"));
        std::vector<uint8_t> bytes(count);
#ifdef __APPLE__
        if (SecRandomCopyBytes(kSecRandomDefault, count, bytes.data()) != errSecSuccess)
            return PrimResult::fail_with(Value::error("random-bytes: generation failed"));
#else
        std::random_device rd;
        for (auto& b : bytes) b = (uint8_t)(rd() & 0xFF);
#endif
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    // random-int: min max -> integer
    r.register_prim("random-int", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("random-int: expected two integers"));
        int64_t lo = in[0].as_integer(), hi = in[1].as_integer();
        if (lo > hi) return PrimResult::fail_with(Value::error("random-int: min > max"));
#ifdef __APPLE__
        uint64_t range = (uint64_t)(hi - lo) + 1;
        uint64_t rnd;
        SecRandomCopyBytes(kSecRandomDefault, sizeof(rnd), &rnd);
        return PrimResult::success(Value::integer(lo + (int64_t)(rnd % range)));
#else
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<int64_t> dist(lo, hi);
        return PrimResult::success(Value::integer(dist(gen)));
#endif
    });

    // uuid: -> string
    r.register_prim("uuid", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        uint8_t bytes[16];
#ifdef __APPLE__
        SecRandomCopyBytes(kSecRandomDefault, 16, bytes);
#else
        std::random_device rd;
        for (auto& b : bytes) b = (uint8_t)(rd() & 0xFF);
#endif
        bytes[6] = (bytes[6] & 0x0F) | 0x40; // version 4
        bytes[8] = (bytes[8] & 0x3F) | 0x80; // variant 1
        char buf[37];
        snprintf(buf, sizeof(buf),
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 bytes[0],bytes[1],bytes[2],bytes[3],
                 bytes[4],bytes[5],bytes[6],bytes[7],
                 bytes[8],bytes[9],bytes[10],bytes[11],
                 bytes[12],bytes[13],bytes[14],bytes[15]);
        return PrimResult::success(Value::string(buf));
    });

    // base64-encode: data -> string
    r.register_prim("base64-encode", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_string()) {
            const auto* s = in[0].as_string();
            return PrimResult::success(Value::string(
                base64_encode((const uint8_t*)s->c_str(), s->length())));
        }
        if (in[0].is_data()) {
            auto* d = in[0].as_data();
            return PrimResult::success(Value::string(
                base64_encode(d->bytes().data(), d->length())));
        }
        return PrimResult::fail_with(Value::error("base64-encode: expected string or data"));
    });

    // base64-decode: string -> string
    r.register_prim("base64-decode", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("base64-decode: expected string"));
        auto decoded = base64_decode(in[0].as_string()->str());
        return PrimResult::success(Value::string(std::string(decoded.begin(), decoded.end())));
    });

    // hex-encode: data -> string
    r.register_prim("hex-encode", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_string()) {
            const auto* s = in[0].as_string();
            return PrimResult::success(Value::string(
                to_hex((const uint8_t*)s->c_str(), s->length())));
        }
        if (in[0].is_data()) {
            auto* d = in[0].as_data();
            return PrimResult::success(Value::string(
                to_hex(d->bytes().data(), d->length())));
        }
        return PrimResult::fail_with(Value::error("hex-encode: expected string or data"));
    });

    // hex-decode: hex string -> string
    r.register_prim("hex-decode", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("hex-decode: expected string"));
        const std::string& hex = in[0].as_string()->str();
        if (hex.size() % 2 != 0) return PrimResult::fail_with(Value::error("hex-decode: odd length"));
        std::string result;
        for (size_t i = 0; i < hex.size(); i += 2) {
            unsigned int byte;
            if (sscanf(hex.c_str() + i, "%2x", &byte) != 1)
                return PrimResult::fail_with(Value::error("hex-decode: invalid hex"));
            result += (char)byte;
        }
        return PrimResult::success(Value::string(std::move(result)));
    });

    // dsa-verify: stub (would need full DSA implementation)
    r.register_prim("dsa-verify", 3, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::fail_with(Value::error("dsa-verify: not yet implemented"));
    });

    // ssl-connect, ssl-send, ssl-recv, ssl-close: stubs
    r.register_prim("ssl-connect", 2, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::fail_with(Value::error("ssl-connect: not yet implemented"));
    });
    r.register_prim("ssl-send", 2, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::fail_with(Value::error("ssl-send: not yet implemented"));
    });
    r.register_prim("ssl-recv", 2, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::fail_with(Value::error("ssl-recv: not yet implemented"));
    });
    r.register_prim("ssl-close", 1, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::fail_with(Value::error("ssl-close: not yet implemented"));
    });
}

} // namespace pho
