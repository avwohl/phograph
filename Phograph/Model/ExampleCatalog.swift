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
        objectOriented,
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
            ExampleEntry(
                name: "Temperature Converter",
                description: "Temperature class converts Celsius to Fahrenheit using OOP methods. Result: \"212°F\".",
                projectJSON: """
                {
                  "name": "Temperature Converter",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Temperature",
                          "attributes": [
                            { "name": "value", "default": 0 },
                            { "name": "unit", "default": "C" }
                          ],
                          "methods": [
                            {
                              "name": "to-fahrenheit",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "value", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "value"] },
                                    { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 9 } },
                                    { "id": 4, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["product"] },
                                    { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 5 } },
                                    { "id": 6, "type": "primitive", "name": "/", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["quotient"] },
                                    { "id": 7, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 32 } },
                                    { "id": 8, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                    { "id": 9, "type": "instance_generator", "name": "Temperature", "num_inputs": 0, "num_outputs": 1, "output_names": ["temp"] },
                                    { "id": 10, "type": "set", "name": "value", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 11, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "F" } },
                                    { "id": 12, "type": "set", "name": "unit", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 13, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 0, "name": "value" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                    { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "v*9" },
                                    { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                    { "source_node": 6, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "v*9/5" },
                                    { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 1 },
                                    { "source_node": 9, "source_pin": 0, "target_node": 10, "target_pin": 0, "name": "new" },
                                    { "source_node": 8, "source_pin": 0, "target_node": 10, "target_pin": 1, "name": "°F" },
                                    { "source_node": 10, "source_pin": 0, "target_node": 12, "target_pin": 0, "name": "temp" },
                                    { "source_node": 11, "source_pin": 0, "target_node": 12, "target_pin": 1 },
                                    { "source_node": 12, "source_pin": 0, "target_node": 13, "target_pin": 0, "name": "result" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 13
                                }
                              ]
                            },
                            {
                              "name": "display",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "value", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "value"] },
                                    { "id": 3, "type": "get", "name": "unit", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "unit"] },
                                    { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "\u{00B0}" } },
                                    { "id": 5, "type": "primitive", "name": "concat", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["joined"] },
                                    { "id": 6, "type": "primitive", "name": "concat", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["joined"] },
                                    { "id": 7, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 5, "target_pin": 0, "name": "value" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                    { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "val°" },
                                    { "source_node": 3, "source_pin": 1, "target_node": 6, "target_pin": 1, "name": "unit" },
                                    { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "display" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 7
                                }
                              ]
                            }
                          ]
                        }
                      ],
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
                                { "id": 2, "type": "instance_generator", "name": "Temperature", "num_inputs": 0, "num_outputs": 1, "output_names": ["temp"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 100 } },
                                { "id": 4, "type": "set", "name": "value", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "C" } },
                                { "id": 6, "type": "set", "name": "unit", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 7, "type": "method_call", "name": "Temperature/to-fahrenheit", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["self"] },
                                { "id": 8, "type": "method_call", "name": "Temperature/display", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["text"] },
                                { "id": 9, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "new" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "100" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "100°C" },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "212°F" },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "display" }
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
                description: "Ball class with position, velocity, and render method. Creates a ball and renders one frame on canvas.",
                projectJSON: """
                {
                  "name": "Bouncing Ball",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Ball",
                          "attributes": [
                            { "name": "x", "default": 100 },
                            { "name": "y", "default": 100 },
                            { "name": "dx", "default": 3 },
                            { "name": "dy", "default": 2 },
                            { "name": "color", "default": "green" }
                          ],
                          "methods": [
                            {
                              "name": "move",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "x", "num_inputs": 1, "num_outputs": 2 },
                                    { "id": 3, "type": "get", "name": "dx", "num_inputs": 1, "num_outputs": 2 },
                                    { "id": 4, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                    { "id": 5, "type": "set", "name": "x", "num_inputs": 2, "num_outputs": 1 },
                                    { "id": 6, "type": "get", "name": "y", "num_inputs": 1, "num_outputs": 2 },
                                    { "id": 7, "type": "get", "name": "dy", "num_inputs": 1, "num_outputs": 2 },
                                    { "id": 8, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                    { "id": 9, "type": "set", "name": "y", "num_inputs": 2, "num_outputs": 1 },
                                    { "id": 10, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["self"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 0, "name": "x" },
                                    { "source_node": 3, "source_pin": 1, "target_node": 4, "target_pin": 1, "name": "dx" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "self" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1, "name": "x+dx" },
                                    { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "self" },
                                    { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "self" },
                                    { "source_node": 6, "source_pin": 1, "target_node": 8, "target_pin": 0, "name": "y" },
                                    { "source_node": 7, "source_pin": 1, "target_node": 8, "target_pin": 1, "name": "dy" },
                                    { "source_node": 7, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "self" },
                                    { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 1, "name": "y+dy" },
                                    { "source_node": 9, "source_pin": 0, "target_node": 10, "target_pin": 0, "name": "self" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 10
                                }
                              ]
                            },
                            {
                              "name": "render",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "x", "num_inputs": 1, "num_outputs": 2 },
                                    { "id": 3, "type": "get", "name": "y", "num_inputs": 1, "num_outputs": 2 },
                                    { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 30 } },
                                    { "id": 5, "type": "primitive", "name": "shape-oval", "num_inputs": 4, "num_outputs": 1, "input_names": ["x", "y", "w", "h"], "output_names": ["shape"] },
                                    { "id": 6, "type": "get", "name": "color", "num_inputs": 1, "num_outputs": 2 },
                                    { "id": 7, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1, "input_names": ["shape", "color"], "output_names": ["shape"] },
                                    { "id": 8, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["shape"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 5, "target_pin": 0, "name": "x" },
                                    { "source_node": 3, "source_pin": 1, "target_node": 5, "target_pin": 1, "name": "y" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 2 },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 3 },
                                    { "source_node": 6, "source_pin": 1, "target_node": 7, "target_pin": 1, "name": "color" },
                                    { "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "ball" },
                                    { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "shape" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 8
                                }
                              ]
                            }
                          ]
                        }
                      ],
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "instance_generator", "name": "Ball", "num_inputs": 0, "num_outputs": 1, "output_names": ["ball"] },
                                { "id": 3, "type": "method_call", "name": "Ball/move", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["self"] },
                                { "id": 4, "type": "method_call", "name": "Ball/render", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["shape"] },
                                { "id": 5, "type": "primitive", "name": "create-canvas", "num_inputs": 2, "num_outputs": 1, "input_names": ["w", "h"], "output_names": ["canvas"] },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 7, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 8, "type": "primitive", "name": "canvas-render", "num_inputs": 2, "num_outputs": 0, "input_names": ["canvas", "scene"] },
                                { "id": 9, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "new ball" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "moved" },
                                { "source_node": 6, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 7, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "canvas" },
                                { "source_node": 4, "source_pin": 0, "target_node": 8, "target_pin": 1, "name": "ball shape" }
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
                description: "Creates lists, appends them, sorts the result, and retrieves the first item.",
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
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": [5, 1] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": [3, 2, 4] },
                                { "id": 4, "type": "primitive", "name": "append", "num_inputs": 2, "num_outputs": 1, "input_names": ["list1", "list2"], "output_names": ["list"] },
                                { "id": 5, "type": "primitive", "name": "sort", "num_inputs": 1, "num_outputs": 1, "input_names": ["list"], "output_names": ["sorted"] },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 } },
                                { "id": 7, "type": "primitive", "name": "get-nth", "num_inputs": 2, "num_outputs": 1, "input_names": ["list", "idx"], "output_names": ["elem"] },
                                { "id": 8, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 9, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["first"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "[5,1,3,2,4]" },
                                { "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "sorted" },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "first" },
                                { "source_node": 7, "source_pin": 0, "target_node": 9, "target_pin": 0 }
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
            ExampleEntry(
                name: "Dictionary Phonebook",
                description: "Contact class with display method. Creates contacts, displays each with name and phone number.",
                projectJSON: """
                {
                  "name": "Dictionary Phonebook",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Contact",
                          "attributes": [
                            { "name": "name", "default": "" },
                            { "name": "phone", "default": "" }
                          ],
                          "methods": [
                            {
                              "name": "display",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "name", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "name"] },
                                    { "id": 3, "type": "get", "name": "phone", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "phone"] },
                                    { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": ": " } },
                                    { "id": 5, "type": "primitive", "name": "concat", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["joined"] },
                                    { "id": 6, "type": "primitive", "name": "concat", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["joined"] },
                                    { "id": 7, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 5, "target_pin": 0, "name": "name" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                    { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "name:" },
                                    { "source_node": 3, "source_pin": 1, "target_node": 6, "target_pin": 1, "name": "phone" },
                                    { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "display" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 7
                                }
                              ]
                            }
                          ]
                        }
                      ],
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "instance_generator", "name": "Contact", "num_inputs": 0, "num_outputs": 1, "output_names": ["c"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Alice" } },
                                { "id": 4, "type": "set", "name": "name", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "555-1234" } },
                                { "id": 6, "type": "set", "name": "phone", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 7, "type": "method_call", "name": "/display", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["text"] },
                                { "id": 8, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 9, "type": "instance_generator", "name": "Contact", "num_inputs": 0, "num_outputs": 1, "output_names": ["c"] },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Bob" } },
                                { "id": 11, "type": "set", "name": "name", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 12, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "555-5678" } },
                                { "id": 13, "type": "set", "name": "phone", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 14, "type": "method_call", "name": "/display", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["text"] },
                                { "id": 15, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 16, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "new" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "Alice" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "contact" },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "Alice: 555-1234" },
                                { "source_node": 9, "source_pin": 0, "target_node": 11, "target_pin": 0, "name": "new" },
                                { "source_node": 10, "source_pin": 0, "target_node": 11, "target_pin": 1 },
                                { "source_node": 11, "source_pin": 0, "target_node": 13, "target_pin": 0, "name": "Bob" },
                                { "source_node": 12, "source_pin": 0, "target_node": 13, "target_pin": 1 },
                                { "source_node": 13, "source_pin": 0, "target_node": 14, "target_pin": 0, "name": "contact" },
                                { "source_node": 14, "source_pin": 0, "target_node": 15, "target_pin": 0, "name": "Bob: 555-5678" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 16
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
                name: "Todo List",
                description: "TodoItem class with toggle and display methods. Creates items, toggles one, displays all.",
                projectJSON: """
                {
                  "name": "Todo List",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "TodoItem",
                          "attributes": [
                            { "name": "text", "default": "" },
                            { "name": "done", "default": false }
                          ],
                          "methods": [
                            {
                              "name": "toggle",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "done", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "done"] },
                                    { "id": 3, "type": "primitive", "name": "not", "num_inputs": 1, "num_outputs": 1, "input_names": ["a"], "output_names": ["result"] },
                                    { "id": 4, "type": "set", "name": "done", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 5, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["self"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "done" },
                                    { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "self" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1, "name": "!done" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "self" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 5
                                }
                              ]
                            },
                            {
                              "name": "display",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "done", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "done"] },
                                    { "id": 3, "type": "get", "name": "text", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "text"] },
                                    { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "[x] " } },
                                    { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "[ ] " } },
                                    { "id": 6, "type": "primitive", "name": "if-else", "num_inputs": 3, "num_outputs": 1, "input_names": ["cond", "then", "else"], "output_names": ["result"] },
                                    { "id": 7, "type": "primitive", "name": "concat", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["joined"] },
                                    { "id": 8, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 6, "target_pin": 0, "name": "done?" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                    { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 2 },
                                    { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "prefix" },
                                    { "source_node": 3, "source_pin": 1, "target_node": 7, "target_pin": 1, "name": "text" },
                                    { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "display" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 8
                                }
                              ]
                            }
                          ]
                        }
                      ],
                      "methods": [
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "instance_generator", "name": "TodoItem", "num_inputs": 0, "num_outputs": 1, "output_names": ["item"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Buy milk" } },
                                { "id": 4, "type": "set", "name": "text", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 5, "type": "method_call", "name": "TodoItem/toggle", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["self"] },
                                { "id": 6, "type": "method_call", "name": "TodoItem/display", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["text"] },
                                { "id": 7, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 8, "type": "instance_generator", "name": "TodoItem", "num_inputs": 0, "num_outputs": 1, "output_names": ["item"] },
                                { "id": 9, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Walk dog" } },
                                { "id": 10, "type": "set", "name": "text", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 11, "type": "method_call", "name": "TodoItem/display", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["text"] },
                                { "id": 12, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 13, "type": "instance_generator", "name": "TodoItem", "num_inputs": 0, "num_outputs": 1, "output_names": ["item"] },
                                { "id": 14, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Code" } },
                                { "id": 15, "type": "set", "name": "text", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 16, "type": "method_call", "name": "TodoItem/display", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["text"] },
                                { "id": 17, "type": "primitive", "name": "log", "num_inputs": 1, "num_outputs": 0, "input_names": ["value"] },
                                { "id": 18, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "new" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "item" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "toggled" },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "[x] Buy milk" },
                                { "source_node": 8, "source_pin": 0, "target_node": 10, "target_pin": 0, "name": "new" },
                                { "source_node": 9, "source_pin": 0, "target_node": 10, "target_pin": 1 },
                                { "source_node": 10, "source_pin": 0, "target_node": 11, "target_pin": 0, "name": "item" },
                                { "source_node": 11, "source_pin": 0, "target_node": 12, "target_pin": 0, "name": "[ ] Walk dog" },
                                { "source_node": 13, "source_pin": 0, "target_node": 15, "target_pin": 0, "name": "new" },
                                { "source_node": 14, "source_pin": 0, "target_node": 15, "target_pin": 1 },
                                { "source_node": 15, "source_pin": 0, "target_node": 16, "target_pin": 0, "name": "item" },
                                { "source_node": 16, "source_pin": 0, "target_node": 17, "target_pin": 0, "name": "[ ] Code" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 18
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

    // MARK: - Object-Oriented

    static let objectOriented = ExampleCategory(
        name: "Object-Oriented",
        icon: "hexagon.fill",
        examples: [
            ExampleEntry(
                name: "Counter",
                description: "Creates a Counter object, increments it three times, and reads the count. Result: 3.",
                projectJSON: """
                {
                  "name": "Counter",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Counter",
                          "attributes": [
                            { "name": "count", "default": 0 }
                          ],
                          "methods": [
                            {
                              "name": "increment",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "count", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "count"] },
                                    { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 } },
                                    { "id": 4, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                    { "id": 5, "type": "set", "name": "count", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 6, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["self"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 0, "name": "count" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                    { "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "self" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1, "name": "count+1" },
                                    { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "self" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 6
                                }
                              ]
                            },
                            {
                              "name": "get-count",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "count", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "count"] },
                                    { "id": 3, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "count" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 3
                                }
                              ]
                            }
                          ]
                        }
                      ],
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
                                { "id": 2, "type": "instance_generator", "name": "Counter", "num_inputs": 0, "num_outputs": 1, "output_names": ["counter"] },
                                { "id": 3, "type": "method_call", "name": "Counter/increment", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["self"] },
                                { "id": 4, "type": "method_call", "name": "Counter/increment", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["self"] },
                                { "id": 5, "type": "method_call", "name": "Counter/increment", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["self"] },
                                { "id": 6, "type": "method_call", "name": "Counter/get-count", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["count"] },
                                { "id": 7, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "new" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "+1" },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "+2" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "+3" },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "3" }
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
                name: "Bank Account",
                description: "Creates a BankAccount, sets the owner, deposits twice, and reads the balance. Result: 150.",
                projectJSON: """
                {
                  "name": "Bank Account",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "BankAccount",
                          "attributes": [
                            { "name": "owner", "default": "" },
                            { "name": "balance", "default": 0 }
                          ],
                          "methods": [
                            {
                              "name": "deposit",
                              "num_inputs": 2,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 2, "output_names": ["self", "amount"] },
                                    { "id": 2, "type": "get", "name": "balance", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "balance"] },
                                    { "id": 3, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                    { "id": 4, "type": "set", "name": "balance", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 5, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["self"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "balance" },
                                    { "source_node": 1, "source_pin": 1, "target_node": 3, "target_pin": 1, "name": "amount" },
                                    { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "self" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1, "name": "new balance" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "self" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 5
                                }
                              ]
                            },
                            {
                              "name": "get-balance",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "balance", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "balance"] },
                                    { "id": 3, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "balance" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 3
                                }
                              ]
                            }
                          ]
                        }
                      ],
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
                                { "id": 2, "type": "instance_generator", "name": "BankAccount", "num_inputs": 0, "num_outputs": 1, "output_names": ["account"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Alice" } },
                                { "id": 4, "type": "set", "name": "owner", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 100 } },
                                { "id": 6, "type": "method_call", "name": "BankAccount/deposit", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "amount"], "output_names": ["self"] },
                                { "id": 7, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 50 } },
                                { "id": 8, "type": "method_call", "name": "BankAccount/deposit", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "amount"], "output_names": ["self"] },
                                { "id": 9, "type": "method_call", "name": "BankAccount/get-balance", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["balance"] },
                                { "id": 10, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "new" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "Alice's" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "+100" },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 1 },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "+50" },
                                { "source_node": 9, "source_pin": 0, "target_node": 10, "target_pin": 0, "name": "150" }
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
                name: "Shapes (Inheritance)",
                description: "Shape and Circle classes with inheritance. Dynamic dispatch calls Circle's describe override. Result: \"circle r=5\".",
                projectJSON: """
                {
                  "name": "Shapes (Inheritance)",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Shape",
                          "attributes": [
                            { "name": "name", "default": "shape" }
                          ],
                          "methods": [
                            {
                              "name": "describe",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "name", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "name"] },
                                    { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": " is a shape" } },
                                    { "id": 4, "type": "primitive", "name": "concat", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["joined"] },
                                    { "id": 5, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 0, "name": "name" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "desc" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 5
                                }
                              ]
                            }
                          ]
                        },
                        {
                          "name": "Circle",
                          "parent": "Shape",
                          "attributes": [
                            { "name": "radius", "default": 1 }
                          ],
                          "methods": [
                            {
                              "name": "describe",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "radius", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "radius"] },
                                    { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "circle r=" } },
                                    { "id": 4, "type": "primitive", "name": "concat", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["joined"] },
                                    { "id": 5, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                    { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 1, "name": "radius" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "desc" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 5
                                }
                              ]
                            }
                          ]
                        }
                      ],
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
                                { "id": 2, "type": "instance_generator", "name": "Circle", "num_inputs": 0, "num_outputs": 1, "output_names": ["circle"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "wheel" } },
                                { "id": 4, "type": "set", "name": "name", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 5 } },
                                { "id": 6, "type": "set", "name": "radius", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 7, "type": "method_call", "name": "/describe", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["desc"] },
                                { "id": 8, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "new" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "named" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "r=5" },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "desc" }
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
                name: "Point (Init)",
                description: "Point class with init constructor and distance-squared method. Result: 25.",
                projectJSON: """
                {
                  "name": "Point (Init)",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Point",
                          "attributes": [
                            { "name": "x", "default": 0 },
                            { "name": "y", "default": 0 }
                          ],
                          "methods": [
                            {
                              "name": "init",
                              "num_inputs": 3,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 3, "output_names": ["self", "x", "y"] },
                                    { "id": 2, "type": "set", "name": "x", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 3, "type": "set", "name": "y", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 4, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["self"] }
                                  ],
                                  "wires": [
                                    { "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0, "name": "self" },
                                    { "source_node": 1, "source_pin": 1, "target_node": 2, "target_pin": 1, "name": "x" },
                                    { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0, "name": "self" },
                                    { "source_node": 1, "source_pin": 2, "target_node": 3, "target_pin": 1, "name": "y" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "self" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 4
                                }
                              ]
                            },
                            {
                              "name": "distance-squared",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "x", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "x"] },
                                    { "id": 3, "type": "get", "name": "y", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "y"] },
                                    { "id": 4, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["product"] },
                                    { "id": 5, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["product"] },
                                    { "id": 6, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                    { "id": 7, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 0, "name": "x" },
                                    { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 1, "name": "x" },
                                    { "source_node": 3, "source_pin": 1, "target_node": 5, "target_pin": 0, "name": "y" },
                                    { "source_node": 3, "source_pin": 1, "target_node": 5, "target_pin": 1, "name": "y" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "x\u{00B2}" },
                                    { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1, "name": "y\u{00B2}" },
                                    { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "x\u{00B2}+y\u{00B2}" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 7
                                }
                              ]
                            }
                          ]
                        }
                      ],
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
                                { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 3 }, "output_names": ["x"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 4 }, "output_names": ["y"] },
                                { "id": 4, "type": "instance_generator", "name": "Point", "num_inputs": 2, "num_outputs": 1, "input_names": ["x", "y"], "output_names": ["point"] },
                                { "id": 5, "type": "method_call", "name": "Point/distance-squared", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["d\u{00B2}"] },
                                { "id": 6, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "3" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1, "name": "4" },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "point" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "25" }
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
        ]
    )

    // MARK: - Game

    static let game = ExampleCategory(
        name: "Game",
        icon: "gamecontroller",
        examples: [
            ExampleEntry(
                name: "Pong",
                description: "Playable pong with a bouncing ball. Left paddle follows the pointer; right paddle tracks the ball. Uses a game loop to animate continuously.",
                projectJSON: """
                {
                  "name": "Pong",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Ball",
                          "attributes": [
                            { "name": "x", "default": 200 },
                            { "name": "y", "default": 142 },
                            { "name": "dx", "default": 0.5 },
                            { "name": "dy", "default": 0.35 }
                          ]
                        }
                      ],
                      "methods": [
                        {
                          "name": "game-tick",
                          "num_inputs": 2,
                          "num_outputs": 2,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 2, "output_names": ["ball", "canvas"] },
                                { "id": 2, "type": "get", "name": "x", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 3, "type": "get", "name": "y", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 4, "type": "get", "name": "dx", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 5, "type": "get", "name": "dy", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 0 } },
                                { "id": 7, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 8, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 9, "type": "primitive", "name": "<", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 388 } },
                                { "id": 11, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 12, "type": "primitive", "name": "or", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 13, "type": "primitive", "name": "<", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 14, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 288 } },
                                { "id": 15, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 16, "type": "primitive", "name": "or", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 17, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 18, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 19, "type": "primitive", "name": "if-else", "num_inputs": 3, "num_outputs": 1 },
                                { "id": 20, "type": "primitive", "name": "if-else", "num_inputs": 3, "num_outputs": 1 },
                                { "id": 21, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 22, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 23, "type": "set", "name": "x", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 24, "type": "set", "name": "y", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 25, "type": "set", "name": "dx", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 26, "type": "set", "name": "dy", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 27, "type": "primitive", "name": "input-pointer-position", "num_inputs": 0, "num_outputs": 1 },
                                { "id": 28, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 2 } },
                                { "id": 29, "type": "primitive", "name": "get-nth", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 30, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 12 } },
                                { "id": 31, "type": "primitive", "name": "shape-oval", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 32, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "white" } },
                                { "id": 33, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 34, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 5 } },
                                { "id": 35, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 10 } },
                                { "id": 36, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 60 } },
                                { "id": 37, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 38, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 39, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 385 } },
                                { "id": 40, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 41, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 42, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 199 } },
                                { "id": 43, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 44, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 45, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "gray" } },
                                { "id": 46, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 47, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 48, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 49, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "black" } },
                                { "id": 50, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 51, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 52, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 53, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 54, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 55, "type": "primitive", "name": "canvas-render", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 56, "type": "output_bar", "num_inputs": 2, "num_outputs": 0, "input_names": ["ball", "canvas"] }
                              ],
                              "wires": [
                                { "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0, "name": "ball" },
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 2, "source_pin": 1, "target_node": 7, "target_pin": 0, "name": "x" },
                                { "source_node": 4, "source_pin": 1, "target_node": 7, "target_pin": 1, "name": "dx" },
                                { "source_node": 3, "source_pin": 1, "target_node": 8, "target_pin": 0, "name": "y" },
                                { "source_node": 5, "source_pin": 1, "target_node": 8, "target_pin": 1, "name": "dy" },
                                { "source_node": 7, "source_pin": 0, "target_node": 9, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 9, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 11, "target_pin": 0 },
                                { "source_node": 10, "source_pin": 0, "target_node": 11, "target_pin": 1 },
                                { "source_node": 9, "source_pin": 0, "target_node": 12, "target_pin": 0, "name": "hit-left" },
                                { "source_node": 11, "source_pin": 0, "target_node": 12, "target_pin": 1, "name": "hit-right" },
                                { "source_node": 8, "source_pin": 0, "target_node": 13, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 13, "target_pin": 1 },
                                { "source_node": 8, "source_pin": 0, "target_node": 15, "target_pin": 0 },
                                { "source_node": 14, "source_pin": 0, "target_node": 15, "target_pin": 1 },
                                { "source_node": 13, "source_pin": 0, "target_node": 16, "target_pin": 0, "name": "hit-top" },
                                { "source_node": 15, "source_pin": 0, "target_node": 16, "target_pin": 1, "name": "hit-bot" },
                                { "source_node": 6, "source_pin": 0, "target_node": 17, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 1, "target_node": 17, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 18, "target_pin": 0 },
                                { "source_node": 5, "source_pin": 1, "target_node": 18, "target_pin": 1 },
                                { "source_node": 12, "source_pin": 0, "target_node": 19, "target_pin": 0 },
                                { "source_node": 17, "source_pin": 0, "target_node": 19, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 1, "target_node": 19, "target_pin": 2 },
                                { "source_node": 16, "source_pin": 0, "target_node": 20, "target_pin": 0 },
                                { "source_node": 18, "source_pin": 0, "target_node": 20, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 1, "target_node": 20, "target_pin": 2 },
                                { "source_node": 2, "source_pin": 1, "target_node": 21, "target_pin": 0 },
                                { "source_node": 19, "source_pin": 0, "target_node": 21, "target_pin": 1, "name": "adj-dx" },
                                { "source_node": 3, "source_pin": 1, "target_node": 22, "target_pin": 0 },
                                { "source_node": 20, "source_pin": 0, "target_node": 22, "target_pin": 1, "name": "adj-dy" },
                                { "source_node": 5, "source_pin": 0, "target_node": 23, "target_pin": 0 },
                                { "source_node": 21, "source_pin": 0, "target_node": 23, "target_pin": 1, "name": "new-x" },
                                { "source_node": 23, "source_pin": 0, "target_node": 24, "target_pin": 0 },
                                { "source_node": 22, "source_pin": 0, "target_node": 24, "target_pin": 1, "name": "new-y" },
                                { "source_node": 24, "source_pin": 0, "target_node": 25, "target_pin": 0 },
                                { "source_node": 19, "source_pin": 0, "target_node": 25, "target_pin": 1 },
                                { "source_node": 25, "source_pin": 0, "target_node": 26, "target_pin": 0 },
                                { "source_node": 20, "source_pin": 0, "target_node": 26, "target_pin": 1 },
                                { "source_node": 27, "source_pin": 0, "target_node": 29, "target_pin": 0 },
                                { "source_node": 28, "source_pin": 0, "target_node": 29, "target_pin": 1 },
                                { "source_node": 21, "source_pin": 0, "target_node": 31, "target_pin": 0 },
                                { "source_node": 22, "source_pin": 0, "target_node": 31, "target_pin": 1 },
                                { "source_node": 30, "source_pin": 0, "target_node": 31, "target_pin": 2 },
                                { "source_node": 30, "source_pin": 0, "target_node": 31, "target_pin": 3 },
                                { "source_node": 31, "source_pin": 0, "target_node": 33, "target_pin": 0 },
                                { "source_node": 32, "source_pin": 0, "target_node": 33, "target_pin": 1 },
                                { "source_node": 34, "source_pin": 0, "target_node": 37, "target_pin": 0 },
                                { "source_node": 29, "source_pin": 0, "target_node": 37, "target_pin": 1, "name": "ptr-y" },
                                { "source_node": 35, "source_pin": 0, "target_node": 37, "target_pin": 2 },
                                { "source_node": 36, "source_pin": 0, "target_node": 37, "target_pin": 3 },
                                { "source_node": 37, "source_pin": 0, "target_node": 38, "target_pin": 0 },
                                { "source_node": 32, "source_pin": 0, "target_node": 38, "target_pin": 1 },
                                { "source_node": 39, "source_pin": 0, "target_node": 40, "target_pin": 0 },
                                { "source_node": 22, "source_pin": 0, "target_node": 40, "target_pin": 1, "name": "ball-y" },
                                { "source_node": 35, "source_pin": 0, "target_node": 40, "target_pin": 2 },
                                { "source_node": 36, "source_pin": 0, "target_node": 40, "target_pin": 3 },
                                { "source_node": 40, "source_pin": 0, "target_node": 41, "target_pin": 0 },
                                { "source_node": 32, "source_pin": 0, "target_node": 41, "target_pin": 1 },
                                { "source_node": 42, "source_pin": 0, "target_node": 44, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 44, "target_pin": 1 },
                                { "source_node": 28, "source_pin": 0, "target_node": 44, "target_pin": 2 },
                                { "source_node": 43, "source_pin": 0, "target_node": 44, "target_pin": 3 },
                                { "source_node": 44, "source_pin": 0, "target_node": 46, "target_pin": 0 },
                                { "source_node": 45, "source_pin": 0, "target_node": 46, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 48, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 48, "target_pin": 1 },
                                { "source_node": 47, "source_pin": 0, "target_node": 48, "target_pin": 2 },
                                { "source_node": 43, "source_pin": 0, "target_node": 48, "target_pin": 3 },
                                { "source_node": 48, "source_pin": 0, "target_node": 50, "target_pin": 0 },
                                { "source_node": 49, "source_pin": 0, "target_node": 50, "target_pin": 1 },
                                { "source_node": 50, "source_pin": 0, "target_node": 51, "target_pin": 0 },
                                { "source_node": 46, "source_pin": 0, "target_node": 51, "target_pin": 1 },
                                { "source_node": 51, "source_pin": 0, "target_node": 52, "target_pin": 0 },
                                { "source_node": 38, "source_pin": 0, "target_node": 52, "target_pin": 1 },
                                { "source_node": 52, "source_pin": 0, "target_node": 53, "target_pin": 0 },
                                { "source_node": 41, "source_pin": 0, "target_node": 53, "target_pin": 1 },
                                { "source_node": 53, "source_pin": 0, "target_node": 54, "target_pin": 0 },
                                { "source_node": 33, "source_pin": 0, "target_node": 54, "target_pin": 1 },
                                { "source_node": 1, "source_pin": 1, "target_node": 55, "target_pin": 0, "name": "canvas" },
                                { "source_node": 54, "source_pin": 0, "target_node": 55, "target_pin": 1, "name": "scene" },
                                { "source_node": 26, "source_pin": 0, "target_node": 56, "target_pin": 0, "name": "ball" },
                                { "source_node": 55, "source_pin": 0, "target_node": 56, "target_pin": 1, "name": "canvas" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 56
                            }
                          ]
                        },
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "instance_generator", "name": "Ball", "num_inputs": 0, "num_outputs": 1, "output_names": ["ball"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 5, "type": "primitive", "name": "create-canvas", "num_inputs": 2, "num_outputs": 1, "input_names": ["w", "h"], "output_names": ["canvas"] },
                                { "id": 6, "type": "method_call", "name": "game-tick", "num_inputs": 2, "num_outputs": 2, "annotation": "loop" },
                                { "id": 7, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "ball" },
                                { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1, "name": "canvas" }
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
                name: "Breakout",
                description: "Playable breakout with a bouncing ball and pointer-controlled paddle. Ball bounces off walls and the paddle area. Uses a game loop for continuous animation.",
                projectJSON: """
                {
                  "name": "Breakout",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Ball",
                          "attributes": [
                            { "name": "x", "default": 200 },
                            { "name": "y", "default": 150 },
                            { "name": "dx", "default": 0.4 },
                            { "name": "dy", "default": 0.35 }
                          ]
                        }
                      ],
                      "methods": [
                        {
                          "name": "game-tick",
                          "num_inputs": 2,
                          "num_outputs": 2,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 2, "output_names": ["ball", "canvas"] },
                                { "id": 2, "type": "get", "name": "x", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 3, "type": "get", "name": "y", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 4, "type": "get", "name": "dx", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 5, "type": "get", "name": "dy", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 0 } },
                                { "id": 7, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 8, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 9, "type": "primitive", "name": "<", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 390 } },
                                { "id": 11, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 12, "type": "primitive", "name": "or", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 13, "type": "primitive", "name": "<", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 14, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 290 } },
                                { "id": 15, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 16, "type": "primitive", "name": "or", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 17, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 18, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 19, "type": "primitive", "name": "if-else", "num_inputs": 3, "num_outputs": 1 },
                                { "id": 20, "type": "primitive", "name": "if-else", "num_inputs": 3, "num_outputs": 1 },
                                { "id": 21, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 22, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 23, "type": "set", "name": "x", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 24, "type": "set", "name": "y", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 25, "type": "set", "name": "dx", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 26, "type": "set", "name": "dy", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 27, "type": "primitive", "name": "input-pointer-position", "num_inputs": 0, "num_outputs": 1 },
                                { "id": 28, "type": "primitive", "name": "first", "num_inputs": 1, "num_outputs": 1 },
                                { "id": 29, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 10 } },
                                { "id": 30, "type": "primitive", "name": "shape-oval", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 31, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "white" } },
                                { "id": 32, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 33, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 280 } },
                                { "id": 34, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 80 } },
                                { "id": 35, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 36, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "blue" } },
                                { "id": 37, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 38, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 50 } },
                                { "id": 39, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 20 } },
                                { "id": 40, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 41, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "orange" } },
                                { "id": 42, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 43, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 44, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 45, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 46, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "red" } },
                                { "id": 47, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 48, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 49, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 50, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 51, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "black" } },
                                { "id": 52, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 53, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 54, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 55, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 56, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 57, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 58, "type": "primitive", "name": "canvas-render", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 59, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 100 } },
                                { "id": 60, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 200 } },
                                { "id": 61, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 62, "type": "output_bar", "num_inputs": 2, "num_outputs": 0, "input_names": ["ball", "canvas"] }
                              ],
                              "wires": [
                                { "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0, "name": "ball" },
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 2, "source_pin": 1, "target_node": 7, "target_pin": 0, "name": "x" },
                                { "source_node": 4, "source_pin": 1, "target_node": 7, "target_pin": 1, "name": "dx" },
                                { "source_node": 3, "source_pin": 1, "target_node": 8, "target_pin": 0, "name": "y" },
                                { "source_node": 5, "source_pin": 1, "target_node": 8, "target_pin": 1, "name": "dy" },
                                { "source_node": 7, "source_pin": 0, "target_node": 9, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 9, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 11, "target_pin": 0 },
                                { "source_node": 10, "source_pin": 0, "target_node": 11, "target_pin": 1 },
                                { "source_node": 9, "source_pin": 0, "target_node": 12, "target_pin": 0 },
                                { "source_node": 11, "source_pin": 0, "target_node": 12, "target_pin": 1 },
                                { "source_node": 8, "source_pin": 0, "target_node": 13, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 13, "target_pin": 1 },
                                { "source_node": 8, "source_pin": 0, "target_node": 15, "target_pin": 0 },
                                { "source_node": 14, "source_pin": 0, "target_node": 15, "target_pin": 1 },
                                { "source_node": 13, "source_pin": 0, "target_node": 16, "target_pin": 0 },
                                { "source_node": 15, "source_pin": 0, "target_node": 16, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 17, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 1, "target_node": 17, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 18, "target_pin": 0 },
                                { "source_node": 5, "source_pin": 1, "target_node": 18, "target_pin": 1 },
                                { "source_node": 12, "source_pin": 0, "target_node": 19, "target_pin": 0 },
                                { "source_node": 17, "source_pin": 0, "target_node": 19, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 1, "target_node": 19, "target_pin": 2 },
                                { "source_node": 16, "source_pin": 0, "target_node": 20, "target_pin": 0 },
                                { "source_node": 18, "source_pin": 0, "target_node": 20, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 1, "target_node": 20, "target_pin": 2 },
                                { "source_node": 2, "source_pin": 1, "target_node": 21, "target_pin": 0 },
                                { "source_node": 19, "source_pin": 0, "target_node": 21, "target_pin": 1 },
                                { "source_node": 3, "source_pin": 1, "target_node": 22, "target_pin": 0 },
                                { "source_node": 20, "source_pin": 0, "target_node": 22, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 23, "target_pin": 0 },
                                { "source_node": 21, "source_pin": 0, "target_node": 23, "target_pin": 1 },
                                { "source_node": 23, "source_pin": 0, "target_node": 24, "target_pin": 0 },
                                { "source_node": 22, "source_pin": 0, "target_node": 24, "target_pin": 1 },
                                { "source_node": 24, "source_pin": 0, "target_node": 25, "target_pin": 0 },
                                { "source_node": 19, "source_pin": 0, "target_node": 25, "target_pin": 1 },
                                { "source_node": 25, "source_pin": 0, "target_node": 26, "target_pin": 0 },
                                { "source_node": 20, "source_pin": 0, "target_node": 26, "target_pin": 1 },
                                { "source_node": 27, "source_pin": 0, "target_node": 28, "target_pin": 0 },
                                { "source_node": 21, "source_pin": 0, "target_node": 30, "target_pin": 0 },
                                { "source_node": 22, "source_pin": 0, "target_node": 30, "target_pin": 1 },
                                { "source_node": 29, "source_pin": 0, "target_node": 30, "target_pin": 2 },
                                { "source_node": 29, "source_pin": 0, "target_node": 30, "target_pin": 3 },
                                { "source_node": 30, "source_pin": 0, "target_node": 32, "target_pin": 0 },
                                { "source_node": 31, "source_pin": 0, "target_node": 32, "target_pin": 1 },
                                { "source_node": 28, "source_pin": 0, "target_node": 35, "target_pin": 0, "name": "ptr-x" },
                                { "source_node": 33, "source_pin": 0, "target_node": 35, "target_pin": 1 },
                                { "source_node": 34, "source_pin": 0, "target_node": 35, "target_pin": 2 },
                                { "source_node": 29, "source_pin": 0, "target_node": 35, "target_pin": 3 },
                                { "source_node": 35, "source_pin": 0, "target_node": 37, "target_pin": 0 },
                                { "source_node": 36, "source_pin": 0, "target_node": 37, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 40, "target_pin": 0 },
                                { "source_node": 39, "source_pin": 0, "target_node": 40, "target_pin": 1 },
                                { "source_node": 38, "source_pin": 0, "target_node": 40, "target_pin": 2 },
                                { "source_node": 39, "source_pin": 0, "target_node": 40, "target_pin": 3 },
                                { "source_node": 40, "source_pin": 0, "target_node": 42, "target_pin": 0 },
                                { "source_node": 41, "source_pin": 0, "target_node": 42, "target_pin": 1 },
                                { "source_node": 59, "source_pin": 0, "target_node": 43, "target_pin": 0 },
                                { "source_node": 39, "source_pin": 0, "target_node": 43, "target_pin": 1 },
                                { "source_node": 38, "source_pin": 0, "target_node": 43, "target_pin": 2 },
                                { "source_node": 39, "source_pin": 0, "target_node": 43, "target_pin": 3 },
                                { "source_node": 43, "source_pin": 0, "target_node": 44, "target_pin": 0 },
                                { "source_node": 41, "source_pin": 0, "target_node": 44, "target_pin": 1 },
                                { "source_node": 60, "source_pin": 0, "target_node": 45, "target_pin": 0 },
                                { "source_node": 39, "source_pin": 0, "target_node": 45, "target_pin": 1 },
                                { "source_node": 38, "source_pin": 0, "target_node": 45, "target_pin": 2 },
                                { "source_node": 39, "source_pin": 0, "target_node": 45, "target_pin": 3 },
                                { "source_node": 45, "source_pin": 0, "target_node": 47, "target_pin": 0 },
                                { "source_node": 46, "source_pin": 0, "target_node": 47, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 50, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 50, "target_pin": 1 },
                                { "source_node": 48, "source_pin": 0, "target_node": 50, "target_pin": 2 },
                                { "source_node": 49, "source_pin": 0, "target_node": 50, "target_pin": 3 },
                                { "source_node": 50, "source_pin": 0, "target_node": 52, "target_pin": 0 },
                                { "source_node": 51, "source_pin": 0, "target_node": 52, "target_pin": 1 },
                                { "source_node": 52, "source_pin": 0, "target_node": 53, "target_pin": 0 },
                                { "source_node": 37, "source_pin": 0, "target_node": 53, "target_pin": 1 },
                                { "source_node": 53, "source_pin": 0, "target_node": 54, "target_pin": 0 },
                                { "source_node": 42, "source_pin": 0, "target_node": 54, "target_pin": 1 },
                                { "source_node": 54, "source_pin": 0, "target_node": 55, "target_pin": 0 },
                                { "source_node": 44, "source_pin": 0, "target_node": 55, "target_pin": 1 },
                                { "source_node": 55, "source_pin": 0, "target_node": 56, "target_pin": 0 },
                                { "source_node": 47, "source_pin": 0, "target_node": 56, "target_pin": 1 },
                                { "source_node": 56, "source_pin": 0, "target_node": 57, "target_pin": 0 },
                                { "source_node": 32, "source_pin": 0, "target_node": 57, "target_pin": 1 },
                                { "source_node": 1, "source_pin": 1, "target_node": 58, "target_pin": 0 },
                                { "source_node": 57, "source_pin": 0, "target_node": 58, "target_pin": 1 },
                                { "source_node": 26, "source_pin": 0, "target_node": 62, "target_pin": 0 },
                                { "source_node": 58, "source_pin": 0, "target_node": 62, "target_pin": 1 }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 62
                            }
                          ]
                        },
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "instance_generator", "name": "Ball", "num_inputs": 0, "num_outputs": 1, "output_names": ["ball"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 300 } },
                                { "id": 5, "type": "primitive", "name": "create-canvas", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 6, "type": "method_call", "name": "game-tick", "num_inputs": 2, "num_outputs": 2, "annotation": "loop" },
                                { "id": 7, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "ball" },
                                { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1, "name": "canvas" }
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
                name: "Snake",
                description: "Animated snake that moves continuously across the screen with a food target. The green snake head bounces off walls while the red food stays fixed. Uses a game loop.",
                projectJSON: """
                {
                  "name": "Snake",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Snake",
                          "attributes": [
                            { "name": "x", "default": 100 },
                            { "name": "y", "default": 200 },
                            { "name": "dx", "default": 0.4 },
                            { "name": "dy", "default": 0.15 },
                            { "name": "size", "default": 20 }
                          ]
                        }
                      ],
                      "methods": [
                        {
                          "name": "game-tick",
                          "num_inputs": 2,
                          "num_outputs": 2,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 2, "output_names": ["snake", "canvas"] },
                                { "id": 2, "type": "get", "name": "x", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 3, "type": "get", "name": "y", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 4, "type": "get", "name": "dx", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 5, "type": "get", "name": "dy", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 0 } },
                                { "id": 7, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 8, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 9, "type": "primitive", "name": "<", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 380 } },
                                { "id": 11, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 12, "type": "primitive", "name": "or", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 13, "type": "primitive", "name": "<", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 14, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 380 } },
                                { "id": 15, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 16, "type": "primitive", "name": "or", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 17, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 18, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 19, "type": "primitive", "name": "if-else", "num_inputs": 3, "num_outputs": 1 },
                                { "id": 20, "type": "primitive", "name": "if-else", "num_inputs": 3, "num_outputs": 1 },
                                { "id": 21, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 22, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 23, "type": "set", "name": "x", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 24, "type": "set", "name": "y", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 25, "type": "set", "name": "dx", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 26, "type": "set", "name": "dy", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 27, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 20 } },
                                { "id": 28, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 29, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "green" } },
                                { "id": 30, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 31, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 240 } },
                                { "id": 32, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 180 } },
                                { "id": 33, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 15 } },
                                { "id": 34, "type": "primitive", "name": "shape-oval", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 35, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "red" } },
                                { "id": 36, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 37, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 38, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 39, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "black" } },
                                { "id": 40, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 41, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 42, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 43, "type": "primitive", "name": "canvas-render", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 44, "type": "output_bar", "num_inputs": 2, "num_outputs": 0, "input_names": ["snake", "canvas"] }
                              ],
                              "wires": [
                                { "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0 },
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 2, "source_pin": 1, "target_node": 7, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 1, "target_node": 7, "target_pin": 1 },
                                { "source_node": 3, "source_pin": 1, "target_node": 8, "target_pin": 0 },
                                { "source_node": 5, "source_pin": 1, "target_node": 8, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 9, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 9, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 11, "target_pin": 0 },
                                { "source_node": 10, "source_pin": 0, "target_node": 11, "target_pin": 1 },
                                { "source_node": 9, "source_pin": 0, "target_node": 12, "target_pin": 0 },
                                { "source_node": 11, "source_pin": 0, "target_node": 12, "target_pin": 1 },
                                { "source_node": 8, "source_pin": 0, "target_node": 13, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 13, "target_pin": 1 },
                                { "source_node": 8, "source_pin": 0, "target_node": 15, "target_pin": 0 },
                                { "source_node": 14, "source_pin": 0, "target_node": 15, "target_pin": 1 },
                                { "source_node": 13, "source_pin": 0, "target_node": 16, "target_pin": 0 },
                                { "source_node": 15, "source_pin": 0, "target_node": 16, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 17, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 1, "target_node": 17, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 18, "target_pin": 0 },
                                { "source_node": 5, "source_pin": 1, "target_node": 18, "target_pin": 1 },
                                { "source_node": 12, "source_pin": 0, "target_node": 19, "target_pin": 0 },
                                { "source_node": 17, "source_pin": 0, "target_node": 19, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 1, "target_node": 19, "target_pin": 2 },
                                { "source_node": 16, "source_pin": 0, "target_node": 20, "target_pin": 0 },
                                { "source_node": 18, "source_pin": 0, "target_node": 20, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 1, "target_node": 20, "target_pin": 2 },
                                { "source_node": 2, "source_pin": 1, "target_node": 21, "target_pin": 0 },
                                { "source_node": 19, "source_pin": 0, "target_node": 21, "target_pin": 1 },
                                { "source_node": 3, "source_pin": 1, "target_node": 22, "target_pin": 0 },
                                { "source_node": 20, "source_pin": 0, "target_node": 22, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 23, "target_pin": 0 },
                                { "source_node": 21, "source_pin": 0, "target_node": 23, "target_pin": 1 },
                                { "source_node": 23, "source_pin": 0, "target_node": 24, "target_pin": 0 },
                                { "source_node": 22, "source_pin": 0, "target_node": 24, "target_pin": 1 },
                                { "source_node": 24, "source_pin": 0, "target_node": 25, "target_pin": 0 },
                                { "source_node": 19, "source_pin": 0, "target_node": 25, "target_pin": 1 },
                                { "source_node": 25, "source_pin": 0, "target_node": 26, "target_pin": 0 },
                                { "source_node": 20, "source_pin": 0, "target_node": 26, "target_pin": 1 },
                                { "source_node": 21, "source_pin": 0, "target_node": 28, "target_pin": 0 },
                                { "source_node": 22, "source_pin": 0, "target_node": 28, "target_pin": 1 },
                                { "source_node": 27, "source_pin": 0, "target_node": 28, "target_pin": 2 },
                                { "source_node": 27, "source_pin": 0, "target_node": 28, "target_pin": 3 },
                                { "source_node": 28, "source_pin": 0, "target_node": 30, "target_pin": 0 },
                                { "source_node": 29, "source_pin": 0, "target_node": 30, "target_pin": 1 },
                                { "source_node": 31, "source_pin": 0, "target_node": 34, "target_pin": 0 },
                                { "source_node": 32, "source_pin": 0, "target_node": 34, "target_pin": 1 },
                                { "source_node": 33, "source_pin": 0, "target_node": 34, "target_pin": 2 },
                                { "source_node": 33, "source_pin": 0, "target_node": 34, "target_pin": 3 },
                                { "source_node": 34, "source_pin": 0, "target_node": 36, "target_pin": 0 },
                                { "source_node": 35, "source_pin": 0, "target_node": 36, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 38, "target_pin": 0 },
                                { "source_node": 6, "source_pin": 0, "target_node": 38, "target_pin": 1 },
                                { "source_node": 37, "source_pin": 0, "target_node": 38, "target_pin": 2 },
                                { "source_node": 37, "source_pin": 0, "target_node": 38, "target_pin": 3 },
                                { "source_node": 38, "source_pin": 0, "target_node": 40, "target_pin": 0 },
                                { "source_node": 39, "source_pin": 0, "target_node": 40, "target_pin": 1 },
                                { "source_node": 40, "source_pin": 0, "target_node": 41, "target_pin": 0 },
                                { "source_node": 36, "source_pin": 0, "target_node": 41, "target_pin": 1 },
                                { "source_node": 41, "source_pin": 0, "target_node": 42, "target_pin": 0 },
                                { "source_node": 30, "source_pin": 0, "target_node": 42, "target_pin": 1 },
                                { "source_node": 1, "source_pin": 1, "target_node": 43, "target_pin": 0 },
                                { "source_node": 42, "source_pin": 0, "target_node": 43, "target_pin": 1 },
                                { "source_node": 26, "source_pin": 0, "target_node": 44, "target_pin": 0 },
                                { "source_node": 43, "source_pin": 0, "target_node": 44, "target_pin": 1 }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 44
                            }
                          ]
                        },
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "instance_generator", "name": "Snake", "num_inputs": 0, "num_outputs": 1, "output_names": ["snake"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 5, "type": "primitive", "name": "create-canvas", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 6, "type": "method_call", "name": "game-tick", "num_inputs": 2, "num_outputs": 2, "annotation": "loop" },
                                { "id": 7, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 6, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 }
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
                name: "Asteroid Dodge",
                description: "Dodge falling rocks by moving the ship with the pointer. The rock falls from the top and wraps back when it reaches the bottom. Uses a game loop for animation.",
                projectJSON: """
                {
                  "name": "Asteroid Dodge",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Rock",
                          "attributes": [
                            { "name": "x", "default": 180 },
                            { "name": "y", "default": 0 },
                            { "name": "speed", "default": 0.25 },
                            { "name": "radius", "default": 15 }
                          ]
                        }
                      ],
                      "methods": [
                        {
                          "name": "game-tick",
                          "num_inputs": 2,
                          "num_outputs": 2,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 2, "output_names": ["rock", "canvas"] },
                                { "id": 2, "type": "get", "name": "y", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 3, "type": "get", "name": "speed", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 4, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 400 } },
                                { "id": 6, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 7, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "real", "value": 0 } },
                                { "id": 8, "type": "primitive", "name": "if-else", "num_inputs": 3, "num_outputs": 1 },
                                { "id": 9, "type": "set", "name": "y", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 10, "type": "get", "name": "x", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 11, "type": "get", "name": "radius", "num_inputs": 1, "num_outputs": 2 },
                                { "id": 12, "type": "primitive", "name": "shape-oval", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 13, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 30 } },
                                { "id": 14, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "gray" } },
                                { "id": 15, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 16, "type": "primitive", "name": "input-pointer-position", "num_inputs": 0, "num_outputs": 1 },
                                { "id": 17, "type": "primitive", "name": "first", "num_inputs": 1, "num_outputs": 1 },
                                { "id": 18, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 350 } },
                                { "id": 19, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 30 } },
                                { "id": 20, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 20 } },
                                { "id": 21, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 22, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "cyan" } },
                                { "id": 23, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 24, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 25, "type": "primitive", "name": "shape-rect", "num_inputs": 4, "num_outputs": 1 },
                                { "id": 26, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "black" } },
                                { "id": 27, "type": "primitive", "name": "shape-set-fill", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 28, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 29, "type": "primitive", "name": "shape-group", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 30, "type": "primitive", "name": "canvas-render", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 31, "type": "output_bar", "num_inputs": 2, "num_outputs": 0, "input_names": ["rock", "canvas"] }
                              ],
                              "wires": [
                                { "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0, "name": "rock" },
                                { "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0 },
                                { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 0, "name": "y" },
                                { "source_node": 3, "source_pin": 1, "target_node": 4, "target_pin": 1, "name": "speed" },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0 },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 6, "source_pin": 0, "target_node": 8, "target_pin": 0 },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 8, "target_pin": 2, "name": "new-y" },
                                { "source_node": 3, "source_pin": 0, "target_node": 9, "target_pin": 0 },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 1, "name": "wrapped-y" },
                                { "source_node": 9, "source_pin": 0, "target_node": 10, "target_pin": 0 },
                                { "source_node": 10, "source_pin": 0, "target_node": 11, "target_pin": 0 },
                                { "source_node": 10, "source_pin": 1, "target_node": 12, "target_pin": 0, "name": "rock-x" },
                                { "source_node": 8, "source_pin": 0, "target_node": 12, "target_pin": 1, "name": "rock-y" },
                                { "source_node": 13, "source_pin": 0, "target_node": 12, "target_pin": 2 },
                                { "source_node": 13, "source_pin": 0, "target_node": 12, "target_pin": 3 },
                                { "source_node": 12, "source_pin": 0, "target_node": 15, "target_pin": 0 },
                                { "source_node": 14, "source_pin": 0, "target_node": 15, "target_pin": 1 },
                                { "source_node": 16, "source_pin": 0, "target_node": 17, "target_pin": 0 },
                                { "source_node": 17, "source_pin": 0, "target_node": 21, "target_pin": 0, "name": "ship-x" },
                                { "source_node": 18, "source_pin": 0, "target_node": 21, "target_pin": 1 },
                                { "source_node": 19, "source_pin": 0, "target_node": 21, "target_pin": 2 },
                                { "source_node": 20, "source_pin": 0, "target_node": 21, "target_pin": 3 },
                                { "source_node": 21, "source_pin": 0, "target_node": 23, "target_pin": 0 },
                                { "source_node": 22, "source_pin": 0, "target_node": 23, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 25, "target_pin": 0 },
                                { "source_node": 7, "source_pin": 0, "target_node": 25, "target_pin": 1 },
                                { "source_node": 24, "source_pin": 0, "target_node": 25, "target_pin": 2 },
                                { "source_node": 24, "source_pin": 0, "target_node": 25, "target_pin": 3 },
                                { "source_node": 25, "source_pin": 0, "target_node": 27, "target_pin": 0 },
                                { "source_node": 26, "source_pin": 0, "target_node": 27, "target_pin": 1 },
                                { "source_node": 27, "source_pin": 0, "target_node": 28, "target_pin": 0 },
                                { "source_node": 15, "source_pin": 0, "target_node": 28, "target_pin": 1 },
                                { "source_node": 28, "source_pin": 0, "target_node": 29, "target_pin": 0 },
                                { "source_node": 23, "source_pin": 0, "target_node": 29, "target_pin": 1 },
                                { "source_node": 1, "source_pin": 1, "target_node": 30, "target_pin": 0 },
                                { "source_node": 29, "source_pin": 0, "target_node": 30, "target_pin": 1 },
                                { "source_node": 11, "source_pin": 0, "target_node": 31, "target_pin": 0, "name": "rock" },
                                { "source_node": 30, "source_pin": 0, "target_node": 31, "target_pin": 1, "name": "canvas" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 31
                            }
                          ]
                        },
                        {
                          "name": "main",
                          "num_inputs": 0,
                          "num_outputs": 0,
                          "cases": [
                            {
                              "nodes": [
                                { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                                { "id": 2, "type": "instance_generator", "name": "Rock", "num_inputs": 0, "num_outputs": 1, "output_names": ["rock"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 400 } },
                                { "id": 4, "type": "primitive", "name": "create-canvas", "num_inputs": 2, "num_outputs": 1 },
                                { "id": 5, "type": "method_call", "name": "game-tick", "num_inputs": 2, "num_outputs": 2, "annotation": "loop" },
                                { "id": 6, "type": "output_bar", "num_inputs": 0, "num_outputs": 0 }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1 }
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
            ExampleEntry(
                name: "Shopping Cart",
                description: "Product and Cart classes. Adds products to a cart and reads the total. Result: 35.",
                projectJSON: """
                {
                  "name": "Shopping Cart",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Product",
                          "attributes": [
                            { "name": "name", "default": "" },
                            { "name": "price", "default": 0 }
                          ],
                          "methods": []
                        },
                        {
                          "name": "Cart",
                          "attributes": [
                            { "name": "total", "default": 0 },
                            { "name": "count", "default": 0 }
                          ],
                          "methods": [
                            {
                              "name": "add-item",
                              "num_inputs": 2,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 2, "output_names": ["self", "product"] },
                                    { "id": 2, "type": "get", "name": "total", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "total"] },
                                    { "id": 3, "type": "get", "name": "price", "num_inputs": 1, "num_outputs": 2, "input_names": ["product"], "output_names": ["product", "price"] },
                                    { "id": 4, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                    { "id": 5, "type": "set", "name": "total", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 6, "type": "get", "name": "count", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "count"] },
                                    { "id": 7, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 1 } },
                                    { "id": 8, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["sum"] },
                                    { "id": 9, "type": "set", "name": "count", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 10, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["self"] }
                                  ],
                                  "wires": [
                                    { "source_node": 1, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "product" },
                                    { "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 0, "name": "total" },
                                    { "source_node": 3, "source_pin": 1, "target_node": 4, "target_pin": 1, "name": "price" },
                                    { "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "self" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1, "name": "new total" },
                                    { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "self" },
                                    { "source_node": 6, "source_pin": 1, "target_node": 8, "target_pin": 0, "name": "count" },
                                    { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 1 },
                                    { "source_node": 6, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "self" },
                                    { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 1, "name": "count+1" },
                                    { "source_node": 9, "source_pin": 0, "target_node": 10, "target_pin": 0, "name": "self" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 10
                                }
                              ]
                            },
                            {
                              "name": "get-total",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "total", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "total"] },
                                    { "id": 3, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "total" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 3
                                }
                              ]
                            }
                          ]
                        }
                      ],
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
                                { "id": 2, "type": "instance_generator", "name": "Product", "num_inputs": 0, "num_outputs": 1, "output_names": ["product"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Widget" } },
                                { "id": 4, "type": "set", "name": "name", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 25 } },
                                { "id": 6, "type": "set", "name": "price", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 7, "type": "instance_generator", "name": "Product", "num_inputs": 0, "num_outputs": 1, "output_names": ["product"] },
                                { "id": 8, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "Gadget" } },
                                { "id": 9, "type": "set", "name": "name", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 10, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 10 } },
                                { "id": 11, "type": "set", "name": "price", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 12, "type": "instance_generator", "name": "Cart", "num_inputs": 0, "num_outputs": 1, "output_names": ["cart"] },
                                { "id": 13, "type": "method_call", "name": "Cart/add-item", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "product"], "output_names": ["self"] },
                                { "id": 14, "type": "method_call", "name": "Cart/add-item", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "product"], "output_names": ["self"] },
                                { "id": 15, "type": "method_call", "name": "Cart/get-total", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["total"] },
                                { "id": 16, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "new" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0, "name": "named" },
                                { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "new" },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 1 },
                                { "source_node": 9, "source_pin": 0, "target_node": 11, "target_pin": 0, "name": "named" },
                                { "source_node": 10, "source_pin": 0, "target_node": 11, "target_pin": 1 },
                                { "source_node": 12, "source_pin": 0, "target_node": 13, "target_pin": 0, "name": "cart" },
                                { "source_node": 6, "source_pin": 0, "target_node": 13, "target_pin": 1, "name": "widget" },
                                { "source_node": 13, "source_pin": 0, "target_node": 14, "target_pin": 0, "name": "+widget" },
                                { "source_node": 11, "source_pin": 0, "target_node": 14, "target_pin": 1, "name": "gadget" },
                                { "source_node": 14, "source_pin": 0, "target_node": 15, "target_pin": 0, "name": "+gadget" },
                                { "source_node": 15, "source_pin": 0, "target_node": 16, "target_pin": 0, "name": "35" }
                              ],
                              "input_bar_id": 1,
                              "output_bar_id": 16
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
                name: "Validator",
                description: "Validator class chains validation checks on a value. Result: false (too short).",
                projectJSON: """
                {
                  "name": "Validator",
                  "sections": [
                    {
                      "name": "Main",
                      "classes": [
                        {
                          "name": "Validator",
                          "attributes": [
                            { "name": "value", "default": "" },
                            { "name": "valid", "default": true }
                          ],
                          "methods": [
                            {
                              "name": "check-not-empty",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "value", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "value"] },
                                    { "id": 3, "type": "primitive", "name": "length", "num_inputs": 1, "num_outputs": 1, "input_names": ["str"], "output_names": ["len"] },
                                    { "id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 0 } },
                                    { "id": 5, "type": "primitive", "name": ">", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["result"] },
                                    { "id": 6, "type": "get", "name": "valid", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "valid"] },
                                    { "id": 7, "type": "primitive", "name": "and", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["result"] },
                                    { "id": 8, "type": "set", "name": "valid", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 9, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["self"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "value" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "len" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1 },
                                    { "source_node": 6, "source_pin": 1, "target_node": 7, "target_pin": 0, "name": "valid" },
                                    { "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 1, "name": "not-empty?" },
                                    { "source_node": 6, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "self" },
                                    { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 1, "name": "valid" },
                                    { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "self" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 9
                                }
                              ]
                            },
                            {
                              "name": "check-min-length",
                              "num_inputs": 2,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 2, "output_names": ["self", "min"] },
                                    { "id": 2, "type": "get", "name": "value", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "value"] },
                                    { "id": 3, "type": "primitive", "name": "length", "num_inputs": 1, "num_outputs": 1, "input_names": ["str"], "output_names": ["len"] },
                                    { "id": 4, "type": "primitive", "name": ">=", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["result"] },
                                    { "id": 5, "type": "get", "name": "valid", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "valid"] },
                                    { "id": 6, "type": "primitive", "name": "and", "num_inputs": 2, "num_outputs": 1, "input_names": ["a", "b"], "output_names": ["result"] },
                                    { "id": 7, "type": "set", "name": "valid", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                    { "id": 8, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["self"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "value" },
                                    { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "len" },
                                    { "source_node": 1, "source_pin": 1, "target_node": 4, "target_pin": 1, "name": "min" },
                                    { "source_node": 5, "source_pin": 1, "target_node": 6, "target_pin": 0, "name": "valid" },
                                    { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 1, "name": "long enough?" },
                                    { "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "self" },
                                    { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 1, "name": "valid" },
                                    { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "self" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 8
                                }
                              ]
                            },
                            {
                              "name": "is-valid",
                              "num_inputs": 1,
                              "num_outputs": 1,
                              "cases": [
                                {
                                  "nodes": [
                                    { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 1, "output_names": ["self"] },
                                    { "id": 2, "type": "get", "name": "valid", "num_inputs": 1, "num_outputs": 2, "input_names": ["self"], "output_names": ["self", "valid"] },
                                    { "id": 3, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                                  ],
                                  "wires": [
                                    { "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0, "name": "valid?" }
                                  ],
                                  "input_bar_id": 1,
                                  "output_bar_id": 3
                                }
                              ]
                            }
                          ]
                        }
                      ],
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
                                { "id": 2, "type": "instance_generator", "name": "Validator", "num_inputs": 0, "num_outputs": 1, "output_names": ["v"] },
                                { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "string", "value": "hi" } },
                                { "id": 4, "type": "set", "name": "value", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "value"], "output_names": ["self"] },
                                { "id": 5, "type": "method_call", "name": "Validator/check-not-empty", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["self"] },
                                { "id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 5 } },
                                { "id": 7, "type": "method_call", "name": "Validator/check-min-length", "num_inputs": 2, "num_outputs": 1, "input_names": ["self", "min"], "output_names": ["self"] },
                                { "id": 8, "type": "method_call", "name": "Validator/is-valid", "num_inputs": 1, "num_outputs": 1, "input_names": ["self"], "output_names": ["valid?"] },
                                { "id": 9, "type": "output_bar", "num_inputs": 1, "num_outputs": 0, "input_names": ["result"] }
                              ],
                              "wires": [
                                { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0, "name": "new" },
                                { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                                { "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0, "name": "v(hi)" },
                                { "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 0, "name": "checked" },
                                { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 1 },
                                { "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0, "name": "checked" },
                                { "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 0, "name": "false" }
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
