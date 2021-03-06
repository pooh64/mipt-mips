/*
 * simulator.h - interface for simulator
 * Copyright 2018 MIPT-MIPS
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <func_sim/trap_types.h>
#include <infra/exception.h>
#include <infra/log.h>
#include <infra/target.h>
#include <infra/types.h>

#include <memory>

struct BearingLost final : Exception {
    BearingLost() : Exception("Bearing lost", "10 nops in a row") { }
};

struct InvalidISA final : Exception
{
    explicit InvalidISA(const std::string& isa)
        : Exception("Invalid ISA", isa)
    { }
};

class FuncMemory;
class Kernel;

class Simulator : public Log {
public:
    explicit Simulator( bool log = false) : Log( log) {}

    virtual Trap run( uint64 instrs_to_run) = 0;
    virtual Trap run_single_step() = 0;
    virtual void set_target( const Target& target) = 0;
    virtual void set_memory( std::shared_ptr<FuncMemory> m) = 0;
    virtual void set_kernel( std::shared_ptr<Kernel> k) = 0;
    virtual void init_checker() = 0;

    Trap run_no_limit() { return run( MAX_VAL64); }
    void set_pc( Addr pc) { set_target( Target( pc, 0)); }
    virtual Addr get_pc() const = 0;

    static std::shared_ptr<Simulator> create_simulator( const std::string& isa, bool functional_only, bool log,
        const std::string &trap_options = "stop_on_halt verbose");
    static std::shared_ptr<Simulator> create_configured_simulator();
    static std::shared_ptr<Simulator> create_configured_isa_simulator( const std::string& isa);
    static std::shared_ptr<Simulator> create_configured_debugger_isa_simulator( const std::string& isa);
    static std::shared_ptr<Simulator> create_functional_simulator( const std::string& isa, bool log = false,
        const std::string &trap_options = "stop_on_halt verbose")
    {
        return create_simulator( isa, true, log, trap_options);
    }

    virtual size_t sizeof_register() const = 0;

    virtual uint64 read_cpu_register( uint8 regno) const = 0;
    virtual uint64 read_gdb_register( uint8 regno) const = 0;
    virtual void write_cpu_register( uint8 regno, uint64 value) = 0;
    virtual void write_gdb_register( uint8 regno, uint64 value) = 0;
    virtual void write_csr_register( std::string_view name, uint64 value) = 0;

    int get_exit_code() const { return exit_code; }

protected:
    int exit_code = 0;
};

class CycleAccurateSimulator : public Simulator {
public:
    explicit CycleAccurateSimulator( bool log = false) : Simulator( log) {}
    virtual void clock() = 0;
    virtual void halt() = 0;
    static std::shared_ptr<CycleAccurateSimulator> create_simulator(const std::string& isa, bool log);
};

#endif // SIMULATOR_H
