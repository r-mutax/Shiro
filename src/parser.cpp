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
  throw std::runtime_error("Expected function definition.");
}

ASTNode *Parser::parseFunctionDefinition() {
  stream.expect(Token::FN);
  Token fname = stream.next();
  if (fname.type != Token::IDENT) {
    throw std::runtime_error("Expected IDENT after fn: " + fname.to_str());
  }

  stream.expect(Token::LPAREN);
  std::vector<ASTNode *> params;
  while (!stream.consume(Token::RPAREN)) {
    if (!params.empty()) {
      stream.expect(Token::COMMA);
    }

    Token token = stream.next();
    if (token.type != Token::IDENT) {
      throw std::runtime_error("Expected IDENT after fn: " + token.to_str());
    }

    std::string type_name = "unknown";
    if (stream.consume(Token::COLON)) {
      Token type_tok = stream.next();
      type_name = type_tok.value;
    }
    auto vd = new VariableDeclareNode(token.value, type_name);
    vd->loc = token.loc;
    params.push_back(vd);
  }

  stream.expect(Token::ARROW);

  // 次のトークンが型名の候補（予約語 or IDENT）かチェックする
  Token ty_tok = stream.next();
  if (!ty_tok.isTypeCandidate()) {
    throw std::runtime_error("Expected type after ->: " + ty_tok.to_str());
  }
  std::string type_name = ty_tok.value;

  ASTNode *block = parseBlock();

  auto fn = new FunctionDefinitionNode(fname.value, type_name, params,
                                       static_cast<BlockNode *>(block));
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
  stream.expect(Token::LET);
  Token vname = stream.next();
  if (vname.type != Token::IDENT) {
    throw std::runtime_error("Expected IDENT after LET: " + vname.to_str());
  }

  std::string type_name = "unknown";
  if (stream.consume(Token::COLON)) {
    Token type_tok = stream.next();
    type_name = type_tok.value;
  }

  stream.expect(Token::SEMICOLON);

  auto vd = new VariableDeclareNode(vname.value, type_name);
  vd->loc = vname.loc;
  return vd;
}

ASTNode *Parser::parseReturnStatement() {
  Token rettok = stream.next();
  ASTNode *expr = parseExpression();
  stream.expect(Token::SEMICOLON);

  auto ret = new ReturnNode(expr);
  ret->loc = rettok.loc;
  return ret;
}

ASTNode *Parser::parseExpressionStatement() {
  ASTNode *expr = parseExpression();

  if (expr->kind != ASTNode::NODE_BLOCK && expr->kind != ASTNode::NODE_IF &&
      expr->kind != ASTNode::NODE_WHILE) {
    stream.expect(Token::SEMICOLON);
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
        if (stream.next().type != Token::RPAREN) {
        throw std::runtime_error("Expected ')' after expression: " +
                                stream.peek().to_str());
        }
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
            if (!args.empty()) stream.expect(Token::COMMA);
            args.push_back(parseExpression());
        }
        stream.expect(Token::RPAREN);
        auto fncall = new FunctionCallNode(token.value, args);
        fncall->loc = token.loc;
        return fncall;
        } else {
        auto var = new VariableNode(token.value);
        var->loc = token.loc;
        return var;
        }
    }
    default:
        throw std::runtime_error("Unexpected token: " + token.to_str());
  }
}

ASTNode *Parser::parseIfExpression() {
  Token iftok = stream.next();
  stream.expect(Token::LPAREN);
  ASTNode *condition = parseExpression();
  stream.expect(Token::RPAREN);

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
  stream.expect(Token::LPAREN);
  ASTNode *condition = parseExpression();
  stream.expect(Token::RPAREN);

  ASTNode *body = parseExpression();
  auto node = new WhileNode(condition, body);
  node->loc = whiletok.loc;
  return node;
}

ASTNode *Parser::parseBlock() {
  Token tok = stream.next();
  if(tok.type != Token::LBRACE){
    throw std::runtime_error("Expected '{' at start of block, got: " + tok.to_str());
  }
  std::vector<ASTNode *> statements;
  while (stream.peek().type != Token::RBRACE) {
    statements.push_back(parseStatement());
  }
  stream.expect(Token::RBRACE);
  auto block = new BlockNode(statements);
  block->loc = tok.loc;
  return block;
}