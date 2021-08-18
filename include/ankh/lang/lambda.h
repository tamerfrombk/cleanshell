#pragma once

#include <ankh/lang/expr.h>
#include <ankh/lang/statement.h>

namespace ankh::lang {

// This is here instead of in lang/expr.h because we need both expression and statement constructs
// to create a lambda
// Including this in expr.h will create a circular include problem between expr.h and statement.h
// since statement.h relies on expr.h which would rely and statement.h and so on
struct LambdaExpression 
    : public Expression 
{
    std::string generated_name;
    std::vector<Token> params;
    StatementPtr body;

    LambdaExpression(std::string generated_name, std::vector<Token> params, StatementPtr body)
        : generated_name(std::move(generated_name)), params(std::move(params)), body(std::move(body)) {}

    virtual ExprResult accept(ExpressionVisitor*visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<LambdaExpression>(generated_name, params, body->clone());
    }

    virtual std::string stringify() const noexcept override
    {
        std::string params = "";
        return "fn (" + params + ") " + body->stringify();
    }
};

}