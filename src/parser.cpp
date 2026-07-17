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

    stream.expect(Token::SEMICOLON);
    
    return new VariableDeclareNode(token.value);
}

ASTNode* Parser::parseExpressionStatement(){
    ASTNode* expr = parseExpression();

    if(expr->type != ASTNode::NODE_BLOCK
        && expr->type != ASTNode::NODE_IF){
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
    auto* node = parseEquality();

    if(stream.consume(Token::EQUAL)){
        return new AssignmentNode(node, parseAssign());
    }
    
    return node;
}

ASTNode* Parser::parseEquality(){
    auto* node = parseShift();

    while(stream.peek().type == Token::EQUAL_EQUAL
        || stream.peek().type == Token::NOT_EQUAL){
            Token tok = stream.next();
            node = new BinaryOpNode(node, tok, parseShift());
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
    auto* node = parsePrimary();

    while(stream.peek().type == Token::ASTERISK
             || stream.peek().type == Token::SLASH
             || stream.peek().type == Token::MOD){
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parsePrimary());
    }

    return node;
}

ASTNode* Parser::parsePrimary(){
    Token token = stream.next();
    if(token.type == Token::NUMBER){
        return new NumberNode(std::stoll(token.value));
    }

    if(token.type == Token::IF){
        return parseIfExpression();
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

ASTNode* Parser::parseBlock(){
    std::vector<ASTNode*> statements;
    while(stream.peek().type != Token::RBRACE){
        statements.push_back(parseStatement());
    }
    stream.expect(Token::RBRACE);
    return new BlockNode(statements);
}