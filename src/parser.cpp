#include "parser.hpp"

ASTNode* Parser::parse(){
    return parseProgram();
}

ASTNode* Parser::parseProgram(){
    auto* node = new TranslationUnitNode();

    node->definitions.push_back(parseDefinition());

    if (!stream.is_eof()) {
        throw std::runtime_error("Unexpected token after definition: " + stream.peek().to_str());
    }

    return node;
}

ASTNode* Parser::parseDefinition(){
    return parseFunctionDefinition();
}

ASTNode* Parser::parseFunctionDefinition(){
    // now only 1 statements
    // for this version, i only support function definition of 'main'
    
    std::vector<ASTNode*> statements;
    while(stream.peek().type != Token::EOF_TOK){
        statements.push_back(parseStatement());
    }
    
    return new FunctionDefinitionNode("main", statements);
}

ASTNode* Parser::parseStatement(){
    
    if(stream.peek().type == Token::LET){
        return parseVariableDeclare();
    }

    return parseExpressionStatement();
}

ASTNode* Parser::parseVariableDeclare(){
    stream.expect(Token::LET);
    Token token = stream.next();
    if(token.type != Token::IDENT){
        throw std::runtime_error("Expected IDENT after LET: " + token.to_str());
    }

    std::string type_name = "i64";
    if(stream.consume(Token::COLON)){
        Token type_tok = stream.next();
        type_name = type_tok.value;
    }

    stream.expect(Token::SEMICOLON);
    
    return new VariableDeclareNode(token.value, type_name);
}

ASTNode* Parser::parseExpressionStatement(){
    ASTNode* expr = parseExpression();

    if(expr->kind != ASTNode::NODE_BLOCK
        && expr->kind != ASTNode::NODE_IF
        && expr->kind != ASTNode::NODE_WHILE){
        stream.expect(Token::SEMICOLON);
    } else {
        stream.consume(Token::SEMICOLON);
    }

    return new ExpressionStatementNode(expr);
}

ASTNode* Parser::parseExpression(){
    return parseAssign();
}

ASTNode* Parser::parseAssign(){
    auto* node = parseLogicalOr();

    if(stream.consume(Token::EQUAL)){
        return new AssignmentNode(node, parseAssign());
    }
    
    return node;
}

ASTNode* Parser::parseLogicalOr(){
    auto* node = parseLogicalAnd();

    while(stream.peek().type == Token::OR_OR){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseLogicalAnd());
    }
    return node;
}

ASTNode* Parser::parseLogicalAnd(){
    auto* node = parseBitOr();

    while(stream.peek().type == Token::AND_AND){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseBitOr());
    }
    return node;
}

ASTNode* Parser::parseBitOr(){
    auto* node = parseBitXor();

    while(stream.peek().type == Token::OR){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseBitXor());
    }
    return node;
}

ASTNode* Parser::parseBitXor(){
    auto* node = parseBitAnd();

    while(stream.peek().type == Token::HAT){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseBitAnd());
    }
    return node;
}

ASTNode* Parser::parseBitAnd(){
    auto* node = parseEquality();

    while(stream.peek().type == Token::AND){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseEquality());
    }
    return node;
}

ASTNode* Parser::parseEquality(){
    auto* node = parseRelational();

    while(stream.peek().type == Token::EQUAL_EQUAL
        || stream.peek().type == Token::NOT_EQUAL){
            Token tok = stream.next();
            node = new BinaryOpNode(node, tok, parseRelational());
    }
    return node;
}

ASTNode* Parser::parseRelational(){
    auto* node = parseShift();

    while(true){
        Token op = stream.peek();
        if(op.type == Token::LT || op.type == Token::LE || op.type == Token::GT || op.type == Token::GE){
            stream.next();
            node = new BinaryOpNode(node, op, parseShift());
        } else {
            break;
        }
    }
    return node;
}

ASTNode* Parser::parseShift(){
    auto* node = parseAddSub();

    while(stream.peek().type == Token::LSHIFT || stream.peek().type == Token::RSHIFT){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseAddSub());
    }

    return node;
}

ASTNode* Parser::parseAddSub(){
    auto* node = parseMulDivMod();

    while(stream.peek().type == Token::PLUS || stream.peek().type == Token::MINUS){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseMulDivMod());
    }

    return node;
}

ASTNode* Parser::parseMulDivMod(){
    auto* node = parseUnary();

    while(stream.peek().type == Token::ASTERISK
             || stream.peek().type == Token::SLASH
             || stream.peek().type == Token::MOD){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseUnary());
    }

    return node;
}

ASTNode* Parser::parseUnary(){
    Token tok = stream.peek();
    
    if(tok.type == Token::NOT
        || tok.type == Token::CHILDA
        || tok.type == Token::MINUS){
        stream.next();
        return new UnaryOpNode(tok, parseUnary());
    }
    return parsePrimary();
}

ASTNode* Parser::parsePrimary(){
    Token token = stream.next();
    if(token.type == Token::NUMBER){
        return new NumberNode(std::stoll(token.value));
    }

    if(token.type == Token::IF){
        return parseIfExpression();
    }

    if(token.type == Token::WHILE){
        return parseWhileExpression();
    }

    if(token.type == Token::LPAREN){
        auto* node = parseExpression();
        if(stream.next().type != Token::RPAREN){
            throw std::runtime_error("Expected ')' after expression: " + stream.peek().to_str());
        }
        return node;
    }

    if(token.type == Token::LBRACE){
        return parseBlock();
    }

    if(token.type == Token::IDENT){
        return new VariableNode(token.value);
    }

    throw std::runtime_error("Unexpected token: " + token.to_str());
}

ASTNode* Parser::parseIfExpression(){   
    stream.expect(Token::LPAREN);
    ASTNode* condition = parseExpression();
    stream.expect(Token::RPAREN);

    ASTNode* then_block = parseExpression();
    
    ASTNode* else_block = nullptr;
    if(stream.peek().type == Token::ELSE){
        stream.next();
        else_block = parseExpression();
    }

    return new IfNode(condition, then_block, else_block);
}

ASTNode* Parser::parseWhileExpression(){
    stream.expect(Token::LPAREN);
    ASTNode* condition = parseExpression();
    stream.expect(Token::RPAREN);

    ASTNode* body = parseExpression();

    return new WhileNode(condition, body);
}

ASTNode* Parser::parseBlock(){
    std::vector<ASTNode*> statements;
    while(stream.peek().type != Token::RBRACE){
        statements.push_back(parseStatement());
    }
    stream.expect(Token::RBRACE);
    return new BlockNode(statements);
}