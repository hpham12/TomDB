# TomDB
Tom DB is a relational database management system built for the purpose of learning about Database System implementation.
It should not be used in any production system.

## Table of Contents
- [Development Setup](#development-setup)
  - [MacOS - Apple Sillion](#macos---apple-silicon)
- [High-Level Architecture](#high-level-architecture)

## Development Setup

To avoid potential issues with different processor architectures or operating systems, development should be done
inside the provider Docker container built from Ubuntu 26.04 LTS based image. See https://hub.docker.com/r/hpham99/tomdb

This section guides you through setting up your local development setup with CLion.

First, navigate to the root directory of TomDB, then build the container:
```
docker compose up -d
```

### MacOS - Apple Silicon
Go to Settings > Build, Execution, Deployment > Docker, make the following setup:

   ![step1.png](assets/local-setup/step1.png)

Go to Toolchains, make the following setup:

   ![step2.png](assets/local-setup/step2.png)

Set debugger to **Rosetta GDB**

   ![step3.png](assets/local-setup/step3.png)

Setup Valgrind:

  ![step4.png](assets/local-setup/step4.png)

Then, you can run Valgrind using the profiler option

  ![run-valgrind.png](assets/local-setup/run-valgrind.png)

## High-Level Architecture!
![High-level-architecture.png](assets/high-level-arch.png)