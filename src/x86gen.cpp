#include "x86gen.hpp"
#include "irgen.hpp"
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
            func.temp_reg_infos[op.temp_id].end == instr_idx) {
          reg.second = -1;
          break;
        }
      }
    }
  };

  auto assign_temp = [&](const Operand &op) {
    if (op.kind == Operand::TEMP) {
      if(regalloc_state.temp_to_reg[op.temp_id] != -1
        || regalloc_state.temp_to_stack_offset[op.temp_id] != -1){
          return ;
        }
      for (size_t i = 0; i < free_regs.size(); i++) {
        if (free_regs[i].second == -1) {
          free_regs[i].second = op.temp_id;
          regalloc_state.temp_to_reg[op.temp_id] = i;
          return;
        }
      }      
      // spill
      regalloc_state.temp_to_reg[op.temp_id] = -1;

      regalloc_state.spill_offset = regalloc_state.align_to(regalloc_state.spill_offset, op.type.bytes);
      regalloc_state.temp_to_stack_offset[op.temp_id] =
          regalloc_state.spill_offset;
      regalloc_state.spill_offset += op.type.bytes;
    } 
  };

  for (auto &instr : func.instructions) {

    for (int t = 0; t < func.temp_count; t++) {
      if (func.temp_reg_infos[t].start == instr_idx) {
        assign_temp(Operand::Temp(t, func.temp_reg_infos[t].type));
      }
    }
    free_temp(instr.src1);
    assign_temp(instr.dst);
    free_temp(instr.src2);
    free_temp(instr.dst);

    instr_idx++;
  }

  return regalloc_state;
}

std::string X86Generator::activateDst(const Operand& op, X86RegAllocState& regalloc_state){

  switch(op.kind)
  {
    case Operand::INT_VAL:
      throw std::runtime_error("Destination operand cannot be integer");
    case Operand::TEMP:
    {
      if(regalloc_state.is_spill(op)){
        return regalloc_state.SpillDstReg;
      } else {
        return regalloc_state.free_regs[regalloc_state.temp_to_reg[op.temp_id]].first;
      }
    }
    case Operand::LLABEL:
    {
      return ".L" + std::to_string(op.label_id);
    }
    case Operand::FUNC:
    {
      throw std::runtime_error("Destination operand cannot be label or function");
    }
  }
  throw std::runtime_error("Invalid operand kind in activateDst");
}

std::string X86Generator::activateSrc1(const Operand& op, X86RegAllocState& regalloc_state){

  switch(op.kind)
  {
    case Operand::INT_VAL:
      return std::to_string(op.imm);
    case Operand::TEMP:
    {
      if(regalloc_state.is_spill(op)){
        regalloc_state.load_from_spill(out, op, regalloc_state.SpillSrc1Reg);
        return regalloc_state.SpillSrc1Reg;
      } else {
        return regalloc_state.free_regs[regalloc_state.temp_to_reg[op.temp_id]].first;
      }
    }
    case Operand::LLABEL:
    {
      return ".L" + std::to_string(op.label_id);
    }
    case Operand::FUNC:
    {
      return op.name;
    }
  }
  throw std::runtime_error("Invalid operand kind in activateSrc1");
}

std::string X86Generator::activateSrc2(const Operand& op, X86RegAllocState& regalloc_state){

  switch(op.kind)
  {
    case Operand::INT_VAL:
      return std::to_string(op.imm);
    case Operand::TEMP:
    {
      if(regalloc_state.is_spill(op)){
        regalloc_state.load_from_spill(out, op, regalloc_state.SpillSrc2Reg);
        return regalloc_state.SpillSrc2Reg;
      } else {
        return regalloc_state.free_regs[regalloc_state.temp_to_reg[op.temp_id]].first;
      }
    }
    case Operand::LLABEL:
    {
      return ".L" + std::to_string(op.label_id);
    }
    case Operand::FUNC:
    {
      return op.name;
    }
  }
  throw std::runtime_error("Invalid operand kind in activateSrc2");
}

void X86Generator::deactivateDst(const Operand& op, X86RegAllocState& regalloc_state){
  if(op.kind == Operand::INT_VAL){
    throw std::runtime_error("Destination operand cannot be integer");
  }

  if(regalloc_state.is_spill(op)){
    switch(op.type.bytes){
      case 1:
        out << "  mov BYTE PTR [rbp - " << regalloc_state.temp_to_stack_offset[op.temp_id] 
          << "], " << regalloc_state.get_reg_8(regalloc_state.SpillDstReg) << std::endl;
        break;
      case 2:
        out << "  mov WORD PTR [rbp - " << regalloc_state.temp_to_stack_offset[op.temp_id] 
          << "], " << regalloc_state.get_reg_16(regalloc_state.SpillDstReg) << std::endl;
        break;
      case 4:
        out << "  mov DWORD PTR [rbp - " << regalloc_state.temp_to_stack_offset[op.temp_id] 
          << "], " << regalloc_state.get_reg_32(regalloc_state.SpillDstReg) << std::endl;
        break;
      case 8:
        out << "  mov QWORD PTR [rbp - " << regalloc_state.temp_to_stack_offset[op.temp_id] 
          << "], " << regalloc_state.SpillDstReg << std::endl;
        break;
      default:
        throw std::runtime_error("Invalid operand type");
    }
  } else {
    std::string reg = regalloc_state.free_regs[regalloc_state.temp_to_reg[op.temp_id]].first; 
    if(op.type.isUnsigned){
      switch(op.type.bytes){
        case 1:
          out << "  movzx " << reg << ", " << regalloc_state.get_reg_8(reg) << std::endl;
          break;
        case 2:
          out << "  movzx " << reg << ", " << regalloc_state.get_reg_16(reg) << std::endl;
          break;
        case 4:
          out << "  mov " << regalloc_state.get_reg_32(reg) << ", " << regalloc_state.get_reg_32(reg) << std::endl;
          break;
        case 8:
          break;
        default:
          throw std::runtime_error("Invalid operand type");
      }
    } else {
      switch(op.type.bytes){
        case 1:
          out << "  movsx " << reg << ", " << regalloc_state.get_reg_8(reg) << std::endl;
          break;
        case 2:
          out << "  movsx " << reg << ", " << regalloc_state.get_reg_16(reg) << std::endl;
          break;
        case 4:
          out << "  movsxd " << reg << ", " << regalloc_state.get_reg_32(reg) << std::endl;
          break;
        case 8:
          break;
        default:
          throw std::runtime_error("Invalid operand type");
      }
    }
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
  out << "  push rbx\n";
  out << "  push r12\n";
  out << "  push r13\n";
  out << "  push r14\n";
  out << "  push r15\n";

  for (auto &instr : func.instructions) {
    gen_instruction(instr, regalloc_state);
  }

  // epilogue
  out << ".L." << func.name << ".return:\n";
  out << "  pop r15\n";
  out << "  pop r14\n";
  out << "  pop r13\n";
  out << "  pop r12\n";
  out << "  pop rbx\n";
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
  case IRInstruction::Op::CALL:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    out << "  call " << instr.src1.name << std::endl;
    out << "  mov " << dst << ", rax" << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::LT:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if (instr.src1.kind == Operand::INT_VAL) {
      out << "  mov rax, " << src1 << std::endl;
      src1 = "rax";
    }
    out << "  cmp " << src1 << ", " << src2 << std::endl;
    if(!instr.src1.type.isUnsigned){
      out << "  setl al" << std::endl;
    } else {
      out << "  setb al" << std::endl;
    }
    out << "  movzx " << dst << ", al" << std::endl;

    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::LE:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if (instr.src1.kind == Operand::INT_VAL) {
      out << "  mov rax, " << src1 << std::endl;
      src1 = "rax";
    }
    out << "  cmp " << src1 << ", " << src2 << std::endl;
    if(!instr.src1.type.isUnsigned){
      out << "  setle al" << std::endl;
    } else {
      out << "  setbe al" << std::endl;
    }
    out << "  movzx " << dst << ", al" << std::endl;

    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::BAND:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }
    out << "  and " << dst << ", " << src2 << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::BOR:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }
    out << "  or " << dst << ", " << src2 << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::BXOR:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }
    out << "  xor " << dst << ", " << src2 << std::endl;
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

    std::string rcx = src2;
    if(instr.src2.kind == Operand::TEMP){
      rcx = "cl";
      out << "  mov rcx, " << src2 << std::endl;
    }

    if(instr.src1.type.isUnsigned){
      out << "  shr " << dst << ", " << rcx << std::endl;
    } else {
      out << "  sar " << dst << ", " << rcx << std::endl;
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
    std::string rdi = src2;
    if(instr.src2.kind == Operand::INT_VAL){
      rdi = "rdi";
      out << "  mov rdi, " << src2 << std::endl;
    }

    if(instr.src1.type.isUnsigned){
      out << "  xor rdx, rdx" << std::endl;
      out << "  div " << rdi << std::endl;
    } else {
      out << "  cqo" << std::endl;
      out << "  idiv " << rdi << std::endl;
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
    break;
  }
  case IRInstruction::Op::BNOT:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }    
    out << "  not " << dst << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::NEG:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }    
    out << "  neg " << dst << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::NOT:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);

    if(src1 != dst){
      out << "  mov " << dst << ", " << src1 << std::endl;
    }    
    out << "  test " << dst << "," << dst << std::endl;
    out << "  setz al" << std::endl;
    out << "  movzx " << dst << ", al" << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::EQ:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    out << "  mov rax, " << src1 << std::endl;
    out << "  mov rcx, " << src2 << std::endl;

    out << "  cmp rax, rcx" << std::endl;
    out << "  sete al" << std::endl;

    out << "  movzx rax, al" << std::endl;
    out << "  mov " << dst << ", rax" << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::NEQ:
  {
    std::string dst = activateDst(instr.dst, regalloc_state);
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    out << "  mov rax, " << src1 << std::endl;
    out << "  mov rcx, " << src2 << std::endl;

    out << "  cmp rax, rcx" << std::endl;
    out << "  setne al" << std::endl;

    out << "  movzx rax, al" << std::endl;
    out << "  mov " << dst << ", rax" << std::endl;
    deactivateDst(instr.dst, regalloc_state);
    break;
  }
  case IRInstruction::Op::JZ:
  {
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    out << "  mov rax, " << src1 << std::endl;
    out << "  test rax, rax" << std::endl;
    out << "  jz " << src2 << std::endl;
    break;
  }
  case IRInstruction::Op::JNZ:
  {
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    std::string src2 = activateSrc2(instr.src2, regalloc_state);

    out << "  mov rax, " << src1 << std::endl;
    out << "  test rax, rax" << std::endl;
    out << "  jnz " << src2 << std::endl;
    break;
  }
  case IRInstruction::Op::JMP:
  {
    std::string src1 = activateSrc1(instr.src1, regalloc_state);

    out << "  jmp " << src1 << std::endl;
    break;
  }
  case IRInstruction::Op::LABEL:
  {
    std::string src1 = activateSrc1(instr.src1, regalloc_state);
    out << src1 << ":" << std::endl;
    break;
  }
  default:
    throw std::runtime_error("Unknown instruction");
  }
}
