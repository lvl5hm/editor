void buffer_paste(Text_Buffer *buffer) {
  buffer_insert_string(buffer, str);
  String str = make_string(buffer->exchange, buffer->exchange_count);
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
