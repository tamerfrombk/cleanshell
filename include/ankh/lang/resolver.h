#pragma once

#include <ankh/lang/expr.h>
#include <ankh/lang/statement.h>

namespace ankh::lang {

class Interpreter;

class Resolver
    : public ExpressionVisitor<void>
    , public StatementVisitor<void>
{
public:
    Resolver(Interpreter *interpreter);

private:
    Interpreter *interpreter_;

private:
    void visit(BinaryExpression *expr) override;
    void visit(UnaryExpression *expr) override;
    void visit(LiteralExpression *expr) override;
    void visit(ParenExpression *expr) override;
    void visit(IdentifierExpression *expr) override;
    void visit(CallExpression *expr) override;
    void visit(LambdaExpression *expr) override;
    void visit(CommandExpression *expr) override;
    void visit(ArrayExpression *expr) override;
    void visit(IndexExpression *expr) override;
    void visit(DictionaryExpression *expr) override;
    void visit(StringExpression *expr) override;
    void visit(AccessExpression *expr) override;

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
};

}