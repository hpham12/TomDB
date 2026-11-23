# Development setup

## Build and test in Docker

The [README quickstart](../README.md#quickstart) builds TomDB in the Compose service. The service uses `hpham99/tomdb:latest` on `linux/amd64` and mounts the checkout at `/workspace`. The quickstart keeps its build directory inside the container. [Compose configuration](../docker-compose.yaml)

Run CTest from the repository root after the build completes. Use a disposable database for tests because some fixtures write to `/.tomdb/`.

```sh
docker compose exec tomdb ctest --test-dir /tmp/tomdb-build --output-on-failure
```

## Build outside Docker

Use CMake 3.20 or newer, a C/C++ toolchain with C++20 and `std::format` support, and `clang-tidy` on `PATH`. CMake downloads GoogleTest and nlohmann/json during configuration. [CMake configuration](../CMakeLists.txt)

```sh
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --parallel
ctest --test-dir cmake-build-debug --output-on-failure
```

Running the CLI and tests that use default metadata requires write access to `/.tomdb/`. The CLI uses that absolute path regardless of its working directory.

## CLion on Apple Silicon

Open **Settings → Build, Execution, Deployment**.

1. Add your Docker connection under **Docker**. [Connection settings](../assets/local-setup/step1.png)
2. Add a Docker toolchain using `hpham99/tomdb:latest` and select **Rosetta GDB**. [Toolchain settings](../assets/local-setup/step2.png)
3. Select the Docker toolchain in a **Debug** CMake profile. [CMake profile](../assets/local-setup/step3.png)
4. To profile with Valgrind, configure it under **Dynamic Analysis Tools** and launch the target with the Valgrind profiler. [Valgrind settings](../assets/local-setup/step4.png) · [Profiler example](../assets/local-setup/run-valgrind.png)
