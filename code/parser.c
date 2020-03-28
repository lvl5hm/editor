#include "parser.h"



String token_to_string(Buffer *b, Token *t) {
  String result = buffer_part_to_string(b, t->start, t->end);
  return result;
}

b32 is_digit(char c) {
  b32 result = c >= '0' && c <= '9';
  return result;
}

b32 is_alpha(char c) {
  b32 result = (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') || (c == '_');
  return result;
}

b32 is_whitespace(char c) {
  b32 result = c == ' ' || c == '\n' || c == '\t' || c == '\r';
  return result;
}



void set_color(Parser *p, Token *t, Syntax color) {
  begin_profiler_function();
  for (i32 i = t->start; i < t->end; i++) {
    p->buffer->cache.colors[i] = color;
  }
  end_profiler_function();
}

void set_color_by_type(Parser *p, Token *t) {
  Syntax syntax = Syntax_DEFAULT;
  if (t->type >= T_KEYWORD_FIRST && t->type <= T_KEYWORD_LAST ||
      t->type == T_POUND) {
    syntax = Syntax_KEYWORD;
  } else if (t->type >= T_OPERATOR_FIRST && t->type <= T_OPERATOR_LAST) {
    syntax = Syntax_OPERATOR;
  } else if (t->type == T_INT_LITERAL || t->type == T_FLOAT_LITERAL) {
    syntax = Syntax_NUMBER;
  } else if (t->type == T_STRING_LITERAL || t->type == T_CHAR_LITERAL) {
    syntax = Syntax_STRING;
  } else if (t->type >= T_TYPE_FIRST && t->type <= T_TYPE_LAST) {
    syntax = Syntax_TYPE;
  }
  set_color(p, t, syntax);
}

i32 get_gap_count(Buffer *);
i32 get_gap_start(Buffer *);


typedef struct {
  String keys[128];
  Syntax values[128];
} Keyword_Map;

Keyword_Map keyword_map;

u32 hash_string(String key) {
  u32 hash = 5381;
  for (i32 i = 0; i < key.count; i++) {
    hash = hash*33 ^ key.data[i];
  }
  
  return hash;
}

u32 keyword_map_get_index(Keyword_Map *map, String name) {
  u32 hash = hash_string(name);
  u32 result = 0;
  
  while (true) {
    hash = hash % array_count(map->keys);
    String key = map->keys[hash];
    
    if (key.count == 0) {
      result = hash;
      break;
    } else if (string_compare(key, name)) {
      result = hash;
      break;
    } else {
      hash++;
    }
  }
  
  return result;
}


Token_Type get_keyword_type(String token_string) {
  begin_profiler_function();
  
  u32 index = keyword_map_get_index(&keyword_map, token_string);
  Syntax result = keyword_map.values[index];
  
  end_profiler_function();
  return result;
}

void buffer_tokenize(Parser *p) {
  begin_profiler_function();
  Buffer *b = p->buffer;
  
  i32 gap_count = get_gap_count(b);
  i32 gap_start = get_gap_start(b);
  
  
  Token t = {0};
  i32 i = 0;
  i32 add = 0;
  
  if (i == gap_start) {
    add = gap_count;
  }
  
  
#define get(index) b->data[i + index + add]
  
#define next() { \
    i++; \
    if (i == gap_start) add = gap_count;\
  }
#define skip_syntax(syntax) { \
    p->buffer->cache.colors[i] = syntax; \
    next(); \
    t.start++; \
  }
#define eat() { \
    next(); \
  }
#define end_no_continue(tok_type) { \
    t.type = tok_type; \
    t.end = i; \
    set_color_by_type(p, &t); \
    sb_push(p->buffer->cache.tokens, t); \
    t = (Token){ .start = i }; \
  }
#define end(tok_type) end_no_continue(tok_type); continue;
  
#define case1(ch0, type0) \
  case ch0: { \
    eat(); \
    end(type0); \
  } break;
#define case2(ch0, type0, ch1, type1) \
  case ch0: { \
    eat(); \
    if (get(0) == ch1) { \
      eat(); \
      end(type1); \
    } else { \
      end(type0); \
    } \
  } break;
#define case3(ch0, type0, ch1, type1, ch2, type2) \
  case ch0: { \
    eat(); \
    if (get(0) == ch1) { \
      eat(); \
      end(type1); \
    } else if (get(0) == ch2) { \
      eat(); \
      end(type2); \
    } else { \
      end(type0); \
    } \
  } break;
  
  while (true) {
    while (get(0) == ' ' || get(0) == '\n' || get(0) == '\r') {
      skip_syntax(Syntax_DEFAULT);
    }
    
    switch (get(0)) {
      case 0: {
        goto end;
      } break;
      
      case '\r': {
        skip_syntax(Syntax_DEFAULT);
      } break;
      case ' ': {
        skip_syntax(Syntax_DEFAULT);
      } break;
      case '\n': {
        skip_syntax(Syntax_DEFAULT);
      } break;
      case '"': {
        eat();
        while (true) {
          if (get(0) == '\\') {
            eat();
            eat();
          } else if (get(0) == '"') {
            break;
          } else if (get(0) == '\n') {
            break;
          } else if (get(0) == '\0') {
            i--;
            break;
          } else {
            eat();
          }
        }
        eat();
        end(T_STRING_LITERAL);
      } break;
      case '\'': {
        eat();
        
        while (true) {
          if (get(0) == '\\') {
            eat();
            eat();
          } else if (get(0) == '\'') {
            break;
          } else if (get(0) == '\n') {
            break;
          } else if (get(0) == '\0') {
            i--;
            break;
          } else {
            eat();
          }
        }
        eat();
        end(T_CHAR_LITERAL);
      } break;
      
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        if (get(0) == '0' && (
          get(1) == 'x' || get(1) == 'X')) 
        {
          eat();
          eat();
          while (is_digit(get(0)) || is_alpha(get(0))) eat();
          
          end(T_INT_LITERAL);
        } else {
          eat();
          while (is_digit(get(0))) eat();
          
          // float
          if (get(0) == '.') {
            eat();
            //if (!is_digit((get0))) syntax_error("Unexpected symbol %c", *stream);
            
            while (is_digit(get(0))) eat();
            if (get(0) == 'f') eat();
            end(T_FLOAT_LITERAL);
          } else {
            if (get(0) == 'L') eat();
            if (get(0) == 'L') eat();
            end(T_INT_LITERAL);
          }
        }
      } break;
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
      case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
      case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
      case 'v': case 'w': case 'x': case 'y': case 'z':
      
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
      case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
      case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
      case 'V': case 'W': case 'X': case 'Y': case 'Z':
      
      case '_': {
        eat();
        while (is_digit(get(0)) || is_alpha(get(0))) eat();
        
        
        String token_string = buffer_part_to_string(b, t.start, i);
        Token_Type type = get_keyword_type(token_string);
        
        if (type) {
          end(type);
        } else {
          end(T_NAME);
        }
      } break;
      
      case '/': {
        if (get(1) == '/') {
          skip_syntax(Syntax_COMMENT);
          skip_syntax(Syntax_COMMENT);
          while (get(0) != '\n') {
            skip_syntax(Syntax_COMMENT);
          }
        } else if (get(1) == '*') {
          skip_syntax(Syntax_COMMENT);
          skip_syntax(Syntax_COMMENT);
          while (!(get(0) == '*' && get(1) == '/')) {
            if (get(0) == '\0') {
              goto end;
            } else {
              skip_syntax(Syntax_COMMENT);
            }
          }
          skip_syntax(Syntax_COMMENT);
          skip_syntax(Syntax_COMMENT);
        } else {
          eat();
          if (get(0) == '=') {
            end(T_ASSIGN_SLASH);
          } else {
            end(T_SLASH);
          }
        }
      } break;
      
      case '#': {
        eat();
        while (is_digit(get(0)) || is_alpha(get(0))) eat();
        end_no_continue(T_POUND);
        
        while (get(0) == ' ') {
          skip_syntax(Syntax_DEFAULT);
        }
        
        i32 saved = i;
        if (get(0) == '<') {
          while (get(0) != '>') {
            eat();
          }
          eat();
          end(T_STRING_LITERAL);
        } else {
          i = saved;
        }
      } break;
      
      case1('?', T_QUESTION);
      case1('\\', T_BACKSLASH);
      case1(',', T_COMMA);
      case1(';', T_SEMI);
      case1('(', T_LPAREN);
      case1(')', T_RPAREN);
      case1('[', T_LBRACKET);
      case1(']', T_RBRACKET);
      case1('{', T_LCURLY);
      case1('}', T_RCURLY);
      case1('~', T_BIT_NOT);
      case1(':', T_COLON);
      
      case2('=', T_ASSIGN, '=', T_EQUALS);
      case2('!', T_NOT, '=', T_NOT_EQUALS);
      case2('+', T_PLUS, '=', T_ASSIGN_PLUS);
      case2('*', T_STAR, '=', T_ASSIGN_STAR);
      case2('^', T_BIT_XOR, '=', T_ASSIGN_BIT_XOR);
      case2('%', T_PERCENT, '=', T_ASSIGN_PERCENT);
      
      case3('-', T_MINUS, '=', T_ASSIGN_MINUS, '>', T_ARROW);
      case3('.', T_DOT, '.', T_DOUBLE_DOT, '.', T_TRIPLE_DOT);
      case3('>', T_GREATER, '=', T_GREATER_EQUALS, '>', T_RSHIFT);
      case3('<', T_LESS, '=', T_LESS_EQUALS, '<', T_LSHIFT);
      case3('|', T_BIT_OR, '|', T_OR, '=', T_ASSIGN_BIT_OR);
      case3('&', T_BIT_AND, '&', T_AND, '=', T_ASSIGN_BIT_AND);
      
      default: {
        eat();
        end(T_NONE);
      } break;
    }
  }
  
  end:
  
  b->cache.colors[b->count-1] = Syntax_DEFAULT;
  end_no_continue(T_END_OF_FILE);
  
  end_profiler_function();
}


u32 scope_get_index(Scope *scope, String name) {
  u32 hash = hash_string(name);
  u32 result = 0;
  
  while (true) {
    hash = hash % scope->capacity;
    String key = scope->keys[hash];
    
    if (!scope->occupancy[hash]) {
      result = hash;
      break;
    } else if (string_compare(key, name)) {
      result = hash;
      break;
    } else {
      hash++;
    }
  }
  
  return result;
}

void scope_insert_symbol(Scope *scope, String name, Symbol s) {
  i32 symbol_index = scope_get_index(scope, name);
  scope->keys[symbol_index] = s.name;
  scope->values[symbol_index] = s;
  scope->occupancy[symbol_index] = true;
  scope->count++;
  assert(scope->count <= scope->capacity);
}

void add_symbol(Scope *scope, String name, Syntax type) {
  begin_profiler_function();
  
  Symbol s = (Symbol){ .type = type };
  s.name = make_string(alloc_array(char, name.count), name.count);
  copy_memory_slow(s.name.data, name.data, name.count);
  scope_insert_symbol(scope, name, s);
  
  end_profiler_function();
}

void add_symbol_buffer(Parser *p, String name, Syntax type) {
  add_symbol(p->scope, name, type);
}

Symbol *get_symbol_in_scope(Scope *scope, String symbol_name) {
  begin_profiler_function();
  
  u32 index = scope_get_index(scope, symbol_name);
  Symbol *result = null;
  
  if (!scope->occupancy[index]) {
    if (scope->parent) {
      result = get_symbol_in_scope(scope->parent, symbol_name);
    }
  } else {
    result = scope->values + index;
  }
  
  end_profiler_function();
  return result;
}

Scope *add_scope(Scope *parent, u32 capacity) {
  begin_profiler_function();
  
  Scope *result = alloc_struct(Scope);
  *result = (Scope){
    .parent = parent,
    .capacity = capacity,
    .count = 0,
    .keys = alloc_array(String, capacity),
    .values = alloc_array(Symbol, capacity),
    .occupancy = alloc_array(bool, capacity),
  };
  
  memset(result->occupancy, 0, sizeof(bool)*capacity);
  
  end_profiler_function();
  return result;
}


Symbol *get_symbol(Parser *p, Token *t) {
  String token_string = token_to_string(p->buffer, t);
  Symbol *result = get_symbol_in_scope(p->scope, token_string);
  
  return result;
}

Token *peek_token(Parser *p, i32 offset) {
  Token *result = p->buffer->cache.tokens + p->token_index + offset;
  return result;
}

void next_token(Parser *p) {
  p->token_index++;
  assert(p->token_index <= (i32)sb_count(p->buffer->cache.tokens));
}

bool accept_token(Parser *p, Token_Type type) {
  bool result = false;
  Token *t = peek_token(p, 0);
  
  if (type == t->type) {
    result = true;
    if (type != T_END_OF_FILE) {
      next_token(p);
    }
  }
  return result;
}




Token *parse_declarator(Parser *, bool, bool);
bool parse_decl_specifier(Parser *);
bool parse_decl(Parser *);
bool parse_decl_specifier(Parser *);
void parse_any(Parser *);


Token *parse_direct_declarator(Parser *p, bool is_typedef, bool is_arg) {
  begin_profiler_function();
  Token *result = null;
  
  if (accept_token(p, T_NAME)) {
    result = peek_token(p, -1);
    
    if (is_typedef) {
      // token_to_string is scratch
      add_symbol_buffer(p, token_to_string(p->buffer, result), Syntax_TYPE);
      set_color(p, result, Syntax_TYPE);
    } else if (is_arg) {
      add_symbol_buffer(p, token_to_string(p->buffer, result), Syntax_ARG);
      set_color(p, result, Syntax_ARG);
    }
  } else if (accept_token(p, T_LPAREN)) {
    result = parse_declarator(p, is_typedef, is_arg);
    accept_token(p, T_RPAREN);
  }
  
  if (accept_token(p, T_LBRACKET)) {
    while (!(accept_token(p, T_RBRACKET) ||
             accept_token(p, T_END_OF_FILE))) {
      parse_any(p);
    }
  } else if (accept_token(p, T_LPAREN)) {
    // function decl
    if (!is_typedef) {
      add_symbol_buffer(p, token_to_string(p->buffer, result), Syntax_FUNCTION);
      set_color(p, result, Syntax_FUNCTION);
    }
    
    p->scope = add_scope(p->scope, 32);
    do {
      if (accept_token(p, T_RPAREN)) {
        break;
      }
      while (parse_decl_specifier(p));
      parse_declarator(p, false, true);
    } while (accept_token(p, T_COMMA));
    
    if (accept_token(p, T_LCURLY)) {
      while (!(accept_token(p, T_RCURLY) ||
               accept_token(p, T_END_OF_FILE))) {
        parse_any(p);
      }
    }
    p->scope = p->scope->parent;
  }
  
  end_profiler_function();
  return result;
}

Token *parse_declarator(Parser *p, bool is_typedef, bool is_arg) {
  accept_token(p, T_STAR);
  return parse_direct_declarator(p, is_typedef, is_arg);
} 


bool parse_initializer(Parser *p) {
  begin_profiler_function();
  while (!(accept_token(p, T_SEMI) || accept_token(p, T_END_OF_FILE))) 
  {
    parse_any(p);
  }
  end_profiler_function();
  return true;
}

bool parse_init_declarator(Parser *p, bool is_typedef) {
  begin_profiler_function();
  bool result = false;
  if (parse_declarator(p, is_typedef, false)) {
    if (accept_token(p, T_ASSIGN)) {
      if (parse_initializer(p)) {
        result = true;
      }
    } else {
      result = true;
    }
  }
  end_profiler_function();
  return result;
}

bool parse_struct_specifier(Parser *p) {
  bool result = false;
  begin_profiler_function();
  
  if (accept_token(p, T_STRUCT) || accept_token(p, T_UNION) || accept_token(p, T_CLASS)) {
    bool has_name = false, has_body = false;
    if (accept_token(p, T_NAME)) {
      Token *struct_name = peek_token(p, -1);
      set_color(p, struct_name, Syntax_TYPE);
      // token_to_string is scratch
      add_symbol_buffer(p, token_to_string(p->buffer, struct_name), Syntax_TYPE);
      has_name = true;
    }
    
    if (accept_token(p, T_LCURLY)) {
      p->scope = add_scope(p->scope, 32);
      
      while (!(accept_token(p, T_RCURLY) ||
               accept_token(p, T_END_OF_FILE))) {
        if (parse_decl_specifier(p)) {
          while (parse_decl_specifier(p));
          do {
            parse_declarator(p, false, false);
          } while (accept_token(p, T_COMMA));
          accept_token(p, T_SEMI);
        } else {
          parse_any(p);
        }
      }
      
      p->scope = p->scope->parent;
      has_body = true;
    }
    result = has_name || has_body;
  }
  
  end_profiler_function();
  return result;
}

bool parse_typename(Parser *p) {
  begin_profiler_function();
  
  Token *t = peek_token(p, 0);
  Symbol *s = get_symbol(p, t);
  bool result = false;
  if (s && s->type == Syntax_TYPE) {
    set_color(p, t, Syntax_TYPE);
    next_token(p);
    result = true;
  }
  end_profiler_function();
  return result;
}

bool parse_enum_specifier(Parser *p) {
  begin_profiler_function();
  bool result = false;
  if (accept_token(p, T_ENUM)) {
    bool has_name = false, has_body = false;
    if (accept_token(p, T_NAME)) {
      Token *struct_name = peek_token(p, -1);
      set_color(p, struct_name, Syntax_TYPE);
      // token_to_string is scratch
      add_symbol_buffer(p, token_to_string(p->buffer, struct_name), Syntax_TYPE);
      has_name = true;
    }
    
    if (accept_token(p, T_LCURLY)) {
      p->scope = add_scope(p->scope, 128);
      
      while (!(accept_token(p, T_RCURLY) ||
               accept_token(p, T_END_OF_FILE))) {
        if (accept_token(p, T_NAME)) {
          Token *name = peek_token(p, -1);
          set_color(p, name, Syntax_ENUM_MEMBER);
          add_symbol_buffer(p, token_to_string(p->buffer, name), Syntax_ENUM_MEMBER);
          if (accept_token(p, T_ASSIGN)) {
            while (!accept_token(p, T_COMMA)) {
              next_token(p);
            }
          }
        } else {
          parse_any(p);
        }
      }
      
      p->scope = p->scope->parent;
      has_body = true;
    }
    result = has_name || has_body;
  }
  
  end_profiler_function();
  return result;
}

bool parse_decl_specifier(Parser *p) {
  Token *t = peek_token(p, 0);
  switch (t->type) {
    case T_CHAR:
    case T_SHORT:
    case T_INT:
    case T_LONG:
    case T_FLOAT:
    case T_DOUBLE:
    case T_VOID:
    
    case T_STATIC:
    case T_TYPEDEF:
    case T_EXTERN:
    case T_AUTO:
    case T_REGISTER:
    
    case T_SIGNED:
    case T_UNSIGNED:
    
    case T_INLINE:
    case T_CONST: 
    case T_VOLATILE: {
      next_token(p);
      return true;
    } break;
    
    case T_NAME: {
      return parse_typename(p);
    } break;
    
    case T_STRUCT:
    case T_UNION: {
      return parse_struct_specifier(p);
    } break;
    
    case T_ENUM: {
      return parse_enum_specifier(p);
    } break;
  }
  return false;
}

bool parse_decl(Parser *p) {
  i32 saved = p->token_index;
  begin_profiler_function();
  
  bool result = false;
  bool is_typedef = false;
  if (parse_decl_specifier(p)) {
    if (peek_token(p, -1)->type == T_TYPEDEF) {
      is_typedef = true;
    }
    while (parse_decl_specifier(p));
    do {
      parse_init_declarator(p, is_typedef);
    } while (accept_token(p, T_COMMA));
    accept_token(p, T_SEMI);
    result = true;
  } else {
    p->token_index = saved;
  }
  
  end_profiler_function();
  return result;
}


Buffer *get_existing_buffer(Editor *editor, String path) {
  begin_profiler_function();
  Buffer *result = null;
  for (u32 buffer_index = 0; 
       buffer_index < sb_count(editor->buffers);
       buffer_index++) 
  {
    Buffer *b = editor->buffers + buffer_index;
    if (string_compare(b->path, path)) {
      result = b;
      break;
    }
  }
  end_profiler_function();
  return result;
}


void parse_any(Parser *p) {
  begin_profiler_function();
  Token *t = peek_token(p, 0);
  switch (t->type) {
    case T_CHAR:
    case T_SHORT:
    case T_INT:
    case T_LONG:
    case T_FLOAT:
    case T_DOUBLE:
    case T_VOID:
    
    case T_STATIC:
    case T_TYPEDEF:
    case T_EXTERN:
    case T_AUTO:
    case T_REGISTER:
    
    case T_SIGNED:
    case T_UNSIGNED:
    
    case T_INLINE:
    case T_CONST: 
    case T_VOLATILE: {
      bool is_typedef = false;
      if (t->type == T_TYPEDEF) {
        is_typedef = true;
      }
      next_token(p);
      while (parse_decl_specifier(p));
      do {
        parse_init_declarator(p, is_typedef);
      } while (accept_token(p, T_COMMA));
      accept_token(p, T_SEMI);
    } break;
    
    case T_NAME: {
      Symbol *s = get_symbol(p, t);
      if (s) {
        switch (s->type) {
          case Syntax_TYPE: {
            set_color(p, t, Syntax_TYPE);
            next_token(p);
            while (parse_decl_specifier(p));
            do {
              parse_init_declarator(p, false);
            } while (accept_token(p, T_COMMA));
            accept_token(p, T_SEMI);
          } break;
          
          case Syntax_FUNCTION: 
          set_color(p, t, Syntax_FUNCTION); 
          next_token(p);
          break;
          case Syntax_MACRO: 
          set_color(p, t, Syntax_MACRO); 
          next_token(p);
          break;
          case Syntax_ENUM_MEMBER: 
          set_color(p, t, Syntax_ENUM_MEMBER); 
          next_token(p);
          break;
          case Syntax_ARG:
          set_color(p, t, Syntax_ARG);
          next_token(p);
          break;
        }
        
        if (accept_token(p, T_DOT) || accept_token(p, T_ARROW)) {
          next_token(p);
        }
      } else {
        next_token(p);
      }
    } break;
    
    case T_CLASS:
    case T_STRUCT:
    case T_UNION: {
      if (parse_struct_specifier(p)) {
        while (parse_decl_specifier(p));
        do {
          parse_init_declarator(p, false);
        } while (accept_token(p, T_COMMA));
        accept_token(p, T_SEMI);
      }
    } break;
    
    case T_ENUM: {
      if (parse_enum_specifier(p)) {
        next_token(p);
        while (parse_decl_specifier(p));
        do {
          parse_init_declarator(p, false);
        } while (accept_token(p, T_COMMA));
        accept_token(p, T_SEMI);
      }
    } break;
    
    case T_POUND: {
      set_color(p, t, Syntax_KEYWORD);
      String token_string = token_to_string(p->buffer, t);
      if (string_compare(token_string, const_string("#define"))) {
        next_token(p);
        Token *macro = peek_token(p, 0);
        set_color(p, macro, Syntax_MACRO);
        add_symbol_buffer(p, token_to_string(p->buffer, macro), Syntax_MACRO);
        next_token(p);
      } else if (string_compare(token_string, const_string("#include"))) {
        next_token(p);
        
        Token *dependency = peek_token(p, 0);
        if (accept_token(p, T_STRING_LITERAL)) {
          String dep_string = token_to_string(p->buffer, dependency);
          if (dep_string.count >= 2) {
            // trimming the quotes
            dep_string.data++;
            dep_string.count -= 2;
            
            sb_push(p->buffer->cache.dependencies, 
                    alloc_string(dep_string.data, dep_string.count));
            
            Buffer *dep_buffer = get_existing_buffer(p->buffer->editor, 
                                                     dep_string);
            if (dep_buffer) {
              Scope *dep_scope = dep_buffer->cache.scope;
              for (u32 symbol_index = 0;
                   symbol_index < dep_scope->capacity;
                   symbol_index++)
              {
                if (dep_scope->occupancy[symbol_index]) {
                  String key = dep_scope->keys[symbol_index];
                  Symbol value = dep_scope->values[symbol_index];
                  scope_insert_symbol(p->scope, key, value);
                }
              }
            }
          }
        }
      } else {
        next_token(p);
      }
    } break;
    
    case T_END_OF_FILE: {
      
    } break;
    
    default: {
      next_token(p);
    } break;
  }
  end_profiler_function();
}

void parse_program(Parser *p) {
  begin_profiler_function();
  while (!accept_token(p, T_END_OF_FILE)) {
    parse_any(p);
  }
  end_profiler_function();
}
