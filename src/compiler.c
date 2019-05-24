#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  vm* cvm;
  token cur;
  token prev;
  bool errored;
  bool panic_mode;
} parser;

typedef struct {
  token name;
  int depth;
} local;

typedef struct {
  local locals[UINT8_COUNT];
  int localc;
  int scope_depth;
} compiler;

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

typedef void (*parse_fn)(parser*, scanner*, compiler*, bool);

typedef struct {
  parse_fn prefix;
  parse_fn infix;
  precedence prec;
} parse_rule;

chunk* comp;

static bool identifiers_equal(token* a, token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static chunk* cur_chunk() {
  return comp;
}

static void error_at(parser* p, token* t, const char* msg) {
  if (p->panic_mode) return;

  p->panic_mode = true;
  fprintf(stderr, "[line %d] Error", t->line);

  if (t->type == TOKEN_EOF) fprintf(stderr, " at end");
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

static bool check(parser* p, token_type type) {
  return p->cur.type == type;
}

static bool match(parser* p, scanner* s, token_type type) {
  if (!check(p, type)) return false;
  advance(p, s);
  return true;
}

static void emit_byte(parser* p, uint8_t byte) {
  write_chunk(cur_chunk(), byte, p->prev.line);
}

#define emit_bytes(p, byte1, byte2) { emit_byte(p, byte1); emit_byte(p, byte2); }

static void emit_return(parser* p) {
  emit_byte(p, OP_RETURN);
}

static int emit_jump(parser* p, uint8_t inst) {
  emit_byte(p, inst);
  emit_byte(p, 0xff);
  emit_byte(p, 0xff);
  return cur_chunk()->count-2;
}

static void emit_loop(parser* p, int loopstart) {
  emit_byte(p, OP_LOOP);

  int offs = cur_chunk()->count - loopstart + 2;
  if (offs > UINT16_MAX) error(p, "Loop body too large.");

  emit_byte(p, (offs >> 8) & 0xff);
  emit_byte(p, offs & 0xff);
}

static void patch_jump(parser* p, int offs) {
  int jump = cur_chunk()->count-offs-2;

  if (jump > UINT16_MAX) error(p, "Too much code to jump over.");

  cur_chunk()->code[offs] = (jump >> 8) & 0xff;
  cur_chunk()->code[offs+1] = jump & 0xff;
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

static int make_constant(parser* p, value v) {
  return add_constant(cur_chunk(), v);
}

static void parse_precedence(parser*, scanner*, compiler*, precedence);
static parse_rule* get_rule(token_type type);

void expression(parser* p, scanner* s, compiler* c) {
  parse_precedence(p, s, c, PREC_ASSIGNMENT);
}

static void number(parser* p, scanner* s, compiler* c, bool _) {
  double v = strtod(p->prev.start, NULL);
  emit_constant(p, NUMBER_VAL(v));
}

static void chr(parser* p, scanner* s, compiler* c, bool _) {
  char v = p->prev.start[p->prev.length-2];
  emit_constant(p, CHAR_VAL(v));
}

static void string(parser* p, scanner* s, compiler* c, bool _) {
  emit_constant(p, OBJ_VAL(copy_str(p->cvm,
                                    p->prev.start + 1,
                                    p->prev.length - 2)));
}

static void grouping(parser* p, scanner* s, compiler* c, bool _) {
  expression(p, s, c);
  consume(p, s, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(parser* p, scanner* s, compiler* c, bool _) {
  token_type op = p->prev.type;

  parse_precedence(p, s, c, PREC_UNARY);

  switch(op) {
    case TOKEN_MINUS: emit_byte(p, OP_NEGATE); break;
    case TOKEN_BANG: emit_byte(p, OP_NOT); break;
    case TOKEN_TILDE: emit_byte(p, OP_BITNOT); break;
    default: return;
  }
}

static void binary(parser* p, scanner* s, compiler* c, bool _) {
  token_type op = p->prev.type;

  parse_rule* rule = get_rule(op);
  parse_precedence(p, s, c, (precedence)(rule->prec + 1));

  switch (op) {
    case TOKEN_BANG_EQUAL:    emit_bytes(p, OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emit_byte(p, OP_EQUAL); break;
    case TOKEN_GREATER:       emit_byte(p, OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emit_bytes(p, OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emit_byte(p, OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emit_bytes(p, OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:          emit_byte(p, OP_ADD); break;
    case TOKEN_MINUS:         emit_byte(p, OP_SUBTRACT); break;
    case TOKEN_STAR:          emit_byte(p, OP_MULTIPLY); break;
    case TOKEN_SLASH:         emit_byte(p, OP_DIVIDE); break;
    case TOKEN_HAT:           emit_byte(p, OP_BITXOR); break;
    case TOKEN_PIPE:          emit_byte(p, OP_BITAND); break;
    case TOKEN_AMP:           emit_byte(p, OP_BITOR); break;
    case TOKEN_SHIFTLEFT:     emit_byte(p, OP_SHIFTLEFT); break;
    case TOKEN_SHIFTRIGHT:     emit_byte(p, OP_SHIFTRIGHT); break;
    default: return;
  }
}

static void literal(parser* p, scanner* s, compiler* c, bool _) {
  switch (p->prev.type) {
    case TOKEN_FALSE:  emit_byte(p, OP_FALSE); break;
    case TOKEN_TRUE:  emit_byte(p, OP_TRUE); break;
    case TOKEN_NIL:  emit_byte(p, OP_NIL); break;
    default: return;
  }
}

static void and_(parser* p, scanner* s, compiler* c, bool _) {
  int endj = emit_jump(p, OP_JUMP_IF_FALSE);
  emit_byte(p, OP_POP);
  parse_precedence(p, s, c, PREC_AND);

  patch_jump(p, endj);
}

static void or_(parser* p, scanner* s, compiler* c, bool _) {
  int elsej = emit_jump(p, OP_JUMP_IF_FALSE);
  int endj = emit_jump(p, OP_JUMP);

  patch_jump(p, elsej);
  emit_byte(p, OP_POP);

  parse_precedence(p, s, c, PREC_OR);
  patch_jump(p, endj);
}

static void variable(parser*, scanner*, compiler* c, bool can_assign);
static void declaration(parser* p, scanner* s, compiler* c);

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
  { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_HAT
  { unary,    NULL,    PREC_FACTOR },     // TOKEN_TILDE
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_PIPE
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_AMP
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SHIFTLEFT
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SHIFTRIGHT
  { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
  { string,   NULL,    PREC_NONE },       // TOKEN_STRING
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
  { chr,      NULL,    PREC_NONE },       // TOKEN_CHR
  { NULL,     and_,    PREC_AND },        // TOKEN_AND
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
  { literal,  NULL,    PREC_NONE },       // TOKEN_FALSE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
  { literal,  NULL,    PREC_NONE },       // TOKEN_NIL
  { NULL,     or_,     PREC_OR },         // TOKEN_OR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
  { literal,  NULL,    PREC_NONE },       // TOKEN_TRUE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

static parse_rule* get_rule(token_type type) {
  return &rules[type];
}

static void parse_precedence(parser* p, scanner* s, compiler* c, precedence prec) {
  advance(p, s);
  parse_fn prefix_rule = get_rule(p->prev.type)->prefix;

  if (!prefix_rule) {
    error(p, "Expect expression.");
    return;
  }

  bool can_assign = prec <= PREC_ASSIGNMENT;
  prefix_rule(p, s, c, can_assign);

  while (prec <= get_rule(p->cur.type)->prec) {
    advance(p, s);
    parse_fn infix_rule = get_rule(p->prev.type)->infix;
    infix_rule(p, s, c, can_assign);
  }

  if (can_assign && match(p, s, TOKEN_EQUAL)) {
    error(p, "Invalid assignment target.");
    expression(p, s, c);
  }
}

static void print_statement(parser* p, scanner* s, compiler* c) {
  expression(p, s, c);
  consume(p, s, TOKEN_SEMICOLON, "Expect ';' after value.");
  emit_byte(p, OP_PRINT);
}

static void expression_statement(parser* p, scanner* s, compiler* c) {
  expression(p, s, c);
  emit_byte(p, OP_POP);
  consume(p, s, TOKEN_SEMICOLON, "Expect ';' after expression.");
}

static void block(parser* p, scanner* s, compiler* c) {
  while (!check(p, TOKEN_RIGHT_BRACE) && !check(p, TOKEN_EOF)) declaration(p, s, c);

  consume(p, s, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void begin_scope(compiler* c) { c->scope_depth++; }
static void end_scope(parser* p, compiler* c) {
  c->scope_depth--;

  while (c->localc > 0 && c->locals[c->localc-1].depth > c->scope_depth) {
    emit_byte(p, OP_POP);
    c->localc--;
  }
}

static void if_statement(parser*, scanner*, compiler*);
static void while_statement(parser*, scanner*, compiler*);
static void for_statement(parser*, scanner*, compiler*);

static void statement(parser* p, scanner* s, compiler* c) {
  if (match(p, s, TOKEN_PRINT)) print_statement(p, s, c);
  else if(match(p, s, TOKEN_IF)) if_statement(p, s, c);
  else if(match(p, s, TOKEN_WHILE)) while_statement(p, s, c);
  else if(match(p, s, TOKEN_FOR)) for_statement(p, s, c);
  else if (match(p, s, TOKEN_LEFT_BRACE)) { begin_scope(c); block(p, s, c); end_scope(p, c); }
  else expression_statement(p, s, c);
}

static void var_declaration(parser*, scanner*, compiler*);

static void for_statement(parser* p, scanner* s, compiler* c) {
  begin_scope(c);
  consume(p, s, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(p, s, TOKEN_VAR)) var_declaration(p, s, c);
  else if (match(p, s, TOKEN_SEMICOLON)) {}
  else expression_statement(p, s, c);

  int loopstart = cur_chunk()->count;

  int exitj = -1;

  if (!match(p, s, TOKEN_SEMICOLON)) {
    expression(p, s, c);
    consume(p, s, TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    exitj = emit_jump(p, OP_JUMP_IF_FALSE);
    emit_byte(p, OP_POP);
  }

  if (!match(p, s, TOKEN_RIGHT_PAREN)) {
    int bodyj = emit_jump(p, OP_JUMP);

    int incrstart = cur_chunk()->count;
    expression(p, s, c);
    emit_byte(p, OP_POP);
    consume(p, s, TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emit_loop(p, loopstart);
    loopstart = incrstart;
    patch_jump(p, bodyj);
  }

  statement(p, s, c);

  emit_loop(p, loopstart);
  if (exitj != -1) {
    patch_jump(p, exitj);
    emit_byte(p, OP_POP);
  }
  end_scope(p, c);
}

static void while_statement(parser* p, scanner* s, compiler* c) {
  int loopstart = cur_chunk()->count;
  consume(p, s, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression(p, s, c);
  consume(p, s, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitj = emit_jump(p, OP_JUMP_IF_FALSE);

  emit_byte(p, OP_POP);
  statement(p, s, c);

  emit_loop(p, loopstart);

  patch_jump(p, exitj);
  emit_byte(p, OP_POP);
}

static void if_statement(parser* p, scanner* s, compiler* c) {
  consume(p, s, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression(p, s, c);
  consume(p, s, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int thenj = emit_jump(p, OP_JUMP_IF_FALSE);
  emit_byte(p, OP_POP);
  statement(p, s, c);

  int elsej = emit_jump(p, OP_JUMP);

  patch_jump(p, thenj);
  emit_byte(p, OP_POP);

  if (match(p, s, TOKEN_ELSE)) statement(p, s, c);
  patch_jump(p, elsej);
}



static void synchronize(parser* p, scanner* s) {
  p->panic_mode = false;

  while (p->cur.type != TOKEN_EOF) {
    if (p->prev.type == TOKEN_SEMICOLON) return;

    switch (p->cur.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:
        ;
    }

    advance(p, s);
  }
}

static int identifier_constant(parser* p) {
  return make_constant(p, OBJ_VAL(copy_str(p->cvm, p->prev.start, p->prev.length)));
}

static int resolve_local(parser* p, compiler* c, token* name) {
  for (int i = c->localc - 1; i >= 0; i--) {
    local* l = &c->locals[i];
    if (identifiers_equal(name, &l->name)) {
      if (l->depth == -1) error(p, "Cannot read local variable in its own initializer.");
      return i;
    }
  }

  return -1;
}

static void named_variable(parser* p, scanner* s, compiler* c, bool can_assign) {
  uint8_t getop, setop;
  int arg = resolve_local(p, c, &p->prev);
  if (arg == -1) {
    arg = identifier_constant(p);
    getop = OP_GET_GLOBAL;
    setop = OP_SET_GLOBAL;
  } else {
    getop = OP_GET_LOCAL;
    setop = OP_SET_LOCAL;
  }

  if (can_assign && match(p, s, TOKEN_EQUAL)) {
    expression(p, s, c);
    emit_bytes(p, setop, (uint8_t)arg);
  } else emit_bytes(p, getop, (uint8_t)arg);
}

static void variable(parser* p, scanner* s, compiler* c, bool can_assign) {
  named_variable(p, s, c, can_assign);
}

static void add_local(parser* p, compiler* c, token name) {
  if (c->localc == UINT8_COUNT) {
    error(p, "Too many local variables in block.");
    return;
  }

  local* l = &c->locals[c->localc++];
  l->name = name;
  l->depth = -1;
}

static void declare_variable(parser* p, compiler* c) {
  if (!c->scope_depth) return;

  token* name = &p->prev;

  for (int i = c->localc-1; i >= 0; i--) {
    local* l = &c->locals[i];
    if (l->depth != -1 && l->depth < c->scope_depth) break;
    if (identifiers_equal(name, &l->name)) {
      error(p, "Variable with this name already declared in this scope.");
    }
  }

  add_local(p, c, *name);
}

static int parse_variable(parser* p, scanner* s, compiler* c, const char* msg) {
  consume(p, s, TOKEN_IDENTIFIER, msg);

  declare_variable(p, c);
  if (c->scope_depth > 0) return 0;

  return identifier_constant(p);
}

static void mark_initialized(compiler* c) {
  if (!c->scope_depth) return;
  c->locals[c->localc-1].depth = c->scope_depth;
}

static void define_variable(parser* p, compiler* c, int global) {
  if (c->scope_depth > 0) { mark_initialized(c); return; }
  emit_bytes(p, OP_DEFINE_GLOBAL, (uint8_t)global);
}

static void var_declaration(parser* p, scanner* s, compiler* c) {
  int glob = parse_variable(p, s, c, "Expect variable name.");

  if (match(p, s, TOKEN_EQUAL)) expression(p, s, c);
  else emit_byte(p, OP_NIL);

  consume(p, s, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  define_variable(p, c, glob);
}

static void declaration(parser* p, scanner* s, compiler* c) {
  if (match(p, s, TOKEN_VAR)) var_declaration(p, s, c);
  else statement(p, s, c);

  if (p->panic_mode) synchronize(p, s);
}

static void init_compiler(compiler* c) {
  c->localc = 0;
  c->scope_depth = 0;
}

bool compile(vm* cvm, const char* source, chunk* c) {
  scanner* s = init_scanner(source);
  compiler com;
  init_compiler(&com);
  parser p;
  p.errored = false;
  p.panic_mode = false;
  p.cvm = cvm;
  comp = c;

  advance(&p, s);
  while (!match(&p, s, TOKEN_EOF)) declaration(&p, s, &com);
  consume(&p, s, TOKEN_EOF, "Expect end of expression.");
  free(s);
  end_compiler(&p);
  return !p.errored;
}
