# Architecture Cache

**Last updated**: 2026-09-21 — Added Makefile build system and complete documentation suite (ISA MAC-1, Simulator Doc, API Spec, Contributing)

## Stack
- **Language**: C, [Pending: UI framework, planned React/Node.js]
- **Target Architecture**: MIC-1 (MAC-1 instruction set confirmed)
- **Framework**: None for C backend
- **Package Manager**: None
- **Build Command**: `make all` (compiles `bin/assembler` and `bin/simulator.o`)
- **Test Runner**: [No test runner configured]

## Module Map
- `simulator/` - Core logic for the assembler and simulator (C files)
  - `assembler_doc.md` - Documentation for the assembler CLI
  - `simulator_doc.md` - Documentation for the simulator engine
- `ui/` - Directory for the web interface (placeholder)
- `docs/` - Project technical documentation
  - `README.md` - Executive summary, navigation, and open critical questions
  - `arquitetura.md` - Layer architecture, Mermaid data flow, and C core data structures
  - `roadmap.md` - Phased roadmap and granular task list (unassigned)
  - `isa_mac1.md` - MAC-1 ISA instruction set manual and opcode table
  - `api_spec.md` - Node.js REST and WebSocket API specification
- `CONTRIBUTING.md` - Code standards, naming conventions, and Git commit rules
- `Makefile` - Build automation for C executables

## Entry Points
- `simulator/assembler.c` (CLI executable `bin/assembler`)
  - Input: command line arguments (`-i` for interactive/web backend, `-o <file>` for output)
  - Auth: None

## Documentation
- Structure:
  - `README.md` (root navigation and credits)
  - `docs/README.md`, `docs/arquitetura.md`, `docs/roadmap.md`, `docs/isa_mac1.md`, `docs/api_spec.md`
  - `simulator/assembler_doc.md`, `simulator/simulator_doc.md`
  - `CONTRIBUTING.md`
- Build Command: None (raw markdown)
- Last drift baseline: 2026-09-21 — 0 gaps remaining

## Open Issues
- [No test runner configured]
- [UI Stack not formally initialized]
- [Pending decision: Execution step granularity (macro-instruction vs microcode)]

## Pending Edge Cases

## Pending Manual Testing Steps
