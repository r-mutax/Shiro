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
    statements.push_back(parseExpressionStatement());
    
    return new FunctionDefinitionNode("main", statements);
}

ASTNode* Parser::parseExpressionStatement(){
    ASTNode* expr = parseExpression();
    return new ExpressionStatementNode(expr);
}

ASTNode* Parser::parseExpression(){
    return parseAddSub();
}

ASTNode* Parser::parseAddSub(){
    auto* node = parsePrimary();

    while(stream.peek().type == Token::PLUS || stream.peek().type == Token::MINUS){
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
    throw std::runtime_error("Unexpected token: " + token.to_str());
}