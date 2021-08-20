#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <ankh/lang/program.h>
#include <ankh/lang/expr.h>
#include <ankh/lang/statement.h>
#include <ankh/lang/lambda.h>
#include <ankh/lang/resolution_table.h>

namespace ankh::lang {

class Resolver
    : public ExpressionVisitor
    , public StatementVisitor<void>
{
    using Scope = std::unordered_map<std::string, bool>;

public:
    void resolve(ankh::lang::Program& program);

private:
    std::vector<Scope> scopes_;
    ResolutionTable table_;

private:
    ExprResult visit(BinaryExpression *expr) override;
    ExprResult visit(UnaryExpression *expr) override;
    ExprResult visit(LiteralExpression *expr) override;
    ExprResult visit(ParenExpression *expr) override;
    ExprResult visit(IdentifierExpression *expr) override;
    ExprResult visit(CallExpression *expr) override;
    ExprResult visit(LambdaExpression *expr) override;
    ExprResult visit(CommandExpression *expr) override;
    ExprResult visit(ArrayExpression *expr) override;
    ExprResult visit(IndexExpression *expr) override;
    ExprResult visit(DictionaryExpression *expr) override;
    ExprResult visit(StringExpression *expr) override;
    ExprResult visit(AccessExpression *expr) override;

    void visit(PrintStatement *stmt) override;
    void visit(ExpressionStatement *stmt) override;
    void visit(VariableDeclaration *stmt) override;
    void visit(AssignmentStatement *stmt) override;
    void visit(IncOrDecIdentifierStatement* stmt) override;
    void visit(IncOrDecAccessStatement *stmt) override;
    void visit(CompoundAssignment* stmt) override;
    void visit(ModifyStatement* stmt) override;
    void visit(CompoundModify* stmt) override;
    void visit(BlockStatement *stmt) override;
    void visit(IfStatement *stmt) override;
    void visit(WhileStatement *stmt) override;
    void visit(ForStatement *stmt) override;
    void visit(BreakStatement *stmt) override;
    void visit(FunctionDeclaration *stmt) override;
    void visit(ReturnStatement *stmt) override;
    void visit(DataDeclaration *stmt) override;

    void resolve(const ExpressionPtr& expr);
    void resolve(const StatementPtr& stmt);
    void resolve(const std::vector<StatementPtr>& stmts);
    void resolve(const Expression* expr, const Token& name);
    void resolve(const Statement* stmt, const Token& name);

    void declare(const Token& name) noexcept;
    void define(const Token& name) noexcept;
    bool is_declared_but_not_defined(const Token& name) const noexcept;

    void begin_scope() noexcept;
    void end_scope() noexcept;

    Scope& top() noexcept;
    const Scope& top() const noexcept;
};

}