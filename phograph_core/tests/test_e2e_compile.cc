// End-to-end test: build a Phograph project, compile to Swift,
// write to file, invoke swiftc, run the binary, check output.

#include "pho_codegen.h"
#include "pho_graph.h"
#include "pho_value.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

using namespace pho;

int main() {
    printf("=== End-to-End Compile Test ===\n");

    // Build project: main() prints double-it(21) = 42
    Project p;
    p.name = "E2ETest";

    Section s;
    s.name = "Main";

    // double-it: x -> x * 2
    {
        Method m;
        m.name = "double-it";
        m.num_inputs = 1;
        m.num_outputs = 1;

        Case c;
        Node input_bar;
        input_bar.type = NodeType::InputBar;
        input_bar.num_inputs = 0;
        input_bar.num_outputs = 1;
        c.input_bar_id = c.add_node(input_bar);

        Node const_2;
        const_2.type = NodeType::Constant;
        const_2.constant_value = Value::integer(2);
        const_2.num_inputs = 0;
        const_2.num_outputs = 1;
        NodeId c2_id = c.add_node(const_2);

        Node mul_node;
        mul_node.type = NodeType::Primitive;
        mul_node.name = "*";
        mul_node.num_inputs = 2;
        mul_node.num_outputs = 1;
        NodeId mul_id = c.add_node(mul_node);

        Node output_bar;
        output_bar.type = NodeType::OutputBar;
        output_bar.num_inputs = 1;
        output_bar.num_outputs = 0;
        c.output_bar_id = c.add_node(output_bar);

        c.add_wire({c.input_bar_id, 0, true}, {mul_id, 0, false});
        c.add_wire({c2_id, 0, true}, {mul_id, 1, false});
        c.add_wire({mul_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        s.methods.push_back(std::move(m));
    }

    // main: () -> print(double-it(21))
    {
        Method m;
        m.name = "main";
        m.num_inputs = 0;
        m.num_outputs = 0;

        Case c;
        Node input_bar;
        input_bar.type = NodeType::InputBar;
        input_bar.num_inputs = 0;
        input_bar.num_outputs = 0;
        c.input_bar_id = c.add_node(input_bar);

        Node const_21;
        const_21.type = NodeType::Constant;
        const_21.constant_value = Value::integer(21);
        const_21.num_inputs = 0;
        const_21.num_outputs = 1;
        NodeId c21_id = c.add_node(const_21);

        Node call_node;
        call_node.type = NodeType::MethodCall;
        call_node.name = "double-it";
        call_node.num_inputs = 1;
        call_node.num_outputs = 1;
        NodeId call_id = c.add_node(call_node);

        Node print_node;
        print_node.type = NodeType::Primitive;
        print_node.name = "print";
        print_node.num_inputs = 1;
        print_node.num_outputs = 1;
        NodeId print_id = c.add_node(print_node);

        Node output_bar;
        output_bar.type = NodeType::OutputBar;
        output_bar.num_inputs = 0;
        output_bar.num_outputs = 0;
        c.output_bar_id = c.add_node(output_bar);

        c.add_wire({c21_id, 0, true}, {call_id, 0, false});
        c.add_wire({call_id, 0, true}, {print_id, 0, false});

        m.cases.push_back(std::move(c));
        s.methods.push_back(std::move(m));
    }

    p.sections.push_back(std::move(s));

    // Compile to Swift
    CodegenOptions opts;
    opts.emit_runtime = false; // use external runtime file
    opts.emit_imports = true;
    opts.emit_main = true;
    opts.entry_method = "main";

    SwiftCodegen gen(opts);
    std::string source;
    std::vector<CodegenError> errors;
    bool ok = gen.compile(p, source, errors);
    assert(ok);

    // Write generated source
    std::string build_dir = "build/e2e_test";
    system(("mkdir -p " + build_dir).c_str());

    std::string gen_path = build_dir + "/generated.swift";
    {
        std::ofstream f(gen_path);
        f << source;
    }

    // Copy runtime
    std::string runtime_src = "../Phograph/Runtime/PhographRuntime.swift";
    std::string runtime_path = build_dir + "/PhographRuntime.swift";
    {
        std::ifstream in(runtime_src);
        std::ofstream out(runtime_path);
        out << in.rdbuf();
    }

    printf("  Generated Swift source: %zu bytes\n", source.size());

    // Compile with swiftc
    std::string binary_path = build_dir + "/e2e_program";
    std::string compile_cmd = "swiftc -o " + binary_path + " " +
                               runtime_path + " " + gen_path + " 2>&1";
    printf("  Compiling: %s\n", compile_cmd.c_str());
    int rc = system(compile_cmd.c_str());
    if (rc != 0) {
        printf("  FAILED: swiftc returned %d\n", rc);
        // Print generated source for debugging
        printf("--- Generated source ---\n%s\n---\n", source.c_str());
        return 1;
    }

    // Run the binary and capture output
    std::string run_cmd = binary_path + " > " + build_dir + "/output.txt 2>&1";
    printf("  Running: %s\n", run_cmd.c_str());
    rc = system(run_cmd.c_str());
    if (rc != 0) {
        printf("  FAILED: program returned %d\n", rc);
        return 1;
    }

    // Check output
    std::ifstream output_file(build_dir + "/output.txt");
    std::string output((std::istreambuf_iterator<char>(output_file)),
                        std::istreambuf_iterator<char>());

    // Trim trailing whitespace
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
        output.pop_back();

    printf("  Output: \"%s\"\n", output.c_str());
    assert(output == "42");

    printf("End-to-end compile test PASSED!\n");
    return 0;
}
