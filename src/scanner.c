#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

scanner* init_scanner(const char* source) {
  scanner* s = malloc(sizeof(scanner));
  s->start = source;
  s->current = source;
  s->line = 1;
  return s;
}

static token make_token(scanner* s, token_type type) {
  token tok;
  tok.type = type;
  tok.start = s->start;
  tok.length = (int)(s->current - s->start);
  tok.line = s->line;

  return tok;
}

static token error_token(scanner* s, const char* message) {
  token tok;
  tok.type = TOKEN_ERROR;
  tok.start = message;
  tok.length = (int)strlen(message);
  tok.line = s->line;

  return tok;
}

static bool is_at_end(scanner* s) {
  return *s->current == '\0';
}

static char advance(scanner* s) {
  return *(s->current++);
}

static bool match(scanner* s, char expected) {
  if (is_at_end(s)) return false;
  if (*s->current != expected) return false;

  s->current++;
  return true;
}

static char peek(scanner* s) {
  return *s->current;
}

static char peek_next(scanner* s) {
  if (is_at_end(s)) return '\0';
  return s->current[1];
}

static token string(scanner* s) {
  char c;
  while (peek(s) != '"' && !is_at_end(s)) {
    if (peek(s) == '\n') s->line++;
    c = advance(s);
    if (c == '\\') {
      if (is_at_end(s)) return error_token(s, "Unterminated string.");
      advance(s);
    }
  }

  if (is_at_end(s)) return error_token(s, "Unterminated string.");

  advance(s);
  return make_token(s, TOKEN_STRING);
}

static void skip_whitespace(scanner* s) {
  for (;;) {
    char c = peek(s);
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance(s);
        break;
      case '\n':
        s->line++;
        advance(s);
        break;
      case '/':
        if (peek_next(s) == '/') {
          while (peek(s) != '\n' && !is_at_end(s)) advance(s);
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

static token number(scanner* s) {
  while (is_digit(peek(s))) advance(s);

  if (peek(s) == '.' && is_digit(peek_next(s))) {
    advance(s);

    while (is_digit(peek(s))) advance(s);
  }

  return make_token(s, TOKEN_NUMBER);
}

static bool is_idstart(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_' || c == '$';
}

static token_type check_keyword(scanner* s, int start, int len,
                                const char* rest, token_type type) {
  if (s->current-s->start == start+len && !memcmp(s->start+start, rest, len)) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static token_type identifier_type(scanner* s) {
  switch (s->start[0]) {
    case 'a': return check_keyword(s, 1, 2, "nd", TOKEN_AND);
    case 'c': return check_keyword(s, 1, 4, "lass", TOKEN_CLASS);
    case 'e': return check_keyword(s, 1, 3, "lse", TOKEN_ELSE);
    case 'f':
      if (s->current-s->start > 1) {
        switch (s->start[1]) {
          case 'a': return check_keyword(s, 2, 3, "lse", TOKEN_FALSE);
          case 'o': return check_keyword(s, 2, 1, "r", TOKEN_FOR);
          case 'u': return check_keyword(s, 2, 1, "n", TOKEN_FUN);
        }
      }
      break;
    case 't':
      if (s->current-s->start > 1) {
        switch (s->start[1]) {
          case 'h': return check_keyword(s, 2, 2, "is", TOKEN_THIS);
          case 'r': return check_keyword(s, 2, 2, "ue", TOKEN_TRUE);
        }
      }
      break;
    case 'i': return check_keyword(s, 1, 1, "f", TOKEN_IF);
    case 'n': return check_keyword(s, 1, 2, "il", TOKEN_NIL);
    case 'o': return check_keyword(s, 1, 1, "r", TOKEN_OR);
    case 'p': return check_keyword(s, 1, 4, "rint", TOKEN_PRINT);
    case 'r': return check_keyword(s, 1, 5, "eturn", TOKEN_RETURN);
    case 's': return check_keyword(s, 1, 4, "uper", TOKEN_SUPER);
    case 'v': return check_keyword(s, 1, 2, "ar", TOKEN_VAR);
    case 'w': return check_keyword(s, 1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

static token identifier(scanner* s) {
  while (is_idstart(peek(s)) || is_digit(peek(s))) advance(s);

  return make_token(s, identifier_type(s));
}

token scan_token(scanner* s) {
  skip_whitespace(s);
  s->start = s->current;

  if (is_at_end(s)) return make_token(s, TOKEN_EOF);

  char c = advance(s);

  if (is_digit(c)) return number(s);
  if (is_idstart(c)) return identifier(s);

  switch (c) {
    case '(': return make_token(s, TOKEN_LEFT_PAREN);
    case ')': return make_token(s, TOKEN_RIGHT_PAREN);
    case '{': return make_token(s, TOKEN_LEFT_BRACE);
    case '}': return make_token(s, TOKEN_RIGHT_BRACE);
    case ';': return make_token(s, TOKEN_SEMICOLON);
    case ',': return make_token(s, TOKEN_COMMA);
    case '.': return make_token(s, TOKEN_DOT);
    case '-': return make_token(s, TOKEN_MINUS);
    case '+': return make_token(s, TOKEN_PLUS);
    case '/': return make_token(s, TOKEN_SLASH);
    case '*': return make_token(s, TOKEN_STAR);
    case '^': return make_token(s, TOKEN_HAT);
    case '~': return make_token(s, TOKEN_TILDE);
    case '|': return make_token(s, TOKEN_PIPE);
    case '&': return make_token(s, TOKEN_AMP);
   case '!':
      return make_token(s, match(s, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return make_token(s, match(s, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return make_token(s, match(s, '=') ?
                            TOKEN_LESS_EQUAL :
                            match(s, '<') ? TOKEN_SHIFTLEFT : TOKEN_LESS);
    case '>':
      return make_token(s, match(s, '=') ?
                            TOKEN_GREATER_EQUAL :
                            match(s, '<') ? TOKEN_SHIFTRIGHT : TOKEN_GREATER);
    case '"': return string(s);
  }

  return error_token(s, "Unexpected character.");
}
