# Multi-Container Runtime (OS Project)

## Team Members

* Rhea Sushanth - PES1UG24CS371
* Manasa K - PES1UG25CS825

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
<img width="1193" height="377" alt="image" src="https://github.com/user-attachments/assets/44e99f24-3ddd-42fa-ba55-cc510734d61d" />


## Start containers

```bash
sudo ./engine start alpha
sudo ./engine start beta
```
<img width="1196" height="330" alt="image" src="https://github.com/user-attachments/assets/99b1ca9f-7f3c-4b73-b92c-f79469f465a6" />

## List containers

```bash
sudo ./engine ps
```


## Stop container

```bash
sudo ./engine stop alpha
```
<img width="1197" height="213" alt="image" src="https://github.com/user-attachments/assets/18115089-48b8-4d5e-9e24-82c79f0653fd" />

---

# Isolation Mechanisms

Each container uses:

* PID namespace
* Mount namespace
* UTS namespace
<img width="1196" height="652" alt="image" src="https://github.com/user-attachments/assets/7eb3e21a-aea3-4dfa-af6c-668e73ccd866" />

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
<img width="1220" height="650" alt="image" src="https://github.com/user-attachments/assets/0bb9a1f1-b75f-4916-8107-37d340e14576" />


# Scheduler Experiment

Two CPU-bound workloads were run concurrently:

```bash
nice -n 0 ./cpu_hog
nice -n 10 ./cpu_hog
```
<img width="1220" height="700" alt="image" src="https://github.com/user-attachments/assets/604faf49-3bd6-4e46-b6e0-674e6f001fd9" />

Result:

The lower nice value received more CPU share, demonstrating Linux CFS priority weighting.

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
