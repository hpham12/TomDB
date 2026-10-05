# Development setup

To avoid potential issues with different processor architectures or operating systems, development should be done
inside the provided Docker container built from Ubuntu 26.04 LTS based image. See https://hub.docker.com/r/hpham99/tomdb

This section guides you through setting up your local development setup with CLion.

First, navigate to the root directory of TomDB, then start the container:
```
docker compose up -d
```

## macOS — Apple Silicon
Go to Settings > Build, Execution, Deployment > Docker, make the following setup:

   ![step1.png](../assets/local-setup/step1.png)

Go to Toolchains, make the following setup:

   ![step2.png](../assets/local-setup/step2.png)

Set debugger to **Rosetta GDB**

   ![step3.png](../assets/local-setup/step3.png)

Setup Valgrind:

  ![step4.png](../assets/local-setup/step4.png)

Then, you can run Valgrind using the profiler option

  ![run-valgrind.png](../assets/local-setup/run-valgrind.png)
