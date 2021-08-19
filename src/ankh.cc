#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>

#include <ankh/log.h>
#include <ankh/ankh.h>

#include <ankh/lang/parser.h>
#include <ankh/lang/interpreter.h>
#include <ankh/lang/exceptions.h>

#include <fmt/color.h>

static void print_error(const char *msg) noexcept
{
    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "{}\n", msg);
}

static void print_error(const std::string& msg) noexcept
{
    print_error(msg.c_str());
}

static int execute(ankh::lang::Interpreter& interpreter, const std::string& script) noexcept
{
    const ankh::lang::Program program = ankh::lang::parse(script);
    if (program.has_errors()) {
        for (const auto& e : program.errors) {
           print_error(e);
        }
        return EXIT_FAILURE;
    }

    try {
        interpreter.interpret(program);
    } catch (const ankh::lang::InterpretationException& e) {
        print_error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static std::optional<std::string> read_file(const std::string& path) noexcept
{
    std::FILE *fp = std::fopen(path.c_str(), "r");
    if (fp == nullptr) {
        return std::nullopt;
    }

    // TODO: instead of reading one character at a time, read a whole bunch to increase performance
    std::string result;
    char c;
    while ((c = std::fgetc(fp)) != EOF) {
        result += c;
    }
    
    std::fclose(fp);

    return { result };
}

static std::optional<std::string> readline(const char *prompt) noexcept
{
    std::cout << prompt;
    if (std::string line; std::getline(std::cin, line)) {
        return { line };
    }

    return std::nullopt;
}

int ankh::shell_loop(int argc, char **argv)
{
    ankh::lang::Interpreter interpreter;

    if (argc > 1) {
        if (auto possible_script = read_file(argv[1]); possible_script) {
            return execute(interpreter, possible_script.value());
        }
        
        ankh::log::error("could not open script '%s'\n", argv[1]);

        return EXIT_FAILURE;
    }

    // when our shell exits, we want to ensure it exits
    // with an exit code equivalent to its last process
    int prev_process_exit_code = EXIT_SUCCESS;
    while (true) {
        auto possible_line = readline("> ");
        if (!possible_line.has_value()) {
            ANKH_DEBUG("EOF");
            break;
        }
        
        const std::string line = possible_line.value();
        if (line.empty()) {
            ANKH_DEBUG("empty line");
        } else {
            ANKH_DEBUG("read line: {}", line);
            prev_process_exit_code = execute(interpreter, line);
        }
    }

    return prev_process_exit_code;
}