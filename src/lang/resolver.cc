#include <ankh/lang/resolver.h>

#include <ankh/def.h>
#include <ankh/log.h>

#include <ankh/lang/exceptions.h>

ankh::lang::ResolutionTable ankh::lang::Resolver::resolve(const Program& program)
{
    table_.clear();

    for (const auto& stmt : program.statements) {
        stmt->accept(this);
    }

    return table_;
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(BinaryExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());

    resolve(expr->left);
    resolve(expr->right);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(UnaryExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());

    resolve(expr->right);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(LiteralExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(ParenExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());

    resolve(expr->expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(IdentifierExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());
    
    if (!scopes_.empty() && is_declared_but_not_defined(expr->name)) {
        lang::panic<ParseException>("{}:{}, a local variable cannot be initialized with a global variable of the same name", expr->name.line, expr->name.col);
    }

    resolve(expr, expr->name);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(CallExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());
    
    resolve(expr->callee);
    for (const auto& arg : expr->args) {
        resolve(arg);
    }

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(LambdaExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());
    
    const Token name(expr->generated_name, TokenType::IDENTIFIER, 0, 0);
    
    declare(name);
    define(name);

    begin_scope();
    for (const Token& param : expr->params) {
        declare(param);
        define(param);
    }
    resolve(expr->body);
    end_scope();

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(CommandExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());
    
    // TODO: at the moment, commands do not contain any expressions to resolve
    // but this is not really the case since we have InterpolationExpressions that _can_
    // have variables
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(ArrayExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());
    
    for (const auto& elem : expr->elems) {
        resolve(elem);
    }

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(IndexExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());
    
    resolve(expr->indexee);
    resolve(expr->index);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(DictionaryExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());
    
    begin_scope();
    for (const auto& [k, v] : expr->entries) {
        resolve(k);
        resolve(v);
    }
    end_scope();

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(StringExpression *expr)
{
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::Resolver::visit(AccessExpression *expr)
{
    ANKH_DEBUG("resolving '{}'", expr->stringify());
    
    resolve(expr->accessible);
    if (!scopes_.empty() && is_declared_but_not_defined(expr->accessor)) {
        panic<ParseException>("{}:{}, '{}' is not a member of '{}'", 
            expr->accessor.line, expr->accessor.col, expr->accessor.str, expr->accessible->stringify());
    }

    resolve(expr, expr->accessor);

    return {};
}

void ankh::lang::Resolver::visit(PrintStatement *stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());
    
    resolve(stmt->expr);
}

void ankh::lang::Resolver::visit(ExpressionStatement *stmt)
{
    resolve(stmt->expr);
}

void ankh::lang::Resolver::visit(VariableDeclaration *stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    declare(stmt->name);
    resolve(stmt->initializer);
    define(stmt->name);
}

void ankh::lang::Resolver::visit(AssignmentStatement *stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    resolve(stmt->initializer);
    resolve(stmt, stmt->name);
}

void ankh::lang::Resolver::visit(IncOrDecIdentifierStatement* stmt)
{
    resolve(stmt->expr);
}

void ankh::lang::Resolver::visit(IncOrDecAccessStatement *stmt)
{
    resolve(stmt->expr);
}

void ankh::lang::Resolver::visit(CompoundAssignment* stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    resolve(stmt->value);
    if (!scopes_.empty() && is_declared_but_not_defined(stmt->target)) {
        panic<ParseException>("{}:{}, '{}' is not defined", 
            stmt->target.line, stmt->target.col, stmt->target.str);
    }
    resolve(stmt, stmt->target);
}

void ankh::lang::Resolver::visit(ModifyStatement* stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    resolve(stmt->value);

    resolve(stmt->object);
    if (!scopes_.empty() && is_declared_but_not_defined(stmt->name)) {
        panic<ParseException>("{}:{}, '{}' is not a member of '{}'", 
            stmt->name.line, stmt->name.col, stmt->name.str, stmt->object->stringify());
    }
    resolve(stmt, stmt->name);
}

void ankh::lang::Resolver::visit(CompoundModify* stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    resolve(stmt->value);

    resolve(stmt->object);
    if (!scopes_.empty() && is_declared_but_not_defined(stmt->name)) {
        panic<ParseException>("{}:{}, '{}' is not a member of '{}'", 
            stmt->name.line, stmt->name.col, stmt->name.str, stmt->object->stringify());
    }
    resolve(stmt, stmt->name);
}

void ankh::lang::Resolver::visit(BlockStatement *stmt)
{
    begin_scope();
    resolve(stmt->statements);
    end_scope();
}

void ankh::lang::Resolver::visit(IfStatement *stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    resolve(stmt->condition);
    resolve(stmt->then_block);
    if (stmt->else_block != nullptr) {
        resolve(stmt->else_block);
    }
}

void ankh::lang::Resolver::visit(WhileStatement *stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    resolve(stmt->condition);
    resolve(stmt->body);
}

void ankh::lang::Resolver::visit(ForStatement *stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    begin_scope();

    if (stmt->init != nullptr)      { resolve(stmt->init);      }
    if (stmt->condition != nullptr) { resolve(stmt->condition); }
    if (stmt->mutator != nullptr)   { resolve(stmt->mutator);   }
    resolve(stmt->body);
    
    end_scope();
}

void ankh::lang::Resolver::visit(BreakStatement *stmt)
{
    ANKH_UNUSED(stmt);
}

void ankh::lang::Resolver::visit(FunctionDeclaration *stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    declare(stmt->name);
    define(stmt->name);

    begin_scope();
    for (const Token& param : stmt->params) {
        declare(param);
        define(param);
    }
    resolve(stmt->body);
    end_scope();
}

void ankh::lang::Resolver::visit(ReturnStatement *stmt)
{
    resolve(stmt->expr);
}

void ankh::lang::Resolver::visit(DataDeclaration *stmt)
{
    ANKH_DEBUG("resolving '{}'", stmt->stringify());

    declare(stmt->name);
    define(stmt->name);

    begin_scope();
    for (const Token& member : stmt->members) {
        declare(member);
        define(member);
    }
    end_scope();
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
            const size_t hops = (it - scopes_.crbegin());
            ANKH_DEBUG("resolver: '{}' @ {} is '{}' hops away from the current environment '{}'",
                name.str, static_cast<const void*>(expr) , hops, scopes_.size());
            ANKH_VERIFY(table_.expr_hops.count(expr) == 0);
            table_.expr_hops[expr] = hops;
            return;
        }
    }
}

void ankh::lang::Resolver::resolve(const Statement *stmt, const Token& name)
{
    if (scopes_.empty()) {
        return;
    }

    for (auto it = scopes_.crbegin(); it != scopes_.crend(); ++it) {
        if (it->count(name.str) > 0) {
            const size_t hops = (it - scopes_.crbegin());
            ANKH_DEBUG("resolver: '{}' @ {} is '{}' hops away from the current environment '{}'",
                name.str, static_cast<const void*>(stmt) , hops, scopes_.size());
            ANKH_VERIFY(table_.stmt_hops.count(stmt) == 0);
            table_.stmt_hops[stmt] = hops;
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

bool ankh::lang::Resolver::is_declared_but_not_defined(const Token& name) const noexcept
{
    // Yes, at() is not nothrow but it is effectively nothrow here
    return top().count(name.str) > 0 && top().at(name.str) == false;
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

const ankh::lang::Resolver::Scope& ankh::lang::Resolver::top() const noexcept
{
    return scopes_.back();
}