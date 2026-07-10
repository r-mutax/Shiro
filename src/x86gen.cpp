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

std::string X86Generator::to_x86(const Operand &op,
                                 X86RegAllocState &regalloc_state) {
  switch (op.kind) {
  case Operand::INT_VAL:
    return std::to_string(op.imm);
  case Operand::TEMP:
    int reg_idx = regalloc_state.temp_to_reg[op.temp_id];
    if (reg_idx != -1) {
      return regalloc_state.free_regs[reg_idx].first;
    }
    // Spill
    int offset = regalloc_state.temp_to_stack_offset[op.temp_id];
    if (offset == -1) {
      offset = regalloc_state.spill_offset;
      regalloc_state.temp_to_stack_offset[op.temp_id] = offset;
      regalloc_state.spill_offset += 8;
    }
    return "QWORD PTR [rbp - " + std::to_string(offset) + "]";
  }
  throw std::runtime_error("Unknown operand kind");
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
    out << "  mov rax, " << to_x86(instr.src1, regalloc_state) << std::endl;
    out << "  mov " << to_x86(instr.dst, regalloc_state) << ", rax"
        << std::endl;
    break;
  case IRInstruction::Op::LSHIFT:
    if (!regalloc_state.is_same_temp(instr.src1, instr.dst)) {
      out << "  mov " << to_x86(instr.dst, regalloc_state) << ", "
          << to_x86(instr.src1, regalloc_state) << std::endl;
    }

    if (instr.src2.kind == Operand::INT_VAL) {
      out << "  sal " << to_x86(instr.dst, regalloc_state) << ", "
          << to_x86(instr.src2, regalloc_state) << std::endl;
    } else {
      out << "  mov rcx, " << to_x86(instr.src2, regalloc_state) << std::endl;
      out << "  sal " << to_x86(instr.dst, regalloc_state) << ", cl"
          << std::endl;
    }
    break;
  case IRInstruction::Op::RSHIFT:
    if (!regalloc_state.is_same_temp(instr.src1, instr.dst)) {
      out << "  mov " << to_x86(instr.dst, regalloc_state) << ", "
          << to_x86(instr.src1, regalloc_state) << std::endl;
    }

    if (instr.src2.kind == Operand::INT_VAL) {
      out << "  sar " << to_x86(instr.dst, regalloc_state) << ", "
          << to_x86(instr.src2, regalloc_state) << std::endl;
    } else {
      out << "  mov rcx, " << to_x86(instr.src2, regalloc_state) << std::endl;
      out << "  sar " << to_x86(instr.dst, regalloc_state) << ", cl"
          << std::endl;
    }
    break;
  case IRInstruction::Op::ADD:
    if (!regalloc_state.is_same_temp(instr.src1, instr.dst)) {
      out << "  mov " << to_x86(instr.dst, regalloc_state) << ", "
          << to_x86(instr.src1, regalloc_state) << std::endl;
    }
    out << "  add " << to_x86(instr.dst, regalloc_state) << ", "
        << to_x86(instr.src2, regalloc_state) << std::endl;
    break;
  case IRInstruction::Op::SUB:
    if (!regalloc_state.is_same_temp(instr.src1, instr.dst)) {
      out << "  mov " << to_x86(instr.dst, regalloc_state) << ", "
          << to_x86(instr.src1, regalloc_state) << std::endl;
    }
    out << "  sub " << to_x86(instr.dst, regalloc_state) << ", "
        << to_x86(instr.src2, regalloc_state) << std::endl;
    break;
  case IRInstruction::Op::MUL:
    if (!regalloc_state.is_same_temp(instr.src1, instr.dst)) {
      out << "  mov " << to_x86(instr.dst, regalloc_state) << ", "
          << to_x86(instr.src1, regalloc_state) << std::endl;
    }
    out << "  imul " << to_x86(instr.dst, regalloc_state) << ", "
        << to_x86(instr.src2, regalloc_state) << std::endl;
    break;
  case IRInstruction::Op::DIV:
    out << "  mov rax, " << to_x86(instr.src1, regalloc_state) << std::endl;
    out << "  cqo" << std::endl;
    if (instr.src2.kind == Operand::INT_VAL) {
      out << "  mov rdi, " << to_x86(instr.src2, regalloc_state) << std::endl;
      out << "  idiv rdi" << std::endl;
    } else {
      out << "  idiv " << to_x86(instr.src2, regalloc_state) << std::endl;
    }
    out << "  mov " << to_x86(instr.dst, regalloc_state) << ", rax"
        << std::endl;
    break;
  case IRInstruction::Op::MOD:
    out << "  mov rax, " << to_x86(instr.src1, regalloc_state) << std::endl;
    out << "  cqo" << std::endl;
    if (instr.src2.kind == Operand::INT_VAL) {
      out << "  mov rdi, " << to_x86(instr.src2, regalloc_state) << std::endl;
      out << "  idiv rdi" << std::endl;
    } else {
      out << "  idiv " << to_x86(instr.src2, regalloc_state) << std::endl;
    }
    out << "  mov " << to_x86(instr.dst, regalloc_state) << ", rdx"
        << std::endl;
    break;
  case IRInstruction::Op::RET:
    out << "  mov rax, " << to_x86(instr.src1, regalloc_state) << std::endl;
    break;
  default:
    throw std::runtime_error("Unknown instruction");
  }
}
