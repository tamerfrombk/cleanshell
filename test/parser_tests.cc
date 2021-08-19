#include <catch/catch.hpp>

#include <string>
#include <vector>

#include <ankh/lang/token.h>
#include <ankh/lang/exceptions.h>
#include <ankh/lang/expr.h>
#include <ankh/lang/statement.h>
#include <ankh/lang/lambda.h>
#include <ankh/lang/parser.h>

static void test_binary_expression_parse(const std::string& op) noexcept
{
    const std::string source("1" + op + "2" + "\n");
    
    auto program = ankh::lang::parse(source);
    REQUIRE(program.size() == 1);

    auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);

    auto binary = ankh::lang::instance<ankh::lang::BinaryExpression>(stmt->expr);
    REQUIRE(binary != nullptr);
    REQUIRE(binary->left != nullptr);
    REQUIRE(binary->right != nullptr);
    REQUIRE(binary->op.str == op);
}

static void test_boolean_binary_expression(const std::string& op) noexcept
{
    const std::string source("true" + op + "false" + "\n");
    
    auto program = ankh::lang::parse(source);
    REQUIRE(program.size() == 1);

    auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);

    auto binary = ankh::lang::instance<ankh::lang::BinaryExpression>(stmt->expr);
    REQUIRE(binary != nullptr);
    REQUIRE(binary->left != nullptr);
    REQUIRE(binary->right != nullptr);
}

static void test_unary_expression(const std::string& op) noexcept
{
    const std::string source(op + "3" + "\n");

    auto program = ankh::lang::parse(source);

    REQUIRE(program.size() == 1);

    auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);
    
    auto unary = ankh::lang::instance<ankh::lang::UnaryExpression>(stmt->expr);
    REQUIRE(unary != nullptr);
    REQUIRE(unary->right != nullptr);
    REQUIRE(unary->op.str == op);
}

TEST_CASE("parse language statements", "[parser]")
{
    SECTION("parse expression statement")
    {
        const std::string source =
        R"(
            1 + 2
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        auto binary = ankh::lang::instance<ankh::lang::BinaryExpression>(stmt->expr);
        REQUIRE(binary != nullptr);
    }

    SECTION("parse declaration statement, local storage")
    {
        const std::string source =
        R"(
            let i = 1
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto declaration = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(declaration != nullptr);

        REQUIRE(declaration->name.str == "i");

        auto literal = ankh::lang::instance<ankh::lang::LiteralExpression>(declaration->initializer);
        REQUIRE(literal != nullptr);

        REQUIRE(literal->literal.str == "1");
    }

    SECTION("parse declaration statement, export storage")
    {
        const std::string source =
        R"(
            export i = 1
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto declaration = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(declaration != nullptr);

        REQUIRE(declaration->name.str == "i");

        auto literal = ankh::lang::instance<ankh::lang::LiteralExpression>(declaration->initializer);
        REQUIRE(literal != nullptr);

        REQUIRE(literal->literal.str == "1");
    }

    SECTION("parse assignment statement")
    {
        const std::string source =
        R"(
            let i = 2
            i = 3
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 2);

        auto assignment = ankh::lang::instance<ankh::lang::AssignmentStatement>(program[1]);
        REQUIRE(assignment != nullptr);

        REQUIRE(assignment->name.str == "i");

        auto literal = ankh::lang::instance<ankh::lang::LiteralExpression>(assignment->initializer);
        REQUIRE(literal != nullptr);

        REQUIRE(literal->literal.str == "3");
    }

    SECTION("parse compound assignment statement")
    {
        std::string ops[] = { "+=", "-=", "*=", "/=" };
        std::string sources[] = {
              "i += 3"
            , "i -= 3"
            , "i *= 3"
            , "i /= 3"
        };

        int i = 0;
        for (const auto& source : sources) {
            INFO(source);

            auto program = ankh::lang::parse(source);
            REQUIRE(!program.has_errors());
            REQUIRE(program.size() == 1);

            auto assignment = ankh::lang::instance<ankh::lang::CompoundAssignment>(program[0]);
            REQUIRE(assignment != nullptr);

            REQUIRE(assignment->target.str == "i");
            REQUIRE(assignment->op.str == ops[i++]);

            auto literal = ankh::lang::instance<ankh::lang::LiteralExpression>(assignment->value);
            REQUIRE(literal != nullptr);

            REQUIRE(literal->literal.str == "3");
        }
    }

    SECTION("parse modify statement")
    {
        const std::string source =
        R"(
            a.b.c = 3
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto modify = ankh::lang::instance<ankh::lang::ModifyStatement>(program[0]);
        REQUIRE(modify != nullptr);
        REQUIRE(modify->name.str == "c");

        auto access = ankh::lang::instance<ankh::lang::AccessExpression>(modify->object);
        REQUIRE(access != nullptr);
        REQUIRE(access->accessor.str == "b");

        auto literal = ankh::lang::instance<ankh::lang::LiteralExpression>(modify->value);;
        REQUIRE(literal != nullptr);

        REQUIRE(literal->literal.str == "3");
    }

    SECTION("parse modify statement, compound modify")
    {
        std::string ops[] = { "+=", "-=", "*=", "/=" };
        std::string sources[] = {
              "i.x += 3"
            , "i.x -= 3"
            , "i.x *= 3"
            , "i.x /= 3"
        };

        int i = 0;
        for (const auto& source : sources) {
            INFO(source);

            auto program = ankh::lang::parse(source);
            REQUIRE(!program.has_errors());
            REQUIRE(program.size() == 1);

            auto modify = ankh::lang::instance<ankh::lang::CompoundModify>(program[0]);
            REQUIRE(modify != nullptr);

            REQUIRE(modify->op.str == ops[i++]);

            auto target = ankh::lang::instance<ankh::lang::IdentifierExpression>(modify->object);
            REQUIRE(target != nullptr);

            auto literal = ankh::lang::instance<ankh::lang::LiteralExpression>(modify->value);
            REQUIRE(literal != nullptr);

            REQUIRE(literal->literal.str == "3");
        }
    }

    SECTION("parse increment statement, identifier")
    {
        const std::string source =
        R"(
            ++i
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);
        REQUIRE(!program.has_errors());

        auto modify = ankh::lang::instance<ankh::lang::IncOrDecIdentifierStatement>(program[0]);
        REQUIRE(modify != nullptr);

        REQUIRE(modify->op.str == "++");
        REQUIRE(ankh::lang::instanceof<ankh::lang::IdentifierExpression>(modify->expr));
    }

    SECTION("parse increment statement, access")
    {
        const std::string source =
        R"(
            ++i.x
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);
        REQUIRE(!program.has_errors());

        auto modify = ankh::lang::instance<ankh::lang::IncOrDecAccessStatement>(program[0]);
        REQUIRE(modify != nullptr);

        REQUIRE(modify->op.str == "++");
        REQUIRE(ankh::lang::instanceof<ankh::lang::AccessExpression>(modify->expr));
    }

    SECTION("parse increment statement, invalid target")
    {
        const std::string source =
            R"(
            ++"foo"
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(program.has_errors());
    }

    SECTION("parse decrement statement, identifier")
    {
        const std::string source =
        R"(
            --i
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);
        REQUIRE(!program.has_errors());

        auto modify = ankh::lang::instance<ankh::lang::IncOrDecIdentifierStatement>(program[0]);
        REQUIRE(modify != nullptr);

        REQUIRE(modify->op.str == "--");
        REQUIRE(ankh::lang::instanceof<ankh::lang::IdentifierExpression>(modify->expr));
    }

    SECTION("parse decrement statement, access")
    {
        const std::string source =
            R"(
            --i.x
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);
        REQUIRE(!program.has_errors());

        auto modify = ankh::lang::instance<ankh::lang::IncOrDecAccessStatement>(program[0]);
        REQUIRE(modify != nullptr);

        REQUIRE(modify->op.str == "--");
        REQUIRE(ankh::lang::instanceof<ankh::lang::AccessExpression>(modify->expr));
    }

    SECTION("parse decrement statement, invalid target")
    {
        const std::string source =
            R"(
            --"foo"
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(program.has_errors());
    }

    SECTION("parse print statement")
    {
        const std::string source =
        R"(
            print "hello"
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto print = ankh::lang::instance<ankh::lang::PrintStatement>(program[0]);
        REQUIRE(print != nullptr);

        auto literal = ankh::lang::instance<ankh::lang::StringExpression>(print->expr);
        REQUIRE(literal != nullptr);
        REQUIRE(literal->str.type == ankh::lang::TokenType::STRING);
    }

    SECTION("parse block statement")
    {
        const std::string source =
        R"(
            {
                print "hello"
                print "world"
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto block = ankh::lang::instance<ankh::lang::BlockStatement>(program[0]);
        REQUIRE(block != nullptr);

        REQUIRE(block->statements.size() == 2);
    }

    SECTION("parse if statement with no else")
    {
        const std::string source =
        R"(
            if 1 == 1 {
                print "yes"
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto if_stmt = ankh::lang::instance<ankh::lang::IfStatement>(program[0]);
        REQUIRE(if_stmt != nullptr);

        auto then_block = ankh::lang::instance<ankh::lang::BlockStatement>(if_stmt->then_block);
        REQUIRE(then_block != nullptr);
        REQUIRE(if_stmt->else_block == nullptr);
    }

    SECTION("parse if statement with else")
    {
        const std::string source =
        R"(
            if 1 == 1 {
                print "yes"
            } else {
                print "no"
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto if_stmt = ankh::lang::instance<ankh::lang::IfStatement>(program[0]);
        REQUIRE(if_stmt != nullptr);

        auto then_block = ankh::lang::instance<ankh::lang::BlockStatement>(if_stmt->then_block);
        REQUIRE(then_block != nullptr);

        auto else_block = ankh::lang::instance<ankh::lang::BlockStatement>(if_stmt->else_block);
        REQUIRE(else_block != nullptr);
    }

    SECTION("parse if statement with else-if")
    {
        const std::string source =
        R"(
            if 1 == 2 {
                print "what?"
            } else if 2 == 2 {
                print "yay"
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto if_stmt = ankh::lang::instance<ankh::lang::IfStatement>(program[0]);
        REQUIRE(if_stmt != nullptr);

        auto then_block = ankh::lang::instance<ankh::lang::BlockStatement>(if_stmt->then_block);
        REQUIRE(then_block != nullptr);

        auto else_block = ankh::lang::instance<ankh::lang::IfStatement>(if_stmt->else_block);
        REQUIRE(else_block != nullptr);
    }

    SECTION("parse while statement")
    {
        const std::string source =
        R"(
            let i = 1
            while i < 2 {
                print "I am less than 2"
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 2);

        auto while_stmt = ankh::lang::instance<ankh::lang::WhileStatement>(program[1]);
        REQUIRE(while_stmt != nullptr);

        auto body = ankh::lang::instance<ankh::lang::BlockStatement>(while_stmt->body);
        REQUIRE(body != nullptr);

        REQUIRE(while_stmt->condition != nullptr);
    }

    SECTION("parse function declaration")
    {
        const std::string source =
        R"(
            fn sum(a, b, c) {
                return a + b + c
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto fn = ankh::lang::instance<ankh::lang::FunctionDeclaration>(program[0]);
        REQUIRE(fn != nullptr);
        REQUIRE(fn->body != nullptr);
        REQUIRE(!fn->name.str.empty());
        REQUIRE(fn->params.size() == 3);

        auto body = ankh::lang::instance<ankh::lang::BlockStatement>(fn->body);
        REQUIRE(body != nullptr);
        REQUIRE(body->statements.size() == 1);

        auto return_stmt = ankh::lang::instance<ankh::lang::ReturnStatement>(body->statements[0]);
        REQUIRE(return_stmt != nullptr);
        REQUIRE(return_stmt->expr != nullptr);
    }

    SECTION("return statement injected into return-less function")
    {
        const std::string source =
            R"(
                fn foo(a, b) {
                    print a + b
                }
            )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto fn = ankh::lang::instance<ankh::lang::FunctionDeclaration>(program[0]);

        auto body = ankh::lang::instance<ankh::lang::BlockStatement>(fn->body);
        REQUIRE(body->statements.size() == 2);

        REQUIRE(ankh::lang::instanceof<ankh::lang::PrintStatement>(body->statements[0]));

        auto return_stmt = ankh::lang::instance<ankh::lang::ReturnStatement>(body->statements[1]);
        REQUIRE(return_stmt != nullptr);
        
        auto nil = ankh::lang::instance<ankh::lang::LiteralExpression>(return_stmt->expr);
        REQUIRE(nil != nullptr);
        REQUIRE(nil->literal.str == "nil");
    }

    SECTION("return statement NOT injected into function with return")
    {
        const std::string source =
            R"(
                fn foo(a, b) {
                    let s = a + b
                    {
                        return s
                    }
                }
            )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto fn = ankh::lang::instance<ankh::lang::FunctionDeclaration>(program[0]);

        auto body = ankh::lang::instance<ankh::lang::BlockStatement>(fn->body);
        REQUIRE(body->statements.size() == 2);

        REQUIRE(ankh::lang::instanceof<ankh::lang::VariableDeclaration>(body->statements[0]));

        auto block = ankh::lang::instance<ankh::lang::BlockStatement>(body->statements[1]);
        REQUIRE(block != nullptr);
        REQUIRE(block->statements.size() == 1);
        REQUIRE(ankh::lang::instanceof<ankh::lang::ReturnStatement>(block->statements[0]));
    }

    SECTION("data declaration")
    {
        const std::string source =
        R"(
            data Point { x y }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto data = ankh::lang::instance<ankh::lang::DataDeclaration>(program[0]);
        REQUIRE(data != nullptr);
        REQUIRE(data->name.str == "Point");
        REQUIRE(data->members.size() == 2);
        REQUIRE(data->members[0].str == "x");
        REQUIRE(data->members[1].str == "y");
    }

    SECTION("data declaration, empty")
    {
        const std::string source =
        R"(
            data Point { }
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(program.has_errors());
    }

    SECTION("data declaration, non-terminated")
    {
        const std::string source =
        R"(
            data Point { x
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(program.has_errors());
    }
}

TEST_CASE("for statements", "[parser]")
{
    SECTION("for loop, 3 components")
    {
        const std::string source = R"(
            for let i = 0; i < 3; ++i {
                print i
            }
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(!program.has_errors());

        auto for_stmt = ankh::lang::instance<ankh::lang::ForStatement>(program[0]);
        REQUIRE(for_stmt->init != nullptr);
        REQUIRE(for_stmt->condition != nullptr);
        REQUIRE(for_stmt->mutator != nullptr);
        REQUIRE(for_stmt->body != nullptr);
    }

    SECTION("for loop, no init")
    {
        const std::string source = R"(
            for ; i < 3; ++i {
                print i
            }
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(!program.has_errors());

        auto for_stmt = ankh::lang::instance<ankh::lang::ForStatement>(program[0]);
        REQUIRE(for_stmt->init == nullptr);
        REQUIRE(for_stmt->condition != nullptr);
        REQUIRE(for_stmt->mutator != nullptr);
        REQUIRE(for_stmt->body != nullptr);
    }

    SECTION("for loop, no condition")
    {
        const std::string source = R"(
            for let i = 0; ; ++i {
                print i
            }
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(!program.has_errors());

        auto for_stmt = ankh::lang::instance<ankh::lang::ForStatement>(program[0]);
        REQUIRE(for_stmt->init != nullptr);
        REQUIRE(for_stmt->condition == nullptr);
        REQUIRE(for_stmt->mutator != nullptr);
        REQUIRE(for_stmt->body != nullptr);
    }

    SECTION("for loop, no mutator")
    {
        const std::string source = R"(
            for let i = 0; i < 3; {
                print i
            }
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(!program.has_errors());

        auto for_stmt = ankh::lang::instance<ankh::lang::ForStatement>(program[0]);
        REQUIRE(for_stmt->init != nullptr);
        REQUIRE(for_stmt->condition != nullptr);
        REQUIRE(for_stmt->mutator == nullptr);
        REQUIRE(for_stmt->body != nullptr);
    }

    SECTION("infinite loop")
    {
        const std::string source = R"(
            for {
                print i
            }
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(!program.has_errors());

        auto for_stmt = ankh::lang::instance<ankh::lang::ForStatement>(program[0]);
        REQUIRE(for_stmt->init == nullptr);
        REQUIRE(for_stmt->condition == nullptr);
        REQUIRE(for_stmt->mutator == nullptr);
        REQUIRE(for_stmt->body != nullptr);
    }
}

TEST_CASE("break statements", "[parser]")
{
    SECTION("keyword")
    {
        const std::string source = R"(
            break
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(!program.has_errors());

        REQUIRE(ankh::lang::instanceof<ankh::lang::BreakStatement>(program[0]));
    }
}

TEST_CASE("parse language expressions", "[parser]")
{
    SECTION("parse primary")
    {
        const std::string source =
        R"(
            1
            true
            false
            nil
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 4);

        for (const auto& stmt : program.statements) {
            auto literal = ankh::lang::instance<ankh::lang::ExpressionStatement>(stmt);
            REQUIRE(literal != nullptr);
            REQUIRE(ankh::lang::instanceof<ankh::lang::LiteralExpression>(literal->expr));
        }
    }

    SECTION("parse paren")
    {
        const std::string source =
        R"(
            ( "an expression" )
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        REQUIRE(ankh::lang::instanceof<ankh::lang::ParenExpression>(stmt->expr));
    }

    SECTION("parse identifier")
    {
        const std::string source =
        R"(
            a
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        REQUIRE(ankh::lang::instanceof<ankh::lang::IdentifierExpression>(stmt->expr));
    }

    SECTION("parse function call, no args")
    {
        const std::string source =
        R"(
            a()
        )";

        auto program = ankh::lang::parse(source);
        for (auto e : program.errors) {
            INFO(e);
        }

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto call = ankh::lang::instance<ankh::lang::CallExpression>(stmt->expr);
        REQUIRE(call != nullptr);
        REQUIRE(call->args.size() == 0);

        auto identifier = ankh::lang::instance<ankh::lang::IdentifierExpression>(call->callee);
        REQUIRE(identifier != nullptr);
        REQUIRE(identifier->name.str == "a");
    }

    SECTION("parse function call, >0 args")
    {
        const std::string source =
        R"(
            a(1, 2)
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto call = ankh::lang::instance<ankh::lang::CallExpression>(stmt->expr);
        REQUIRE(call != nullptr);
        REQUIRE(call->args.size() == 2);

        auto identifier = ankh::lang::instance<ankh::lang::IdentifierExpression>(call->callee);
        REQUIRE(identifier != nullptr);
        REQUIRE(identifier->name.str == "a");
    }

    SECTION("parse function call, multicall")
    {
        const std::string source =
        R"(
            a(1, 2)()
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto inner_call = ankh::lang::instance<ankh::lang::CallExpression>(stmt->expr);
        REQUIRE(inner_call != nullptr);
        REQUIRE(inner_call->args.size() == 0);

        auto callee = ankh::lang::instance<ankh::lang::CallExpression>(inner_call->callee);
        REQUIRE(callee != nullptr);
        REQUIRE(callee->args.size() == 2);

        auto callee_name = ankh::lang::instance<ankh::lang::IdentifierExpression>(callee->callee);
        REQUIRE(callee_name != nullptr);
        REQUIRE(callee_name->name.str == "a");
    }

    SECTION("parse lambda expression")
    {
        const std::string source =
        R"(
            let lambda = fn (a, b) {
                return a + b
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto lambda = ankh::lang::instance<ankh::lang::LambdaExpression>(stmt->initializer);
        REQUIRE(lambda != nullptr);
        REQUIRE(lambda->params.size() == 2);
        REQUIRE(lambda->body != nullptr);
        REQUIRE(!lambda->generated_name.empty());
    }

    SECTION("parse unary !")
    {
        test_unary_expression("!");
        test_unary_expression("-");
    } 

    SECTION("parse factor")
    {
        test_binary_expression_parse("*");
        test_binary_expression_parse("/");
    }

    SECTION("parse term")
    {
        test_binary_expression_parse("-");
        test_binary_expression_parse("+");
    }

    SECTION("parse comparison")
    {
        test_binary_expression_parse(">");
        test_binary_expression_parse(">=");
        test_binary_expression_parse("<");
        test_binary_expression_parse("<=");
    }

    SECTION("parse equality")
    {
        test_binary_expression_parse("!=");
        test_binary_expression_parse("==");
    }

    SECTION("parse logical")
    {
        test_boolean_binary_expression("&&");
        test_boolean_binary_expression("||");
    }

    SECTION("parse command")
    {
        const std::string source =
        R"(
            $(echo hello)
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto cmd = ankh::lang::instance<ankh::lang::CommandExpression>(stmt->expr);
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->cmd.str == "echo hello");
        REQUIRE(cmd->cmd.type == ankh::lang::TokenType::COMMAND);
    }

    SECTION("parse empty command")
    {
        const std::string source =
        R"(
            $()
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.has_errors());
    }

    SECTION("parse array")
    {
        const std::string source =
        R"(
            [1, 2]
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        auto arr = ankh::lang::instance<ankh::lang::ArrayExpression>(stmt->expr);
        REQUIRE(arr != nullptr);
        REQUIRE(arr->elems.size() == 2);
    }

    SECTION("parse empty array")
    {
        const std::string source =
        R"(
            []
        )";

        auto program = ankh::lang::parse(source);
        REQUIRE(program.size() == 1);
        
        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        auto arr = ankh::lang::instance<ankh::lang::ArrayExpression>(stmt->expr);
        REQUIRE(arr != nullptr);
        REQUIRE(arr->elems.size() == 0);
    }

    SECTION("interleave call and index expressions")
    {
        const std::string source =
        R"(
            foo()[0]
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto idx = ankh::lang::instance<ankh::lang::IndexExpression>(stmt->expr);
        REQUIRE(idx != nullptr);
    }

    SECTION("interleave index and call expressions")
    {
        const std::string source =
        R"(
            foo[0]()
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto idx = ankh::lang::instance<ankh::lang::CallExpression>(stmt->expr);
        REQUIRE(idx != nullptr);
    }

    SECTION("index with no index expression")
    {
        const std::string source =
        R"(
            foo[]
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.has_errors());
    }

    SECTION("dictionary, one key")
    {
        const std::string source =
        R"(
            let dict = {
                hello: "world"
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = ankh::lang::instance<ankh::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 1);
        
        auto key = ankh::lang::instance<ankh::lang::StringExpression>(dict->entries[0].key);
        REQUIRE(key != nullptr);

        auto value = ankh::lang::instance<ankh::lang::StringExpression>(dict->entries[0].value);
        REQUIRE(value != nullptr);
    }

    SECTION("dictionary, empty")
    {
        const std::string source =
        R"(
            let dict = {}
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = ankh::lang::instance<ankh::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.empty());
    }

    SECTION("dictionary, multi entry")
    {
        const std::string source =
        R"(
            let dict = {
                hello: "world"
                , foo: "1"
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = ankh::lang::instance<ankh::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 2);
        
        for (const auto& e : dict->entries) {
            auto key = ankh::lang::instance<ankh::lang::StringExpression>(e.key);
            REQUIRE(key != nullptr);

            auto value = ankh::lang::instance<ankh::lang::StringExpression>(e.value);
            REQUIRE(value != nullptr);
        }
    }

    SECTION("dictionary, expression key, single member")
    {
        const std::string source =
        R"(
            let dict = {
                [1 + 1] : 2
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = ankh::lang::instance<ankh::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 1);

        auto key = ankh::lang::instance<ankh::lang::BinaryExpression>(dict->entries[0].key);
        REQUIRE(key != nullptr);

        auto value = ankh::lang::instance<ankh::lang::LiteralExpression>(dict->entries[0].value);
        REQUIRE(value != nullptr);
    }

    SECTION("dictionary, expression key, multi member")
    {
        const std::string source =
        R"(
            let dict = {
                [1 + 1] : 2
                , [3 + 4] : 2
                , foo : "bar"
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = ankh::lang::instance<ankh::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 3);
    }

    SECTION("dictionary, multi member, missing comma")
    {
        const std::string source =
        R"(
            let dict = {
                [1 + 1] : 2
                 [3 + 4] : 2
                , welp
            }
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(program.has_errors());
    }

    SECTION("dictionary, lookup")
    {
        const std::string source =
        R"(
            let dict = {
                [1 + 1] : 2
                , [3 + 4] : 2
                , welp: "gulp"
            }

            dict["f"]
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 2);

        auto decl = ankh::lang::instance<ankh::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = ankh::lang::instance<ankh::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 3);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[1]);
        REQUIRE(stmt != nullptr);

        auto lookup = ankh::lang::instance<ankh::lang::IndexExpression>(stmt->expr);
        REQUIRE(lookup != nullptr);
    }

    SECTION("accessible, field")
    {
        const std::string source =
        R"(
            a.b
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto access = ankh::lang::instance<ankh::lang::AccessExpression>(stmt->expr);
        REQUIRE(access != nullptr);
        REQUIRE(access->accessor.str == "b");
    }

    SECTION("accessible, method")
    {
        const std::string source =
        R"(
            a.b()
        )";

        auto program = ankh::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto stmt = ankh::lang::instance<ankh::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto call = ankh::lang::instance<ankh::lang::CallExpression>(stmt->expr);
        REQUIRE(call != nullptr);
        REQUIRE(call->args.size() == 0);

        auto access = ankh::lang::instance<ankh::lang::AccessExpression>(call->callee);
        REQUIRE(access != nullptr);
        REQUIRE(access->accessor.str == "b");
    }
}

TEST_CASE("test parse statement without a empty line at the end does not infinite loop", "[parser]")
{
    auto program = ankh::lang::parse("1 + 2");

    REQUIRE(program.size() == 1);
}

TEST_CASE("parse two arrays as two separate statements rather than an index operation", "[parser]")
{
    const std::string source =
    R"(
        [1, 2];
        [0]
    )";

    auto program = ankh::lang::parse(source);
    REQUIRE(program.size() == 2);

    for (auto& stmt : program.statements) {
        auto ptr = ankh::lang::instance<ankh::lang::ExpressionStatement>(stmt);
        REQUIRE(ptr != nullptr);
        REQUIRE(ankh::lang::instanceof<ankh::lang::ArrayExpression>(ptr->expr));   
    }
}
