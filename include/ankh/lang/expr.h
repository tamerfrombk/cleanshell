#pragma once

#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <ankh/lang/token.h>
#include <ankh/lang/expr_result.h>

namespace ankh::lang {

// forward declare our expression types for the visitor
struct BinaryExpression;
struct UnaryExpression;
struct LiteralExpression;
struct ParenExpression;
struct IdentifierExpression;
struct CallExpression;
struct LambdaExpression;
struct CommandExpression;
struct ArrayExpression;
struct IndexExpression;
struct DictionaryExpression;
struct StringExpression;
struct AccessExpression;

struct ExpressionVisitor {
    virtual ~ExpressionVisitor() = default;

    virtual ExprResult visit(BinaryExpression *expr) = 0;
    virtual ExprResult visit(UnaryExpression *expr) = 0;
    virtual ExprResult visit(LiteralExpression *expr) = 0;
    virtual ExprResult visit(ParenExpression *expr) = 0;
    virtual ExprResult visit(IdentifierExpression *expr) = 0;
    virtual ExprResult visit(CallExpression *expr) = 0;
    virtual ExprResult visit(LambdaExpression *expr) = 0;
    virtual ExprResult visit(CommandExpression *expr) = 0;
    virtual ExprResult visit(ArrayExpression *expr) = 0;
    virtual ExprResult visit(IndexExpression *expr) = 0;
    virtual ExprResult visit(DictionaryExpression *expr) = 0;
    virtual ExprResult visit(StringExpression *expr) = 0;
    virtual ExprResult visit(AccessExpression *expr) = 0;
};

struct Expression;
using ExpressionPtr = std::unique_ptr<Expression>;

struct Expression {
    virtual ~Expression() = default;

    virtual ExprResult    accept(ExpressionVisitor *visitor) = 0;
    virtual ExpressionPtr clone() const noexcept = 0;
    virtual std::string stringify() const noexcept = 0;
};

template <class T>
std::string stringify(const std::vector<T>& elems) noexcept {
    if (elems.empty()) {
        return "";
    }

    std::string result(elems[0]->stringify());
    for (size_t i = 1; i < elems.size(); ++i) {
        result += ", ";
        result += elems[i]->stringify();
    }

    return result;
}

template <class T, class... Args>
ExpressionPtr make_expression(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

struct BinaryExpression 
    : public Expression 
{
    ExpressionPtr left;
    Token op;
    ExpressionPtr right;

    BinaryExpression(ExpressionPtr left, Token op, ExpressionPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<BinaryExpression>(left->clone(), op, right->clone());
    }

    virtual std::string stringify() const noexcept override
    {
        return left->stringify() + " " + op.str + " " + right->stringify();
    }
};

struct UnaryExpression 
    : public Expression 
{
    Token op;
    ExpressionPtr right;

    UnaryExpression(Token op, ExpressionPtr right)
        : op(std::move(op)), right(std::move(right)) {}
    
    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<UnaryExpression>(op, right->clone());
    }

    virtual std::string stringify() const noexcept override
    {
        return op.str + right->stringify();
    }
};

struct LiteralExpression 
    : public Expression 
{
    Token literal;

    LiteralExpression(Token literal)
        : literal(std::move(literal)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<LiteralExpression>(literal);
    }

    virtual std::string stringify() const noexcept override
    {
        return literal.str;
    }
};

struct StringExpression 
    : public Expression 
{
    Token str;

    StringExpression(Token str)
        : str(std::move(str)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<StringExpression>(str);
    }

    virtual std::string stringify() const noexcept override
    {
        return str.str;
    }
};

struct ParenExpression 
    : public Expression 
{
    ExpressionPtr expr;

    ParenExpression(ExpressionPtr expr)
        : expr(std::move(expr)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<ParenExpression>(expr->clone());
    }

    virtual std::string stringify() const noexcept override
    {
        return expr->stringify();
    }
};

struct IdentifierExpression 
    : public Expression 
{
    Token name;

    IdentifierExpression(Token name)
        : name(std::move(name)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<IdentifierExpression>(name);
    }

    virtual std::string stringify() const noexcept override
    {
        return name.str;
    }
};

struct CallExpression 
    : public Expression 
{
    ExpressionPtr callee;
    std::vector<ExpressionPtr> args;

    CallExpression(ExpressionPtr callee, std::vector<ExpressionPtr> args)
        : callee(std::move(callee)), args(std::move(args)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        std::vector<ExpressionPtr> cloned;
        cloned.reserve(args.size());

        for (const auto& arg : args) {
            cloned.push_back(arg->clone());
        }

        return make_expression<CallExpression>(callee->clone(), std::move(cloned));
    }

    virtual std::string stringify() const noexcept override
    {
        return callee->stringify() + "(" + ankh::lang::stringify(args) + ")";
    }
};

struct CommandExpression 
    : public Expression 
{
    Token cmd;

    CommandExpression(Token cmd)
        : cmd(std::move(cmd)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<CommandExpression>(cmd);
    }

    virtual std::string stringify() const noexcept override
    {
        return cmd.str;
    }
};

struct ArrayExpression 
    : public Expression 
{
    std::vector<ExpressionPtr> elems;

    ArrayExpression(std::vector<ExpressionPtr> elems)
        : elems(std::move(elems)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        std::vector<ExpressionPtr> cloned;
        cloned.reserve(elems.size());

        for (const auto& elem : elems) {
            cloned.push_back(elem->clone());
        }

        return make_expression<ArrayExpression>(std::move(cloned));
    }

    size_t size() const noexcept
    {
        return elems.size();
    }

    virtual std::string stringify() const noexcept override
    {
        return "[" + ankh::lang::stringify(elems) + "]";
    }
};

struct IndexExpression 
    : public Expression 
{
    ExpressionPtr indexee;
    ExpressionPtr index;

    IndexExpression(ExpressionPtr indexee, ExpressionPtr index)
        : indexee(std::move(indexee)), index(std::move(index)) {}

    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<IndexExpression>(indexee->clone(), index->clone());
    }

    virtual std::string stringify() const noexcept override
    {
        return indexee->stringify() + "[" + index->stringify() + "]";
    }
};

struct DictionaryExpression 
    : public Expression
{
    std::vector<Entry<ExpressionPtr>> entries;

    DictionaryExpression(std::vector<Entry<ExpressionPtr>> entries)
        : entries(std::move(entries)) {}
    
    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        std::vector<Entry<ExpressionPtr>> cloned;
        for (const auto& [key, value] : entries) {
            cloned.emplace_back(key->clone(), value->clone());
        }

        return make_expression<DictionaryExpression>(std::move(cloned));
    }

    virtual std::string stringify() const noexcept override
    {
        if (entries.empty()) {
            return "{}";
        }

        std::string result("{\n");
        result += entries[0].key->stringify() + entries[0].value->stringify();
        for (const auto& [k, v] : entries) {
            result += "\n";
            result += k->stringify() + " : " + v->stringify();
        }
        result += "}\n";

        return result;
    }
};

struct AccessExpression 
    : public Expression
{
    ExpressionPtr accessible;
    Token accessor;

    AccessExpression(ExpressionPtr accessible, Token accessor)
        : accessible(std::move(accessible)), accessor(std::move(accessor)) {}
    
    virtual ExprResult accept(ExpressionVisitor *visitor) override
    {
        return visitor->visit(this);
    }

    virtual ExpressionPtr clone() const noexcept override
    {
        return make_expression<AccessExpression>(accessible->clone(), accessor);
    }

    virtual std::string stringify() const noexcept override
    {
        return accessible ->stringify() + "." + accessor.str;
    }
};

}
