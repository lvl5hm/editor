#ifndef PARSER_H
#include <lvl5_types.h>

typedef enum {
  T_NONE,
  
  T_TYPEDEF,
  T_KEYWORD_FIRST = T_TYPEDEF,
  T_POUND,
  T_IF,
  T_FOR,
  T_WHILE,
  T_DO,
  T_ELSE,
  T_SWITCH,
  T_CASE,
  T_DEFAULT,
  T_RETURN,
  T_BREAK,
  
  T_SIGNED,
  T_UNSIGNED,
  
  T_STRUCT,
  T_ENUM,
  T_UNION,
  T_EXTERN,
  T_AUTO,
  T_REGISTER,
  T_CONST,
  T_VOLATILE,
  
  T_INLINE,
  T_STATIC,
  T_CONTINUE,
  T_KEYWORD_LAST = T_CONTINUE,
  
  
  
  T_LPAREN,
  T_RPAREN,
  T_LBRACKET,
  T_RBRACKET,
  T_LCURLY,
  T_RCURLY,
  T_SINGLE_QUOTE,
  T_DOUBLE_QUOTE,
  
  T_COMMA,
  T_SEMI,
  T_DOT,
  T_ARROW,
  
  T_PLUS,
  T_OPERATOR_FIRST = T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_BACKSLASH,
  T_PERCENT,
  T_COLON,
  T_NOT,
  T_GREATER,
  T_LESS,
  T_LESS_EQUALS,
  T_GREATER_EQUALS,
  T_RSHIFT,
  T_LSHIFT,
  T_OR,
  T_AND,
  T_BIT_OR,
  T_BIT_AND,
  T_BIT_NOT,
  T_BIT_XOR,
  T_EQUALS,
  T_NOT_EQUALS,
  T_TRIPLE_DOT,
  T_DOUBLE_DOT,
  T_QUESTION,
  
  T_ASSIGN,
  T_ASSIGN_PLUS,
  T_ASSIGN_MINUS,
  T_ASSIGN_STAR,
  T_ASSIGN_SLASH,
  T_ASSIGN_RSHIFT,
  T_ASSIGN_LSHIFT,
  T_ASSIGN_BIT_OR,
  T_ASSIGN_BIT_AND,
  T_ASSIGN_PERCENT,
  T_ASSIGN_BIT_XOR,
  
  T_OPERATOR_LAST = T_ASSIGN_BIT_XOR,
  
  T_NAME,
  T_INT,
  T_FLOAT,
  T_STRING,
  T_CHAR,
} Token_Type;

#define arr_string(chars) {chars, array_count(chars)-1}

String Token_Kind_To_String[] = {
  [T_SIGNED] = arr_string("signed"),
  [T_UNSIGNED] = arr_string("unsigned"),
  [T_UNION] = arr_string("union"),
  [T_EXTERN] = arr_string("extern"),
  [T_AUTO] = arr_string("auto"),
  [T_REGISTER] = arr_string("register"),
  [T_CONST] = arr_string("const"),
  [T_VOLATILE] = arr_string("volatile"),
  
  [T_INLINE] = arr_string("inline"),
  [T_STATIC] = arr_string("static"),
  [T_RETURN] = arr_string("return"),
  [T_BREAK] = arr_string("break"),
  [T_STRUCT] = arr_string("struct"),
  [T_ENUM] = arr_string("enum"),
  [T_ELSE] = arr_string("else"),
  [T_IF] = arr_string("if"),
  [T_FOR] = arr_string("for"),
  [T_CONTINUE] = arr_string("continue"),
  [T_WHILE] = arr_string("while"),
  [T_TYPEDEF] = arr_string("typedef"),
  [T_SWITCH] = arr_string("switch"),
  [T_CASE] = arr_string("case"),
  [T_DEFAULT] = arr_string("default"),
  
  [T_NONE] = arr_string("NONE"),
  [T_NAME] = arr_string("NAME"),
  [T_INT] = arr_string("INT"),
  [T_CHAR] = arr_string("CHAR"),
  [T_FLOAT] = arr_string("FLOAT"),
  [T_LPAREN] = arr_string("("),
  [T_RPAREN] = arr_string(")"),
  [T_LBRACKET] = arr_string("["),
  [T_RBRACKET] = arr_string("]"),
  [T_LCURLY] = arr_string("{"),
  [T_RCURLY] = arr_string("}"),
  [T_NOT] = arr_string("!"),
  [T_BIT_NOT] = arr_string("~"),
  [T_SEMI] = arr_string(";"),
  [T_COMMA] = arr_string(","),
  [T_DOUBLE_QUOTE] = arr_string("\""),
  [T_SINGLE_QUOTE] = arr_string("'"),
  [T_DOT] = arr_string("."),
  [T_TRIPLE_DOT] = arr_string("..."),
  
  [T_STAR] = arr_string("*"),
  [T_SLASH] = arr_string("/"),
  [T_PERCENT] = arr_string("%"),
  [T_RSHIFT] = arr_string(">>"),
  [T_LSHIFT] = arr_string("<<"),
  [T_BIT_AND] = arr_string("&"),
  
  [T_PLUS] = arr_string("+"),
  [T_MINUS] = arr_string("-"),
  [T_BIT_OR] = arr_string("|"),
  [T_BIT_XOR] = arr_string("^"),
  
  [T_NOT_EQUALS] = arr_string("!="),
  [T_EQUALS] = arr_string("=="),
  [T_LESS] = arr_string("<"),
  [T_LESS_EQUALS] = arr_string("<="),
  [T_GREATER] = arr_string(">"),
  [T_GREATER_EQUALS] = arr_string(">="),
  
  [T_OR] = arr_string("||"),
  [T_AND] = arr_string("&&"),
  [T_QUESTION] = arr_string("?"),
  [T_POUND] = arr_string("#"),
  [T_BACKSLASH] = arr_string("\\"),
  [T_COLON] = arr_string(":"),
  
  [T_ASSIGN] = arr_string("="),
  [T_ASSIGN_BIT_OR] = arr_string("|="),
  [T_ASSIGN_BIT_AND] = arr_string("&="),
  [T_ASSIGN_PLUS] = arr_string("+="),
  [T_ASSIGN_STAR] = arr_string("*="),
  [T_ASSIGN_SLASH] = arr_string("/="),
  [T_ASSIGN_MINUS] = arr_string("-="),
  [T_ASSIGN_PERCENT] = arr_string("%="),
  [T_ASSIGN_BIT_XOR] = arr_string("^="),
};


enum {
  Syntax_DEFAULT,
  Syntax_BACKGROUND,
  Syntax_COMMENT,
  Syntax_TYPE,
  Syntax_MACRO,
  Syntax_FUNCTION,
  Syntax_ARG,
  Syntax_OPERATOR,
  Syntax_KEYWORD,
  Syntax_CURSOR,
  Syntax_NUMBER,
  Syntax_STRING,
  Syntax_ENUM_MEMBER,
  
  Syntax_count,
};

typedef i8 Syntax;

typedef struct Token Token;
typedef struct Symbol Symbol;
typedef struct Symbol {
  union {
    struct {
      Symbol *predecl;
      Symbol **members;
    } struct_def;
    struct {
      Symbol **args;
      Symbol *return_type;
    } function;
  };
  
  String name;
  Token *token;
  Syntax type;
} Symbol;

typedef struct Token {
  Token_Type type;
  i32 start;
  i32 end;
  Symbol declaration;
} Token;

typedef struct Buffer Buffer;




typedef struct Scope Scope;
typedef struct Scope {
  String *keys;
  Symbol *values;
  u32 count;
  u32 capacity;
  
  Scope *parent;
} Scope;

typedef struct Parser {
  Scope *scope;
  i32 token_index;
  Buffer *buffer;
} Parser;


#define PARSER_H
#endif