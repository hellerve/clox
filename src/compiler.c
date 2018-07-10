#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  token cur;
  token prev;
  bool errored;
  bool panic_mode;
} parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! - +
  PREC_CALL,        // . () []
  PREC_PRIMARY
} precedence;

typedef void (*parse_fn)(parser*, scanner*);

typedef struct {
  parse_fn prefix;
  parse_fn infix;
  precedence prec;
} parse_rule;

chunk* comp;

static chunk* cur_chunk() {
  return comp;
}

static void error_at(parser* p, token* t, const char* msg) {
  if (p->panic_mode) return;

  p->panic_mode = true;
  fprintf(stderr, "[line %d] Error", t->line);

  if (t->type == TOKEN_EOF) fprintf(stderr, "at end");
  else if (t->type == TOKEN_ERROR) {}
  else fprintf(stderr, " at '%.*s'", t->length, t->start);

  fprintf(stderr, ": %s\n", msg);
  p->errored = true;
}

static void error(parser* p, const char* msg) {
  error_at(p, &p->prev, msg);
}

static void error_at_current(parser* p, const char* msg) {
  error_at(p, &p->cur, msg);
}

static void advance(parser* p, scanner* s) {
  p->prev = p->cur;

  for (;;) {
    p->cur = scan_token(s);
    if (p->cur.type != TOKEN_ERROR) break;
    error_at_current(p, p->cur.start);
  }
}

static void consume(parser* p, scanner* s, token_type type, const char* msg) {
  if (p->cur.type == type) {
    advance(p, s);
    return;
  }

  error_at_current(p, msg);
}

static void emit_byte(parser* p, uint8_t byte) {
  write_chunk(cur_chunk(), byte, p->prev.line);
}

#define emit_bytes(p, byte1, byte2) { emit_byte(p, byte1); emit_byte(p, byte2); }

static void emit_return(parser* p) {
  emit_byte(p, OP_RETURN);
}

static void end_compiler(parser* p) {
  emit_return(p);

#ifdef DEBUG_PRINT_CODE
  if (!p->errored) disassemble_chunk(cur_chunk(), "code");
#endif
}

static void emit_constant(parser* p, value v) {
  write_constant(cur_chunk(), v, p->prev.line);
}

static void parse_precedence(parser*, scanner*, precedence);
static parse_rule* get_rule(token_type type);

void expression(parser* p, scanner* s) {
  parse_precedence(p, s, PREC_ASSIGNMENT);
}

static void number(parser* p, scanner* s) {
  value v = strtod(p->prev.start, NULL);
  emit_constant(p, v);
}

static void grouping(parser* p, scanner* s) {
  expression(p, s);
  consume(p, s, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(parser* p, scanner* s) {
  token_type op = p->prev.type;

  parse_precedence(p, s, PREC_UNARY);

  switch(op) {
    case TOKEN_MINUS: emit_byte(p, OP_NEGATE); break;
    default: return;
  }
}

static void binary(parser* p, scanner* s) {
  token_type op = p->prev.type;

  parse_rule* rule = get_rule(op);
  parse_precedence(p, s, (precedence)(rule->prec + 1));

  switch (op) {
    case TOKEN_PLUS:  emit_byte(p, OP_ADD); break;
    case TOKEN_MINUS: emit_byte(p, OP_SUBTRACT); break;
    case TOKEN_STAR:  emit_byte(p, OP_MULTIPLY); break;
    case TOKEN_SLASH: emit_byte(p, OP_DIVIDE); break;
    default: return;
  }
}

parse_rule rules[] = {
  { grouping, NULL,    PREC_CALL },       // TOKEN_LEFT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
  { NULL,     NULL,    PREC_CALL },       // TOKEN_DOT
  { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
  { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     NULL,    PREC_EQUALITY },   // TOKEN_BANG_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
  { NULL,     NULL,    PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     NULL,    PREC_COMPARISON }, // TOKEN_LESS_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
  { NULL,     NULL,    PREC_NONE },       // TOKEN_STRING
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
  { NULL,     NULL,    PREC_AND },        // TOKEN_AND
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FALSE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
  { NULL,     NULL,    PREC_NONE },       // TOKEN_NIL
  { NULL,     NULL,    PREC_OR },         // TOKEN_OR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_TRUE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

static parse_rule* get_rule(token_type type) {
  return &rules[type];
}

static void parse_precedence(parser* p, scanner* s, precedence prec) {
  advance(p, s);
  parse_fn prefix_rule = get_rule(p->prev.type)->prefix;

  if (!prefix_rule) {
    error(p, "Expect expression.");
    return;
  }

  prefix_rule(p, s);

  while (prec <= get_rule(p->cur.type)->prec) {
    advance(p, s);
    parse_fn infix_rule = get_rule(p->prev.type)->infix;
    infix_rule(p, s);
  }
}

bool compile(const char* source, chunk* c) {
  scanner* s = init_scanner(source);
  parser p;
  p.errored = false;
  p.panic_mode = false;
  comp = c;

  advance(&p, s);
  expression(&p, s);
  consume(&p, s, TOKEN_EOF, "Expect end of expression.");
  free(s);
  end_compiler(&p);
  return !p.errored;
}
