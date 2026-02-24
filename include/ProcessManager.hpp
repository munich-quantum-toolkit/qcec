/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#pragma once

#include "EquivalenceCriterion.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define QCEC_WINDOWS
#include <windows.h>
#else
#define QCEC_POSIX
#include <sys/types.h>
#endif

namespace ec {

/// @brief Result from a process or thread execution
struct ProcessResult {
  std::size_t id;                   ///< Process/thread identifier
  EquivalenceCriterion equivalence; ///< Result of the equivalence check
  bool completed;                   ///< True if process completed normally
  bool timedOut;                    ///< True if killed due to timeout
  bool hasException;                ///< True if an exception was thrown
  int exceptionCode;                ///< Exception type code (see ExceptionType)
};

/// @brief Exception type codes for communication across process boundaries
/// @details These codes enable serializing exception information through IPC
enum class ExceptionType : int {
  None = 0,
  InvalidArgument = 1,
  RuntimeError = 2,
  LogicError = 3,
  Other = 99
};

/**
 * @brief Manages execution of equivalence checkers in isolated
 * processes/threads
 *
 * @details This class provides a platform-abstracted interface for running
 * equivalence checking tasks in parallel with hard termination support.
 *
 * **Platform Support:**
 * - **POSIX (Linux/macOS):** Uses `fork()` for true process isolation with
 *   `SIGKILL` for immediate termination
 * - **Windows:** Falls back to `std::thread` with cooperative cancellation
 *   (no true process isolation on Windows due to lack of `fork()`)
 *
 * **Usage Pattern:**
 * 1. Call `spawn()` to start tasks
 * 2. Call `waitForAny()` to retrieve results with timeout support
 * 3. Call `terminateAll()` to forcefully stop remaining tasks
 *
 * @note Exception information is propagated from child to parent through IPC
 */
class ProcessManager {
public:
  ProcessManager() = default;
  ~ProcessManager();

  // Delete copy constructor and assignment
  ProcessManager(const ProcessManager&) = delete;
  ProcessManager& operator=(const ProcessManager&) = delete;

  // Allow move
  ProcessManager(ProcessManager&&) noexcept = default;
  ProcessManager& operator=(ProcessManager&&) noexcept = default;

  /**
   * @brief Spawn a new process/thread to execute a task
   * @param id Unique identifier for this task
   * @param task Function returning EquivalenceCriterion to execute
   * @return true if spawned successfully, false on error
   *
   * @details On POSIX systems, `task` executes in a forked child process with
   * copy-on-write memory. On Windows, it runs in a new thread with shared
   * memory.
   *
   * @warning Task should not modify shared state or capture by reference unless
   * the behavior is well-defined for the platform's memory model.
   */
  bool spawn(std::size_t id, const std::function<EquivalenceCriterion()>& task);

  /**
   * @brief Wait for any task to complete with optional timeout
   * @param timeout Maximum wait duration (zero = wait indefinitely)
   * @return ProcessResult on completion, std::nullopt on timeout
   *
   * @details This method blocks until a task completes or the timeout expires.
   * Results include exception information if the task threw.
   */
  std::optional<ProcessResult>
  waitForAny(std::chrono::duration<double> timeout =
                 std::chrono::duration<double>::zero());

  /**
   * @brief Forcefully terminate all running tasks
   * @details POSIX: Sends SIGTERM then SIGKILL. Windows: Cooperative
   * cancellation.
   */
  void terminateAll();

  /// @brief Check if any tasks are still running
  [[nodiscard]] bool hasRunningProcesses() const noexcept {
    return !processes.empty();
  }

  /// @brief Get the number of currently running tasks
  [[nodiscard]] std::size_t numRunningProcesses() const noexcept {
    return processes.size();
  }

private:
#ifdef QCEC_POSIX
  struct ProcessInfo {
    std::size_t id;
    pid_t pid;
    int pipeFd; // Read end of pipe for receiving result
  };
#else // QCEC_WINDOWS
  struct ProcessInfo {
    std::size_t id;
    HANDLE processHandle;
    HANDLE pipeHandle; // Read end of pipe for receiving result
  };
#endif

  std::vector<ProcessInfo> processes;

  void removeProcess(std::size_t idx);

#ifdef QCEC_POSIX
  void killProcess(pid_t pid);
  std::optional<EquivalenceCriterion> readResult(int fd,
                                                 ExceptionType& exceptionType);
  bool writeResult(int fd, EquivalenceCriterion result,
                   ExceptionType exceptionType = ExceptionType::None);
#else // QCEC_WINDOWS
  void killProcess(HANDLE processHandle);
  std::optional<EquivalenceCriterion> readResult(HANDLE pipeHandle,
                                                 ExceptionType& exceptionType);
  bool writeResult(HANDLE pipeHandle, EquivalenceCriterion result,
                   ExceptionType exceptionType = ExceptionType::None);
#endif
};

} // namespace ec
