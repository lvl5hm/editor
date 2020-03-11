

inline i32 get_gap_count(Text_Buffer *b) {
}


inline i32 get_gap_start(Text_Buffer *b) {
  i32 gap_count = get_gap_count(b);
  i32 result = b->cursor;
  assert(result <= b->capacity - gap_count);
  return result;
}


typedef struct TheFoo {
  int bar;
} Foo;

Foo (*foo)(int *) = 0;

#include "editor.h"
#include <lvl5_stretchy_buffer.h>






inline i32 get_buffer_pos(Text_Buffer *b, i32 pos) {
  i32 gap_start = get_gap_start(b);
  i32 gap_count = get_gap_count(b);
  
  i32 result = pos + (pos >= gap_start)*gap_count;
  
  return result;
}

char get_buffer_char(Text_Buffer *b, i32 pos) {
  char result = b->data[get_buffer_pos(b, pos)];
  return result;
}

V2 get_buffer_xy(Text_Buffer *b, i32 pos) {
  V2 result = v2_zero();
  
  for (i32 char_index = 0; char_index < pos; char_index++) {
    char buffer_char = get_buffer_char(b, char_index);
    if (buffer_char == '\n') {
      result.x = 0;
      result.y++;
    } else {
      result.x++;
    }
  }
  
  return result;
}

V2 get_screen_position_in_buffer(Font *font, Text_Buffer *b, i32 pos) {
  V2 result = v2(0, -(f32)font->line_spacing);
  
  for (i32 char_index = 0; char_index < pos; char_index++) {
    char buffer_char = get_buffer_char(b, char_index);
    char first = buffer_char - font->first_codepoint;
    if (buffer_char == '\n') {
      result.x = 0;
      result.y -= font->line_spacing;
      continue;
    }
    
    char second = get_buffer_char(b, char_index+1) - font->first_codepoint;
    result.x += font->advance[first*font->codepoint_count+second];
  }
  return result;
}

i32 seek_line_start(Text_Buffer *b, i32 start) {
  i32 result = max(start, 0);
  while (result > 0 && get_buffer_char(b, result-1) != '\n') {
    result--;
  }
  return result;
}

i32 seek_line_end(Text_Buffer *b, i32 start) {
  i32 result = min(start, b->count - 1);
  while (result < (i32)b->count-1 && get_buffer_char(b, result) != '\n') {
    result++;
  }
  return result;
}

void set_cursor(Text_Buffer *b, i32 pos) {
  assert(pos >= 0 && pos < b->count);
  i32 old_gap_start = get_gap_start(b);
  b->cursor = pos;
  i32 gap_start = get_gap_start(b);
  i32 moved_by = gap_start - old_gap_start;
  
  if (moved_by != 0) {
    // move chars from one gap end to the other
    i32 gap_count = get_gap_count(b);
    
    if (moved_by < 0) {
      for (i32 i = 0; i < -moved_by; i++) {
        b->data[old_gap_start+gap_count-1-i] = b->data[old_gap_start-1-i];
      }
    } else {
      for (i32 i = 0; i < moved_by; i++) {
        b->data[old_gap_start+i] = b->data[old_gap_start+gap_count+i];
      }
    }
    
    for (i32 i = 0; i < gap_count; i++) {
      b->data[gap_start+i] = '|';
    }
  }
}

void buffer_insert_string(Text_Buffer *b, String str) {
  if (b->count + (i32)str.count > b->capacity) {
    char *old_data = b->data;
    i32 old_gap_start = get_gap_start(b);
    i32 old_gap_count = get_gap_count(b);
    
    while (b->count + (i32)str.count > b->capacity) {
      b->capacity = b->capacity*2;
    }
    b->data = alloc_array(char, b->capacity);
    i32 gap_start = get_gap_start(b);
    i32 gap_count = get_gap_count(b);
    
    i32 first_count = min(gap_start, b->count);
    for (i32 i = 0; i < first_count; i++) {
      b->data[i] = old_data[i];
    }
    
    i32 second_count = b->count - first_count;
    for (i32 i = 0; i < second_count; i++) {
      b->data[gap_start+gap_count+i] = old_data[old_gap_start+old_gap_count+i];
    }
  }
  
  
  for (i32 i = 0; i < (i32)str.count; i++) {
    b->data[b->cursor+i] = str.data[i];
  }
  
  if (b->mark > b->cursor) {
    b->mark += str.count;
  }
  b->cursor += str.count;
  b->count += str.count;
  
}

void buffer_remove_backward(Text_Buffer *b, i32 count) {
  assert(b->cursor - count >= 0);
  
  if (b->mark >= b->cursor) {
    b->mark -= count;
  }
  b->cursor -= count;
  b->count -= count;
}

void buffer_remove_forward(Text_Buffer *b, i32 count) {
  assert(b->cursor < b->count - 1);
  
  if (b->mark > b->cursor) {
    b->mark -= count;
  }
  b->count -= count;
}

b32 move_cursor_direction(Text_Buffer *b, os_Keycode direction) {
  b32 result = false;
  i32 cursor = b->cursor;
  switch (direction) {
    case os_Keycode_ARROW_RIGHT: {
      if (cursor < b->count-1) {
        cursor++;
        result = true;
      }
    } break;
    
    case os_Keycode_ARROW_LEFT: {
      if (cursor > 0) {
        cursor--;
        result = true;
      }
    } break;
    
    case os_Keycode_ARROW_DOWN: {
      i32 line_start = seek_line_start(b, cursor);
      i32 col = cursor - line_start;
      i32 line_end = seek_line_end(b, cursor);
      i32 want = line_end + 1 + col;
      i32 max_possible = seek_line_end(b, line_end+1);
      
      if (want > max_possible) {
        want = max_possible;
      }
      cursor = want;
    } break;
    
    case os_Keycode_ARROW_UP: {
      i32 line_start = seek_line_start(b, cursor);
      i32 col = cursor - line_start;
      i32 prev_line_start = seek_line_start(b, line_start-1);
      i32 want = prev_line_start + col;
      
      i32 max_possible = max(line_start - 1, 0);
      
      if (want > max_possible) {
        want = max_possible;
      }
      
      cursor = want;
    } break;
    
    default: assert(false);
  }
  set_cursor(b, cursor);
  
  return result;
}

String buffer_part_to_string(Text_Buffer *buffer, i32 start, i32 end) {
  String str = {0};
  str.count = end - start;
  str.data = scratch_push_array(char, str.count);
  for (i32 i = 0; i < (i32)str.count; i++) {
    str.data[i] = get_buffer_char(buffer, start + i);
  }
  
  return str;
}

String text_buffer_to_string(Text_Buffer *buffer) {
  String result = buffer_part_to_string(buffer, 0, buffer->count);
  return result;
}

void buffer_copy(Text_Buffer *buffer) {
  i32 start = min(buffer->cursor, buffer->mark);
  i32 end = max(buffer->cursor, buffer->mark);
  buffer->exchange_count = end - start;
  assert(buffer->exchange_count <= MAX_EXCHANGE_COUNT);
  for (i32 i = 0; i < buffer->exchange_count; i++) {
    buffer->exchange[i] = get_buffer_char(buffer, start+i);
  }
}

void buffer_paste(Text_Buffer *buffer) {
  String str = make_string(buffer->exchange, buffer->exchange_count);
  buffer_insert_string(buffer, str);
}

void buffer_cut(Text_Buffer *buffer) {
  buffer_copy(buffer);
  i32 count = buffer->cursor - buffer->mark;
  if (buffer->cursor < buffer->mark) {
    i32 tmp = buffer->cursor;
    buffer->cursor = buffer->mark;
    buffer->mark = tmp;
    count *= -1;
  }
  buffer_remove_backward(buffer, count);
}


String token_to_string(Text_Buffer *b, Token *t) {
  String result = buffer_part_to_string(b, t->start, t->start+t->count);
  return result;
}

Token_Type get_keyword_kind(String str) {
  i32 result = 0;
  for (i32 i = T_KEYWORD_FIRST; i <= T_KEYWORD_LAST; i++) {
    String keyword_string = Token_Kind_To_String[i];
    if (string_compare(keyword_string, str)) {
      result = i;
      break;
    }
  }
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


Token *buffer_tokenize(Text_Buffer *b) {
  Token *tokens = sb_new(Token, 1024);
  
  i32 line = 1;
  i32 col = 1;
  i32 token_start = 0;
  
  Token t = {0};
  i32 i = 0;
  
#define get(index) get_buffer_char(b, i + index)
  
#define next(n) { \
    col += n; \
    i += n; \
  }
#define skip(n) { \
    next(n); \
    token_start += n; \
  }
#define eat() { \
    next(1); \
  }
#define end(tok_kind) { \
    t.line = line; \
    t.col = col - (i - token_start); \
    t.kind = tok_kind; \
    t.start = token_start; \
    t.count = i - token_start; \
    token_start = i; \
    sb_push(tokens, t); \
    continue; \
  }
  
#define case1(ch0, kind0) \
  case ch0: { \
    eat(); \
    end(kind0); \
  } break;
#define case2(ch0, kind0, ch1, kind1) \
  case ch0: { \
    eat(); \
    if (get(0) == ch1) { \
      eat(); \
      end(kind1); \
    } else { \
      end(kind0); \
    } \
  } break;
#define case3(ch0, kind0, ch1, kind1, ch2, kind2) \
  case ch0: { \
    eat(); \
    if (get(0) == ch1) { \
      eat(); \
      end(kind1); \
    } else if (get(0) == ch2) { \
      eat(); \
      end(kind2); \
    } else { \
      end(kind0); \
    } \
  } break;
  
  while (true) {
    switch (get(0)) {
      case 0: {
        goto end;
      } break;
      case ' ': {
        eat();
        end(T_SPACE);
      } break;
      case '\n': {
        line++;
        col = 1;
        eat();
        end(T_NEWLINE);
      } break;
      case '"': {
        eat();
        // TODO(lvl5): deal with escape sequences
        while ((get(0) != '"') ||
               (get(-1) == '\\')) {
          eat();
        }
        eat();
        end(T_STRING);
      } break;
      case '\'': {
        eat();
        eat();
        eat();
        end(T_CHAR);
      } break;
      
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        eat();
        while (is_digit(get(0))) eat();
        
        // float
        if (get(0) == '.') {
          eat();
          //if (!is_digit((get0))) syntax_error("Unexpected symbol %c", *stream);
          
          while (is_digit(get(0))) eat();
          end(T_FLOAT);
        } else {
          end(T_INT);
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
        String value = buffer_part_to_string(b, token_start, i);
        Token_Type kind = get_keyword_kind(value);
        if (kind) {
          t.line = line; 
          t.col = col - (i - token_start); 
          t.kind = kind;
          t.start = token_start;
          t.count = i - token_start;
          token_start = i; 
          sb_push(tokens, t); 
          continue;
        } else {
          end(T_NAME);
        }
      } break;
      
      case '/': {
        if (get(1) == '/') {
          skip(2);
          while (get(0) != '\n') {
            skip(1);
          }
        } else if (get(1) == '*') {
          skip(2);
          while (!(get(0) == '*' && get(1) == '/')) {
            skip(1);
          }
          skip(2);
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
        skip(1);
        while (is_digit(get(0)) || is_alpha(get(0))) eat();
        end(T_POUND);
      } break;
      
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
      case3('<', T_LESS, '=', T_LESS_EQUALS, '>', T_LSHIFT);
      case3('|', T_BIT_OR, '|', T_OR, '=', T_ASSIGN_BIT_OR);
      case3('&', T_BIT_AND, '&', T_AND, '=', T_ASSIGN_BIT_AND);
      
      default: assert(false);
    }
  }
  
  end:
  
  return tokens;
}

typedef struct {
  i32 i;
  Token *tokens;
  String *type_table;
  String *function_table;
  Text_Buffer *buffer;
} Parser;



i32 skip_whitespace_backward(Parser *p, i32 offset) {
  i32 result = offset;
  while (p->tokens[result].kind == T_SPACE  || p->tokens[result].kind == T_NEWLINE) {
    result--;
  }
  return result;
}


i32 skip_whitespace_forward(Parser *p, i32 offset) {
  i32 result = offset;
  while (p->tokens[result].kind == T_SPACE  || p->tokens[result].kind == T_NEWLINE) {
    result++;
  }
  return result;
}

Token *peek_token(Parser *p, i32 offset) {
  assert(p->i + offset < (i32)sb_count(p->tokens));
  
  i32 index = p->i;
  while (offset < 0) {
    index--;
    index = skip_whitespace_backward(p, index);
    offset++;
  }
  while (offset > 0) {
    index++;
    index = skip_whitespace_forward(p, index);
    offset--;
  }
  
  Token *result = p->tokens + index;
  return result;
}

void next_token(Parser *p) {
  p->i++;
  p->i = skip_whitespace_forward(p, p->i);
  assert(p->i <= (i32)sb_count(p->tokens));
}

bool accept_token(Parser *p, Token_Type kind) {
  bool result = false;
  if (kind == peek_token(p, 0)->kind) {
    result = true;
    next_token(p);
  }
  return result;
}

void parse_type_extra(Parser *p) {
  i32 saved = p->i;
  
  Token *t = peek_token(p, 0);
  
  switch (t->kind) {
    case T_ENUM:
    case T_STRUCT: {
      next_token(p); // struct
      
      Token *struct_name = peek_token(p, 0);
      struct_name->ast_kind = A_TYPE;
      
      accept_token(p, T_NAME);
      accept_token(p, T_LCURLY);
      
      while (!accept_token(p, T_RCURLY)) {
        parse_type_extra(p);
        accept_token(p, T_SEMI);
      }
      
      while (!accept_token(p, T_SEMI)) {
        if (accept_token(p, T_NAME)) {
          Token *type_name = peek_token(p, -1);
          type_name->ast_kind = A_TYPE;
        } else {
          next_token(p);
        }
      }
    } break;
    
    case T_NAME: {
      Token *type_name = peek_token(p, 0);
      accept_token(p, T_NAME);
      if (accept_token(p, T_LPAREN)) {
        // function call
        type_name->ast_kind = A_FUNCTION;
        type_name = null;
      } else {
        while (accept_token(p, T_STAR)) {
        }
      }
      
      if (type_name) {
        Token *decl_name = peek_token(p, 0);
        if (accept_token(p, T_NAME)) {
          if (accept_token(p, T_LPAREN)) {
            decl_name->ast_kind = A_FUNCTION;
            type_name->ast_kind = A_TYPE;
          } else if (accept_token(p, T_SEMI) || 
                     accept_token(p, T_ASSIGN) || 
                     accept_token(p, T_COMMA) ||
                     accept_token(p, T_RPAREN)) {
            type_name->ast_kind = A_TYPE;
          }
        }
      }
    } break;
    
    default: {
      next_token(p);
    } break;
  }
  
}

void parse_program(Parser *p) {
  while (p->i < (i32)sb_count(p->tokens)) {
    parse_type_extra(p);
  }
}

Token *buffer_parse(Text_Buffer *b) {
  Parser parser = {
    .i = 0,
    .tokens = buffer_tokenize(b),
    .type_table = sb_new(String, 64),
    .function_table = sb_new(String, 64),
    .buffer = b,
  };
  sb_push(parser.type_table, const_string("void"));
  sb_push(parser.type_table, const_string("char"));
  sb_push(parser.type_table, const_string("int"));
  sb_push(parser.type_table, const_string("short"));
  sb_push(parser.type_table, const_string("float"));
  sb_push(parser.type_table, const_string("long"));
  sb_push(parser.type_table, const_string("double"));
  sb_push(parser.type_table, const_string("unsigned"));
  sb_push(parser.type_table, const_string("signed"));
  parse_program(&parser);
  return parser.tokens;
}