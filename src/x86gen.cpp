#include "x86gen.hpp"
#include <iostream>

void X86Generator::generate() {
  out << ".intel_syntax noprefix" << std::endl;
  out << ".section .text" << std::endl;

  for (auto &func : program.functions) {
    gen_function(func);
  }
}

X86RegAllocState X86Generator::alloc_registers(const IRFunction &func) {
  X86RegAllocState regalloc_state;
  regalloc_state.temp_to_reg.resize(func.temp_count, -1);
  regalloc_state.temp_to_stack_offset.resize(func.temp_count, -1);

  auto &free_regs = regalloc_state.free_regs;
  int instr_idx = 0;

  auto free_temp = [&](const Operand &op) {
    if (op.kind == Operand::TEMP) {
      for (auto &reg : free_regs) {
        if (reg.second == op.temp_id &&
            func.live_intervals[op.temp_id].end == instr_idx) {
          reg.second = -1;
          break;
        }
      }
    }
  };

  auto assign_temp = [&](const Operand &op) {
    if (op.kind == Operand::TEMP) {
      for (size_t i = 0; i < free_regs.size(); i++) {
        if (free_regs[i].second == -1) {
          free_regs[i].second = op.temp_id;
          regalloc_state.temp_to_reg[op.temp_id] = i;
          return;
        }
      }      
      // spill
      regalloc_state.temp_to_reg[op.temp_id] = -1;
      regalloc_state.temp_to_stack_offset[op.temp_id] =
          regalloc_state.spill_offset;
      regalloc_state.spill_offset += 8;
    } 
  };

  for (auto &instr : func.instructions) {
    free_temp(instr.src1);
    assign_temp(instr.dst);
    free_temp(instr.dst);

    instr_idx++;
  }

  return regalloc_state;
}

std::string X86Generator::activateDst(const Operand& op, X86RegAllocState& regalloc_state){
  if(op.kind == Operand::INT_VAL){
    throw std::runtime_error("Destination operand cannot be integer");
  }

  if(regalloc_state.is_spill(op)){
    return regalloc_state.SpillDstReg;
  } else {
    return regalloc_state.free_regs[regalloc_state.temp_to_reg[op.temp_id]].first;
  }
}

std::string X86Generator::activateSrc1(const Operand& op, X86RegAllocState& regalloc_state){
  if(op.kind == Operand::INT_VAL){
    return std::to_string(op.imm);
  }

  if(regalloc_state.is_spill(op)){
    out << "  mov " << regalloc_state.SpillSrc1Reg << ", QWORD PTR [rbp - " << regalloc_state.temp_to_stack_offset[op.temp_id] << "]" << std::endl;
    return regalloc_state.SpillSrc1Reg;
  } else {
    return regalloc_state.free_regs[regalloc_state.temp_to_reg[op.temp_id]].first;
  }
}

std::string X86Generator::activateSrc2(const Operand& op, X86RegAllocState& regalloc_state){
  if(op.kind == Operand::INT_VAL){
    return std::to_string(op.imm);
  }

  if(regalloc_state.is_spill(op)){
    out << "  mov " << regalloc_state.SpillSrc2Reg << ", QWORD PTR [rbp - " << regalloc_state.temp_to_stack_offset[op.temp_id] << "]" << std::endl;
    return regalloc_state.SpillSrc2Reg;
  } else {
    return regalloc_state.free_regs[regalloc_state.temp_to_reg[op.temp_id]].first;
  }
}

void X86Generator::deactivateDst(const Operand& op, X86RegAllocState& regalloc_state){
  if(op.kind == Operand::INT_VAL){
    throw std::runtime_error("Destination operand cannot be integer");
  }

  if(regalloc_state.is_spill(op)){
    out << "  mov QWORD PTR [rbp - " << regalloc_state.temp_to_stack_offset[op.temp_id] << "], " << regalloc_state.SpillDstReg << std::endl;
  }
}

void X86Generator::gen_function(const IRFunction &func) {
  X86RegAllocState regalloc_state = alloc_registers(func);

  out << ".globl " << func.name << std::endl;
  out << ".type " << func.name << ", @function" << std::endl;
  out << func.name << ":" << std::endl;

  // NOTE: we will use stack for temporaries.
  size_t max_spill_offset = regalloc_state.spill_offset;
  size_t stack_size = max_spill_offset;

  if (stack_size % 16 != 0) {
    stack_size = (stack_size / 16 + 1) * 16;
  }
  // align 16 bytes
  if (stack_size == 0)
    stack_size = 16;

  // prologue
  out << "  push rbp\n";
  out << "  mov rbp, rsp\n";
  out << "  sub rsp, " << stack_size << "\n";

  for (auto &instr : func.instructions) {
    gen_instruction(instr, regalloc_state);
  }

  // epilogue
  out << "  leave\n";
  out << "  ret\n";
}

void X86Generator::gen_instruction(const IRInstruction &instr,
                                   X86RegAllocState &regalloc_state) {
  switch (instr.op) {
  case IRInstruction::Op::MOV:
  {
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string dst = activateDst(instr.dst, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }

    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::LSHIFT:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }

    if (instr.src2.kind == Operand::INT_VAL) {
      out << "  sal " << dst << ", " << src2 << std::endl;
    } else {
      out << "  mov rcx, " << src2 << std::endl;
      out << "  sal " << dst << ", cl"
          << std::endl;
    }

    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::RSHIFT:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }

    if (instr.src2.kind == Operand::INT_VAL) {
      out << "  sar " << dst << ", " << src2 << std::endl;
    } else {
      out << "  mov rcx, " << src2 << std::endl;
      out << "  sar " << dst << ", cl" << std::endl;
    }

    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::ADD:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }    
    out << "  add " << dst << ", " << src2 << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::SUB:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }    
    out << "  sub " << dst << ", " << src2 << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::MUL:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }    
    out << "  imul " << dst << ", " << src2 << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::DIV:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    out << "  mov rax, " << src1 << std::endl;
    out << "  cqo" << std::endl;

    if(instr.src2.kind == Operand::INT_VAL){
      out << "  mov rdi, " << src2 << std::endl;
      out << "  idiv rdi" << std::endl;
    } else {
      out << "  idiv " << src2 << std::endl;
    }

    out << "  mov " << dst << ", rax" << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::MOD:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    out << "  mov rax, " << src1 << std::endl;
    out << "  cqo" << std::endl;

    if(instr.src2.kind == Operand::INT_VAL){
      out << "  mov rdi, " << src2 << std::endl;
      out << "  idiv rdi" << std::endl;
    } else {
      out << "  idiv " << src2 << std::endl;
    }

    out << "  mov " << dst << ", rdx" << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::RET:
  {
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    if(src1 != "rax"){
      out << "  mov rax, " << src1 << std::endl;    
    }
  }
    break;
  default:
    throw std::runtime_error("Unknown instruction");
  }
}
