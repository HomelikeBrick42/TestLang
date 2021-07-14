#include "bdd_for_c.h"
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef struct {
	u8* Data;
	u64 Length;
} String;

#define typeof __typeof__
#define cast(type) (type)
#define StringFromLiteral(s) ((String){ .Data = cast(u8*) (s), .Length = sizeof(s) - 1 })

#define TokenKindDecls \
    TokenKindDecl(EndOfFile, "EndOfFile") \
    TokenKindDecl(Invalid,   "Invalid") \
    \
    TokenKindDeclStart(Operator) \
        TokenKindDecl(Plus,            "+") \
        TokenKindDecl(Minus,           "-") \
        TokenKindDecl(Asterisk,        "*") \
        TokenKindDecl(Slash,           "/") \
        TokenKindDecl(Percent,         "%") \
        TokenKindDecl(ExclamationMark, "!") \
        \
        TokenKindDeclStart(Assignment) \
            TokenKindDecl(Equals,         "=") \
            TokenKindDecl(PlusEquals,     "+=") \
            TokenKindDecl(MinusEquals,    "-=") \
            TokenKindDecl(AsteriskEquals, "*=") \
            TokenKindDecl(SlashEquals,    "/=") \
            TokenKindDecl(PercentEquals,  "%=") \
        TokenKindDeclEnd(Assignment) \
        \
        TokenKindDeclStart(Comparison) \
            TokenKindDecl(EqualsEquals,          "==") \
            TokenKindDecl(ExclamationMarkEquals, "!=") \
            TokenKindDecl(LessThan,              "<") \
            TokenKindDecl(GreaterThan,           ">") \
            TokenKindDecl(LessThanEquals,        "<=") \
            TokenKindDecl(GreaterThanEquals,     ">=") \
        TokenKindDeclEnd(Comparison) \
    TokenKindDeclEnd(Operator) \
    \
    TokenKindDecl(LParen,    "(") \
    TokenKindDecl(RParen,    ")") \
    TokenKindDecl(LBrace,    "{") \
    TokenKindDecl(RBrace,    "}") \
    TokenKindDecl(LBracket,  "[") \
    TokenKindDecl(RBracket,  "]") \
    TokenKindDecl(Comma,     ",") \
    TokenKindDecl(Period,    ".") \
    TokenKindDecl(Semicolon, ";") \
    TokenKindDecl(Colon,     ":") \

typedef enum {
    TokenKind_,
#define TokenKindDeclStart(name) TokenKind__ ## name ## _Start,
#define TokenKindDeclEnd(name) TokenKind__ ## name ## _End,
#define TokenKindDecl(name, str) TokenKind_ ## name,
    TokenKindDecls
#undef TokenKindDeclStart
#undef TokenKindDeclEnd
#undef TokenKindDecl
} TokenKind;

String TokenKind_ToString(TokenKind kind) {
    switch (kind) {
    #define TokenKindDeclStart(name)
    #define TokenKindDeclEnd(name)
    #define TokenKindDecl(name, str) case TokenKind_ ## name: return StringFromLiteral(str);
        TokenKindDecls
    #undef TokenKindDeclStart
    #undef TokenKindDeclEnd
    #undef TokenKindDecl
        default: {
            assert(false);
            return (String){};
        } break;
    }
}

#define TokenKindDeclStart(name) \
    bool TokenKind_Is ## name(TokenKind kind) { \
        return kind > TokenKind__ ## name ## _Start && kind < TokenKind__ ## name ## _End; \
    }
#define TokenKindDeclEnd(name)
#define TokenKindDecl(name, str)
    TokenKindDecls
#undef TokenKindStart
#undef TokenKindEnd
#undef TokenKindDecl

typedef struct {
    TokenKind Kind;
    u64 Position;
    u64 Line;
    u64 Column;
    u64 Length;
} Token;

typedef struct {
	String Source;
    u64 Position;
    u64 Line;
    u64 Column;
    u8 Current;
} Lexer;

void Lexer_Create(Lexer* lexer, String source) {
    lexer->Source = source;
    lexer->Position = 0;
    lexer->Line = 1;
    lexer->Column = 1;
    lexer->Current = source.Length != 0 ? source.Data[0] : '\0';
}

u8 Lexer_NextChar(Lexer* lexer) {
    u8 current = lexer->Current;
    lexer->Position++;
    lexer->Column++;
    if (current == '\n') {
        lexer->Line++;
        lexer->Column = 1;
    }

    lexer->Current =
        (lexer->Position < lexer->Source.Length)
        ? lexer->Source.Data[lexer->Position]
        : '\0';
    
    return current;
}

Token Lexer_NextToken(Lexer* lexer) {
Start:
    u64 startPosition = lexer->Position;
    u64 startLine     = lexer->Line;
    u64 startColumn   = lexer->Column;

    switch (lexer->Current) {
        case '\0': {
            return (Token){
                .Kind     = TokenKind_EndOfFile,
                .Position = startPosition,
                .Line     = startLine,
                .Column   = startColumn,
                .Length   = 0,
            };
        } break;

        case ' ':
        case '\t':
        case '\n':
        case '\r': {
            Lexer_NextChar(lexer);
            goto Start;
        } break;

    #define SINGLE_CHAR(chr, kind) \
        case chr: { \
            Lexer_NextChar(lexer); \
            return (Token){ \
                .Kind     = kind, \
                .Position = startPosition, \
                .Line     = startLine, \
                .Column   = startColumn, \
                .Length   = 1, \
            }; \
        } break

        SINGLE_CHAR('(', TokenKind_LParen);
        SINGLE_CHAR(')', TokenKind_RParen);
        SINGLE_CHAR('{', TokenKind_LBrace);
        SINGLE_CHAR('}', TokenKind_RBrace);
        SINGLE_CHAR('[', TokenKind_LBracket);
        SINGLE_CHAR(']', TokenKind_RBracket);
        SINGLE_CHAR(',', TokenKind_Comma);
        SINGLE_CHAR('.', TokenKind_Period);
        SINGLE_CHAR(';', TokenKind_Semicolon);
        SINGLE_CHAR(':', TokenKind_Colon);

    #undef SINGLE_CHAR

    #define DOUBLE_CHAR(chr1, kind1, chr2, kind2) \
        case chr1: { \
            Lexer_NextChar(lexer); \
            if (lexer->Current == chr2) { \
                Lexer_NextChar(lexer); \
                return (Token){ \
                    .Kind     = kind2, \
                    .Position = startPosition, \
                    .Line     = startLine, \
                    .Column   = startColumn, \
                    .Length   = 2, \
                }; \
            } \
            return (Token){ \
                .Kind     = kind1, \
                .Position = startPosition, \
                .Line     = startLine, \
                .Column   = startColumn, \
                .Length   = 1, \
            }; \
        } break

        DOUBLE_CHAR('+', TokenKind_Plus,     '=', TokenKind_PlusEquals);
        DOUBLE_CHAR('-', TokenKind_Minus,    '=', TokenKind_MinusEquals);
        DOUBLE_CHAR('*', TokenKind_Asterisk, '=', TokenKind_AsteriskEquals);
        DOUBLE_CHAR('/', TokenKind_Slash,    '=', TokenKind_SlashEquals);
        DOUBLE_CHAR('%', TokenKind_Percent,  '=', TokenKind_PercentEquals);

        DOUBLE_CHAR('=', TokenKind_Equals,          '=', TokenKind_EqualsEquals);
        DOUBLE_CHAR('!', TokenKind_ExclamationMark, '=', TokenKind_ExclamationMarkEquals);
        DOUBLE_CHAR('<', TokenKind_LessThan,        '=', TokenKind_LessThanEquals);
        DOUBLE_CHAR('>', TokenKind_GreaterThan,     '=', TokenKind_GreaterThanEquals);

    #undef DOUBLE_CHAR

        default: {
            printf("Error: Unknown character '%c'\n", lexer->Current);
            Lexer_NextChar(lexer);
            return (Token){
                .Kind     = TokenKind_Invalid,
                .Position = startPosition,
                .Line     = startLine,
                .Column   = startColumn,
                .Length   = 1,
            };
        } break;
    }
}

bool Token_Equals(Token a, Token b) {
    return false;
}

spec("Lexer") {
    it("Single Characters") {
        Lexer lexer;
        Lexer_Create(&lexer, StringFromLiteral("+ - * / % ( ) [ ] { } , . ; :"));

        check(Token_Equals(Lexer_NextToken(&lexer), (Token){
            .Kind     = TokenKind_Plus,
            .Position = 0,
            .Line     = 1,
            .Column   = 1,
            .Length   = 1,
        }));

        check(Lexer_NextToken(&lexer).Kind == TokenKind_Minus);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_Asterisk);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_Slash);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_Percent);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_LParen);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_RParen);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_LBracket);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_RBracket);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_LBrace);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_RBrace);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_Comma);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_Period);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_Semicolon);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_Colon);
        check(Lexer_NextToken(&lexer).Kind == TokenKind_EndOfFile);
    }

    it("Double Characters") {
        Lexer lexer;
        Lexer_Create(&lexer, StringFromLiteral("+= -= *= /= >="));
    }
}
