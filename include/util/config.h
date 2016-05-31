#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>

#include <functional>
#include <string>

namespace Config {
    enum Error {
        NotScalar = 1,
        NotSequence,
        NotMap,
        BadParse,
        BadTypeConversion,
        AttributeMissing
    };
    extern const std::map<Error, std::string> error_names;

    typedef std::function<void(const Error, const std::string&, const YAML::Node&)> ErrorFn;
    typedef const std::function<void(ErrorFn, const YAML::Node&)> Def;
    typedef std::vector<Def> Defs;

    namespace detail {
        void default_error_handler (const Error err, const std::string& name, const YAML::Node& node);

        template <typename... Args>
        inline void def (Defs& definitions, Def& definition, Args... defs) {
            def(definitions, definition);
            def(definitions, defs...);
        }

        template <>
        inline void def (Defs& definitions, Def& definition) {
            definitions.push_back(definition);
        }

        template <typename... Definitions>
        Def make_parser (Definitions... defs) {
            Defs def_fns = Defs{};
            def(def_fns, defs...);
            return [def_fns](ErrorFn error_cb, const YAML::Node& node) {
                for (auto fn : def_fns) {
                    fn(error_cb, node);
                }
            };
        }

        template <typename... Definitions>
        struct Parser {
            static inline std::function<void(const YAML::Node&)> make (Definitions... defs) {
                return Parser<ErrorFn, Definitions...>::make(default_error_handler, defs...);
            }
        };

        template <typename... Definitions>
        struct Parser<ErrorFn, Definitions...> {
            static inline std::function<void(const YAML::Node&)> make (ErrorFn error_cb, Definitions... defs) {
                auto parse = detail::make_parser(defs...);
                return [parse, error_cb](const YAML::Node& node) {
                    parse(error_cb, node);
                };
            }
        };
    }

    /**
     * make_parser ( attribute definitions... )
     * make_parser ( error_callback, attribute definitions... )
     *
     * Create a parser that is capable of parsing YAML data as specified
     * by the attribute definitions.
     *
     * If error_callback is provided, it will be called when a parsing error occurs.
     * error_callback must be of the type ErrorFn (and any lambda should be cast to this
     * type). ErrorFn is a void function that takes three const arguments:
     *
     * 	void error_callback (const Config::Error, const std::string& node_name, const YAML::Node node);
     *
     * If no error_callback is provided, errors will be written to stderr.
     *
     *
     * Example:
     * 	struct { int num; } foo;
     * 	auto parser = Config::make_parser(
     * 		Config::scalar("num", foo.num)
     * 	);
     * 	parser(YAML::Load("num: 5"));
     * 	// foo.num == 5
     */
    template <typename... Args>
    std::function<void(const YAML::Node&)> make_parser (Args... args) {
        return detail::Parser<Args...>::make(args...);
    }

    /**
     * scalar (attribute_name, output_reference)
     *
     * Parse a scalar attribute named by attribute_name into the
     * reference variable output_reference.
     * The value named by attribute_name must be convertable into
     * the data type of output_reference or a parse error occurs.
     *
     * Example:
     * 	struct { int num; std::string str; } foo;
     * 	auto parser = Config::make_parser(
     * 		Config::scalar("num", foo.num),
     * 		Config::scalar("str", foo.str)
     * 	);
     * 	parser(YAML::Load("num: 5\nstr: hello"));
     */
    template <typename Type>
    Def scalar (const std::string& name, Type& output) {
        return [name, &output](ErrorFn error, const YAML::Node& input) mutable {
            YAML::Node node = input[name];
            if (node.IsScalar()) {
                try {
                    output = node.as<Type>();
                } catch (...) {
                    error(BadTypeConversion, name, node);
                }
            } else {
                error(NotScalar, name, node);
            }
        };
    }

    /**
     * sequence (attribute_name, output_reference)
     *
     * Parse a sequence attribute named by attribute_name into the
     * reference variable output_reference.
     * The value named by attribute_name must be convertable into
     * the data type of output_reference or a parse error occurs.
     *
     * Example:
     * 	struct { std::vector<int> nums; std::vector<std::string> strs; } foo;
     * 	auto parser = Config::make_parser(
     * 		Config::sequence("nums", foo.nums),
     * 		Config::sequence("strs", foo.strs)
     * 	);
     * 	parser(YAML::Load("nums: [1, 2, 3]\nstrs:\n  - hello\n  - world"));
     */
    template <typename Type>
    Def sequence (const std::string& name, Type& output) {
        return [name, &output](ErrorFn error, const YAML::Node& input) mutable {
            YAML::Node node = input[name];
            if (node.IsSequence()) {
                try {
                    output = node.as<Type>();
                } catch (...) {
                    error(BadTypeConversion, name, node);
                }
            } else {
                error(NotSequence, name, node);
            }
        };
    }

    /**
     * sequence (output_reference)
     *
     * Parse a sequence into the reference variable output_reference.
     * The value named by attribute_name must be convertable into
     * the data type of output_reference or a parse error occurs.
     *
     * This is typically not used alone, but rather as the nested
     * sequence parser when parsing sequences that are nested inside
     * other sequences.
     * Generally used with the sequence(attr, tempref, outref, nested)
     * variant below, as the value for `nested`.
     *
     * Example:
     * 	struct { std::vector<std::vector<int>> nums; } foo;
     * 	std::vector<int> temp;
     * 	auto parser = Config::make_parser(
     * 		Config::sequence("nums",
     * 			temp, foo.nums, // <-- This packs temp into foo.nums for each outer seq
     * 			Config::sequence(temp) // <-- This parses the nested seqs into temp
     * 		)
     * 	);
     * 	parser(YAML::Load("nums:\n  - [1, 2, 3]\n  - [4, 5, 6]"));
     */
    template <typename Type>
    Def sequence (Type& output) {
        return [&output](ErrorFn error, const YAML::Node& input) mutable {
            if (input.IsSequence()) {
                try {
                    output = input.as<Type>();
                } catch (...) {
                    error(BadTypeConversion, "[...]", input);
                }
            } else {
                error(NotSequence, "[...]", input);
            }
        };
    }

    /**
     * sequence (attribute_name, temp_reference, output_reference, attribute definitions...)
     *
     * Parse a sequence of nested attribute definitions, named by attribute_name, into output_reference.
     * This is used to parse sequences of nested data (sequences of maps or sequences of sequences; not
     * required for sequences of scalars - use the sequence(attribute_name, output_reference) variant
     * instead.
     * Each element in the sequence named by attribute_name is parsed into temp and then before moving
     * onto the next element, temp is written to output_reference using output_reference.push_back(temp_reference)
     * That is, temp must refer to a type capable of holding a single element and is used to temporarily
     * store the element before it is added to output_reference.
     *
     * Example:
     * 	struct Nested { int a; std::string b; };
     * 	struct { std::vector<Nested> data; } foo;
     * 	Nested temp;
     * 	auto parser = Config::make_parser(
     * 		Config::sequence("data",
     * 			temp, foo.data,
     * 			Config::scalar("a", temp.a),
     * 			Config::scalar("b", temp.b)
     * 		)
     * 	);
     * 	parser(YAML::Load("data:\n  - a: 1\n    b: hello\n  - a: 2\n    b: world"));
     *
     */
    template <typename Type, typename... Children>
    Def sequence (const std::string& name, Type& temp, std::vector<Type>& output, Children... children) {
        auto parse = detail::make_parser(children...);
        return [name, parse, &temp, &output](ErrorFn error, const YAML::Node& input) mutable {
            YAML::Node node = input[name];
            if (node.IsSequence()) {
                for (auto child : node) {
                    try {
                        parse(error, child);
                        output.push_back(temp);
                    } catch (...) {
                        error(BadParse, name, child);
                    }
                }
            } else {
                error(NotSequence, name, node);
            }
        };
    }

    /**
     * map (attribute_name, attribute definitions...)
     *
     * Parse a map (key/value pair) named by attribute_name.
     *
     * Example:
     * 	struct { struct { int a; std::string b; } bar; } foo;
     * 	auto parser = Config::make_parser(
     * 		Config::map("bar",
     * 			Config::scalar("a", foo.bar.a),
     * 			Config::scalar("b", foo.bar.b)
     * 		)
     * 	);
     * 	parser(YAML::Load("bar:\n   a: 1\n  b: hello"));
     */
    template <typename... Children>
    Def map (const std::string& name, Children... children) {
        auto parse = detail::make_parser(children...);
        return [name, parse](ErrorFn error, const YAML::Node& input) mutable {
            YAML::Node node = input[name];
            if (node.IsMap()) {
                try {
                    parse(error, node);
                } catch (...) {
                    error(AttributeMissing, name, node);
                }
            } else {
                error(NotMap, name, node);
            }
        };
    }

    /**
     * Custom parsing:
     *
     * You can get the raw YAML::Node at any point in the tree by passing
     * a funciton (or lambda) that conforms to Def in place of any attribute definition.
     *
     * Example:
     * auto parser = Config::make_parser(
     * 		[](YAML::Node node){
     * 			std::cout << node.IsSequence()
     * 		}
     * 	);
     * 	parser(...)
     */
}
#endif // CONFIG_H
