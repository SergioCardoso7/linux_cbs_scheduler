# Linux CBS Scheduler - A practical implementation of the Constant Bandwidth Server scheduler in the linux kernel 

## Overview

The project implements Earliest Deadline First (EDF) scheduling combined with a Constant Bandwidth Server (CBS) mechanism in the Linux kernel. The goal was to explore advanced real-time scheduling theory and its integration within the Linux scheduler infrastructure.

Unlike Linux’s `SCHED_DEADLINE`, which uses a more pragmatic enforcement strategy, this implementation follows the CBS model more closely to its original theoretical specification, with emphasis on precise timing behavior.

For simplicity and deterministic evaluation, the system was restricted to a single CPU core, eliminating multicore effects such as migration and load balancing to focus strictly on EDF/CBS correctness.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
    - [Key Technologies Used](#key-technologies-used)
- [Quick Start](#quick-start)
- [Requirements & Installation Guide](#requirements-and-installation-guide)
- [Building the Kernel](#building-the-kernel)
- [Running Examples](#running-examples)
- [Architecture](#architecture)
- [Implementation](#implementation)
- [Results](#results)
- [Lessons Learned](#lessons-learned)
- [Report](#report)
- [Repository Structure](#repository-structure)
- [References](#references)



## Features

- **Pure EDF Scheduling** - EDF (Earliest Deadline First) Dynamic Priority assignment based on absolute deadlines
- **Constant Bandwidth Server** - CBS (Constant Bandwidth Server) Temporal Isolation for aperiodic/sporadic tasks
- **Nanosecond Accuracy** - High-resolution hardware timer for budget control
- **Temporal Isolation** - Hard real-time tasks protected from soft task overload
- **O(1) Task Selection** - O(1) complexity for earliest deadline lookup using the leftmost-cached red-black tree (`rb_root_cached`)
- **Seamless Integration to Kernel** Kernel scheduler class integration and customizable through menuconfig (Kconfig)
- **Deterministic Behavior** - Single-core deterministic execution

    ### Key Technologies Used
    - C
    - Linux Kernel
    - Scheduler internals
    - Virtme-ng (for quick and easy kernel recompilation and testing), 
        - check it here: [virtme-ng](https://github.com/arighi/virtme-ng)
    - hrtimers
    - Red-black trees
    - QEMU/KVM

### Quick Start
```console
$ uname -r
[Your current kernel version will appear here]
$ git clone git@github.com:SergioCardoso7/linux_cbs_scheduler.git
$ sudo ./scripts/kcompile.sh
$ vng -r linux-6.19.9-moker/arch/x86/boot/bzImage --cpus 1
$ uname -r
6.19.9-moker
^
|___  Now you have a shell inside a virtualized copy of your entire system, that is running the new kernel! \o/

$ ./launcher taskset.txt server_taskset.txt
^
|__ Example on how to run the tests, the command follows this pattern: ./launcher [periodic taskset file] [aperiodic task 1 jobs file] [aperiodic task 2 jobs file] [aperiodic task n jobs file]
You can also just run one of the example scripts in scripts/examples

$ cd ../plot
$ python3 execution_plot.py cbs-logs.csv
^
|__ This generates the plot of the runned tasksets

Then simply type "exit" to return back to the real system.
```

## Requirements and Installation Guide

- Need to be on a Linux machine (or VM with nested virtualization enabled)
- **Check if KVM is available**
    ```console
    ls /dev/kvm
    ```
    If you see `/dev/kvm` you're good. Otherwise check BIOS/UEFI settings.
- **Install dependencies of virtme-ng**
    ```console 
    $ sudo apt update
    $ sudo apt install python3-pip flake8 pylint cargo rustc qemu-system-x86
    $ sudo apt install virtiofsd
    ``` 
- **Install virtme-ng**
    
    Instructions might not be up-to-date, check latest instructions on virtme-ng repository
    
    Install from PyPI
    ```console
    pipx install virtme-ng
    ```
    or install from source
    ```console
    $ git clone https://github.com/arighi/virtme-ng.git
    $ cd virtme-ng
    $ BUILD_VIRTME_NG_INIT=1 pip3 install .
    ```
    or from your distro package manager
    Example:
    ```console
    sudo apt install virtme-ng
    ```

## Building the Kernel


























































































































