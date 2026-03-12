// Test harness: load a .phograph.json file, run "main", report result.
// Usage: ./test_harness <path.json>

#include "pho_value.h"
#include "pho_graph.h"
#include "pho_eval.h"
#include "pho_prim.h"
#include "pho_serial.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

// Console capture
static std::string g_console;
namespace pho {
void pho_console_write(const std::string& text) {
    g_console += text;
}
void pho_display_blit(const uint8_t* data, int32_t w, int32_t h) {
    // Just record that display was produced
    if (data && w > 0 && h > 0) {
        g_console += "[canvas " + std::to_string(w) + "x" + std::to_string(h) + " rendered]\n";
    }
}
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: test_harness <file.json>\n";
        return 1;
    }

    // Read JSON file
    std::ifstream ifs(argv[1]);
    if (!ifs) {
        std::cout << "ERROR: cannot open " << argv[1] << "\n";
        std::cout << "FAIL\n";
        return 1;
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string json = ss.str();

    // Register all primitives
    pho::register_all_prims();

    // Load project
    pho::Project project;
    std::string load_error;
    if (!pho::load_project_from_json(json, project, load_error)) {
        std::cout << "LOAD ERROR: " << load_error << "\n";
        std::cout << "FAIL\n";
        return 1;
    }

    // Find entry method: try "main", then "/main"
    std::string entry;
    for (auto& section : project.sections) {
        for (auto& method : section.methods) {
            if (method.name == "main" || method.name == "/main") {
                entry = method.name;
                break;
            }
        }
        if (!entry.empty()) break;
    }
    if (entry.empty()) {
        std::cout << "NO MAIN METHOD\n";
        std::cout << "FAIL\n";
        return 1;
    }

    // Run
    g_console.clear();
    pho::Evaluator eval;
    eval.max_steps = 50000;
    auto result = eval.call_method(project, entry, {});

    // Report
    switch (result.status) {
        case pho::EvalStatus::Success:
            std::cout << "STATUS: success\n";
            break;
        case pho::EvalStatus::Failure:
            std::cout << "STATUS: failure\n";
            break;
        case pho::EvalStatus::Error:
            std::cout << "STATUS: error\n";
            if (!result.err_val.is_null()) {
                std::cout << "ERROR: " << result.err_val.to_display_string() << "\n";
            }
            break;
    }

    if (!result.outputs.empty()) {
        std::cout << "OUTPUTS:";
        for (size_t i = 0; i < result.outputs.size(); i++) {
            std::cout << " " << result.outputs[i].to_display_string();
        }
        std::cout << "\n";
    }

    if (!g_console.empty()) {
        std::cout << "CONSOLE: " << g_console;
        if (g_console.back() != '\n') std::cout << "\n";
    }

    if (result.status == pho::EvalStatus::Success) {
        std::cout << "PASS\n";
        return 0;
    } else {
        std::cout << "FAIL\n";
        return 1;
    }
}
