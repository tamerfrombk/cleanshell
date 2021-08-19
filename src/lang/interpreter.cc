#include "ankh/lang/statement.h"
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <initializer_list>
#include <functional>
#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <fmt/format.h>

#include <ankh/def.h>
#include <ankh/log.h>

#include <ankh/lang/interpreter.h>
#include <ankh/lang/parser.h>

#include <ankh/lang/token.h>
#include <ankh/lang/expr.h>
#include <ankh/lang/exceptions.h>

#include <ankh/lang/types/array.h>
#include <ankh/lang/types/dictionary.h>
#include <ankh/lang/types/data.h>
#include <ankh/lang/types/object.h>

#include <unordered_map>
#include <vector>

struct ReturnException
    : public std::runtime_error
{
    explicit ReturnException(ankh::lang::ExprResult result)
        : std::runtime_error(""), result(std::move(result)) {}

    ankh::lang::ExprResult result;
};

struct BreakException
    : public std::runtime_error
{
    BreakException() 
        : std::runtime_error("") {}
};

template <class... Args>
ANKH_NO_RETURN static void panic(const char *fmt, Args&&... args)
{
    const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
    ankh::lang::panic<ankh::lang::InterpretationException>("runtime error: {}", msg);
}

template <class... Args>
ANKH_NO_RETURN static void panic(const ankh::lang::ExprResult& result, const char *fmt, Args&&... args)
{
    const std::string typestr = ankh::lang::expr_result_type_str(result.type);

    const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);

    ankh::lang::panic<ankh::lang::InterpretationException>("runtime error: {} instead of '{}'", msg, typestr);
}

template <class... Args>
ANKH_NO_RETURN static void panic(
    const ankh::lang::ExprResult& left
    , const ankh::lang::ExprResult& right
    , const char *fmt
    , Args&&... args
)
{
    const std::string ltypestr = ankh::lang::expr_result_type_str(left.type);
    const std::string rtypestr = ankh::lang::expr_result_type_str(right.type);
    const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);

    ankh::lang::panic<ankh::lang::InterpretationException>("runtime error: {} with LHS '{}', RHS '{}'", msg, ltypestr, rtypestr);
}

static bool operands_are(ankh::lang::ExprResultType type, std::initializer_list<ankh::lang::ExprResult> elems) noexcept
{
    return std::all_of(elems.begin(), elems.end(), [=](const ankh::lang::ExprResult& result) {
        return type == result.type;
    });
}

static ankh::lang::Number to_num(const std::string& s)
{
    char *end;

    ankh::lang::Number n = std::strtod(s.c_str(), &end);
    if (*end == '\0') {
        return n;
    }

    const std::string errno_msg(std::strerror(errno));

    panic("'{}' could not be turned into a number due to {}", s, errno_msg);
}

static bool is_integer(ankh::lang::Number n) noexcept
{
    double intpart;
    return modf(n, &intpart) == 0.0;
}

static ankh::lang::ExprResult negate(const ankh::lang::ExprResult& result)
{
    if (result.type == ankh::lang::ExprResultType::RT_NUMBER) {
        return -1 * result.n;
    }

    panic(result, "unary (-) operator expects a number expression");
}

static ankh::lang::ExprResult invert(const ankh::lang::ExprResult& result)
{
    if (result.type == ankh::lang::ExprResultType::RT_BOOL) {
        return !(result.b);
    }

    panic(result, "(!) operator expects a boolean expression");
}

static ankh::lang::ExprResult eqeq(const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return left.n == right.n;
    }

    if (operands_are(ankh::lang::ExprResultType::RT_STRING, {left, right})) {
        return left.str == right.str;
    }

    if (operands_are(ankh::lang::ExprResultType::RT_BOOL, {left, right})) {
        return left.b == right.b;
    }

    if (operands_are(ankh::lang::ExprResultType::RT_NIL, {left, right})) {
        return true;
    }

    panic(left, right, "unknown overload of (==) operator");
}

template <class BinaryOperation>
static ankh::lang::ExprResult 
arithmetic(const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right, BinaryOperation op)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return op(left.n, right.n);
    }

    panic(left, right, "unknown overload of arithmetic operator");
}

static ankh::lang::ExprResult division(const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        if (right.n == 0) {
            panic("division by zero");
        }
        return left.n / right.n;
    }

    panic(left, right, "unknown overload of arithmetic operator");
}

// We handle + separately as it has two overloads for numbers and strings
// The generic arithmetic() function overloads all of the general arithmetic operations
// on only numbers
static ankh::lang::ExprResult plus(const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return left.n + right.n;
    }

    if (operands_are(ankh::lang::ExprResultType::RT_STRING, {left, right})) {
        return left.str + right.str;
    }

    panic(left, right, "unknown overload of (+) operator");
}

template <class Compare>
static ankh::lang::ExprResult compare(const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right, Compare cmp)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return cmp(left.n, right.n);
    }

    if (operands_are(ankh::lang::ExprResultType::RT_STRING, {left, right})) {
        return cmp(left.str, right.str);
    }

    panic(left, right, "unknown overload of comparison operator");
}

template <class Compare>
static ankh::lang::ExprResult logical(const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right, Compare cmp)
{
    if (operands_are(ankh::lang::ExprResultType::RT_BOOL, {left, right})) {
        return cmp(left.b, right.b);
    }

    panic(left, right, "unknown overload of logical operator");
}

static bool truthy(const ankh::lang::ExprResult& result) noexcept
{
    if (result.type == ankh::lang::ExprResultType::RT_BOOL) {
        return result.b;
    }

    ankh::lang::panic<ankh::lang::InterpretationException>("'{}' is not a boolean expression", result.stringify());
}

static void print(const ankh::lang::ExprResult& result)
{
    const std::string stringy = result.stringify();
    std::puts(stringy.c_str());
}

ankh::lang::Interpreter::Interpreter()
    : current_env_(make_env<ExprResult>()) {}

void ankh::lang::Interpreter::interpret(const Program& program)
{
    const auto& statements = program.statements();
    for (const auto& stmt : statements) {
#ifndef NDEBUG
        ANKH_DEBUG("{}", stmt->stringify());
#endif
        execute(stmt);
    }
}

void ankh::lang::Interpreter::resolve(const Expression *expr, size_t hops)
{
    ANKH_UNUSED(expr);
    ANKH_UNUSED(hops);

    ANKH_FATAL("unimplemented");
}

void ankh::lang::Interpreter::resolve(const Statement *stmt, size_t hops)
{
    ANKH_UNUSED(stmt);
    ANKH_UNUSED(hops);

    ANKH_FATAL("unimplemented");
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(BinaryExpression *expr)
{
    const ExprResult left  = evaluate(expr->left);
    const ExprResult right = evaluate(expr->right);

    switch (expr->op.type) {
    case TokenType::EQEQ:
        return eqeq(left, right);
    case TokenType::NEQ:
        return invert(eqeq(left, right));
    case TokenType::GT:
        return compare(left, right, std::greater<>{});
    case TokenType::GTE:
        return compare(left, right, std::greater_equal<>{});
    case TokenType::LT:
        return compare(left, right, std::less<>{});
    case TokenType::LTE:
        return compare(left, right, std::less_equal<>{});
    case TokenType::MINUS:
        return arithmetic(left, right, std::minus<>{});
    case TokenType::PLUS:
        return plus(left, right);
    case TokenType::STAR:
        return arithmetic(left, right, std::multiplies<>{});
    case TokenType::AND:
        return logical(left, right, std::logical_and<>{});
    case TokenType::OR:
        return logical(left, right, std::logical_or<>{});
    case TokenType::FSLASH:
        return division(left, right);
    default:
        ::panic(left, right, "unknown binary operator '{}'", expr->op.str);
    }
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(UnaryExpression *expr)
{
    const ExprResult result = evaluate(expr->right);
    switch (expr->op.type) {
    case TokenType::MINUS:
        return negate(result);
    case TokenType::BANG:
        return invert(result);
    default:
        ::panic(result, "unknown unary operator '{}'", expr->op.str);
    }
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(LiteralExpression *expr)
{
    switch (expr->literal.type) {
    case TokenType::NUMBER:
        return to_num(expr->literal.str);
    case TokenType::STRING:
        return expr->literal.str;
    case TokenType::ANKH_TRUE:
        return true;
    case TokenType::ANKH_FALSE:
        return false;
    case TokenType::NIL:
        return {};
    default:
        ::panic("unknown literal expression '{}''", expr->literal.str);
    }
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ParenExpression *expr)
{
    return evaluate(expr->expr);
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(IdentifierExpression *expr)
{
    ANKH_DEBUG("evaluating identifier expression '{}'", expr->name.str);

    auto possible_value = current_env_->value(expr->name.str);
    if (possible_value.has_value()) {
        return possible_value.value();
    }

    ::panic("identifier '{}' not defined", expr->name.str);
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(CallExpression *expr)
{
    ANKH_DEBUG("evaluating call expression");

    const ExprResult callee = evaluate(expr->callee);
    if (callee.type != ExprResultType::RT_CALLABLE) {
        ::panic("only functions and classes are callable");
    }

    Callable *callable = callee.callable;
    const std::string name = callable->name();
    if (expr->args.size() != callable->arity()) {
        ::panic("expected {} arguments to function '{}' instead of {}", callable->arity(), name, expr->args.size());
    }

    ANKH_DEBUG("function '{}' with matching arity '{}' found", name, expr->args.size());

    try {
        callable->invoke(expr->args);
        
        // TODO: I'm not totally happy with this but this is a solution for constructors
        // Constructors do not return anything (yet)
        // Every other function has a return statement.
        // We take advantage of this here to implement object generation.
        ANKH_VERIFY(data_declarations_[name]);
        
        auto data = static_cast<Data<ExprResult, Interpreter>*>(functions_[name].get());

        return make_object<ExprResult>(make_env<ExprResult>(data->env()));

    } catch (const ReturnException& e) {
        return e.result;
    }

    ANKH_FATAL("callables should always return");
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(LambdaExpression *expr)
{
    const std::string& name = expr->generated_name;
    if (functions_.count(name) > 0) {
        ANKH_FATAL("lambda function generated name '{}' is duplicated", name);
    }

    CallablePtr callable = make_callable<Lambda<ExprResult, Interpreter>>(this, expr->clone(), current_env_);

    ExprResult result { callable.get() };
    
    functions_[name] = std::move(callable);
    
    if (!current_env_->declare(name, result)) {
        ::panic("'{}' is already defined", name);
    }

    ANKH_DEBUG("function '{}' added to scope {}", name, current_env_->scope());

    return result;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::CommandExpression *expr)
{
    ANKH_DEBUG("executing {}", expr->cmd.str);

    // TODO: popen() uses the underlying shell to invoke commands
    // This limits the language from being used as a shell itself
    // Come back and explore if that's something we want to consider doing
    std::FILE *fp = popen(expr->cmd.str.c_str(), "r");
    if (fp == nullptr) {
        ANKH_FATAL("popen: unable to launch {}", expr->cmd.str);
    }

    char buf[512];
    std::string output;
    while (std::fread(buf, sizeof(buf[0]), sizeof(buf), fp) > 0) {
        output += buf;
    }
    fclose(fp);

    return output;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ArrayExpression *expr)
{
    Array<ExprResult> array;
    for (const auto& e : expr->elems) {
        array.append(evaluate(e));
    }

    return array;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::IndexExpression *expr)
{
    const ExprResult indexee = evaluate(expr->indexee);
    if (indexee.type != ExprResultType::RT_ARRAY && indexee.type != ExprResultType::RT_DICT) {
        ::panic("lookup expects array or dict operand");
    }

    const ExprResult index = evaluate(expr->index);
    if (index.type == ExprResultType::RT_NUMBER) {
        if (!is_integer(index.n)) {
            ::panic("index must be an integral numeric expression");
        }

        if (indexee.type != ExprResultType::RT_ARRAY) {
            ::panic("operand must be an array for a numeric index");
        }

        if (index.n >= indexee.array.size()) {
            ::panic("index {} must be less than array size {}", index.n, indexee.array.size());
        }

        return indexee.array[index.n];
    }

    if (index.type == ExprResultType::RT_STRING) {
        if (indexee.type != ExprResultType::RT_DICT) {
            ::panic("operand must be a dict for a string index");
        }

        if (auto possible_value = indexee.dict.value(index.str); possible_value.has_value()) {
            return possible_value->value;
        }

        return {};
    }

    ::panic("'{}' is not a valid lookup expression", index.stringify());
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::DictionaryExpression *expr)
{
    Dictionary<ExprResult> dict;
    for (const auto& [key, value] : expr->entries) {
        const ExprResult& key_result = evaluate(key);
        if (key_result.type != ExprResultType::RT_STRING) {
            ::panic("expression key does not evaluate to a string");
        }
        dict.insert(key_result, evaluate(value));
    }

    return dict;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::StringExpression *expr)
{
    return substitute(expr);
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::AccessExpression *expr)
{
    ExprResult result = evaluate(expr->accessible);
    if (result.type == ExprResultType::RT_OBJECT) {
        if (auto possible_value = result.obj->env->value(expr->accessor.str); possible_value) {
            return possible_value.value();
        }
        ::panic("'{}' is not a member of '{}'", expr->accessor.str, expr->accessible->stringify());
    }

    ::panic("'{}' is not accessible", expr->accessible->stringify());
}

void ankh::lang::Interpreter::visit(PrintStatement *stmt)
{
    const ExprResult result = evaluate(stmt->expr);
    print(result);
}

void ankh::lang::Interpreter::visit(ExpressionStatement *stmt)
{
    ANKH_DEBUG("executing expression statement");
    const ExprResult result = evaluate(stmt->expr);
    print(result);
}

void ankh::lang::Interpreter::visit(VariableDeclaration *stmt)
{
    if (current_env_->contains(stmt->name.str)) {
        ::panic("'{}' is already declared in this scope", stmt->name.str);
    }

    const ExprResult result = evaluate(stmt->initializer);

    ANKH_DEBUG("DECLARATION '{}' = '{}'", stmt->name.str, result.stringify());

    if (!current_env_->declare(stmt->name.str, result)) {
        ::panic("'{}' is already defined", stmt->name.str);
    }

    if (stmt->storage_class == StorageClass::EXPORT) {
        const std::string result_str = result.stringify();
        if (setenv(stmt->name.str.c_str(), result_str.c_str(), 1) == -1) {
            const std::string errno_msg(std::strerror(errno));
            ::panic("'{}' could not be exported due to {}", stmt->name.str, errno_msg);
        }
    }
}

void ankh::lang::Interpreter::visit(AssignmentStatement *stmt)
{
    const ExprResult result = evaluate(stmt->initializer);
    if (!current_env_->assign(stmt->name.str, result)) {
        ::panic("'{}' is not defined", stmt->name.str);
    }
}

void ankh::lang::Interpreter::visit(CompoundAssignment* stmt)
{
    auto possible_target = current_env_->value(stmt->target.str);
    if (!possible_target) {
        ::panic("'{}' is not defined", stmt->target.str);
    }

    ExprResult target = possible_target.value();

    ExprResult value;
    if (stmt->op.str == "+=") {
        value = plus(target, evaluate(stmt->value));
    } else if (stmt->op.str == "-=") {
        value = arithmetic(target, evaluate(stmt->value), std::minus<>{});
    } else if (stmt->op.str == "*=") {
        value = arithmetic(target, evaluate(stmt->value), std::multiplies<>{});
    } else if (stmt->op.str == "/=") {
        value = division(target, evaluate(stmt->value));
    } else {
        ::panic("{}:{}, '{}' is not a valid compound assignment operation", stmt->target.line, stmt->target.col, stmt->op.str);
    }

    if (!current_env_->assign(stmt->target.str, value)) {
        ::panic("{}:{}, unable to assign the result of the compound assignment", stmt->target.line, stmt->target.col);
    }
}

void ankh::lang::Interpreter::visit(ankh::lang::ModifyStatement* expr)
{
    ExprResult object = evaluate(expr->object);
    if (object.type != ExprResultType::RT_OBJECT) {
        ::panic("only objects have members");
    }

    ExprResult value = evaluate(expr->value);
    if (!object.obj->set(expr->name.str, value)) {
        ::panic("'{}' is not a member", expr->name.str);
    }
}

void ankh::lang::Interpreter::visit(ankh::lang::CompoundModify* stmt)
{
    ExprResult object = evaluate(stmt->object);
    if (object.type != ExprResultType::RT_OBJECT) {
        ::panic("only objects have members");
    }

    auto possible_target = object.obj->env->value(stmt->name.str);
    if (!possible_target) {
        ::panic("'{}' is not a member", stmt->name.str);
    }

    ExprResult target = possible_target.value();
    
    ExprResult value;
    if (stmt->op.str == "+=") {
        value = plus(target, evaluate(stmt->value));
    } else if (stmt->op.str == "-=") {
        value = arithmetic(target, evaluate(stmt->value), std::minus<>{});
    } else if (stmt->op.str == "*=") {
        value = arithmetic(target, evaluate(stmt->value), std::multiplies<>{});
    } else if (stmt->op.str == "/=") {
        value = division(target, evaluate(stmt->value));
    } else {
        ::panic("{}:{}, '{}' is not a valid compound assignment operation", stmt->name.line, stmt->name.col, stmt->op.str);
    }

    if (!object.obj->set(stmt->name.str, value)) {
        ::panic("{}:{}, unable to assign the result of the compound assignment", stmt->name.line, stmt->name.col);
    }
}

void ankh::lang::Interpreter::visit(ankh::lang::IncOrDecAccessStatement *stmt)
{
    AccessExpression* access = static_cast<AccessExpression*>(stmt->expr.get());
    
    ExprResult obj = evaluate(access->accessible);
    
    auto possible_rhs = obj.obj->env->value(access->accessor.str);
    if (!possible_rhs) {
        ::panic("{}:{}, '{}' is not a member", access->accessor.line, access->accessor.col, access->accessor.str);
    }

    ExprResult value;
    if (stmt->op.str == "++") {
        value = plus(*possible_rhs, 1.0);
    } else if (stmt->op.str == "--") {
        value = arithmetic(*possible_rhs, 1.0, std::minus<>{});
    } else {
        // this shouldn't happen since the parser validates the token is one of the above
        // but those are famous last words ;)
        ANKH_FATAL("'{}' is not a valid increment or decrement operation", stmt->op.str);
    }

    if (!obj.obj->env->assign(access->accessor.str, value)) {
        ANKH_FATAL("{}:{}, unable to assign '{}'", access->accessor.str);
    }
}

void ankh::lang::Interpreter::visit(ankh::lang::IncOrDecIdentifierStatement* stmt)
{
    ExprResult rhs = evaluate(stmt->expr);
    
    ExprResult value;
    if (stmt->op.str == "++") {
        value = plus(rhs, 1.0);
    }
    else if (stmt->op.str == "--") {
        value = arithmetic(rhs, 1.0, std::minus<>{});
    }
    else {
        // this shouldn't happen since the parser validates the token is one of the above
        // but those are famous last words ;)
        ANKH_FATAL("'{}' is not a valid increment or decrement operation", stmt->op.str);
    }

    IdentifierExpression* expr = static_cast<IdentifierExpression*>(stmt->expr.get());
    if (!current_env_->assign(expr->name.str, value)) {
        ANKH_FATAL("{}:{}, unable to assign '{}'", expr->name.str);
    }
}

void ankh::lang::Interpreter::visit(BlockStatement *stmt)
{
    execute_block(stmt, current_env_);
}

void ankh::lang::Interpreter::execute_block(const BlockStatement *stmt, EnvironmentPtr<ExprResult> environment)
{
    Scope block_scope(this, environment);
    for (const StatementPtr& statement : stmt->statements) {
        execute(statement);
    }
}

void ankh::lang::Interpreter::visit(IfStatement *stmt)
{
    const ExprResult result = evaluate(stmt->condition);
    if (truthy(result)) {
        execute(stmt->then_block);
    } else if (stmt->else_block != nullptr) {
        execute(stmt->else_block);
    }
}

void ankh::lang::Interpreter::visit(WhileStatement *stmt)
{
    while (truthy(evaluate(stmt->condition))) {
        try {
            execute(stmt->body);
        } catch (const BreakException&) {
            return;
        }
    }
}

void ankh::lang::Interpreter::visit(ForStatement *stmt)
{
    Scope for_scope(this, current_env_);
    
    if (stmt->init) {
        execute(stmt->init);
    }

    while (stmt->condition ? truthy(evaluate(stmt->condition)) : true) {
        try {
            execute(stmt->body);
        } catch (const BreakException&) {
            return;
        }

        if (stmt->mutator) {
            execute(stmt->mutator);
        }
    }
}

void ankh::lang::Interpreter::visit(ankh::lang::BreakStatement *stmt)
{
    ANKH_UNUSED(stmt);
    
    throw BreakException();
}

void ankh::lang::Interpreter::visit(ankh::lang::FunctionDeclaration *stmt)
{
    declare_function(stmt, current_env_);
}

void ankh::lang::Interpreter::declare_function(FunctionDeclaration *decl, EnvironmentPtr<ExprResult> env)
{
    ANKH_DEBUG("evaluating function declaration of '{}'", decl->name.str);

    const std::string& name = decl->name.str;
    if (functions_.count(name) > 0) {
        ::panic("function '{}' is already declared", name);
    }

    CallablePtr callable = make_callable<Function<ExprResult, Interpreter>>(this, decl->clone(), env);

    ExprResult result { callable.get() };
    
    functions_[name] = std::move(callable);
    
    if (!current_env_->declare(name, result)) {
        ::panic("'{}' is already defined", name);
    }

    ANKH_DEBUG("function '{}' added to scope {}", name, current_env_->scope());
}

void ankh::lang::Interpreter::visit(ReturnStatement *stmt)
{
    ANKH_DEBUG("evaluating return statement");

    const ExprResult result = evaluate(stmt->expr);

    throw ReturnException(result);
}

void ankh::lang::Interpreter::visit(DataDeclaration *stmt)
{
    ANKH_DEBUG("evaluating data '{}' declaration", stmt->name.str);

    if (data_declarations_[stmt->name.str]) {
        ::panic("{}:{}, '{}' is already a data declaration", stmt->name.line, stmt->name.col, stmt->name.str);
    }

    EnvironmentPtr<ExprResult> env = make_env<ExprResult>(current_env_);
    std::vector<std::string> members;
    for (const auto& member : stmt->members) {
        if (!env->declare(member.str, ExprResult{})) {
            ANKH_FATAL("unable to declare data member '{}'", member.str);
        }
        members.push_back(member.str);
    }

    functions_[stmt->name.str] = make_callable<Data<ExprResult, Interpreter>>(this, stmt->name.str, env, members);

    if (!current_env_->declare(stmt->name.str, functions_[stmt->name.str].get())) {
        ANKH_FATAL("unable to declare constructor for data declaration '{}'", stmt->name.str);
    }

    data_declarations_[stmt->name.str] = true;

    ANKH_DEBUG("data '{}' declared", stmt->name.str);
}

ankh::lang::ExprResult ankh::lang::Interpreter::evaluate(const ExpressionPtr& expr)
{
    return expr->accept(this);
}

std::string ankh::lang::Interpreter::substitute(const StringExpression *expr)
{
    std::vector<size_t> opening_brace_indexes;
    bool is_outer = true;

    std::string result;
    for (size_t i = 0; i < expr->str.str.length(); ++i) {
        auto c = expr->str.str[i];
        if (c == '\\') {
            if (i < expr->str.str.length() - 1) {
                char next = expr->str.str[i + 1];
                if (next == '{' || next == '}') {
                    result += next;
                    ++i;
                    continue;
                }
            }
            ::panic("unterminated \\");
        } else if (c == '{') {
            if (is_outer) {
                opening_brace_indexes.push_back(i);
                is_outer = false;   
            } else {
                ::panic("{}:{}, nested brace substitution expressions are not allowed", expr->str.line, expr->str.col + i);
            }
        } else if (c == '}') {
            if (opening_brace_indexes.empty()) {
                ::panic("{}:{}, mismatched '}'", expr->str.line, expr->str.col);
            }

            const size_t start_idx = opening_brace_indexes.back();
            opening_brace_indexes.pop_back();

            const size_t expr_length = i - start_idx - 1;
            if (expr_length == 0) {
                ::panic("{}:{}, empty expression evaluation", expr->str.line, expr->str.col);
            }

            const std::string expr_str = expr->str.str.substr(start_idx + 1, expr_length);
            ANKH_DEBUG("{}:{}, parsed expression string '{}' starting @ {}", expr->str.line, expr->str.col, expr_str, start_idx);

            const ExprResult expr_result = evaluate_single_expr(expr_str);
            ANKH_DEBUG("'{}' => '{}'", expr_str, expr_result.stringify());

            result += expr_result.stringify();

            is_outer = true;
        } else if (is_outer) {
            result += c;
        }
    }

    if (!opening_brace_indexes.empty()) {
        ::panic("{}:{}, mismatched '{'", expr->str.line, expr->str.col);
    }

    return result;
}

ankh::lang::ExprResult ankh::lang::Interpreter::evaluate_single_expr(const std::string& str)
{
    auto program = ankh::lang::parse(str);
    if (program.has_errors()) {
        // TODO: print out why
        ::panic("expression '{}' is not valid", str);
    }

    const auto& statements = program.statements();
    if (statements.size() > 1) {
        ::panic("'{}' is a multi return expression");
    }

    if (ExpressionStatement *stmt = instance<ExpressionStatement>(program[0]); stmt == nullptr) {
        ::panic("'{}' is not an expression", str);
    } else {
        return evaluate(stmt->expr);
    }
}

void ankh::lang::Interpreter::execute(const StatementPtr& stmt)
{
    stmt->accept(this);
}

ankh::lang::Interpreter::Scope::Scope(ankh::lang::Interpreter *interpreter, ankh::lang::EnvironmentPtr<ExprResult> enclosing)
    : interpreter_(interpreter)
    , prev_(nullptr)
{
    prev_ = interpreter->current_env_;
    interpreter->current_env_ = make_env<ExprResult>(enclosing);
    ANKH_DEBUG("new scope created from {} to {} through {}", prev_->scope(), interpreter_->current_env_->scope(), enclosing->scope());
}

ankh::lang::Interpreter::Scope::~Scope()
{
    ANKH_DEBUG("scope exiting from {} to {}", interpreter_->current_env_->scope(), prev_->scope());
    interpreter_->current_env_ = prev_;
}