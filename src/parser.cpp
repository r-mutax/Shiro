#include "parser.hpp"
#include "AST.hpp"

ASTNode *Parser::parse() { return parseProgram(); }

ASTNode *Parser::parseProgram() {
  auto *node = new TranslationUnitNode();

  while (!stream.is_eof()) {
    node->definitions.push_back(parseDefinition());
  }

  return node;
}

ASTNode *Parser::parseDefinition() {
  if (stream.peek().type == Token::FN) {
    return parseFunctionDefinition();
  }

  // cannnot find fn error
  error(stream.peek().loc, "Expected function definition.");
}

ASTNode *Parser::parseFunctionDefinition() {
  expect(Token::FN, "Expected fn keyword for function definition.");
  Token fname = stream.next();
  if (fname.type != Token::IDENT) {
    error(fname.loc, "Expected IDENT after fn, but got " + fname.value);
  }

  expect(Token::LPAREN, "Expected '(' after function name.");
  std::vector<ASTNode *> params;
  while (!stream.consume(Token::RPAREN)) {
    if (!params.empty()) {
      expect(Token::COMMA, "Expected ',' before next parameter.");
    }

    Token token = stream.next();
    if (token.type != Token::IDENT) {
      error(token.loc, "Expected IDENT for function parameter " + token.value);
    }

    std::string type_name = "unknown";
    Token type_tok;
    if (stream.consume(Token::COLON)) {
      type_tok = stream.next();
      type_name = type_tok.value;
    }
    auto vd = new VariableDeclareNode(token.value, type_tok.loc, type_name);
    vd->loc = token.loc;
    params.push_back(vd);
  }

  expect(Token::ARROW, "Expected '->' after function parameters.");

  // check token is type name candidate (keyword or IDENT)
  Token ty_tok = stream.next();
  if (!ty_tok.isTypeCandidate()) {
    error(ty_tok.loc, "Expected type after ->, but got " + ty_tok.value);
  }
  std::string type_name = ty_tok.value;

  ASTNode *block = parseBlock();

  auto fn = new FunctionDefinitionNode(fname.value, type_name, params,
                                       static_cast<BlockNode *>(block), ty_tok.loc);
  fn->loc = fname.loc;
  return fn;
}

ASTNode *Parser::parseStatement() {

  switch (stream.peek().type) {
  case Token::LET:
    return parseVariableDeclare();
  case Token::RETURN:
    return parseReturnStatement();
  default:
    return parseExpressionStatement();
  }
}

ASTNode *Parser::parseVariableDeclare() {
  stream.next();
  
  Token vname = stream.next();
  if (vname.type != Token::IDENT) {
    error(vname.loc, "Expected IDENT after let, but got " + vname.value);
  }

  std::string type_name = "unknown";
  Token type_tok;
  if (stream.consume(Token::COLON)) {
    type_tok = stream.next();
    type_name = type_tok.value;
  }

  expect(Token::SEMICOLON, "Expected ';' after variable declare.");

  auto vd = new VariableDeclareNode(vname.value, type_tok.loc, type_name);
  vd->loc = vname.loc;
  return vd;
}

ASTNode *Parser::parseReturnStatement() {
  Token rettok = stream.next();
  ASTNode *expr = parseExpression();
  expect(Token::SEMICOLON, "Expected ';' after return statement.");

  auto ret = new ReturnNode(expr);
  ret->loc = rettok.loc;
  return ret;
}

ASTNode *Parser::parseExpressionStatement() {
  ASTNode *expr = parseExpression();

  if (expr->kind != ASTNode::NODE_BLOCK && expr->kind != ASTNode::NODE_IF &&
      expr->kind != ASTNode::NODE_WHILE) {
    expect(Token::SEMICOLON, "Expected ';' after expression.");
  } else {
    stream.consume(Token::SEMICOLON);
  }

  auto expr_stmt = new ExpressionStatementNode(expr);
  expr_stmt->loc = expr->loc;
  return expr_stmt;
}

ASTNode *Parser::parseExpression() { return parseAssign(); }

ASTNode *Parser::parseAssign() {
  auto *node = parseLogicalOr();

  if (stream.consume(Token::EQUAL)) {
    auto assign = new AssignmentNode(node, parseAssign());
    assign->loc = node->loc;
    return assign;
  }

  return node;
}

ASTNode *Parser::parseLogicalOr() {
  auto *node = parseLogicalAnd();

  while (stream.peek().type == Token::OR_OR) {
    Token op = stream.next();
    node = new BinaryOpNode(node, op, parseLogicalAnd());
    node->loc = op.loc;
  }
  return node;
}

ASTNode *Parser::parseLogicalAnd() {
  auto *node = parseBitOr();

  while (stream.peek().type == Token::AND_AND) {
    Token op = stream.next();
    node = new BinaryOpNode(node, op, parseBitOr());
    node->loc = op.loc;
  }
  return node;
}

ASTNode *Parser::parseBitOr() {
  auto *node = parseBitXor();

  while (stream.peek().type == Token::OR) {
    Token op = stream.next();
    node = new BinaryOpNode(node, op, parseBitXor());
    node->loc = op.loc;
  }
  return node;
}

ASTNode *Parser::parseBitXor() {
  auto *node = parseBitAnd();

  while (stream.peek().type == Token::HAT) {
    Token op = stream.next();
    node = new BinaryOpNode(node, op, parseBitAnd());
    node->loc = op.loc;
  }
  return node;
}

ASTNode *Parser::parseBitAnd() {
  auto *node = parseEquality();

  while (stream.peek().type == Token::AND) {
    Token op = stream.next();
    node = new BinaryOpNode(node, op, parseEquality());
    node->loc = op.loc;
  }
  return node;
}

ASTNode *Parser::parseEquality() {
  auto *node = parseRelational();

  while (stream.peek().type == Token::EQUAL_EQUAL ||
         stream.peek().type == Token::NOT_EQUAL) {
    Token tok = stream.next();
    node = new BinaryOpNode(node, tok, parseRelational());
    node->loc = tok.loc;
  }
  return node;
}

ASTNode *Parser::parseRelational() {
  auto *node = parseShift();

  while (true) {
    Token op = stream.peek();
    if (op.type == Token::LT || op.type == Token::LE || op.type == Token::GT ||
        op.type == Token::GE) {
      stream.next();
      node = new BinaryOpNode(node, op, parseShift());
      node->loc = op.loc;
    } else {
      break;
    }
  }
  return node;
}

ASTNode *Parser::parseShift() {
  auto *node = parseAddSub();

  while (stream.peek().type == Token::LSHIFT ||
         stream.peek().type == Token::RSHIFT) {
    Token op = stream.next();
    node = new BinaryOpNode(node, op, parseAddSub());
    node->loc = op.loc;
  }

  return node;
}

ASTNode *Parser::parseAddSub() {
  auto *node = parseMulDivMod();

  while (stream.peek().type == Token::PLUS ||
         stream.peek().type == Token::MINUS) {
    Token op = stream.next();
    node = new BinaryOpNode(node, op, parseMulDivMod());
    node->loc = op.loc;
  }

  return node;
}

ASTNode *Parser::parseMulDivMod() {
  auto *node = parseUnary();

  while (stream.peek().type == Token::ASTERISK ||
         stream.peek().type == Token::SLASH ||
         stream.peek().type == Token::MOD) {
    Token op = stream.next();
    node = new BinaryOpNode(node, op, parseUnary());
    node->loc = op.loc;
  }

  return node;
}

ASTNode *Parser::parseUnary() {
  Token tok = stream.peek();

  if (tok.type == Token::NOT || tok.type == Token::CHILDA ||
      tok.type == Token::MINUS) {
    stream.next();
    auto unary = new UnaryOpNode(tok, parseUnary());
    unary->loc = tok.loc;
    return unary;
  }
  return parsePrimary();
}

ASTNode *Parser::parsePrimary() {

  Token token = stream.peek();

  //    Token token = stream.next();

  switch (token.type) {
    case Token::NUMBER: {
        stream.next();
        auto num = new NumberNode(std::stoll(token.value));
        num->loc = token.loc;
        return num;
    }
    case Token::IF: {
        return parseIfExpression();
    }
    case Token::WHILE: {
        return parseWhileExpression();
    }
    case Token::LPAREN: {
        stream.next();
        auto *node = parseExpression();
        if (stream.peek().type != Token::RPAREN) {
          error(stream.peek().loc, "Expected ')' after expression: " + stream.peek().value);
        }
        stream.next();
        node->loc = token.loc;
        return node;
    }
    case Token::LBRACE: {
        return parseBlock();
    }
    case Token::IDENT: {
        stream.next();
        if (stream.peek().type == Token::LPAREN) {
        stream.next();
        std::vector<ASTNode *> args;
        while (stream.peek().type != Token::RPAREN) {
            if (!args.empty()) {
              expect(Token::COMMA, "Expected ',' before next argument.");
            }
            args.push_back(parseExpression());
        }
        expect(Token::RPAREN, "Expected ')' after function call.");
        auto fncall = new FunctionCallNode(token.value, args);
        fncall->loc = token.loc;
        return fncall;
        } else {
        auto var = new VariableNode(token.value);
        var->loc = token.loc;
        return var;
        }
    }
    default:{
      std::string val = (token.type == Token::EOF_TOK) ? "end of file" : token.value;
      error(token.loc, "Unexpected token: " + val);
    }
  }
}

ASTNode *Parser::parseIfExpression() {
  Token iftok = stream.next();
  expect(Token::LPAREN, "Expected '(' after 'if'.");
  ASTNode *condition = parseExpression();
  expect(Token::RPAREN, "Expected ')' after 'if' condition.");

  ASTNode *then_block = parseExpression();

  ASTNode *else_block = nullptr;
  if (stream.peek().type == Token::ELSE) {
    stream.next();
    else_block = parseExpression();
  }

  auto node = new IfNode(condition, then_block, else_block);
  node->loc = iftok.loc;
  return node;
}

ASTNode *Parser::parseWhileExpression() {
  Token whiletok = stream.next();
  expect(Token::LPAREN, "Expected '(' after 'while'.");
  ASTNode *condition = parseExpression();
  expect(Token::RPAREN, "Expected ')' after 'while' condition.");

  ASTNode *body = parseExpression();
  auto node = new WhileNode(condition, body);
  node->loc = whiletok.loc;
  return node;
}

ASTNode *Parser::parseBlock() {
  Token tok = stream.next();
  if(tok.type != Token::LBRACE){
    error(tok.loc, "Expected '{' at start of block: " + tok.value);
  }
  std::vector<ASTNode *> statements;
  while (stream.peek().type != Token::RBRACE) {
    statements.push_back(parseStatement());
  }
  expect(Token::RBRACE, "Expected '}' at end of block.");
  auto block = new BlockNode(statements);
  block->loc = tok.loc;
  return block;
}