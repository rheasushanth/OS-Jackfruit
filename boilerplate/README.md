# Multi-Container Runtime (OS Project)

## Team Members

* Rhea Sushanth
* Manasa K

---

# Project Summary

This project implements a lightweight Linux container runtime in C with:

* Long-running supervisor daemon
* Multi-container support
* PID / UTS / Mount namespace isolation
* Per-container metadata tracking
* CLI commands (`start`, `stop`, `ps`)
* Kernel module memory monitor
* Soft memory limit warnings
* Hard memory limit enforcement
* Linux scheduler experiments

---

# Build Instructions

## Install dependencies

```bash
sudo apt update
sudo apt install build-essential gcc make linux-headers-$(uname -r)
```

## Build project

```bash
make
```

## Load kernel module

```bash
sudo insmod monitor.ko
```

## Verify device

```bash
ls -l /dev/container_monitor
```

---

# Running the Runtime

## Start supervisor

```bash
sudo ./engine supervisor
```

## Start containers

```bash
sudo ./engine start alpha
sudo ./engine start beta
```

## List containers

```bash
sudo ./engine ps
```

## Stop container

```bash
sudo ./engine stop alpha
```

---

# Isolation Mechanisms

Each container uses:

* PID namespace
* Mount namespace
* UTS namespace

Separate rootfs copies were used:

* rootfs-alpha
* rootfs-beta

This gives filesystem and hostname isolation.

---

# Kernel Memory Monitor

Kernel module provides:

```bash
/dev/container_monitor
```

Supervisor registers container PIDs using ioctl.

## Policies

* Soft limit: warning in dmesg
* Hard limit: process killed

Example:

```bash
sudo dmesg | tail
```

---

# Scheduler Experiment

Two CPU-bound workloads were run concurrently:

```bash
nice -n 0 ./cpu_hog
nice -n 10 ./cpu_hog
```

Result:

The lower nice value received more CPU share, demonstrating Linux CFS priority weighting.

---

# Screenshots Included

1. Multi-container running simultaneously
2. ps metadata output
3. Namespace proof using lsns
4. Kernel registration logs
5. Soft limit warning
6. Hard limit kill
7. Scheduler experiment
8. Clean teardown

---

# Design Decisions

## Supervisor Process

Single long-running supervisor simplifies lifecycle management and metadata tracking.

## UNIX Socket IPC

Used for CLI ↔ supervisor communication.

## Kernel Module

Memory enforcement done in kernel space for direct process control.

## Separate RootFS Copies

Avoids conflicts between live containers.

---

# Engineering Analysis

## Isolation

Namespaces isolate processes while still sharing the host kernel.

## Lifecycle

Supervisor manages child creation, termination, and cleanup.

## Synchronization

Metadata updates are coordinated centrally by supervisor.

## Memory Management

RSS tracks resident memory usage. Soft limits warn; hard limits terminate.

## Scheduling

Linux CFS balances fairness and priority using nice values.

---

# Cleanup

```bash
sudo pkill engine
sudo rmmod monitor
rm -f /tmp/mini_runtime.sock
```

---
