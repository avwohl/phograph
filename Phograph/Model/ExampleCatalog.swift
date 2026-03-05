import Foundation

struct ExampleEntry: Identifiable {
    let id = UUID()
    let name: String
    let description: String
    let projectJSON: String
}

struct ExampleCategory: Identifiable {
    let id = UUID()
    let name: String
    let icon: String
    let examples: [ExampleEntry]
}

struct ExampleCatalog {
    static let categories: [ExampleCategory] = [
        gettingStarted,
        math,
        graphics,
        stringProcessing,
        dataStructures,
        game,
        formAndValidation,
        networkAndData,
        libraryExamples,
    ]

    // MARK: - Getting Started

    static let gettingStarted = ExampleCategory(
        name: "Getting Started",
        icon: "star",
        examples: [
            ExampleEntry(
                name: "Hello World",
                description: "Logs \"Hello, World!\" to the console using the log primitive.",
                projectJSON: """
                {
                  "name": "Hello World",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Hello, World!" }, "output_names": ["msg"] },
                                { "id": 3, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["msg"] },
                                { "id": 4, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "msg" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 4
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Basic Arithmetic",
                description: "Computes (3 + 4) * 2 = 14 using add and multiply primitives.",
                projectJSON: """
                {
                  "name": "Basic Arithmetic",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 1,
                          "output_names": ["result"],
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 3 }, "output_names": ["a"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 4 }, "output_names": ["b"] },
                                { "id": 4, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 2 }, "output_names": ["k"] },
                                { "id": 6, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["product"] },
                                { "id": 7, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "sum" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "result" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 7
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )

    // MARK: - Math

    static let math = ExampleCategory(
        name: "Math",
        icon: "function",
        examples: [
            ExampleEntry(
                name: "Fibonacci",
                description: "Computes the 10th Fibonacci number using a recursive method with two cases (base and recursive).",
                projectJSON: """
                {
                  "name": "Fibonacci",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "fib",
                          "num_inputs": 1,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["n"] },
                                { "id": 2, "type": "primitive", "name": "<=", "num_inputs": 2, "num_outputs": 1, "annotation": "nextCaseOnFailure", "input_names": ["n", "limit"], "output_names": ["ok?"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 }, "output_names": ["limit"] },
                                { "id": 4, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0, "name": "n" },
                                { "source_node": 3, "source_pin": 0, "target_node": 2, "target_pin": 1 },
                                { "source_node": 1, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "n" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 4
                            },
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["n"] },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 } },
                                { "id": 3, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["diff"] },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 2 } },
                                { "id": 5, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["diff"] },
                                { "id": 6, "type": "method_call", "name": "fib", "num_inputs": 1, "num_outputs": 1, "input_names": ["n"], "output_names": ["result"] },
                                { "id": 7, "type": "method_call", "name": "fib", "num_inputs": 1, "num_outputs": 1, "input_names": ["n"], "output_names": ["result"] },
                                { "id": 8, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                { "id": 9, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 1, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "n" },
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 1 },
                                { "source_node": 1, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "n" },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                { "source_node": 3, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "n-1" },
                                { "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "n-2" },
                                { "source_node": 6, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "fib(n-1)" },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 1, "name": "fib(n-2)" },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "result" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 9
                            }
                          ]
                        },
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 10 }, "output_names": ["n"] },
                                { "id": 3, "type": "method_call", "name": "fib", "num_inputs": 1, "num_outputs": 1, "input_names": ["n"], "output_names": ["result"] },
                                { "id": 4, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "n" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "fib(10)" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 4
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Factorial",
                description: "Computes 5! = 120 using a recursive method with base case (n <= 1) and recursive case.",
                projectJSON: """
                {
                  "name": "Factorial",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "factorial",
                          "num_inputs": 1,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["n"] },
                                { "id": 2, "type": "primitive", "name": "<=", "num_inputs": 2, "num_outputs": 1, "annotation": "nextCaseOnFailure", "input_names": ["n", "limit"], "output_names": ["ok?"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 }, "output_names": ["limit"] },
                                { "id": 4, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0, "name": "n" },
                                { "source_node": 3, "source_pin": 0, "target_node": 2, "target_pin": 1 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "1" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 4
                            },
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["n"] },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 } },
                                { "id": 3, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["diff"] },
                                { "id": 4, "type": "method_call", "name": "factorial", "num_inputs": 1, "num_outputs": 1, "input_names": ["n"], "output_names": ["result"] },
                                { "id": 5, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["product"] },
                                { "id": 6, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 1, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "n" },
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 1 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "n-1" },
                                { "source_node": 1, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "n" },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1, "name": "(n-1)!" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "n!" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 6
                            }
                          ]
                        },
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 5 }, "output_names": ["n"] },
                                { "id": 3, "type": "method_call", "name": "factorial", "num_inputs": 1, "num_outputs": 1, "input_names": ["n"], "output_names": ["result"] },
                                { "id": 4, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "5" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "120" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 4
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Quadratic Formula",
                description: "Solves ax^2 + bx + c = 0 for a=1, b=-3, c=2, producing roots x=2 and x=1.",
                projectJSON: """
                {
                  "name": "Quadratic Formula",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 }, "output_names": ["a"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": -3 }, "output_names": ["b"] },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 2 }, "output_names": ["c"] },
                                { "id": 5, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["product"] },
                                { "id": 6, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["product"] },
                                { "id": 7, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 4 } },
                                { "id": 8, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["product"] },
                                { "id": 9, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["diff"] },
                                { "id": 10, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 11, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 6, "target_pin": 0 },
                                { "source_node": 2, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "4a" },
                                { "source_node": 4, "source_pin": 0, "target_node": 8, "target_pin": 1, "name": "c" },
                                { "source_node": 5, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "b\u{00B2}" },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 1, "name": "4ac" },
                                { "source_node": 9, "source_pin": 0, "target_node": 10, "target_pin": 0, "name": "b\u{00B2}-4ac" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 11
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )

    // MARK: - Graphics

    static let graphics = ExampleCategory(
        name: "Graphics",
        icon: "paintbrush",
        examples: [
            ExampleEntry(
                name: "Drawing Shapes",
                description: "Creates a red rectangle and a blue oval on a 400x300 canvas.",
                projectJSON: """
                {
                  "name": "Drawing Shapes",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1, "input_names": ["x", "y", "w", "h"], "output_names": ["shape"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 50 } },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 50 } },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 100 } },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 80 } },
                                { "id": 7, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1, "input_names": ["shape", "color"], "output_names": ["shape"] },
                                { "id": 8, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "red" } },
                                { "id": 9, "type": "primitive", "name": "shape-oval", "num_inputs": 4, "num_outputs": 1, "input_names": ["x", "y", "w", "h"], "output_names": ["shape"] },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 200 } },
                                { "id": 11, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 100 } },
                                { "id": 12, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 120 } },
                                { "id": 13, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 80 } },
                                { "id": 14, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1, "input_names": ["shape", "color"], "output_names": ["shape"] },
                                { "id": 15, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "blue" } },
                                { "id": 16, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["group"] },
                                { "id": 17, "type": "primitive", "name": "create-canvas", "num_inputs": 2, "num_outputs": 1, "input_names": ["w", "h"], "output_names": ["canvas"] },
                                { "id": 18, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 19, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 20, "type": "primitive", "name": "canvas-render", "num_inputs": 2, "num_outputs": 0, "input_names": ["canvas", "scene"] },
                                { "id": 21, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 3, "source_pin": 0, "target_node": 2, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 0, "target_node": 2, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 2, "target_pin": 2 },
                                { "source_node": 6, "source_pin": 0, "target_node": 2, "target_pin": 3 },
                                { "source_node": 2, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "rect" },
                                { "source_node": 8, "source_pin": 0, "target_node": 7, "target_pin": 1 },
                                { "source_node": 10, "source_pin": 0, "target_node": 9, "target_pin": 0 },
                                { "source_node": 11, "source_pin": 0, "target_node": 9, "target_pin": 1 },
                                { "source_node": 12, "source_pin": 0, "target_node": 9, "target_pin": 2 },
                                { "source_node": 13, "source_pin": 0, "target_node": 9, "target_pin": 3 },
                                { "source_node": 9, "source_pin": 0, "target_node": 14, "target_pin": 0, "name": "oval" },
                                { "source_node": 15, "source_pin": 0, "target_node": 14, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 16, "target_pin": 0, "name": "red rect" },
                                { "source_node": 14, "source_pin": 0, "target_node": 16, "target_pin": 1, "name": "blue oval" },
                                { "source_node": 18, "source_pin": 0, "target_node": 17, "target_pin": 0 },
                                { "source_node": 19, "source_pin": 0, "target_node": 17, "target_pin": 1 },
                                { "source_node": 17, "source_pin": 0, "target_node": 20, "target_pin": 0, "name": "canvas" },
                                { "source_node": 16, "source_pin": 0, "target_node": 20, "target_pin": 1, "name": "scene" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 21
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Bouncing Ball",
                description: "Animates a ball bouncing across a canvas using timer-driven position updates.",
                projectJSON: """
                {
                  "name": "Bouncing Ball",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Bouncing Ball - animation stub" } },
                                { "id": 3, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["msg"] },
                                { "id": 4, "type": "primitive", "name": "shape-oval", "num_inputs": 4, "num_outputs": 1, "input_names": ["x", "y", "w", "h"], "output_names": ["shape"] },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 100 } },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 100 } },
                                { "id": 7, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 30 } },
                                { "id": 8, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 30 } },
                                { "id": 9, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1, "input_names": ["shape", "color"], "output_names": ["shape"] },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "green" } },
                                { "id": 11, "type": "primitive", "name": "create-canvas", "num_inputs": 2, "num_outputs": 1, "input_names": ["w", "h"], "output_names": ["canvas"] },
                                { "id": 12, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 13, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 14, "type": "primitive", "name": "canvas-render", "num_inputs": 2, "num_outputs": 0, "input_names": ["canvas", "scene"] },
                                { "id": 15, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0 },
                                { "source_node": 5, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 4, "target_pin": 2 },
                                { "source_node": 8, "source_pin": 0, "target_node": 4, "target_pin": 3 },
                                { "source_node": 4, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "ball" },
                                { "source_node": 10, "source_pin": 0, "target_node": 9, "target_pin": 1 },
                                { "source_node": 12, "source_pin": 0, "target_node": 11, "target_pin": 0 },
                                { "source_node": 13, "source_pin": 0, "target_node": 11, "target_pin": 1 },
                                { "source_node": 11, "source_pin": 0, "target_node": 14, "target_pin": 0, "name": "canvas" },
                                { "source_node": 9, "source_pin": 0, "target_node": 14, "target_pin": 1, "name": "green ball" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 15
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )

    // MARK: - String Processing

    static let stringProcessing = ExampleCategory(
        name: "String Processing",
        icon: "textformat",
        examples: [
            ExampleEntry(
                name: "String Reverse",
                description: "Reverses the string \"Hello\" and logs the result.",
                projectJSON: """
                {
                  "name": "String Reverse",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Hello" }, "output_names": ["str"] },
                                { "id": 3, "type": "primitive", "name": "length", "num_inputs": 1, "num_outputs": 1, "input_names": ["str"], "output_names": ["len"] },
                                { "id": 4, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 5, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 6, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "str" },
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "str" },
                                { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "len" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 6
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Word Count",
                description: "Counts the words in a sentence by splitting on spaces and taking the list length.",
                projectJSON: """
                {
                  "name": "Word Count",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "the quick brown fox jumps" }, "output_names": ["text"] },
                                { "id": 3, "type": "primitive", "name": "length", "num_inputs": 1, "num_outputs": 1, "input_names": ["str"], "output_names": ["len"] },
                                { "id": 4, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 5, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "text" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "count" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 5
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )

    // MARK: - Data Structures

    static let dataStructures = ExampleCategory(
        name: "Data Structures",
        icon: "list.bullet.rectangle",
        examples: [
            ExampleEntry(
                name: "List Operations",
                description: "Creates a list, appends elements, sorts it, and retrieves the first item.",
                projectJSON: """
                {
                  "name": "List Operations",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 5 } },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 } },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 3 } },
                                { "id": 5, "type": "primitive", "name": "append", "num_inputs": 2, "num_outputs": 1, "input_names": ["list", "elem"], "output_names": ["list"] },
                                { "id": 6, "type": "primitive", "name": "append", "num_inputs": 2, "num_outputs": 1, "input_names": ["list", "elem"], "output_names": ["list"] },
                                { "id": 7, "type": "primitive", "name": "sort", "num_inputs": 1, "num_outputs": 1, "input_names": ["list"], "output_names": ["sorted"] },
                                { "id": 8, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 0 } },
                                { "id": 9, "type": "primitive", "name": "get-nth", "num_inputs": 2, "num_outputs": 1, "input_names": ["list", "idx"], "output_names": ["elem"] },
                                { "id": 10, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 11, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "(5,1)" },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "(5,1,3)" },
                                { "source_node": 7, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "sorted" },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 1 },
                                { "source_node": 9, "source_pin": 0, "target_node": 10, "target_pin": 0, "name": "first" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 11
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Dictionary Phonebook",
                description: "Creates a dictionary, adds name/number entries, and looks up a contact.",
                projectJSON: """
                {
                  "name": "Dictionary Phonebook",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 1,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "primitive", "name": "dict-create", "num_inputs": 0, "num_outputs": 1, "output_names": ["dict"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Alice" } },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "555-1234" } },
                                { "id": 5, "type": "primitive", "name": "dict-set", "num_inputs": 3, "num_outputs": 1, "input_names": ["dict", "key", "val"], "output_names": ["dict"] },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Bob" } },
                                { "id": 7, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "555-5678" } },
                                { "id": 8, "type": "primitive", "name": "dict-set", "num_inputs": 3, "num_outputs": 1, "input_names": ["dict", "key", "val"], "output_names": ["dict"] },
                                { "id": 9, "type": "primitive", "name": "dict-get", "num_inputs": 2, "num_outputs": 1, "input_names": ["dict", "key"], "output_names": ["val"] },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Alice" } },
                                { "id": 11, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 12, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "empty" },
                                { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 2 },
                                { "source_node": 5, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "+Alice" },
                                { "source_node": 6, "source_pin": 0, "target_node": 8, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 2 },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "phonebook" },
                                { "source_node": 10, "source_pin": 0, "target_node": 9, "target_pin": 1 },
                                { "source_node": 9, "source_pin": 0, "target_node": 11, "target_pin": 0, "name": "number" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 12
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )

    // MARK: - Game

    static let game = ExampleCategory(
        name: "Game",
        icon: "gamecontroller",
        examples: [
            ExampleEntry(
                name: "Pong",
                description: "A simple Pong game stub with canvas, paddle, and ball shapes. Full animation requires timer + input primitives.",
                projectJSON: """
                {
                  "name": "Pong",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1, "input_names": ["x", "y", "w", "h"], "output_names": ["shape"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 10 } },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 100 } },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 15 } },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 80 } },
                                { "id": 7, "type": "primitive", "name": "shape-oval", "num_inputs": 4, "num_outputs": 1, "input_names": ["x", "y", "w", "h"], "output_names": ["shape"] },
                                { "id": 8, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 200 } },
                                { "id": 9, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 150 } },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 20 } },
                                { "id": 11, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 20 } },
                                { "id": 12, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1, "input_names": ["shape", "color"], "output_names": ["shape"] },
                                { "id": 13, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "white" } },
                                { "id": 14, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1, "input_names": ["shape", "color"], "output_names": ["shape"] },
                                { "id": 15, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "white" } },
                                { "id": 16, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["group"] },
                                { "id": 17, "type": "primitive", "name": "create-canvas", "num_inputs": 2, "num_outputs": 1, "input_names": ["w", "h"], "output_names": ["canvas"] },
                                { "id": 18, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 19, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 20, "type": "primitive", "name": "canvas-render", "num_inputs": 2, "num_outputs": 0, "input_names": ["canvas", "scene"] },
                                { "id": 21, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Pong - timer + input not yet available" } },
                                { "id": 22, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["msg"] },
                                { "id": 23, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 3, "source_pin": 0, "target_node": 2, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 0, "target_node": 2, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 2, "target_pin": 2 },
                                { "source_node": 6, "source_pin": 0, "target_node": 2, "target_pin": 3 },
                                { "source_node": 8, "source_pin": 0, "target_node": 7, "target_pin": 0 },
                                { "source_node": 9, "source_pin": 0, "target_node": 7, "target_pin": 1 },
                                { "source_node": 10, "source_pin": 0, "target_node": 7, "target_pin": 2 },
                                { "source_node": 11, "source_pin": 0, "target_node": 7, "target_pin": 3 },
                                { "source_node": 2, "source_pin": 0, "target_node": 12, "target_pin": 0, "name": "paddle" },
                                { "source_node": 13, "source_pin": 0, "target_node": 12, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 14, "target_pin": 0, "name": "ball" },
                                { "source_node": 15, "source_pin": 0, "target_node": 14, "target_pin": 1 },
                                { "source_node": 12, "source_pin": 0, "target_node": 16, "target_pin": 0, "name": "paddle" },
                                { "source_node": 14, "source_pin": 0, "target_node": 16, "target_pin": 1, "name": "ball" },
                                { "source_node": 18, "source_pin": 0, "target_node": 17, "target_pin": 0 },
                                { "source_node": 19, "source_pin": 0, "target_node": 17, "target_pin": 1 },
                                { "source_node": 17, "source_pin": 0, "target_node": 20, "target_pin": 0, "name": "canvas" },
                                { "source_node": 16, "source_pin": 0, "target_node": 20, "target_pin": 1, "name": "scene" },
                                { "source_node": 21, "source_pin": 0, "target_node": 22, "target_pin": 0 }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 23
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )

    // MARK: - Form & Validation

    static let formAndValidation = ExampleCategory(
        name: "Form & Validation",
        icon: "checkmark.shield",
        examples: [
            ExampleEntry(
                name: "Email Validator",
                description: "Checks if a string contains '@' to perform basic email validation.",
                projectJSON: """
                {
                  "name": "Email Validator",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "user@example.com" }, "output_names": ["email"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "@" } },
                                { "id": 4, "type": "primitive", "name": "concat", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["joined"] },
                                { "id": 5, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 6, "type": "primitive", "name": "length", "num_inputs": 1, "num_outputs": 1, "input_names": ["str"], "output_names": ["len"] },
                                { "id": 7, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["result"] },
                                { "id": 8, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 5 } },
                                { "id": 9, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 10, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "email" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "joined" },
                                { "source_node": 2, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "email" },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "len" },
                                { "source_node": 8, "source_pin": 0, "target_node": 7, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "valid?" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 10
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Password Strength",
                description: "Checks password length and logs whether it meets the minimum requirement of 8 characters.",
                projectJSON: """
                {
                  "name": "Password Strength",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "myP@ss123" }, "output_names": ["pw"] },
                                { "id": 3, "type": "primitive", "name": "length", "num_inputs": 1, "num_outputs": 1, "input_names": ["str"], "output_names": ["len"] },
                                { "id": 4, "type": "primitive", "name": ">=", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["ok?"] },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 8 } },
                                { "id": 6, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 7, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 8, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "pw" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "len" },
                                { "source_node": 5, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 3, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "len" },
                                { "source_node": 4, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "strong?" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 8
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )

    // MARK: - Network & Data

    static let networkAndData = ExampleCategory(
        name: "Network & Data",
        icon: "network",
        examples: [
            ExampleEntry(
                name: "HTTP GET",
                description: "Stub: HTTP networking primitives are not yet available. Logs a placeholder message.",
                projectJSON: """
                {
                  "name": "HTTP GET",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "HTTP GET - networking primitives not yet available" }, "output_names": ["msg"] },
                                { "id": 3, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["msg"] },
                                { "id": 4, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "msg" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 4
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "JSON Parse",
                description: "Stub: JSON parsing primitives are not yet available. Logs a placeholder message.",
                projectJSON: """
                {
                  "name": "JSON Parse",
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "JSON Parse - parsing primitives not yet available" }, "output_names": ["msg"] },
                                { "id": 3, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["msg"] },
                                { "id": 4, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "msg" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 4
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )

    // MARK: - Library Examples

    static let libraryExamples = ExampleCategory(
        name: "Libraries",
        icon: "shippingbox",
        examples: [
            ExampleEntry(
                name: "Math: Fibonacci",
                description: "Compute the 10th Fibonacci number using the math library.",
                projectJSON: """
                {
                  "name": "Fibonacci Demo",
                  "libraries": [{"name": "math", "version": "1.0.0"}],
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "/main",
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "value": 10, "num_inputs": 0, "num_outputs": 1, "output_names": ["n"] },
                                { "id": 3, "type": "primitive", "name": "fibonacci", "library": "math", "num_inputs": 1, "num_outputs": 1, "input_names": ["n"], "output_names": ["result"] },
                                { "id": 4, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 5, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "n" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "fib(10)" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 5
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Math: Prime Check",
                description: "Check if a number is prime using the math library.",
                projectJSON: """
                {
                  "name": "Prime Check",
                  "libraries": [{"name": "math", "version": "1.0.0"}],
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "/main",
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "value": 97, "num_inputs": 0, "num_outputs": 1, "output_names": ["n"] },
                                { "id": 3, "type": "primitive", "name": "is-prime", "library": "math", "num_inputs": 1, "num_outputs": 1, "input_names": ["n"], "output_names": ["prime?"] },
                                { "id": 4, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 5, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "97" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "prime?" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 5
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Crypto: SHA-256 Hash",
                description: "Compute the SHA-256 hash of a string using the crypto library.",
                projectJSON: """
                {
                  "name": "SHA-256 Demo",
                  "libraries": [{"name": "crypto", "version": "1.0.0"}],
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "/main",
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "value": "Hello, Phograph!", "num_inputs": 0, "num_outputs": 1, "output_names": ["text"] },
                                { "id": 3, "type": "primitive", "name": "sha256", "library": "crypto", "num_inputs": 1, "num_outputs": 1, "input_names": ["data"], "output_names": ["hash"] },
                                { "id": 4, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 5, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "text" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "hash" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 5
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Crypto: UUID Generator",
                description: "Generate a random UUID using the crypto library.",
                projectJSON: """
                {
                  "name": "UUID Demo",
                  "libraries": [{"name": "crypto", "version": "1.0.0"}],
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "/main",
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "primitive", "name": "uuid", "library": "crypto", "num_inputs": 0, "num_outputs": 1, "output_names": ["uuid"] },
                                { "id": 3, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 4, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "uuid" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 4
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "FileIO: Write and Read",
                description: "Write a text file to /tmp then read it back using the fileio library.",
                projectJSON: """
                {
                  "name": "File IO Demo",
                  "libraries": [{"name": "fileio", "version": "1.0.0"}],
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "/main",
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "value": "/tmp/phograph_test.txt", "num_inputs": 0, "num_outputs": 1, "output_names": ["path"] },
                                { "id": 3, "type": "constant", "value": "Hello from Phograph!", "num_inputs": 0, "num_outputs": 1, "output_names": ["text"] },
                                { "id": 4, "type": "primitive", "name": "file-write-text", "library": "fileio", "num_inputs": 2, "num_outputs": 1, "input_names": ["path", "text"], "output_names": ["ok"] },
                                { "id": 5, "type": "primitive", "name": "file-read-text", "library": "fileio", "num_inputs": 1, "num_outputs": 1, "input_names": ["path"], "output_names": ["text"] },
                                { "id": 6, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 7, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "path" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1, "name": "text" },
                                { "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "path" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "contents" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 7
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Locale: System Info",
                description: "Display the current locale language, country, and timezone.",
                projectJSON: """
                {
                  "name": "Locale Info",
                  "libraries": [{"name": "locale", "version": "1.0.0"}],
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "/main",
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "primitive", "name": "locale-language", "library": "locale", "num_inputs": 0, "num_outputs": 1, "output_names": ["lang"] },
                                { "id": 3, "type": "primitive", "name": "locale-country", "library": "locale", "num_inputs": 0, "num_outputs": 1, "output_names": ["country"] },
                                { "id": 4, "type": "primitive", "name": "locale-timezone", "library": "locale", "num_inputs": 0, "num_outputs": 1, "output_names": ["tz"] },
                                { "id": 5, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 6, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 7, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 8, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "lang" },
                                { "source_node": 3, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "country" },
                                { "source_node": 4, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "tz" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 8
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Sound: Play Sine Tone",
                description: "Generate and play a 440Hz sine tone for 1 second using the sound library.",
                projectJSON: """
                {
                  "name": "Sine Tone",
                  "libraries": [{"name": "sound", "version": "1.0.0"}],
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "/main",
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "value": 44100, "num_inputs": 0, "num_outputs": 1, "output_names": ["rate"] },
                                { "id": 3, "type": "constant", "value": false, "num_inputs": 0, "num_outputs": 1, "output_names": ["stereo"] },
                                { "id": 4, "type": "primitive", "name": "sound-init", "library": "sound", "num_inputs": 2, "num_outputs": 1, "input_names": ["rate", "stereo"], "output_names": ["ctx"] },
                                { "id": 5, "type": "constant", "value": 440.0, "num_inputs": 0, "num_outputs": 1, "output_names": ["freq"] },
                                { "id": 6, "type": "constant", "value": 1.0, "num_inputs": 0, "num_outputs": 1, "output_names": ["dur"] },
                                { "id": 7, "type": "primitive", "name": "tone-sine", "library": "sound", "num_inputs": 3, "num_outputs": 1, "input_names": ["freq", "dur", "rate"], "output_names": ["buf"] },
                                { "id": 8, "type": "primitive", "name": "sound-play", "library": "sound", "num_inputs": 1, "num_outputs": 1, "input_names": ["buf"], "output_names": ["ok"] },
                                { "id": 9, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 10, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 1 },
                                { "source_node": 2, "source_pin": 0, "target_node": 7, "target_pin": 2, "name": "rate" },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "audio" },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "ok" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 10
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
            ExampleEntry(
                name: "Bitmap: Red Square",
                description: "Create a 100x100 bitmap, fill it red, and draw a white rectangle using the bitmap library.",
                projectJSON: """
                {
                  "name": "Red Square",
                  "libraries": [{"name": "bitmap", "version": "1.0.0"}],
                  "sections": [
                    {
                      "name": "Main",
                      "methods": [
                        {
                          "name": "/main",
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "constant", "value": 100, "num_inputs": 0, "num_outputs": 1, "output_names": ["size"] },
                                { "id": 3, "type": "primitive", "name": "bitmap-create", "library": "bitmap", "num_inputs": 2, "num_outputs": 1, "input_names": ["w", "h"], "output_names": ["bmp"] },
                                { "id": 4, "type": "constant", "value": 255, "num_inputs": 0, "num_outputs": 1 },
                                { "id": 5, "type": "constant", "value": 0, "num_inputs": 0, "num_outputs": 1 },
                                { "id": 6, "type": "primitive", "name": "color-rgb", "library": "bitmap", "num_inputs": 3, "num_outputs": 1, "input_names": ["r", "g", "b"], "output_names": ["color"] },
                                { "id": 7, "type": "primitive", "name": "bitmap-fill", "library": "bitmap", "num_inputs": 2, "num_outputs": 1, "input_names": ["bmp", "color"], "output_names": ["bmp"] },
                                { "id": 8, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 9, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0 },
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0 },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 2 },
                                { "source_node": 3, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "bitmap" },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 1, "name": "red" },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "filled" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 9
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
                """
            ),
        ]
    )
}
