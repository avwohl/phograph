#include "pho_prim.h"
#include <clocale>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <xlocale.h>

static std::string cfStringToStd(CFStringRef cfStr) {
    if (!cfStr) return "";
    char buf[256];
    if (CFStringGetCString(cfStr, buf, sizeof(buf), kCFStringEncodingUTF8))
        return buf;
    return "";
}
#endif

namespace pho {

void register_locale_prims() {
    auto& r = PrimitiveRegistry::instance();

    // locale-language: -> string (e.g. "en")
    r.register_prim("locale-language", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFArrayRef langs = CFLocaleCopyPreferredLanguages();
        if (langs && CFArrayGetCount(langs) > 0) {
            CFStringRef lang = (CFStringRef)CFArrayGetValueAtIndex(langs, 0);
            std::string result = cfStringToStd(lang);
            CFRelease(langs);
            // Strip region (e.g. "en-US" -> "en")
            auto dash = result.find('-');
            if (dash != std::string::npos) result = result.substr(0, dash);
            return PrimResult::success(Value::string(std::move(result)));
        }
        if (langs) CFRelease(langs);
#endif
        return PrimResult::success(Value::string("en"));
    });

    // locale-country: -> string (e.g. "US")
    r.register_prim("locale-country", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFLocaleRef loc = CFLocaleCopyCurrent();
        CFStringRef country = (CFStringRef)CFLocaleGetValue(loc, kCFLocaleCountryCode);
        std::string result = cfStringToStd(country);
        CFRelease(loc);
        return PrimResult::success(Value::string(std::move(result)));
#else
        return PrimResult::success(Value::string("US"));
#endif
    });

    // locale-currency-symbol: -> string (e.g. "$")
    r.register_prim("locale-currency-symbol", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFLocaleRef loc = CFLocaleCopyCurrent();
        CFStringRef sym = (CFStringRef)CFLocaleGetValue(loc, kCFLocaleCurrencySymbol);
        std::string result = cfStringToStd(sym);
        CFRelease(loc);
        return PrimResult::success(Value::string(std::move(result)));
#else
        return PrimResult::success(Value::string("$"));
#endif
    });

    // locale-decimal-sep: -> string
    r.register_prim("locale-decimal-sep", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFLocaleRef loc = CFLocaleCopyCurrent();
        CFStringRef sep = (CFStringRef)CFLocaleGetValue(loc, kCFLocaleDecimalSeparator);
        std::string result = cfStringToStd(sep);
        CFRelease(loc);
        return PrimResult::success(Value::string(std::move(result)));
#else
        return PrimResult::success(Value::string("."));
#endif
    });

    // locale-thousands-sep: -> string
    r.register_prim("locale-thousands-sep", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFLocaleRef loc = CFLocaleCopyCurrent();
        CFStringRef sep = (CFStringRef)CFLocaleGetValue(loc, kCFLocaleGroupingSeparator);
        std::string result = cfStringToStd(sep);
        CFRelease(loc);
        return PrimResult::success(Value::string(std::move(result)));
#else
        return PrimResult::success(Value::string(","));
#endif
    });

    // locale-timezone: -> string (e.g. "America/New_York")
    r.register_prim("locale-timezone", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFTimeZoneRef tz = CFTimeZoneCopySystem();
        CFStringRef name = CFTimeZoneGetName(tz);
        std::string result = cfStringToStd(name);
        CFRelease(tz);
        return PrimResult::success(Value::string(std::move(result)));
#else
        return PrimResult::success(Value::string("UTC"));
#endif
    });

    // locale-tz-offset: -> integer (seconds from UTC)
    r.register_prim("locale-tz-offset", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFTimeZoneRef tz = CFTimeZoneCopySystem();
        CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
        CFTimeInterval offset = CFTimeZoneGetSecondsFromGMT(tz, now);
        CFRelease(tz);
        return PrimResult::success(Value::integer((int64_t)offset));
#else
        time_t t = time(nullptr);
        struct tm lt;
        localtime_r(&t, &lt);
        return PrimResult::success(Value::integer(lt.tm_gmtoff));
#endif
    });

    // locale-is-dst: -> boolean
    r.register_prim("locale-is-dst", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFTimeZoneRef tz = CFTimeZoneCopySystem();
        CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
        bool dst = CFTimeZoneIsDaylightSavingTime(tz, now);
        CFRelease(tz);
        return PrimResult::success(Value::boolean(dst));
#else
        time_t t = time(nullptr);
        struct tm lt;
        localtime_r(&t, &lt);
        return PrimResult::success(Value::boolean(lt.tm_isdst > 0));
#endif
    });

    // locale-date-format: -> string (e.g. "MM/dd/yyyy")
    r.register_prim("locale-date-format", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFLocaleRef loc = CFLocaleCopyCurrent();
        CFDateFormatterRef df = CFDateFormatterCreate(nullptr, loc, kCFDateFormatterShortStyle, kCFDateFormatterNoStyle);
        CFStringRef fmt = CFDateFormatterGetFormat(df);
        std::string result = cfStringToStd(fmt);
        CFRelease(df);
        CFRelease(loc);
        return PrimResult::success(Value::string(std::move(result)));
#else
        return PrimResult::success(Value::string("yyyy-MM-dd"));
#endif
    });

    // locale-time-format: -> string
    r.register_prim("locale-time-format", 0, 1, [](const std::vector<Value>&) -> PrimResult {
#ifdef __APPLE__
        CFLocaleRef loc = CFLocaleCopyCurrent();
        CFDateFormatterRef df = CFDateFormatterCreate(nullptr, loc, kCFDateFormatterNoStyle, kCFDateFormatterShortStyle);
        CFStringRef fmt = CFDateFormatterGetFormat(df);
        std::string result = cfStringToStd(fmt);
        CFRelease(df);
        CFRelease(loc);
        return PrimResult::success(Value::string(std::move(result)));
#else
        return PrimResult::success(Value::string("HH:mm:ss"));
#endif
    });

    // locale-format-number: number -> string (locale-formatted)
    r.register_prim("locale-format-number", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric()) return PrimResult::fail_with(Value::error("locale-format-number: expected number"));
#ifdef __APPLE__
        CFLocaleRef loc = CFLocaleCopyCurrent();
        CFNumberFormatterRef nf = CFNumberFormatterCreate(nullptr, loc, kCFNumberFormatterDecimalStyle);
        double val = in[0].as_number();
        CFNumberRef num = CFNumberCreate(nullptr, kCFNumberDoubleType, &val);
        CFStringRef str = CFNumberFormatterCreateStringWithNumber(nullptr, nf, num);
        std::string result = cfStringToStd(str);
        CFRelease(str);
        CFRelease(num);
        CFRelease(nf);
        CFRelease(loc);
        return PrimResult::success(Value::string(std::move(result)));
#else
        std::ostringstream oss;
        oss << in[0].as_number();
        return PrimResult::success(Value::string(oss.str()));
#endif
    });

    // locale-format-currency: number -> string (locale-formatted currency)
    r.register_prim("locale-format-currency", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric()) return PrimResult::fail_with(Value::error("locale-format-currency: expected number"));
#ifdef __APPLE__
        CFLocaleRef loc = CFLocaleCopyCurrent();
        CFNumberFormatterRef nf = CFNumberFormatterCreate(nullptr, loc, kCFNumberFormatterCurrencyStyle);
        double val = in[0].as_number();
        CFNumberRef num = CFNumberCreate(nullptr, kCFNumberDoubleType, &val);
        CFStringRef str = CFNumberFormatterCreateStringWithNumber(nullptr, nf, num);
        std::string result = cfStringToStd(str);
        CFRelease(str);
        CFRelease(num);
        CFRelease(nf);
        CFRelease(loc);
        return PrimResult::success(Value::string(std::move(result)));
#else
        std::ostringstream oss;
        oss << "$" << std::fixed << std::setprecision(2) << in[0].as_number();
        return PrimResult::success(Value::string(oss.str()));
#endif
    });
}

} // namespace pho
