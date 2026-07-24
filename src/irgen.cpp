#include "irgen.hpp"
#include "AST.hpp"
#include <iostream>

bool IRGenerator::generate(ASTNode* ast) {

    if(ast->kind != ASTNode::NODE_TRANSLATION_UNIT) {
        throw std::runtime_error("Expected translation unit");
    }

    TranslationUnitNode* tu = static_cast<TranslationUnitNode*>(ast);
    for(auto& def : tu->definitions){
        IRFunction func = gen_function(def);
        program.functions.push_back(std::move(func));
    }

    return true;
}

IRFunction IRGenerator::gen_function(ASTNode* node){

    if(node->kind != ASTNode::NODE_FUNCTION_DEFINITION) {
        throw std::runtime_error("Expected function definition");
    }

    FunctionDefinitionNode* func_def = static_cast<FunctionDefinitionNode*>(node);
    IRFunction func;
    func.name = func_def->fn_name;

    instructions.clear();
    next_temp = 0;
    symid_to_temp.clear();

    for(auto* param : func_def->params){
        auto* vd = static_cast<VariableDeclareNode*>(param);
        Operand param_op = Operand::Temp(next_temp++, vd->evaluated_type);
        symid_to_temp[vd->symbol_id] = param_op;
        func.param_temps.push_back(param_op);
    }

    Operand ret = Operand::IntVal(0);
    ret = gen_expr(func_def->body);
    emit_ret(ret);
    
    func.constructCFG(instructions);
    func.analyzeLiveness();

    func.instructions = instructions;
    func.temp_count = next_temp;

    func.temp_reg_infos.resize(next_temp);
    for(size_t i = 0; i < instructions.size(); i++){
        IRInstruction& instr = instructions[i];
        
        auto update_interval = [&](const Operand& op){
            if(op.kind == Operand::TEMP){
                int tid = op.temp_id;
                if(func.temp_reg_infos[tid].start == -1){
                    func.temp_reg_infos[tid].start = i;
                }
                func.temp_reg_infos[tid].end = i;
                func.temp_reg_infos[tid].type = op.type;
            }
        };

        update_interval(instr.dst);
        update_interval(instr.src1);
        update_interval(instr.src2);
    }

    for(const auto& bb : func.blocks){
        for(int temp : bb->live_ins){
            // initialize live interval or update live interval 
            if(func.temp_reg_infos[temp].start == -1
                || func.temp_reg_infos[temp].start > bb->start_index){
                    func.temp_reg_infos[temp].start = bb->start_index;
            }                
        }

        for(int temp : bb->live_outs){
            if(func.temp_reg_infos[temp].end < bb->end_index){
                func.temp_reg_infos[temp].end = bb->end_index;
            }
        }
    }
    
    return func;
}

Operand IRGenerator::gen_stmt(ASTNode* node){
    if(node->kind == ASTNode::NODE_EXPRESSION_STATEMENT){
        ExpressionStatementNode* stmt = static_cast<ExpressionStatementNode*>(node);
        return gen_expr(stmt->expr);
    } else if(node->kind == ASTNode::NODE_VARIABLE_DECLARE){
        VariableDeclareNode* vd = static_cast<VariableDeclareNode*>(node);
        Operand temp_val = Operand::Temp(next_temp++, vd->evaluated_type);
        symid_to_temp[vd->symbol_id] = temp_val;
        emit_mov(temp_val, Operand::IntVal(0, vd->evaluated_type));
        return temp_val;
    } else {
        throw std::runtime_error("Unexpected statement");
    }
}

Operand IRGenerator::gen_expr(ASTNode* node){
    if(node->kind == ASTNode::NODE_INTEGER){
        NumberNode* num = static_cast<NumberNode*>(node);
        return Operand::IntVal(num->value);
    } else if(node->kind == ASTNode::NODE_VARIABLE){
        VariableNode* v = static_cast<VariableNode*>(node);
        auto it = symid_to_temp.find(v->symbol_id);
        if(it == symid_to_temp.end()){
            throw std::runtime_error("IRGen Error: variable'" + v->name + "'not found in symbol map");
        }
        return it->second;
    } else if(node->kind == ASTNode::NODE_FUNCTION_CALL){
        FunctionCallNode* fc = static_cast<FunctionCallNode*>(node);

        std::vector<Operand> args;
        for(auto* arg_node : fc->args){
            args.push_back(gen_expr(arg_node));
        }

        Operand res_temp = Operand::Temp(next_temp++, fc->evaluated_type);
        emit_call(res_temp, Operand::Func(fc->fn_name), args);
        return res_temp;
    } else if(node->kind == ASTNode::NODE_IF){
        IfNode* if_node = static_cast<IfNode*>(node);
        Operand condition = gen_expr(if_node->condition);

        int else_label = next_label++;
        int end_label = next_label++;
        Operand res_temp = Operand::Temp(next_temp++, if_node->then_block->evaluated_type);

        emit_jz(condition, Operand::LLabel(else_label));

        // Then
        Operand then_val = gen_expr(if_node->then_block);
        emit_mov(res_temp, then_val);
        emit_jmp(Operand::LLabel(end_label));

        // else
        emit_label(Operand::LLabel(else_label));

        if(if_node->else_block){
            Operand else_val = gen_expr(if_node->else_block);
            emit_mov(res_temp, else_val);
        } else {
            emit_mov(res_temp, Operand::IntVal(0));
        }
        emit_label(Operand::LLabel(end_label));
        return res_temp;
    } else if(node->kind == ASTNode::NODE_WHILE){
        WhileNode* while_node = static_cast<WhileNode*>(node);
        int start_label = next_label++;
        int end_label = next_label++;
        Operand res_temp = Operand::Temp(next_temp++, while_node->evaluated_type);

        emit_label(Operand::LLabel(start_label));
        Operand condition = gen_expr(while_node->condition);
        emit_jz(condition, Operand::LLabel(end_label));

        // Body
        Operand body_val = gen_expr(while_node->body);
        emit_mov(res_temp, body_val);
        emit_jmp(Operand::LLabel(start_label));

        // End
        emit_label(Operand::LLabel(end_label));
        return res_temp;
    } else if(node->kind == ASTNode::NODE_BLOCK){
        BlockNode* block = static_cast<BlockNode*>(node);
        Operand res_temp = Operand::IntVal(0);
        for(auto* stmt : block->statements){
            res_temp = gen_stmt(stmt);
        }
        return res_temp;
    } else if(node->kind == ASTNode::NODE_ASSIGNMENT){
        AssignmentNode* as = static_cast<AssignmentNode*>(node);
        VariableNode* var = static_cast<VariableNode*>(as->lvalue);
        
        Operand value = gen_expr(as->expr);
        auto it = symid_to_temp.find(var->symbol_id);

        if(it == symid_to_temp.end()){
            throw std::runtime_error("IRGen Error: variable'" + var->name + "'not found in symbol map");
        }
        emit_mov(it->second, value);
        return value;
    } else if(node->kind == ASTNode::NODE_UNARY_OP){
        UnaryOpNode* un_op = static_cast<UnaryOpNode*>(node);
        Operand res_temp = Operand::Temp(next_temp++, un_op->evaluated_type);
        Operand value = gen_expr(un_op->value);
        
        if(un_op->op.type == Token::NOT){
            emit_not(res_temp, value);
        } else if(un_op->op.type == Token::MINUS){
            emit_neg(res_temp, value);
        } else if(un_op->op.type == Token::CHILDA){
            emit_bnot(res_temp, value);
        }
        return res_temp;
    } else if(node->kind == ASTNode::NODE_BINARY_OP){
        BinaryOpNode* bin_op = static_cast<BinaryOpNode*>(node);

        if(bin_op->op.type == Token::AND_AND){
            Operand left = gen_expr(bin_op->left);
            int false_label = next_label++;
            int end_label = next_label++;
            Operand res_temp = Operand::Temp(next_temp++, bin_op->evaluated_type);

            emit_jz(left, Operand::LLabel(false_label));

            Operand right = gen_expr(bin_op->right);
            emit_jz(right, Operand::LLabel(false_label));
            emit_mov(res_temp, Operand::IntVal(1));
            emit_jmp(Operand::LLabel(end_label));

            emit_label(Operand::LLabel(false_label));
            emit_mov(res_temp, Operand::IntVal(0));
            emit_label(Operand::LLabel(end_label));
            return res_temp;
        } else if(bin_op->op.type == Token::OR_OR){
            Operand left = gen_expr(bin_op->left);
            int true_label = next_label++;
            int end_label = next_label++;
            Operand res_temp = Operand::Temp(next_temp++, bin_op->evaluated_type);

            emit_jnz(left, Operand::LLabel(true_label));
            
            Operand right = gen_expr(bin_op->right);
            emit_jnz(right, Operand::LLabel(true_label));
            emit_mov(res_temp, Operand::IntVal(0));
            emit_jmp(Operand::LLabel(end_label));

            emit_label(Operand::LLabel(true_label));
            emit_mov(res_temp, Operand::IntVal(1));
            emit_label(Operand::LLabel(end_label));
            return res_temp;
        } else {
            Operand left = gen_expr(bin_op->left);
            Operand right = gen_expr(bin_op->right);
            Operand ret = Operand::Temp(next_temp++, bin_op->evaluated_type);

            switch(bin_op->op.type){
                case Token::OR:
                    emit_bitor(ret, left, right);
                    break;
                case Token::HAT:
                    emit_bitxor(ret, left, right);
                    break;
                case Token::AND:
                    emit_bitand(ret, left, right);
                    break;
                case Token::EQUAL_EQUAL:
                    emit_eq(ret, left, right);
                    break;
                case Token::NOT_EQUAL:
                    emit_neq(ret, left, right);
                    break;
                case Token::LT:
                    emit_lt(ret, left, right);
                    break;
                case Token::LE:
                    emit_le(ret, left, right);
                    break;
                case Token::GT:
                    emit_lt(ret, right, left);
                    break;
                case Token::GE:
                    emit_le(ret, right, left);
                    break;
                case Token::LSHIFT:
                    emit_lshift(ret, left, right);
                    break;
                case Token::RSHIFT:
                    emit_rshift(ret, left, right);
                    break;
                case Token::PLUS:
                    emit_add(ret, left, right);
                    break;
                case Token::MINUS:
                    emit_sub(ret, left, right);
                    break;
                case Token::ASTERISK:
                    emit_mul(ret, left, right);
                    break;
                case Token::SLASH:
                    emit_div(ret, left, right);
                    break;
                case Token::MOD:
                    emit_mod(ret, left, right);
                    break;
                default:
                    throw std::runtime_error("Unexpected operator: " + bin_op->op.to_str());
            }
            return ret;
        }
    }

    throw std::runtime_error("Expected expression");
}

void IRGenerator::dump(){
    for(auto& func : program.functions){
        std::cout << "define " << func.name << " {" << std::endl;
        for(auto& instr : func.instructions){
            std::cout << "  " << instr.to_str() << std::endl;
        }
        std::cout << "}" << std::endl;
    }
}

void IRFunction::constructCFG(const std::vector<IRInstruction>& instructions){
    blocks.clear();

    if(instructions.empty()) return;

    BasicBlock* bb = nullptr;
    bool is_leader = true;      // first instruction is leader.

    std::unordered_map<int, BasicBlock*> label_to_bb;

    for(size_t i = 0; i < instructions.size(); ++i){
        const auto instr = instructions[i];
        
        if(instr.op == IRInstruction::LABEL){
            is_leader = true;
        }

        // check is leader instruction
        if(is_leader){
            bb = createBB();
            is_leader = false;
            bb->start_index = i;
            if(instr.op == IRInstruction::LABEL){
                label_to_bb[instr.src1.label_id] = bb;
            }
        }

        bb->instructions.push_back(instr);
        bb->end_index = i;

        // set is_leader = true when meet the instruction that end a basic block
        if(instr.op == IRInstruction::JMP || 
           instr.op == IRInstruction::JZ  ||
           instr.op == IRInstruction::JNZ ||
           instr.op == IRInstruction::RET
           ){
            is_leader = true;
        }
    }
    
    for(size_t i = 0; i < blocks.size(); ++i){
        auto& bb = blocks[i];
        if(bb->instructions.empty()) continue;

        const auto& last_instr = bb->instructions.back();

        if(last_instr.op == IRInstruction::JMP){
            int target_id = last_instr.src1.label_id;
            auto it = label_to_bb.find(target_id);
            if(it != label_to_bb.end()){
                bb->succs.push_back(it->second);
                it->second->preds.push_back(bb.get());
            }
        } else if(last_instr.op == IRInstruction::JZ
            || last_instr.op == IRInstruction::JNZ){
            int target_id = last_instr.src2.label_id;
            auto it = label_to_bb.find(target_id);
            if(it != label_to_bb.end()){
                bb->succs.push_back(it->second);
                it->second->preds.push_back(bb.get());
            }
        }

        if(last_instr.op != IRInstruction::RET
            && last_instr.op != IRInstruction::JMP){

            if(i + 1 < blocks.size()){
                BasicBlock* next_bb = blocks[i + 1].get();
                bb->succs.push_back(next_bb);
                next_bb->preds.push_back(bb.get());
            }
        }
    }
}

void IRFunction::analyzeLiveness(){
    for(const auto& bb : blocks){
        for(const auto& instr : bb->instructions){
            if(instr.src1.kind == Operand::TEMP
                && !bb->def.count(instr.src1.temp_id)){
                    bb->use.insert(instr.src1.temp_id);
                }
            if(instr.src2.kind == Operand::TEMP
                && !bb->def.count(instr.src2.temp_id)){
                    bb->use.insert(instr.src2.temp_id);
                }
            if((instr.op != IRInstruction::LABEL)
                && (instr.op != IRInstruction::JMP)
                && (instr.op != IRInstruction::JZ)
                && (instr.op != IRInstruction::JNZ)){
                if(instr.dst.kind == Operand::TEMP){
                    bb->def.insert(instr.dst.temp_id);
                }
            }
        }
    }

    bool changed;
    do {
        changed = false;
        for(auto it = blocks.rbegin(); it != blocks.rend(); ++it){
            auto& bb = *it;

            std::set<int> new_out;
            for(auto succ : bb->succs){
                new_out.insert(succ->live_ins.begin(), succ->live_ins.end());
            }

            std::set<int> new_in = bb->use;
            for(int temp : new_out){
                if(!bb->def.count(temp)){
                    new_in.insert(temp);
                }
            }

            if(new_in != bb->live_ins || new_out != bb->live_outs) {
                changed = true;
                bb->live_ins = new_in;
                bb->live_outs = new_out;
            } 
        }
    }while(changed);
}