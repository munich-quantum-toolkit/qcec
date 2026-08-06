/*
 * Copyright (c) 2023 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "ProcessManager.hpp"

#include <algorithm>
#include <iostream>

#ifdef QCEC_POSIX
#include <csignal>
#include <cstring>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#endif

namespace ec {

namespace {
/// @brief Categorize and capture exception information for IPC
[[nodiscard]] ExceptionType categorizeException() noexcept {
  try {
    throw; // Re-throw current exception to inspect it
  } catch (const std::invalid_argument&) {
    return ExceptionType::InvalidArgument;
  } catch (const std::runtime_error&) {
    return ExceptionType::RuntimeError;
  } catch (const std::logic_error&) {
    return ExceptionType::LogicError;
  } catch (...) {
    return ExceptionType::Other;
  }
}
} // namespace

ProcessManager::~ProcessManager() { terminateAll(); }

#ifdef QCEC_POSIX
// ============================================================================
// POSIX Implementation (Linux, macOS, Unix)
// ============================================================================

bool ProcessManager::spawn(std::size_t id,
                           const std::function<EquivalenceCriterion()>& task) {
  int pipeFds[2];
  if (pipe(pipeFds) == -1) {
    std::cerr << "Failed to create pipe: " << std::strerror(errno) << '\n';
    return false;
  }

  const pid_t pid = fork();

  if (pid == -1) {
    std::cerr << "Failed to fork process: " << std::strerror(errno) << '\n';
    close(pipeFds[0]);
    close(pipeFds[1]);
    return false;
  }

  if (pid == 0) {
    // Child process
    close(pipeFds[0]); // Close read end

    auto result = EquivalenceCriterion::NoInformation;
    auto exceptionType = ExceptionType::None;

    try {
      result = task();
    } catch (...) {
      exceptionType = categorizeException();
    }

    // Write result and exception info to pipe
    writeResult(pipeFds[1], result, exceptionType);

    close(pipeFds[1]);
    _exit(0); // Use _exit to avoid flushing parent's buffers
  }

  // Parent process
  close(pipeFds[1]); // Close write end

  processes.push_back({id, pid, pipeFds[0]});
  return true;
}

std::optional<ProcessResult>
ProcessManager::waitForAny(std::chrono::duration<double> timeout) {
  if (processes.empty()) {
    return std::nullopt;
  }

  // Setup poll for all pipe file descriptors
  std::vector<pollfd> fds;
  fds.reserve(processes.size());
  for (const auto& proc : processes) {
    fds.push_back({proc.pipeFd, POLLIN, 0});
  }

  // Calculate timeout in milliseconds
  int timeoutMs = -1; // -1 means infinite
  if (timeout.count() > 0) {
    timeoutMs = static_cast<int>(timeout.count() * 1000);
  }

  const int pollResult = poll(fds.data(), fds.size(), timeoutMs);

  if (pollResult == -1) {
    std::cerr << "Poll failed: " << std::strerror(errno) << '\n';
    return std::nullopt;
  }

  if (pollResult == 0) {
    // Timeout occurred
    return std::nullopt;
  }

  // Find which process has data ready
  for (std::size_t i = 0; i < fds.size(); ++i) {
    if ((fds[i].revents & POLLIN) != 0) {
      const auto& proc = processes[i];

      // Try to read result
      ExceptionType exceptionType = ExceptionType::None;
      auto result = readResult(proc.pipeFd, exceptionType);

      // Wait for process to exit
      int status = 0;
      waitpid(proc.pid, &status, 0);

      ProcessResult procResult;
      procResult.id = proc.id;
      procResult.completed = result.has_value();
      procResult.timedOut = false;
      procResult.hasException = (exceptionType != ExceptionType::None);
      procResult.exceptionCode = static_cast<int>(exceptionType);

      if (result.has_value()) {
        procResult.equivalence = *result;
      } else {
        procResult.equivalence = EquivalenceCriterion::NoInformation;
      }

      // Clean up
      close(proc.pipeFd);
      removeProcess(i);

      return procResult;
    }

    // Check for errors
    if ((fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      const auto& proc = processes[i];

      // Wait for process
      int status = 0;
      waitpid(proc.pid, &status, 0);

      ProcessResult procResult;
      procResult.id = proc.id;
      procResult.completed = false;
      procResult.timedOut = false;
      procResult.hasException = false;
      procResult.exceptionCode = 0;
      procResult.equivalence = EquivalenceCriterion::NoInformation;

      // Clean up
      close(proc.pipeFd);
      removeProcess(i);

      return procResult;
    }
  }

  return std::nullopt;
}

void ProcessManager::terminateAll() {
  for (const auto& proc : processes) {
    killProcess(proc.pid);
    close(proc.pipeFd);
  }
  processes.clear();
}

void ProcessManager::killProcess(pid_t pid) {
  // First try SIGTERM for graceful shutdown
  kill(pid, SIGTERM);

  // Wait briefly
  int status = 0;
  const pid_t result = waitpid(pid, &status, WNOHANG);

  if (result == 0) {
    // Process still running, use SIGKILL
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
  }
}

std::optional<EquivalenceCriterion>
ProcessManager::readResult(int fd, ExceptionType& exceptionType) {
  struct {
    int equivalence;
    int exception;
  } data;

  const ssize_t bytesRead = read(fd, &data, sizeof(data));

  if (bytesRead != sizeof(data)) {
    exceptionType = ExceptionType::None;
    return std::nullopt;
  }

  exceptionType = static_cast<ExceptionType>(data.exception);
  return static_cast<EquivalenceCriterion>(data.equivalence);
}

bool ProcessManager::writeResult(int fd, EquivalenceCriterion result,
                                 ExceptionType exceptionType) {
  struct {
    int equivalence;
    int exception;
  } data;

  data.equivalence = static_cast<int>(result);
  data.exception = static_cast<int>(exceptionType);

  const ssize_t bytesWritten = write(fd, &data, sizeof(data));
  return bytesWritten == sizeof(data);
}

#else // QCEC_WINDOWS
// ============================================================================
// Windows Implementation - Thread-based fallback
// ============================================================================
// Note: Windows doesn't support fork(), so we use threads with cooperative
// cancellation. This is not as robust as process-based approach, but provides
// functional multi-tasking support on Windows.

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace {
struct ThreadData {
  std::thread thread;
  std::atomic<bool> finished{false};
  std::atomic<bool> shouldStop{false};
  EquivalenceCriterion result{EquivalenceCriterion::NoInformation};
  ExceptionType exceptionType{ExceptionType::None};
  std::mutex mutex;
  std::condition_variable cv;
};

std::map<std::size_t, std::unique_ptr<ThreadData>> threadMap;
std::mutex threadMapMutex;
} // namespace

bool ProcessManager::spawn(std::size_t id,
                           const std::function<EquivalenceCriterion()>& task) {
  auto threadData = std::make_unique<ThreadData>();
  auto* data = threadData.get();

  data->thread = std::thread([task, data]() {
    auto result = EquivalenceCriterion::NoInformation;
    auto exceptionType = ExceptionType::None;

    try {
      result = task();
    } catch (...) {
      exceptionType = categorizeException();
    }

    {
      const std::lock_guard lock(data->mutex);
      data->result = result;
      data->exceptionType = exceptionType;
      data->finished = true;
    }
    data->cv.notify_one();
  });

  {
    const std::lock_guard lock(threadMapMutex);
    threadMap[id] = std::move(threadData);
  }

  ProcessInfo info;
  info.id = id;
  info.processHandle = reinterpret_cast<HANDLE>(id); // Dummy handle
  info.pipeHandle = nullptr;
  processes.push_back(info);

  return true;
}

std::optional<ProcessResult>
ProcessManager::waitForAny(std::chrono::duration<double> timeout) {
  if (processes.empty()) {
    return std::nullopt;
  }

  const auto deadline = std::chrono::steady_clock::now() + timeout;
  const auto hasTimeout = timeout.count() > 0;

  // Poll threads for completion
  while (true) {
    {
      const std::lock_guard mapLock(threadMapMutex);

      for (std::size_t i = 0; i < processes.size(); ++i) {
        const auto& proc = processes[i];
        const auto it = threadMap.find(proc.id);
        if (it == threadMap.end()) {
          continue;
        }

        auto& threadData = it->second;
        std::unique_lock lock(threadData->mutex);

        if (threadData->finished) {
          ProcessResult result;
          result.id = proc.id;
          result.equivalence = threadData->result;
          result.completed = true;
          result.timedOut = false;
          result.hasException =
              (threadData->exceptionType != ExceptionType::None);
          result.exceptionCode = static_cast<int>(threadData->exceptionType);

          // Clean up thread
          lock.unlock();
          if (threadData->thread.joinable()) {
            threadData->thread.join();
          }
          threadMap.erase(it);
          removeProcess(i);

          return result;
        }
      }
    }

    // Check for timeout
    if (hasTimeout && std::chrono::steady_clock::now() >= deadline) {
      return std::nullopt;
    }

    // Brief sleep to avoid busy-waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void ProcessManager::terminateAll() {
  const std::lock_guard mapLock(threadMapMutex);

  // Signal all threads to stop
  for (auto& [id, threadData] : threadMap) {
    threadData->shouldStop = true;
  }

  // Wait for all threads to finish
  for (auto& [id, threadData] : threadMap) {
    if (threadData->thread.joinable()) {
      threadData->thread.join();
    }
  }

  threadMap.clear();
  processes.clear();
}

void ProcessManager::killProcess(HANDLE /*processHandle*/) {
  // Threads are signaled via shouldStop flag
  // They will terminate cooperatively
}

std::optional<EquivalenceCriterion>
ProcessManager::readResult(HANDLE /*pipeHandle*/,
                           ExceptionType& exceptionType) {
  // Not used in thread-based implementation
  exceptionType = ExceptionType::None;
  return std::nullopt;
}

bool ProcessManager::writeResult(HANDLE /*pipeHandle*/,
                                 EquivalenceCriterion /*result*/,
                                 ExceptionType /*exceptionType*/) {
  // Not used in thread-based implementation
  return false;
}

#endif // QCEC_WINDOWS

// ============================================================================
// Common Implementation (both platforms)
// ============================================================================

void ProcessManager::removeProcess(std::size_t idx) {
  if (idx < processes.size()) {
    processes.erase(processes.begin() + static_cast<std::ptrdiff_t>(idx));
  }
}

} // namespace ec
