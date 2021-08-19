#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <ankh/lang/expr_result.h>
#include <ankh/lang/expr.h>
#include <ankh/lang/statement.h>
#include <ankh/lang/program.h>
#include <ankh/lang/lambda.h>
#include <ankh/lang/env.h>
#include <ankh/lang/callable.h>
#include <ankh/lang/types/data.h>

namespace ankh::lang {

class Interpreter
    : public ExpressionVisitor
    , public StatementVisitor<void>
{
public:
    Interpreter();

    void interpret(const Program& program);

    virtual ExprResult evaluate(const ExpressionPtr& expr);
    void execute(const StatementPtr& stmt);

    void execute_block(const BlockStatement *stmt, EnvironmentPtr<ExprResult> environment);

    inline const Environment<ExprResult>& environment() const noexcept
    {
        return *current_env_;
    }

    inline const std::unordered_map<std::string, CallablePtr>& functions() const noexcept
    {
        return functions_;
    }

    void resolve(const Expression *expr, size_t hops);
    void resolve(const Statement *stmt, size_t hops);

private:
    virtual ExprResult visit(BinaryExpression *expr) override;
    virtual ExprResult visit(UnaryExpression *expr) override;
    virtual ExprResult visit(LiteralExpression *expr) override;
    virtual ExprResult visit(ParenExpression *expr) override;
    virtual ExprResult visit(IdentifierExpression *expr) override;
    virtual ExprResult visit(CallExpression *expr) override;
    virtual ExprResult visit(LambdaExpression *expr) override;
    virtual ExprResult visit(CommandExpression *cmd) override;
    virtual ExprResult visit(ArrayExpression *cmd) override;
    virtual ExprResult visit(IndexExpression *cmd) override;
    virtual ExprResult visit(DictionaryExpression *expr) override;
    virtual ExprResult visit(StringExpression *expr) override;
    virtual ExprResult visit(AccessExpression *expr) override;

    virtual void visit(PrintStatement *stmt) override;
    virtual void visit(ExpressionStatement *stmt) override;
    virtual void visit(VariableDeclaration *stmt) override;
    virtual void visit(AssignmentStatement *stmt) override;
    virtual void visit(CompoundAssignment* stmt) override;
    virtual void visit(ModifyStatement* stmt) override;
    virtual void visit(CompoundModify* expr) override;
    virtual void visit(IncOrDecIdentifierStatement* stmt) override;
    virtual void visit(IncOrDecAccessStatement* stmt) override;
    virtual void visit(BlockStatement *stmt) override;
    virtual void visit(IfStatement *stmt) override;
    virtual void visit(WhileStatement *stmt) override;
    virtual void visit(ForStatement *stmt) override;
    virtual void visit(BreakStatement *stmt) override;
    virtual void visit(FunctionDeclaration *stmt) override;
    virtual void visit(ReturnStatement *stmt) override;
    virtual void visit(DataDeclaration *stmt) override;

    std::string substitute(const StringExpression *expr);
    ExprResult evaluate_single_expr(const std::string& str);

    std::optional<ExprResult> lookup(const Token& name, const Expression *expr);

private:
    EnvironmentPtr<ExprResult> current_env_;
    EnvironmentPtr<ExprResult> global_;
    std::unordered_map<const Expression*, size_t> locals_;

    class Scope {
    public:
        Scope(ankh::lang::Interpreter *interpreter, ankh::lang::EnvironmentPtr<ExprResult> enclosing);
        ~Scope();
    private:
        ankh::lang::Interpreter *interpreter_;
        ankh::lang::EnvironmentPtr<ExprResult> prev_;
    };

    // TODO: this assumes all functions are in global namespace
    // That's OK for now but needs to be revisited when implementing modules
    std::unordered_map<std::string, CallablePtr> functions_;

    std::unordered_map<std::string, bool> data_declarations_;

};

}

