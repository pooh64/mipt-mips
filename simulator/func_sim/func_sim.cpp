/*
 * func_sim.cpp - extremely simple simulator
 * Copyright 2018 MIPT-MIPS
 */

#include "func_sim.h"
#include <kernel/kernel.h>

#include <boost/tokenizer.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>


template <typename ISA>
FuncSim<ISA>::FuncSim( Endian endian, bool log, const std::string &trap_options)
    : Simulator( log)
    , imem( endian)
    , kernel( Kernel::create_dummy_kernel())
{
    boost::char_separator sepr(" ");
    boost::tokenizer tokens(trap_options, sepr);
    for ( auto it = tokens.begin(); it != tokens.end(); ++it) {
             if ( *it == "stop")
            handle_trap_mode = HandleTrapMode::STOP;
        else if ( *it == "stop_on_halt")
            handle_trap_mode = HandleTrapMode::STOP_ON_HALT;
        else if ( *it == "ignore")
            handle_trap_mode = HandleTrapMode::IGNORE;
        else if ( *it == "critical")
            handle_trap_critical = true;
        else if ( *it == "verbose")
            handle_trap_verbose = true;
    }
}

template <typename ISA>
void FuncSim<ISA>::set_memory( std::shared_ptr<FuncMemory> m)
{
    mem = std::move( m);
    imem.set_memory( mem);
}

template <typename ISA>
void FuncSim<ISA>::update_and_check_nop_counter( const FuncInstr& instr)
{
    if ( instr.is_nop())
        ++nops_in_a_row;
    else
        nops_in_a_row = 0;

    if (nops_in_a_row > 10)
        throw BearingLost();
}

template <typename ISA>
typename FuncSim<ISA>::FuncInstr FuncSim<ISA>::step()
{
    FuncInstr instr = imem.fetch_instr( pc[0]);;
    instr.set_sequence_id(sequence_id);
    sequence_id++;
    rf.read_sources( &instr);
    instr.execute();
    mem->load_store( &instr);
    rf.write_dst( instr);
    update_pc( instr);
    update_and_check_nop_counter( instr);
    return instr;
}

template <typename ISA>
void FuncSim<ISA>::update_pc( const FuncInstr& instr)
{
    auto current_pc = pc[0];
    for (size_t i = 0; i < instr.get_delayed_slots(); ++i) {
        current_pc += 4;
        pc[i] = current_pc;
    }
    pc[instr.get_delayed_slots()] = instr.get_new_PC();
    if (delayed_slots > 0) {
        for (size_t i = 0; i < delayed_slots; ++i)
            pc[i] = pc[i + 1];
        --delayed_slots;
    }
    else {
        delayed_slots = instr.get_delayed_slots();
    }
}

static SyscallResult execute_syscall( Kernel* kernel)
{
    do try {
        return kernel->execute();
    }
    catch (const BadInputValue& e) {
        std::cerr << e.what();
    } while (true);

    return {SyscallResult::UNSUPPORTED, 0};
}

template <typename ISA>
Trap FuncSim<ISA>::handle_syscall()
{
    auto result = execute_syscall( kernel.get());
    switch ( result.type) {
    case SyscallResult::HALT:        exit_code = result.code; return Trap(Trap::HALT);
    case SyscallResult::IGNORED:     return Trap(Trap::SYSCALL);
    case SyscallResult::SUCCESS:     return Trap(Trap::NO_TRAP);
    case SyscallResult::UNSUPPORTED: return Trap(Trap::UNSUPPORTED_SYSCALL);
    default: assert( 0);
    }
    return Trap(Trap::NO_TRAP);
}

template<typename ISA>
Trap FuncSim<ISA>::step_system()
{
    const auto& instr = step();
    sout << instr << std::endl;

    if ( instr.trap_type() == Trap::UNKNOWN_INSTRUCTION)
        throw UnknownInstruction( instr.string_dump() + ' ' + instr.bytes_dump());

    return instr.trap_type();
}

template <typename ISA>
Trap FuncSim<ISA>::handle_trap( Trap trap)
{
    if ( trap == Trap::SYSCALL)
        trap = handle_syscall();
    if ( trap == Trap::NO_TRAP)
        return trap;

    if ( handle_trap_verbose)
        std::cout << "\tFuncSim trap: " << trap << std::endl;

    if ( handle_trap_critical)
        throw std::runtime_error( "critical trap");

    switch ( handle_trap_mode)
    {
    case HandleTrapMode::STOP:
        return trap;

    case HandleTrapMode::STOP_ON_HALT:
        if ( trap == Trap::HALT)
            return trap;
        return Trap(Trap::NO_TRAP);

    case HandleTrapMode::IGNORE:
        return Trap(Trap::NO_TRAP);
    }

    return trap;
}

template <typename ISA>
Trap FuncSim<ISA>::run( uint64 instrs_to_run)
{
    nops_in_a_row = 0;
    for ( uint64 i = 0; i < instrs_to_run; ++i) {
        auto trap = step_system();
        if ( trap != Trap::NO_TRAP)
            trap = handle_trap(trap);
        if ( trap != Trap::NO_TRAP)
            return trap;
    }
    return Trap(Trap::NO_TRAP);
}

template <typename ISA>
Trap FuncSim<ISA>::run_single_step()
{
    auto trap = step_system();
    if ( trap == Trap::SYSCALL)
        trap = handle_syscall();
    return trap == Trap(Trap::NO_TRAP) ? Trap(Trap::BREAKPOINT) : trap;
}

template <typename ISA>
uint64 FuncSim<ISA>::read_gdb_register( uint8 regno) const
{
    if ( regno == Register::get_gdb_pc_index())
        return get_pc();

    return read_register( Register::from_gdb_index( regno));
}

template <typename ISA>
void FuncSim<ISA>::write_gdb_register( uint8 regno, uint64 value)
{
    if ( regno == Register::get_gdb_pc_index())
        set_pc( value);
    else
        write_register( Register::from_gdb_index( regno), value);
}

#include <mips/mips.h>
#include <risc_v/risc_v.h>

template class FuncSim<MIPSI>;
template class FuncSim<MIPSII>;
template class FuncSim<MIPSIII>;
template class FuncSim<MIPSIV>;
template class FuncSim<MIPS32>;
template class FuncSim<MIPS64>;
template class FuncSim<MARS>;
template class FuncSim<MARS64>;
template class FuncSim<RISCV32>;
template class FuncSim<RISCV64>;
template class FuncSim<RISCV128>;

