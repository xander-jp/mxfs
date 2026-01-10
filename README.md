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
* Full hardware FSM implementation planned for future work

Contributions and discussions are welcome.
