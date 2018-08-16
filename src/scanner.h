#ifndef clox_scanner_h
#define clox_scanner_h

typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

  // One or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,
  TOKEN_HAT, TOKEN_TILDE, TOKEN_PIPE,
  TOKEN_AMP, TOKEN_SHIFTLEFT, TOKEN_SHIFTRIGHT,

  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

  // Keywords.
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FUN, TOKEN_FOR, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

  TOKEN_ERROR,
  TOKEN_EOF
} token_type;

typedef struct {
  const char* start;
  const char* current;
  int line;
} scanner;

typedef struct {
  token_type type;
  const char* start;
  int length;
  int line;
} token;

scanner* init_scanner(const char* source);

token scan_token(scanner*);

#endif
