#include "parser.hpp"
#include "AST.hpp"
#include "token.hpp"

ASTNode* Parser::parse() { return parseProgram(); }

ASTNode* Parser::parseProgram() {
    auto* node = new TranslationUnitNode();

    while (!stream.is_eof()) {
        node->definitions.push_back(parseDefinition());
    }

    return node;
}

ASTNode* Parser::parseDefinition() {
    if (stream.peek().type == Token::FN) {
        return parseFunctionDefinition();
    }
    if (stream.peek().type == Token::STRUCT) {
        return parseStructDefinition();
    }

    // cannnot find fn error
    error(stream.peek().loc, "Expected function definition.");
}


std::pair<std::string, SourceLoc> Parser::parseTypeName(){
    std::string prefix = "";
    SourceLoc loc = stream.peek().loc;
    while (true) {
        if (stream.peek().type == Token::AND) {
            stream.next();
            prefix += "&";
        } else if (stream.peek().type == Token::AND_AND) {
            stream.next();
            prefix += "&&"; // Token::AND_AND なら & 2個分として扱う
        } else {
            break;
        }
    }
    Token type_tok = stream.next();
    if (!type_tok.isTypeCandidate()) {
        error(type_tok.loc, "Expected type name, but got " + type_tok.value);
    }
    return {prefix + type_tok.value, loc};
}

ASTNode* Parser::parseFunctionDefinition() {
    expect(Token::FN, "Expected fn keyword for function definition.");
    Token fname = stream.next();
    if (fname.type != Token::IDENT) {
        error(fname.loc, "Expected IDENT after fn, but got " + fname.value);
    }

    expect(Token::LPAREN, "Expected '(' after function name.");
    std::vector<ASTNode*> params;
    while (!stream.consume(Token::RPAREN)) {
        if (!params.empty()) {
            expect(Token::COMMA, "Expected ',' before next parameter.");
        }

        Token token = stream.next();
        if (token.type != Token::IDENT) {
            error(token.loc,
                  "Expected IDENT for function parameter " + token.value);
        }

        std::string type_name = "unknown";
        SourceLoc loc;
        if (stream.consume(Token::COLON)) {
            auto [type_name_val, type_loc] = parseTypeName();
            type_name = type_name_val;
            loc = type_loc;
        }
        auto vd = new VariableDeclareNode(token.value, loc, type_name);
        vd->loc = token.loc;
        params.push_back(vd);
    }

    expect(Token::ARROW, "Expected '->' after function parameters.");

    // check token is type name candidate (keyword or IDENT)
    auto [type_name_func, type_loc_func] = parseTypeName();

    ASTNode* block = parseBlock();

    auto fn =
        new FunctionDefinitionNode(fname.value, type_name_func, params,
                                   static_cast<BlockNode*>(block), type_loc_func);
    fn->loc = fname.loc;
    return fn;
}

ASTNode* Parser::parseStructDefinition(){
    expect(Token::STRUCT, "Expected struct keyword for struct definition.");
    Token sname = stream.next();
    if (sname.type != Token::IDENT) {
        error(sname.loc, "Expected IDENT after struct, but got " + sname.value);
    }

    expect(Token::LBRACE, "Expected '{' after struct name.");

    std::vector<ASTNode*> members;
    while (!stream.consume(Token::RBRACE)) {
        if (!members.empty()) {
            expect(Token::COMMA, "Expected ',' before next member.");
        }

        bool is_pub = stream.consume(Token::PUB);

        if(stream.peek().type == Token::FN){
            // function field 
            ASTNode* node = parseFunctionDefinition();
            FunctionDefinitionNode* fn = static_cast<FunctionDefinitionNode*>(node);
            fn->is_pub = is_pub;
            members.push_back(fn);
        } else {
            // variable field
            if(stream.peek().type != Token::IDENT) {
                error(stream.peek().loc, "Expected IDENT for struct member.");
            }

            Token token = stream.next();
            expect(Token::COLON, "Expected ':' after struct member name.");

            auto [type_name_field, type_loc_field] = parseTypeName();
            
            auto vd = new VariableDeclareNode(token.value, type_loc_field, type_name_field);
            vd->loc = token.loc;
            vd->is_pub = is_pub;
            members.push_back(vd);
        }
    }
    expect(Token::SEMICOLON, "Expected ';' after struct definition.");
    
    return new StructDefinitionNode(sname.value, members); 
}

ASTNode* Parser::parseStatement() {

    switch (stream.peek().type) {
        case Token::LET:
            return parseVariableDeclare();
        case Token::RETURN:
            return parseReturnStatement();
        default:
            return parseExpressionStatement();
    }
}

ASTNode* Parser::parseVariableDeclare() {
    stream.next();

    Token vname = stream.next();
    if (vname.type != Token::IDENT) {
        error(vname.loc, "Expected IDENT after let, but got " + vname.value);
    }

    std::string type_name = "unknown";
    Token type_tok;
    if (stream.consume(Token::COLON)) {
        auto [type_name_let, type_loc_let] = parseTypeName();
        type_name = type_name_let;
        type_tok.loc = type_loc_let;
    }

    expect(Token::SEMICOLON, "Expected ';' after variable declare.");

    auto vd = new VariableDeclareNode(vname.value, type_tok.loc, type_name);
    vd->loc = vname.loc;
    return vd;
}

ASTNode* Parser::parseReturnStatement() {
    Token rettok = stream.next();
    ASTNode* expr = parseExpression();
    expect(Token::SEMICOLON, "Expected ';' after return statement.");

    auto ret = new ReturnNode(expr);
    ret->loc = rettok.loc;
    return ret;
}

ASTNode* Parser::parseExpressionStatement() {
    ASTNode* expr = parseExpression();

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

ASTNode* Parser::parseExpression() { return parseAssign(); }

ASTNode* Parser::parseAssign() {
    auto* node = parseLogicalOr();

    if (stream.consume(Token::EQUAL)) {
        auto assign = new AssignmentNode(node, parseAssign());
        assign->loc = node->loc;
        return assign;
    }

    return node;
}

ASTNode* Parser::parseLogicalOr() {
    auto* node = parseLogicalAnd();

    while (stream.peek().type == Token::OR_OR) {
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseLogicalAnd());
        node->loc = op.loc;
    }
    return node;
}

ASTNode* Parser::parseLogicalAnd() {
    auto* node = parseBitOr();

    while (stream.peek().type == Token::AND_AND) {
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseBitOr());
        node->loc = op.loc;
    }
    return node;
}

ASTNode* Parser::parseBitOr() {
    auto* node = parseBitXor();

    while (stream.peek().type == Token::OR) {
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseBitXor());
        node->loc = op.loc;
    }
    return node;
}

ASTNode* Parser::parseBitXor() {
    auto* node = parseBitAnd();

    while (stream.peek().type == Token::HAT) {
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseBitAnd());
        node->loc = op.loc;
    }
    return node;
}

ASTNode* Parser::parseBitAnd() {
    auto* node = parseEquality();

    while (stream.peek().type == Token::AND) {
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseEquality());
        node->loc = op.loc;
    }
    return node;
}

ASTNode* Parser::parseEquality() {
    auto* node = parseRelational();

    while (stream.peek().type == Token::EQUAL_EQUAL ||
           stream.peek().type == Token::NOT_EQUAL) {
        Token tok = stream.next();
        node = new BinaryOpNode(node, tok, parseRelational());
        node->loc = tok.loc;
    }
    return node;
}

ASTNode* Parser::parseRelational() {
    auto* node = parseShift();

    while (true) {
        Token op = stream.peek();
        if (op.type == Token::LT || op.type == Token::LE ||
            op.type == Token::GT || op.type == Token::GE) {
            stream.next();
            node = new BinaryOpNode(node, op, parseShift());
            node->loc = op.loc;
        } else {
            break;
        }
    }
    return node;
}

ASTNode* Parser::parseShift() {
    auto* node = parseAddSub();

    while (stream.peek().type == Token::LSHIFT ||
           stream.peek().type == Token::RSHIFT) {
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseAddSub());
        node->loc = op.loc;
    }

    return node;
}

ASTNode* Parser::parseAddSub() {
    auto* node = parseMulDivMod();

    while (stream.peek().type == Token::PLUS ||
           stream.peek().type == Token::MINUS) {
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseMulDivMod());
        node->loc = op.loc;
    }

    return node;
}

ASTNode* Parser::parseMulDivMod() {
    auto* node = parseUnary();

    while (stream.peek().type == Token::ASTERISK ||
           stream.peek().type == Token::SLASH ||
           stream.peek().type == Token::MOD) {
        Token op = stream.next();
        node = new BinaryOpNode(node, op, parseUnary());
        node->loc = op.loc;
    }

    return node;
}

ASTNode* Parser::parseUnary() {
    Token tok = stream.peek();

    if (tok.type == Token::NOT || tok.type == Token::CHILDA ||
        tok.type == Token::MINUS || tok.type == Token::AND) {
        stream.next();
        auto unary = new UnaryOpNode(tok, parseUnary());
        unary->loc = tok.loc;
        return unary;
    }
    return parsePostfix();
}

ASTNode* Parser::parsePostfix(){
    ASTNode* lhs = parsePrimary();

    while (true) {
        Token tok = stream.peek();
        if (tok.type == Token::DOT) {
            stream.next();
            Token member = stream.next();
            if (member.type != Token::IDENT) {
                error(member.loc, "Expected IDENT after '.', but got " + member.value);
            }
            lhs = new MemberAccessNode(lhs, member.value);
            lhs->loc = tok.loc;
        } else {
            break;
        }
    }
    return lhs;
}

ASTNode* Parser::parsePrimary() {

    Token token = stream.peek();

    //    Token token = stream.next();

    switch (token.type) {
        case Token::NUMBER: {
            stream.next();
            auto num = new NumberNode(std::stoll(token.value));
            num->loc = token.loc;
            return num;
        }
        case Token::CHAR: {
            stream.next();
            auto num = new CharLiteralNode(std::stoll(token.value));
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
            auto* node = parseExpression();
            if (stream.peek().type != Token::RPAREN) {
                error(stream.peek().loc,
                      "Expected ')' after expression: " + stream.peek().value);
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
                std::vector<ASTNode*> args;
                while (stream.peek().type != Token::RPAREN) {
                    if (!args.empty()) {
                        expect(Token::COMMA,
                               "Expected ',' before next argument.");
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
        default: {
            std::string val =
                (token.type == Token::EOF_TOK) ? "end of file" : token.value;
            error(token.loc, "Unexpected token: " + val);
        }
    }
}

ASTNode* Parser::parseIfExpression() {
    Token iftok = stream.next();
    expect(Token::LPAREN, "Expected '(' after 'if'.");
    ASTNode* condition = parseExpression();
    expect(Token::RPAREN, "Expected ')' after 'if' condition.");

    ASTNode* then_block = parseExpression();

    ASTNode* else_block = nullptr;
    if (stream.peek().type == Token::ELSE) {
        stream.next();
        else_block = parseExpression();
    }

    auto node = new IfNode(condition, then_block, else_block);
    node->loc = iftok.loc;
    return node;
}

ASTNode* Parser::parseWhileExpression() {
    Token whiletok = stream.next();
    expect(Token::LPAREN, "Expected '(' after 'while'.");
    ASTNode* condition = parseExpression();
    expect(Token::RPAREN, "Expected ')' after 'while' condition.");

    ASTNode* body = parseExpression();
    auto node = new WhileNode(condition, body);
    node->loc = whiletok.loc;
    return node;
}

ASTNode* Parser::parseBlock() {
    Token tok = stream.next();
    if (tok.type != Token::LBRACE) {
        error(tok.loc, "Expected '{' at start of block: " + tok.value);
    }
    std::vector<ASTNode*> statements;
    while (stream.peek().type != Token::RBRACE) {
        statements.push_back(parseStatement());
    }
    expect(Token::RBRACE, "Expected '}' at end of block.");
    auto block = new BlockNode(statements);
    block->loc = tok.loc;
    return block;
}