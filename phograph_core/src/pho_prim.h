#pragma once
#include "pho_value.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace pho {

// Result of a primitive execution
struct PrimResult {
    std::vector<Value> outputs;
    bool failed = false;        // controlled failure (not error)
    Value error;                // error value if failed with error

    static PrimResult success(std::vector<Value> out) {
        return {std::move(out), false, Value::null_val()};
    }
    static PrimResult success(Value out) {
        return {std::vector<Value>{std::move(out)}, false, Value::null_val()};
    }
    static PrimResult success() {
        return {{}, false, Value::null_val()};
    }
    static PrimResult fail() {
        return {{}, true, Value::null_val()};
    }
    static PrimResult fail_with(Value err) {
        return {{}, true, std::move(err)};
    }
};

using PrimFn = std::function<PrimResult(const std::vector<Value>& inputs)>;

struct PrimInfo {
    std::string name;
    uint32_t num_inputs;
    uint32_t num_outputs;
    PrimFn fn;
};

class PrimitiveRegistry {
public:
    static PrimitiveRegistry& instance();

    void register_prim(const std::string& name, uint32_t num_inputs,
                       uint32_t num_outputs, PrimFn fn);

    const PrimInfo* find(const std::string& name) const;

    const std::unordered_map<std::string, PrimInfo>& all() const { return prims_; }

private:
    PrimitiveRegistry() = default;
    std::unordered_map<std::string, PrimInfo> prims_;
};

// Registration helpers used by pho_prim_*.cc files
void register_arith_prims();
void register_compare_prims();
void register_logic_prims();
void register_string_prims();
void register_list_prims();
void register_dict_prims();
void register_type_prims();
void register_data_prims();
void register_error_prims();
void register_canvas_prims();
void register_scene_prims();
void register_draw_prims();
void register_input_prims();
void register_anim_prims();
void register_io_prims();
void register_future_prims();
void register_channel_prims();
void register_effect_prims();

// Library primitives
void register_math_prims();
void register_fileio_prims();
void register_socket_prims();
void register_sound_prims();
void register_midi_prims();
void register_locale_prims();
void register_crypto_prims();
void register_image_prims();
void register_bitmap_prims();
void register_net_prims();

// Phase 13-14 primitives
void register_date_prims();
void register_methodref_prims();

void register_all_phase1_prims();
void register_all_prims();

// Console output capture -- prims call this instead of fprintf(stderr,...)
void pho_console_write(const std::string& text);

// Display buffer -- canvas-render calls this to push pixels to the IDE display
void pho_display_blit(const uint8_t* data, int32_t width, int32_t height);

} // namespace pho
