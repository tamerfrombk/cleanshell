#include <ankh/lang/resolver.h>

#include <ankh/def.h>

#include <ankh/lang/exceptions.h>
#include <ankh/lang/interpreter.h>

ankh::lang::Resolver::Resolver(Interpreter *interpreter)
    : interpreter_(interpreter) {}

ankh::lang::ExprResult ankh::lang::Resolver::visit(BinaryExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(UnaryExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(LiteralExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(ParenExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(IdentifierExpression *expr)
{
    if (!scopes_.empty() && !is_defined(expr->name)) {
        lang::panic<ParseException>("{}:{}, a local variable cannot be initialized with a global variable of the same name", expr->name.line, expr->name.col);
    }

    resolve(expr, expr->name);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(CallExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(LambdaExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(CommandExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(ArrayExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(IndexExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(DictionaryExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(StringExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(AccessExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

void ankh::lang::Resolver::visit(PrintStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(ExpressionStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(VariableDeclaration *stmt)
{
    declare(stmt->name);
    resolve(stmt->initializer);
    define(stmt->name);
}

void ankh::lang::Resolver::visit(AssignmentStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(IncOrDecIdentifierStatement* stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(IncOrDecAccessStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(CompoundAssignment* stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(ModifyStatement* stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(CompoundModify* stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(BlockStatement *stmt)
{
    begin_scope();
    resolve(stmt->statements);
    end_scope();
}

void ankh::lang::Resolver::visit(IfStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(WhileStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(ForStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(BreakStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(FunctionDeclaration *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(ReturnStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(DataDeclaration *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::resolve(const ExpressionPtr& expr)
{
    expr->accept(this);
}

void ankh::lang::Resolver::resolve(const StatementPtr& stmt)
{
    stmt->accept(this);
}

void ankh::lang::Resolver::resolve(const std::vector<StatementPtr>& stmts)
{
    for (const auto& stmt : stmts) {
        resolve(stmt);
    }
}

void ankh::lang::Resolver::resolve(const Expression *expr, const Token& name)
{
    if (scopes_.empty()) {
        return;
    }

    for (auto it = scopes_.crbegin(); it != scopes_.crend(); ++it) {
        if (it->count(name.str) > 0) {
            const size_t hops = it - scopes_.crbegin();
            interpreter_->resolve(expr, scopes_.size() - 1 - hops);
            return;
        }
    }
}

void ankh::lang::Resolver::declare(const Token& name) noexcept
{
    if (scopes_.empty()) {
        return;
    }

    top()[name.str] = false;
}

void ankh::lang::Resolver::define(const Token& name) noexcept
{
    if (scopes_.empty()) {
        return;
    }

    top()[name.str] = true;
}

bool ankh::lang::Resolver::is_defined(const Token& name) noexcept
{
    return top().count(name.str) > 0 && top()[name.str] == true;
}

void ankh::lang::Resolver::begin_scope() noexcept
{
    scopes_.push_back({});
}

void ankh::lang::Resolver::end_scope() noexcept
{
    scopes_.pop_back();
}

ankh::lang::Resolver::Scope& ankh::lang::Resolver::top() noexcept
{
    return scopes_.back();
}