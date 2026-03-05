#include "pho_prim.h"

namespace pho {

PrimitiveRegistry& PrimitiveRegistry::instance() {
    static PrimitiveRegistry reg;
    return reg;
}

void PrimitiveRegistry::register_prim(const std::string& name, uint32_t num_inputs,
                                       uint32_t num_outputs, PrimFn fn) {
    prims_[name] = PrimInfo{name, num_inputs, num_outputs, std::move(fn)};
}

const PrimInfo* PrimitiveRegistry::find(const std::string& name) const {
    auto it = prims_.find(name);
    if (it != prims_.end()) return &it->second;
    return nullptr;
}

void register_all_phase1_prims() {
    register_arith_prims();
    register_compare_prims();
    register_logic_prims();
}

void register_all_prims() {
    register_arith_prims();
    register_compare_prims();
    register_logic_prims();
    register_string_prims();
    register_list_prims();
    register_dict_prims();
    register_type_prims();
    register_data_prims();
    register_error_prims();
    register_canvas_prims();
    register_scene_prims();
    register_draw_prims();
    register_input_prims();
    register_anim_prims();
    register_io_prims();
    register_future_prims();
    register_channel_prims();
    register_effect_prims();

    // Library primitives
    register_math_prims();
    register_fileio_prims();
    register_socket_prims();
    register_sound_prims();
    register_midi_prims();
    register_locale_prims();
    register_crypto_prims();
    register_image_prims();
    register_bitmap_prims();
    register_net_prims();

    // Phase 13-14 primitives
    register_date_prims();
    register_methodref_prims();
}

} // namespace pho
