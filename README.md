# MXFS (Temporal-Locality-Optimized Log-Structured File System)

MXFS is a **hardware-assisted, log-structured file system** designed for **resource-constrained IoT and embedded systems** operating in **bare-metal or minimal-OS environments**.
It is the reference implementation accompanying the technical paper:

> *A Hardware-Assisted Log-Structured File System with Temporal Locality Optimization under Resource Constraints*

This repository provides both **software and hardware-side components** that demonstrate how temporal locality, append-only storage semantics, and hardware–software cooperation can be combined to achieve:

* Strong resilience to unexpected power loss
* Fast mount and recovery times
* Low power consumption
* Extremely small memory footprint (≤ 10 KB RAM, ~1 KB FS code)

without relying on journaling file systems, full operating systems, or proprietary storage controllers.

---

## Motivation

Conventional file systems (FAT, ext4, F2FS, etc.) assume:

* Stable power
* Sufficient RAM and CPU resources
* Block-device abstractions (FTL, page cache)

These assumptions break down in **battery-driven IoT devices**, where:

* Power failures are frequent
* RAM is severely limited
* Storage is directly accessed via SPI/NAND flash

MXFS rethinks the boundary between **file system control** and **hardware control**, restoring the original philosophy of Log-Structured File Systems (LFS):

> *A single, chronological append-only structure that preserves temporal locality down to the physical storage layer.*

---

## Key Concepts

### 1. Append-Only Write (AOW)

* All data and metadata are written sequentially in page-sized units
* No overwrite, no journal, no fsync()/msync()
* Incomplete pages caused by power loss are safely skipped during read

### 2. Temporal Locality Optimization

* The file system is optimized for **"recent data access"** (last few days of logs)
* Metadata scanning during mount is minimized using bitmap headers
* Mount time remains constant regardless of total flash size

### 3. Hardware-Cooperative Architecture

MXFS assumes **multiple heterogeneous MCU domains**:

* **Child (low-power domain)**
  Performs sensor logging and append-only writes

* **Parent/Mother (stable-power domain)**
  Performs garbage collection, wear leveling, and recovery

SPI/NAND ownership is arbitrated by a **hardware-level finite state machine (FSM)** (or GPIO-based handshake in the PoC).

### 4. No Block Device Abstraction

* MXFS bypasses block devices entirely
* Eliminates duplicated log structures between FS and FTL
* Preserves wear leveling uniformity and reduces write amplification

---

## Repository Structure

```
mxfs/
├── hw/
│   └── FSM simulations and Verilog testbenches
│
├── sw/
│   ├── child/
│   │   ├── MXFS implementation for low-resource MCUs
│   │   └── Tested on RP2040 / BCM2711 / macOS
│   │
│   └── parent/
│       └── Formatter, verifier, and recovery-side tools
│
└── docs/
    └── Design notes and diagrams (optional)
```

---

## Supported Interfaces

MXFS intentionally exposes a **minimal API surface**:

### MXFS-native API

* `append(layer, buffer, size)`
  Append data to a logical layer (flush is implicit)

* `create_instance()` / `release_instance()`
  Initialize or release the file system (mount/unmount/boot)

### POSIX-like subset (optional)

* `open`, `close`, `read`, `write`

Path resolution and complex metadata handling are intentionally omitted to minimize overhead.

---

## Target Use Cases

* Battery-powered IoT data loggers
* Bare-metal or minimal-RTOS systems
* Multi-sensor embedded devices
* Systems requiring fast reboot after power loss
* Designs without dedicated storage controllers

---

## Hardware Reference Implementation

The reference prototype uses:

* **Child**: RP2040 (Raspberry Pi Pico)
  Dual-core Cortex-M0+, 264 KB SRAM

* **Parent**: BCM2711 (Raspberry Pi 4)
  Quad-core Cortex-A72

* **Storage**: W25N01GV SPI NAND Flash (128 MB)

SPI bus ownership is switched dynamically between domains.

---

## Hardware FSM Architecture

The SPI bus ownership arbitration is implemented as a **Moore machine** — the output `Q` depends only on the current state, not directly on input signal changes. This ensures **glitch-free, deterministic** bus ownership switching.

### Modules

| Module | File | Role |
|---|---|---|
| `mxfs_fast` | `hw/fsm_fast.v` | Fast domain FSM (100-cycle transitions) |
| `mxfs_slow` | `hw/fsm_slow.v` | Slow domain FSM (200-cycle transitions) |
| `mxfs_selector` | `hw/fsm_selector.v` | Moore machine bus ownership selector |

### Per-Domain FSM State Transitions (`mxfs_fast` / `mxfs_slow`)

Both `mxfs_fast` and `mxfs_slow` share the same 4-state Moore machine structure.
The only difference is the counter threshold (fast: 100 cycles, slow: 200 cycles).

```
          ┌──────────────────────────────────────────────────┐
          │              Asynchronous Reset                   │
          │          (upl_arst_n = L → INIT)                 │
          └──────────────┬───────────────────────────────────┘
                         ▼
                  ┌─────────────┐
                  │    INIT     │  out_ctrl1 = L
                  │   (4'd1)    │
                  └──────┬──────┘
                         │ counter >= threshold
                         ▼
                  ┌─────────────┐
                  │  READY_0    │  out_ctrl1 = L
                  │   (4'd2)    │
                  └──────┬──────┘
                         │ counter >= threshold
                         ▼
                  ┌─────────────┐
                  │  READY_1    │  out_ctrl1 = H
                  │   (4'd3)    │
                  └──────┬──────┘
                         │ counter >= threshold
                         ▼
                  ┌─────────────┐
                  │   VALID     │  out_ctrl1 = in_ctrl
                  │   (4'd4)    │  (terminal state)
                  └─────────────┘
```

* `out_ctrl0` — Software-controlled via `debug_status_ingress[0]`
* `out_ctrl1` — Determined by FSM state; in `VALID`, follows `in_ctrl`
* Threshold: `mxfs_fast` = 100 cycles, `mxfs_slow` = 200 cycles

### Bus Ownership Selector (`mxfs_selector`) — Moore Machine

`mxfs_selector` takes `out_ctrl1` from each domain and produces a single output `Q` that determines bus ownership.
Input synchronization uses a **2-stage flip-flop** for metastability mitigation across clock domains.

**State Table:**

| State | `out_ctrl1_fast` | `out_ctrl1_slow` | Q | Bus Owner |
|:---:|:---:|:---:|:---:|:---|
| S0 | L | L | **H** | Mother (fast domain) |
| S1 | H | L | **H** | Mother (fast domain) |
| S2 | L | H | **L** | Child (slow domain) |
| S3 | H | H | **H** | Mother (fast domain) |

**Design rationale:**
* The Child (slow domain) acquires the bus **only** when `out_ctrl1_slow = H` and `out_ctrl1_fast = L` (state S2)
* In all other cases, including contention (`H, H`), the Mother (fast domain) retains ownership
* Output `Q` is registered, guaranteeing **glitch-free** operation on the bus select line
* On reset, `Q = H` (Mother owns the bus)

### Cross-Domain Connection

```
  ┌──────────┐                          ┌──────────┐
  │mxfs_fast │  out_ctrl0_fast ───────► │mxfs_slow │
  │          │ ◄─────── out_ctrl0_slow  │          │
  │          │                          │          │
  │  out_ctrl1_fast ──┐    ┌── out_ctrl1_slow     │
  └──────────┘        │    │            └──────────┘
                      ▼    ▼
                ┌──────────────┐
                │mxfs_selector │
                │ (Moore FSM)  │
                └──────┬───────┘
                       │
                       ▼
                    Q (Bus Select)
                  H = Mother (fast)
                  L = Child  (slow)
```

* `out_ctrl0` of each domain is cross-connected to `in_ctrl` of the other domain
* `out_ctrl1` of each domain feeds into `mxfs_selector`

### Verification Status

Logic verification has been completed via Verilog testbench (`hw/fsm_tb.v`):

| Testbench | Description | Result |
|---|---|---|
| `mxfs_fast_tb` | Fast domain unit test (INIT→READY_0→READY_1→VALID) | PASS |
| `mxfs_slow_tb` | Slow domain unit test (INIT→READY_0→READY_1→VALID) | PASS |
| `mxfs_fast_slow_pattern_a_tb` | Cross-domain integration (pin Q logic table) | PASS |
| `mxfs_fast_slow_pattern_b_tb` | Software-controlled `out_ctrl0` + all 4 Q states | PASS |

> **Note:** The hardware FSM is currently verified in simulation only. Physical implementation on FPGA/ASIC is planned for future work.

---

## Research Background

This implementation is based on the accompanying research paper, which provides:

* Formal motivation and design rationale
* Performance and power measurements
* Comparison with ext4, FAT, LOGFS, and commercial solutions
* Discussion of FTL, PLR, and block-device abstraction pitfalls

If you use MXFS in academic or industrial research, please refer to the paper.

---

## License

This project is released under an open-source license (see `LICENSE`).

---

## Citation

If you use this work, please cite:

> Y. Sugisawa, D. Sugisawa,
> *A Hardware-Assisted Log-Structured File System with Temporal Locality Optimization under Resource Constraints*, 2025.

---

## Status

* Core concepts validated via prototype
* Software-based arbitration used in PoC
* **Moore machine FSM for bus ownership arbitration: logic verified via Verilog testbench**
* Physical hardware FSM implementation (FPGA/ASIC) planned for future work

Contributions and discussions are welcome.
