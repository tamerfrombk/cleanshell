#include <ankh/lang/token.hpp>
#include <ankh/log.hpp>

std::string ankh::lang::token_type_str(ankh::lang::TokenType type) noexcept {
    switch (type) {
    case ankh::lang::TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case ankh::lang::TokenType::EQ:
        return "EQ";
    case ankh::lang::TokenType::EQEQ:
        return "EQEQ";
    case ankh::lang::TokenType::NEQ:
        return "NEQ";
    case ankh::lang::TokenType::LT:
        return "LT";
    case ankh::lang::TokenType::LTE:
        return "LTE";
    case ankh::lang::TokenType::GT:
        return "GT";
    case ankh::lang::TokenType::GTE:
        return "GTE";
    case ankh::lang::TokenType::MINUS:
        return "MINUS";
    case ankh::lang::TokenType::MINUSEQ:
        return "MINUSEQ";
    case ankh::lang::TokenType::PLUS:
        return "PLUS";
    case ankh::lang::TokenType::PLUSEQ:
        return "PLUSEQ";
    case ankh::lang::TokenType::FSLASH:
        return "FSLASH";
    case ankh::lang::TokenType::FSLASHEQ:
        return "FSLASHEQ";
    case ankh::lang::TokenType::STAR:
        return "STAR";
    case ankh::lang::TokenType::STAREQ:
        return "STAREQ";
    case ankh::lang::TokenType::BANG:
        return "BANG";
    case ankh::lang::TokenType::LPAREN:
        return "LPAREN";
    case ankh::lang::TokenType::RPAREN:
        return "RPAREN";
    case ankh::lang::TokenType::LBRACE:
        return "LBRACE";
    case ankh::lang::TokenType::RBRACE:
        return "RBRACE";
    case ankh::lang::TokenType::LBRACKET:
        return "LBRACKET";
    case ankh::lang::TokenType::RBRACKET:
        return "RBRACKET";
    case ankh::lang::TokenType::ANKH_TRUE:
        return "BTRUE";
    case ankh::lang::TokenType::ANKH_FALSE:
        return "BFALSE";
    case ankh::lang::TokenType::NIL:
        return "NIL";
    case ankh::lang::TokenType::IF:
        return "IF";
    case ankh::lang::TokenType::ELSE:
        return "ELSE";
    case ankh::lang::TokenType::AND:
        return "AND";
    case ankh::lang::TokenType::OR:
        return "OR";
    case ankh::lang::TokenType::WHILE:
        return "WHILE";
    case ankh::lang::TokenType::FOR:
        return "FOR";
    case ankh::lang::TokenType::BREAK:
        return "BREAK";
    case ankh::lang::TokenType::SEMICOLON:
        return "SEMICOLON";
    case ankh::lang::TokenType::LET:
        return "LET";
    case ankh::lang::TokenType::COMMA:
        return "COMMA";
    case ankh::lang::TokenType::FN:
        return "FN";
    case ankh::lang::TokenType::ANKH_RETURN:
        return "RETURN";
    case ankh::lang::TokenType::DATA:
        return "DATA";
    case ankh::lang::TokenType::INC:
        return "INC";
    case ankh::lang::TokenType::DEC:
        return "DEC";
    case ankh::lang::TokenType::COLON:
        return "COLON";
    case ankh::lang::TokenType::DOT:
        return "DOT";
    case ankh::lang::TokenType::NUMBER:
        return "NUMBER";
    case ankh::lang::TokenType::STRING:
        return "STRING";
    case ankh::lang::TokenType::COMMAND:
        return "COMMAND";
    case ankh::lang::TokenType::ANKH_EOF:
        return "EOF";
    case ankh::lang::TokenType::UNKNOWN:
        return "UNKNOWN";
    default:
        ANKH_FATAL("unknown token type: this should never happen");
    }
}
