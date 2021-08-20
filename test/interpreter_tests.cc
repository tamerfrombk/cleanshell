#include <catch/catch.hpp>

#include <initializer_list>
#include <string>
#include <memory>
#include <vector>

#include <ankh/lang/expr.h>
#include <ankh/lang/statement.h>
#include <ankh/lang/program.h>
#include <ankh/lang/parser.h>
#include <ankh/lang/interpreter.h>

#include <ankh/def.h>

class TracingInterpreter
    : public ankh::lang::Interpreter
{
public:
    TracingInterpreter(std::unique_ptr<ankh::lang::Interpreter> interp)
        : interp_(std::move(interp)) {}

    virtual ankh::lang::ExprResult evaluate(const ankh::lang::ExpressionPtr& expr) override
    {
        ankh::lang::ExprResult result = Interpreter::evaluate(expr);

        results_.push_back(result);

        return result;
    }

    const std::vector<ankh::lang::ExprResult>& results() const noexcept
    {
        return results_;
    }

    ANKH_NO_DISCARD bool has_function(const std::string& name) const noexcept
    {
        return functions().count(name) == 1;
    }

private:
    std::unique_ptr<ankh::lang::Interpreter> interp_;
    std::vector<ankh::lang::ExprResult> results_;
};

struct ExecutionResult
{
    ankh::lang::Program program;
    std::vector<ankh::lang::ExprResult> results;    
};

ExecutionResult interpret(TracingInterpreter& interpreter, const std::string& source)
{
    ankh::lang::Program program = ankh::lang::parse(source);
    if (program.has_errors()) {
        return { std::move(program), {} };
    }

    interpreter.interpret(program);

    return { std::move(program), interpreter.results() };
}

TEST_CASE("primary expressions", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("literals")
    {
        const std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "123", { 123.0 } }
            , { "\"here is a string\"", { std::string("here is a string") } }
            , { "true", { true } }
            , { "false", { false } }
            , { "nil", {} }
        };

        for (const auto& [source, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, source);

            REQUIRE(!program.has_errors());
            REQUIRE(interpreter.functions().size() == 0);

            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            switch (expected_result.type) {
            case ankh::lang::ExprResultType::RT_NUMBER: REQUIRE(expected_result.n == actual_result.n);     break;
            case ankh::lang::ExprResultType::RT_STRING: REQUIRE(expected_result.str == actual_result.str); break;
            case ankh::lang::ExprResultType::RT_BOOL:   REQUIRE(expected_result.b == actual_result.b);     break;
            case ankh::lang::ExprResultType::RT_NIL:                                                       break;
            default: FAIL("unknown ExprResultType not accounted for");
            };
        }
    }

    SECTION("strings, substitution expression")
    {
        const std::string source =
        R"(
            let a = "lol"
            "the value of a is {a}"
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(actual_result.str == "the value of a is lol");
    }

    SECTION("strings, substitution expression, missing closing brace")
    {
        const std::string source =
        R"(
            let a = "lol"
            "the value of a is {a"
        )";

        REQUIRE_THROWS(interpret(interpreter, source));
    }

    SECTION("strings, substitution expression, missing opening brace")
    {
        const std::string source =
        R"(
            let a = "lol"
            "the value of a is a}"
        )";

        REQUIRE_THROWS(interpret(interpreter, source));
    }

    SECTION("strings, substitution expression, non-string expression")
    {
        const std::string source =
        R"(
            "the value is {1 == 2}"
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(actual_result.str == "the value is false");
    }

    SECTION("strings, substitution expression, raw braces")
    {
        const std::string source =
        R"(
            let a = 1 > 2
            "the value is \{\} {a}"
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(actual_result.str == "the value is {} false");
    }

    SECTION("strings, substitution expression, multi")
    {
        const std::string source =
        R"(
            "the value is {true || false} is { true }"
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(actual_result.str == "the value is true is true");
    }

    SECTION("lambda, rvalue")
    {
        const std::string source = R"(
            let function = fn (a, b) {
                return a + b
            }
        )";
        
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.functions().size() == 1);

        ankh::lang::ExprResult identifier = results[0];
        REQUIRE(identifier.type == ankh::lang::ExprResultType::RT_CALLABLE);
    }

    SECTION("command")
    {
        const std::string source = R"(
            let result = $(echo hello)
        )";
        
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult identifier = results[0];
        REQUIRE(identifier.type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(identifier.str == "hello\n");
    }

    SECTION("command, piping")
    {
        const std::string source = R"(
            let result = $(echo hello | tr -s 'h' "j")
        )";
        
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult identifier = results[0];
        REQUIRE(identifier.type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(identifier.str == "jello\n");
    }

    SECTION("parenthetic expression")
    {
        const std::string source = R"(
            let result = ( 1 + 2 )
        )";
        
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult identifier = results.back();
        REQUIRE(identifier.type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(identifier.n == 3);
    }
}

TEST_CASE("call expressions", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("function call, non-recursive")
    {
        const std::string source = R"(
            fn foo(str) {
                return str
            }

            foo("foobar")
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.has_function("foo"));

        REQUIRE(results.size() == 3);

        REQUIRE(results[0].type == ankh::lang::ExprResultType::RT_CALLABLE);
        
        REQUIRE(results[1].type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(results[1].str == "foobar");
       
        REQUIRE(results[2].type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(results[2].str == "foobar");
    }

    SECTION("function call, recursive")
    {
        const std::string source = R"(
            fn fib(n) {
                # base case
                if n <= 1 { return n }

                # general case
                return fib(n - 2) + fib(n - 1)
            }

            fib(3)
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.has_function("fib"));

        ankh::lang::ExprResult result = results.back();
        REQUIRE(result.type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(result.n == 2);
    }

    SECTION("function call, no return statement -- should return nil")
    {
        const std::string source = R"(
            fn foo() {
                "bar"
            }

            foo()
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.has_function("foo"));

        ankh::lang::ExprResult result = results.back();
        REQUIRE(result.type == ankh::lang::ExprResultType::RT_NIL);
    }
    
    SECTION("lambda call")
    {
        const std::string source = R"(
            let f = fn (a, b) {
                return a + b
            }

            f("a", "b")
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().contains("f"));

        ankh::lang::ExprResult result = results.back();
        REQUIRE(result.type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(result.str == "ab");
    }

    SECTION("lambda call, no return statement -- should return nil")
    {
        const std::string source = R"(
            let f = fn (a, b) {
                a + b
            }

            f("a", "b")
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().contains("f"));

        ankh::lang::ExprResult result = results.back();
        REQUIRE(result.type == ankh::lang::ExprResultType::RT_NIL);
    }
}

TEST_CASE("unary expressions", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("operator (!), boolean")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "!true", { false } }
            , { "!false", { true } }
            , { "!(1 == 2)", { true } }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("operator (!), non-boolean")
    {
        const std::string source = R"(
            !9
        )";

        REQUIRE_THROWS(interpret(interpreter, source));
    }

    SECTION("operator (-), number")
    {
        const std::string source = R"(
            -2
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult result = results.back();
        REQUIRE(result.type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(result.n == -2);
    }

    SECTION("operator (-), non-number")
    {
        const std::string source = R"(
            -"what"
        )";

        REQUIRE_THROWS(interpret(interpreter, source));
    }
}

TEST_CASE("PEMDAS", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("factors, numbers")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "4 / 2", ankh::lang::Number{ 2 } }
            , { "4.2 / 2", ankh::lang::Number{ 2.1 } }
            , { "6 / (1 + 1)", ankh::lang::Number{ 3 } }
            , { "4 * 3", ankh::lang::Number{ 12 } }
            , { "2 * 8.3", ankh::lang::Number{ 16.6 } }
            , { "(2 * 3) / 2", ankh::lang::Number{ 3 } }
            , { "12 / 3 * 2", ankh::lang::Number{ 8 } }
            , { "12 * 3 / 2", ankh::lang::Number{ 18 } }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.n == expected_result.n);
        }
    }    
    
    SECTION("factors, non-numbers")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "true / 2", {} }
            , { "\"fwat\" / 2", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("terms, numbers")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "1 - 2", ankh::lang::Number{ -1 } }
            , { "2 + 5.4", ankh::lang::Number{ 7.4 } }
            , { "1 - 2 + 3", ankh::lang::Number{ 2 } }
            , { "2 + 3 - 1", ankh::lang::Number{ 4 } }
            , { "1 - 3 + 2", ankh::lang::Number{ 0 } }
            , { "7 - (2 + 3)", ankh::lang::Number{ 2 } }
            , { "7 + (2 - 3)", ankh::lang::Number{ 6 } }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.n == expected_result.n);
        }
    }    
    
    SECTION("terms, non-number")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "1 + true", {} }
            , { "\"fwat\" - true", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("terms, string operator(+)")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "\"foo\" + \"bar\"", std::string{"foobar"} }
            , { "\"\" + \"huh\"", std::string{"huh"} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.str == expected_result.str);
        }
    }

    SECTION("interleaved terms and factors")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "1 + 2 * 3", ankh::lang::Number{ 7 } }
            , { "24 / (3 * 4)", ankh::lang::Number{ 2 } }
            , { "8 + 2 * 3 / 2", ankh::lang::Number{ 11 } }
            , { "(1 - (2 * 3)) * 2 * (21 / 7)", ankh::lang::Number{ -24 } }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.str == expected_result.str);
        }
    }

    SECTION("divide by zero")
    {
        REQUIRE_THROWS(interpret(interpreter, "3 / 0"));
    }
}

TEST_CASE("ordering", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("comparison")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "2 > 1", true }
            , { "2 < 3", true }
            , { "1 >= 1", true }
            , { "5 <= 4", false }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("comparison, non-numbers")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "2 > \"foo\"", {} }
            , { "2 < true", {} }
            , { "1 >= \"\"", {} }
            , { "5 <= def () {}", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("equality")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "1 != 2", true }
            , { "3 == 2", false }
            , { "true == true", true}
            , { "false != true", true}
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("equality, non-numbers")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "1 != \"foo\"", {} }
            , { "false == 9.1", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }
}

TEST_CASE("boolean", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("and")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "true && true", true }
            , { "true && false", false }
            , { "false && false", false }
            , { "false && true", false }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("and, non-boolean")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "2 && \"foo\"", {} }
            , { "true && -1", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("and, strict evaluation")
    {
        const std::string source = R"(
            let count = 0

            fn update() {
                count = count + 1
                return count
            }

            if update() > 0 && update() < 0 {
                print "yay"
            } else {
                print "nay"
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().value("count").value().n == 2);
    }

    SECTION("or")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "true || true", true }
            , { "true || false", true }
            , { "false || true", true}
            , { "false || false", false}
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("or, non-boolean")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "1 || \"foo\"", {} }
            , { "false || 9.1", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("or, strict evaluation")
    {
        const std::string source = R"(
            let count = 0

            fn update() {
                count = count + 1
                return count
            }

            if update() > 0 || update() < 0 {
                print "yay"
            } else {
                print "nay"
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().value("count").value().n == 2);
    }
}

TEST_CASE("for statement interpretation", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("for-loop, 3 components")
    {
        const std::string source = R"(
            let result = 0
            for let i = 0; i < 2; ++i {
                result = result + 1
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!interpreter.environment().value("i"));
        REQUIRE(interpreter.environment().value("result")->n == 2.0);
    }

    SECTION("for-loop, init missing")
    {
        const std::string source = R"(
            let result = 0
            for ; result < 2; ++result {}
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(interpreter.environment().value("result")->n == 2.0);
    }

    SECTION("for-loop, mutator missing")
    {
        const std::string source = R"(
            let result = 0
            for let i = 0; i < 2; {
                ++result
                ++i
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!interpreter.environment().value("i"));
        REQUIRE(interpreter.environment().value("result")->n == 2.0);
    }

    SECTION("for-loop, infinite")
    {
        const std::string source = R"(
            let result = 0
            for {
                if result == 2 {
                    break
                }
                ++result
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(interpreter.environment().value("result")->n == 2.0);
    }
}

TEST_CASE("while statement interpretation", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("while-loop")
    {
        const std::string source = R"(
            let result = 0
            while result != 2 {
                ++result
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(interpreter.environment().value("result")->n == 2.0);
    }

    SECTION("while-loop, infinite")
    {
        const std::string source = R"(
            let result = 0
            while true {
                if result == 2 {
                    break
                }
                ++result
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(interpreter.environment().value("result")->n == 2.0);
    }
}

TEST_CASE("top level break statement disallowed", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    const std::string source = R"(
        break
    )";

    INFO(source);
    
    REQUIRE_THROWS(interpret(interpreter, source));
}

TEST_CASE("increment/decrement statements", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("increment, identifier")
    {
        const std::string source = R"(
            let i = 0
            ++i
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().value("i")->n == 1.0);
    }

    SECTION("increment, access")
    {
        const std::string source = R"(
            data Point { x y }
            
            let p = Point(1, 2)

            ++p.x

            let result = p.x
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().value("result")->n == 2.0);
    }

    SECTION("increment, unallowed expression")
    {
        const std::string source = R"(
            data Point { x y }
            
            let p = Point(1, 2)

            ++p
        )";

        INFO(source);
        REQUIRE_THROWS(interpret(interpreter, source));
    }

    SECTION("decrement, identifier")
    {
        const std::string source = R"(
            let i = 0
            --i
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().value("i")->n == -1.0);
    }

    SECTION("decrement, access")
    {
        const std::string source = R"(
            data Point { x y }
            
            let p = Point(1, 2)

            --p.x

            let result = p.x
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().value("result")->n == 0.0);
    }

    SECTION("decrement, unallowed expression")
    {
        const std::string source = R"(
            data Point { x y }
            
            let p = Point(1, 2)

            --p
        )";

        INFO(source);
        REQUIRE_THROWS(interpret(interpreter, source));
    }
}

TEST_CASE("arrays", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("array expressions")
    {
        std::unordered_map<std::string, ankh::lang::ExprResult> src_to_expected_result = {
            { "[1, 2]", ankh::lang::Array(std::vector<ankh::lang::ExprResult>{ankh::lang::Number{1}, ankh::lang::Number{2}}) }
            , { "[]", ankh::lang::Array(std::vector<ankh::lang::ExprResult>{}) }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            ankh::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.array == expected_result.array);
        }
    }

    SECTION("index expressions")
    {
        const std::string source = R"(
            let a = [1, 2]
            a[0] + a[1]
        )";

        INFO(source);
        
        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(actual_result.n == 3);
    }

    SECTION("index expressions, out of range")
    {
        const std::string source = R"(
            let a = [1, 2]
            a[3]
        )";

        INFO(source);
        
        REQUIRE_THROWS(interpret(interpreter, source));
    }
}

TEST_CASE("assignments", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("assignment")
    {
        const std::string source = "let i = 0; i = 1";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(interpreter.environment().value("i")->n == 1.0);
    }

    SECTION("assignment, non-existent")
    {
        const std::string source = "let i = 0; x = 1";

        INFO(source);

        REQUIRE_THROWS(interpret(interpreter, source));
    }
}

TEST_CASE("compound assignments", "[interpreter]")
{
    std::unordered_map<std::string, ankh::lang::Number> srcToExpected = {
          { "let i = 0; i += 3", 3.0 }
        , { "let i = 0; i -= 3", -3.0 }
        , { "let i = 1; i *= 3", 3.0 }
        , { "let i = 6; i /= 3", 2.0 }
    };

    for (const auto& [source, expected]: srcToExpected) {
        INFO(source);

        TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(interpreter.environment().value("i")->n == expected);
    }
}

TEST_CASE("compound modify", "[interpreter]")
{
    const std::string source_template = R"(
        data Point {
            x y
        }

        let p = Point(6, 2)

        p.x OP 3

        let result = p.x
    )";

    std::unordered_map<std::string, ankh::lang::Number> opToResult = {
        { "+=", 9.0 }
        , { "-=", 3.0 }
        , { "*=", 18.0 }
        , { "/=", 2.0 }
    };

    const size_t op_pos = source_template.find("OP");
    for (const auto& [op, expected] : opToResult) {
        // This next line is why people hate C++ >_>
        auto temp = source_template; 
        const std::string source = temp.replace(op_pos, op.length(), op);

        INFO(source);

        TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(interpreter.environment().value("result")->n == expected);
    }
}

TEST_CASE("dicts", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("dict declaration")
    {
        const std::string source = R"(
            let a = {
                a: "b",
                c: 2,
                d: []
            }
        )";

        INFO(source);
        
        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == ankh::lang::ExprResultType::RT_DICT);

        std::initializer_list<std::string> keys{"a", "c", "d"};
        for (const auto& k : keys) {
            REQUIRE(actual_result.dict.value(k)->key.type == ankh::lang::ExprResultType::RT_STRING);
        }
    }

    SECTION("dict declaration, key expression")
    {
        const std::string source = R"(
            let a = {
                ["a" + "b"]: "abc"
            }
        )";

        INFO(source);
        
        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == ankh::lang::ExprResultType::RT_DICT);

        REQUIRE(actual_result.dict.value(std::string{"ab"})->key.type == ankh::lang::ExprResultType::RT_STRING);  
        REQUIRE(actual_result.dict.value(std::string{"ab"})->value.str == "abc");      
    }

    SECTION("dict lookup, string")
    {
        const std::string source = R"(
            let a = {
                f: "g"
            }

            a["f"]
        )";

        INFO(source);
        
        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        ankh::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == ankh::lang::ExprResultType::RT_STRING);
        REQUIRE(actual_result.str == "g");    
    }

    SECTION("dict lookup, non-string")
    {
        const std::string source = R"(
            let a = {
                f: "g"
            }

            a[x]
        )";

        INFO(source);
        REQUIRE_THROWS(interpret(interpreter, source));    
    }
}

TEST_CASE("strings, substitution expression, nested", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    const std::string source =
    R"(
        "the value is {{expression will be unevaluated}}"
    )";

    REQUIRE_THROWS(interpret(interpreter, source));
}

TEST_CASE("if statements")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("if, no else, positive")
    {
        const std::string source = R"(
            let a = false
            if 2 > 1 {
                a = true
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_BOOL);
        REQUIRE(interpreter.environment().value("a")->b == true);
    }

    SECTION("if, no else, negative")
    {
        const std::string source = R"(
            let a = false
            if 1 > 2 {
                a = true
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_BOOL);
        REQUIRE(interpreter.environment().value("a")->b == false);
    }

    SECTION("if, with else, positive")
    {
        const std::string source = R"(
            let a = 0
            if 1 < 2 {
                a = 1
            } else {
                a = 2
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("a")->n == 1);
    }

    SECTION("if, with else, negative")
    {
        const std::string source = R"(
            let a = 0
            if 1 > 2 {
                a = 1
            } else {
                a = 2
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("a")->n == 2);
    }

    SECTION("if, with else-if, positive, no else")
    {
        const std::string source = R"(
            let a = 0
            if 1 > 2 {
                a = 1
            } else if 2 == 2 {
                a = 2
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("a")->n == 2);
    }

    SECTION("if, with else-if, negative, no else")
    {
        const std::string source = R"(
            let a = 0
            if 1 > 2 {
                a = 1
            } else if 2 > 3 {
                a = 2
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("a")->n == 0);
    }

    SECTION("if, with else-if, positive, including else")
    {
        const std::string source = R"(
            let a = 0
            if 1 > 2 {
                a = 1
            } else if 2 < 3 {
                a = 2
            } else {
                a = 3
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("a")->n == 2);
    }

    SECTION("if, with else-if, negative, including else")
    {
        const std::string source = R"(
            let a = 0
            if 1 > 2 {
                a = 1
            } else if 2 > 3 {
                a = 2
            } else {
                a = 3
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("a")->n == 3);
    }

    SECTION("if, with else-if, fallthrough")
    {
        const std::string source = R"(
            let a = 0
            if a == 1 {
                a = 2
            } else if a == 2 {
                a = 3
            } else if a == 3 {
                a = 3
            } else {
                a = 4
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("a")->n == 4);
    }
}

TEST_CASE("declarations")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("let declaration")
    {
        const std::string source = R"(
            let a = 0;
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("a")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("a")->n == 0);
    }

    SECTION("let declaration, no initializer")
    {
        const std::string source = R"(
            let a;
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(program.has_errors());
    }

    SECTION("export declaration")
    {
        const std::string source = R"(
            export LANGUAGE_DEV_TEST_123 = 0;
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("LANGUAGE_DEV_TEST_123")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("LANGUAGE_DEV_TEST_123")->n == 0);

        const std::string value(getenv("LANGUAGE_DEV_TEST_123"));
        REQUIRE(value == "0.000000");
    }

    SECTION("export declaration, no initializer")
    {
        const std::string source = R"(
            export a;
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(program.has_errors());
    }

    SECTION("export declaration, assignment does not change environment value")
    {
        const std::string source = R"(
            export LANGUAGE_DEV_TEST_123 = 0;
            LANGUAGE_DEV_TEST_123 = 1
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("LANGUAGE_DEV_TEST_123")->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.environment().value("LANGUAGE_DEV_TEST_123")->n == 1.0);

        const std::string value(getenv("LANGUAGE_DEV_TEST_123"));
        REQUIRE(value == "0.000000");
    }

    SECTION("data declaration")
    {
        const std::string source = R"(
            data Point {
                x y
            }
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.has_function("Point"));

        REQUIRE(!interpreter.environment().contains("x"));
        REQUIRE(!interpreter.environment().contains("y"));
    }

    SECTION("data declaration, constructor")
    {
        const std::string source = R"(
            data Point {
                x y
            }

            let d = Point(1, 2)
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("d")->type == ankh::lang::ExprResultType::RT_OBJECT);
    }

    SECTION("data declaration, redeclaration")
    {
        const std::string source = R"(
            data Point {
                x y
            }

            data Point {
                z
            }
        )";

        INFO(source);

        REQUIRE_THROWS(interpret(interpreter, source));
    }
}

TEST_CASE("test access", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("access expression, not a member")
    {
        const std::string source = R"(
            data Point {
                x y
            }

            let d = Point(1, 2)

            d.z
        )";

        INFO(source);

        REQUIRE_THROWS(interpret(interpreter, source));
    }

    SECTION("access expression, data")
    {
        const std::string source = R"(
            data Point {
                x y
            }

            let d = Point(1, 2)

            let b = d.x + d.y
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        auto possible_b = interpreter.environment().value("b");
        REQUIRE(possible_b->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(possible_b->n == 3.0);
    }

    SECTION("access expression, inner data member")
    {
        const std::string source = R"(
            data A {
                a
            }

            data B {
                a
            }

            let a = A(3)
            let b = B(a)

            let result = b.a.a
        )";

        INFO(source);

        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        auto possible_result = interpreter.environment().value("result");
        REQUIRE(possible_result->type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(possible_result->n == 3.0);
    }
}

TEST_CASE("modify expressions", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    SECTION("test accessing data member")
    {
        const std::string source = R"(
            data Point {
                x y
            }

            let p = Point(1, 2)

            let x = p.x

            p.x = 3

            p.x
        )";

        INFO(source);


        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        REQUIRE(interpreter.environment().value("x")->n == 1.0);
        REQUIRE(interpreter.results().back().type == ankh::lang::ExprResultType::RT_NUMBER);
        REQUIRE(interpreter.results().back().n == 3.0);
    }

    SECTION("test accessing member, non-existent")
    {
        const std::string source = R"(
            data Point {
                x y
            }

            let p = Point(1, 2)

            p.z = 3
        )";

        INFO(source);

        REQUIRE_THROWS(interpret(interpreter, source));
    }

    SECTION("test accessing non-data")
    {
        const std::string source = R"(
            let p = 3

            p.z = "foo"
        )";

        INFO(source);

        REQUIRE_THROWS(interpret(interpreter, source));
    }

}

TEST_CASE("constructor environment overrides each other", "[interpreter][!mayfail]")
{
    const std::string source = R"(
        data Point {
            x y
        }

        data Line {
            p1 p2
        }

        let d = Line(Point(1, 2), Point(3, 4))

        let x1 = d.p1.x
        let y1 = d.p1.y
        let x2 = d.p2.x
        let y2 = d.p2.y
    )";

    INFO(source);

    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    auto [program, results] = interpret(interpreter, source);
    REQUIRE(!program.has_errors());

    REQUIRE(interpreter.environment().value("x1")->n == 1.0);
    REQUIRE(interpreter.environment().value("y1")->n == 2.0);
    REQUIRE(interpreter.environment().value("x2")->n == 3.0);
    REQUIRE(interpreter.environment().value("y2")->n == 4.0);
}

TEST_CASE("lambdas can be shadowed because they are scope bound", "[interpreter][!mayfail]")
{
    const std::string source = R"(
        let f = fn (a, b) { return a + b }

        fn addAll(a, b, c) {
            let f = fn (x) { return f(a, b) + x }
            
            return f(f(a, b), c)
        }
    )";

    INFO(source);

    TracingInterpreter interpreter(std::make_unique<ankh::lang::Interpreter>());

    auto [program, results] = interpret(interpreter, source);
    REQUIRE(!program.has_errors());

    REQUIRE(interpreter.functions().size() == 1);
}
