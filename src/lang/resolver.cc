#include <ankh/lang/resolver.h>

#include <ankh/def.h>

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
    ANKH_UNUSED(expr);

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
    ANKH_UNUSED(stmt);
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

void ankh::lang::Resolver::begin_scope() noexcept
{
    scopes_.push_back({});
}

void ankh::lang::Resolver::end_scope() noexcept
{
    scopes_.pop_back();
}

