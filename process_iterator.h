// Copyright (c) 2021 Kasper Laudrup

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the “Software”), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#ifdef _WIN32
# include <windows.h>
# include <psapi.h>
# include <tlhelp32.h>
# include <array>
#else
# include <sys/types.h>
#endif

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>

class process_iterator {
public:
  class entry {
#ifdef _WIN32
    using pid_type = DWORD;
#else
    using pid_type = pid_t;
#endif

  public:
    entry() = default;

    pid_type pid() const;
    std::filesystem::path exe() const;
    std::filesystem::path exe(std::error_code& ec) const noexcept;

    template<class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const entry& e) {
      return os << e.pid();
    }

  private:
    friend class process_iterator;
    friend bool operator==(const process_iterator&, const process_iterator&);
    friend bool operator!=(const process_iterator&, const process_iterator&);

#ifdef _WIN32
    friend process_iterator begin(process_iterator) noexcept;

    std::shared_ptr<PROCESSENTRY32> proc_entry_;
#else
    entry(std::filesystem::directory_iterator it)
      : dir_it_(it) {
    }

    std::filesystem::directory_iterator dir_it_;
#endif
  };

  using iterator_category = std::input_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = entry;
  using pointer = const value_type*;
  using reference = const value_type&;

  process_iterator() = default;

  reference operator*() const {
    return val_;
  }

  pointer operator->() const {
    return &val_;
  }

  const process_iterator& operator++();
  process_iterator operator++(int);
  friend bool operator==(const process_iterator&, const process_iterator&);
  friend bool operator!=(const process_iterator&, const process_iterator&);

#ifdef __GNUC__
  // Workaround "default member initializer for
  // 'process_iterator::val_' required before the end of its enclosing
  // class" bug in GCC/Clang
  static process_iterator begin(process_iterator = process_iterator{{}}) noexcept;
  static process_iterator end(process_iterator = process_iterator{{}}) noexcept;
#else
  static process_iterator begin(process_iterator = process_iterator{}) noexcept;
  static process_iterator end(process_iterator = process_iterator{}) noexcept;
#endif

private:
#ifdef _WIN32
  std::shared_ptr<std::remove_pointer_t<HANDLE>> snapshot_;
#else
  process_iterator(std::filesystem::directory_iterator it)
    : val_(it) {
  }

  void skip_non_proc_entries() {
    while (++val_.dir_it_ != std::filesystem::directory_iterator{}) {
      if (!val_.dir_it_->is_directory()) {
        continue;
      }
      const auto dir_name  = std::prev(val_.dir_it_->path().end())->string();
      if (std::all_of(dir_name.begin(), dir_name.end(), [](const auto c) {
        return c >= '0' && c <= '9';
      })) {
        break;
      }
    }
  }
#endif

  value_type val_{};
};

#ifdef _WIN32
//
// Windows implementation
//
process_iterator::entry::pid_type process_iterator::entry::pid() const {
  return proc_entry_->th32ProcessID;
}

std::filesystem::path process_iterator::entry::exe() const {
  std::error_code ec;
  const auto p = exe(ec);
  if (ec) {
    throw std::system_error(ec);
  }
  return p;
}

std::filesystem::path process_iterator::entry::exe(std::error_code& ec) const noexcept {
  const auto flags = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
  std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)> handle{OpenProcess(flags, FALSE, proc_entry_->th32ProcessID), CloseHandle};
  if (!handle) {
    ec = {static_cast<int>(GetLastError()), std::system_category()};
    return {};
  }
  std::array<TCHAR, MAX_PATH> filename;
  const auto size = GetModuleFileNameEx(handle.get(), nullptr, filename.data(), static_cast<DWORD>(filename.size()));
  if (size == 0) {
    ec = {static_cast<int>(GetLastError()), std::system_category()};
    return {};
  }
  ec.clear();
  return filename.data();
}

const process_iterator& process_iterator::operator++() {
  if (!Process32Next(snapshot_.get(), val_.proc_entry_.get())) {
    val_.proc_entry_.reset();
  }
  return *this;
}

process_iterator process_iterator::operator++(int count) {
  std::advance(*this, count);
  return *this;
}

bool operator==(const process_iterator& a, const process_iterator& b) {
  if (a.val_.proc_entry_ && b.val_.proc_entry_) {
    return a.val_.proc_entry_->th32ProcessID == b.val_.proc_entry_->th32ProcessID;
  } else {
    return a.val_.proc_entry_ == b.val_.proc_entry_;
  }
}

bool operator!=(const process_iterator& a, const process_iterator& b) {
  return !(a == b);
}

process_iterator process_iterator::begin(process_iterator it) noexcept {
  it.snapshot_ = std::shared_ptr<std::remove_pointer_t<HANDLE>>(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0), CloseHandle);
  it.val_.proc_entry_ = std::make_shared<PROCESSENTRY32>();
  it.val_.proc_entry_->dwSize = sizeof(PROCESSENTRY32);
  if (!Process32First(it.snapshot_.get(), it.val_.proc_entry_.get())) {
    return process_iterator{};
  }
  return it;
}

process_iterator process_iterator::end(process_iterator) noexcept {
  return process_iterator{};
}

#else
//
// Un*x implementation
//
process_iterator::entry::pid_type process_iterator::entry::pid() const {
  const auto dir_name  = std::prev(dir_it_->path().end())->string();
  return std::stoi(dir_name);
}

std::filesystem::path process_iterator::entry::exe() const {
  return std::filesystem::read_symlink(dir_it_->path() / "exe");
}

std::filesystem::path process_iterator::entry::exe(std::error_code& ec) const noexcept {
  return std::filesystem::read_symlink(dir_it_->path() / "exe", ec);
}

const process_iterator& process_iterator::operator++() {
  skip_non_proc_entries();
  return *this;
}

process_iterator process_iterator::operator++(int count) {
  std::advance(val_.dir_it_, count);
  return *this;
}

bool operator==(const process_iterator& a, const process_iterator& b) {
  return a.val_.dir_it_ == b.val_.dir_it_;
}

bool operator!=(const process_iterator& a, const process_iterator& b) {
  return !(a == b);
}

process_iterator process_iterator::begin(process_iterator) noexcept {
  process_iterator it{std::filesystem::directory_iterator{"/proc"}};
  it.skip_non_proc_entries();
  return it;
}

process_iterator process_iterator::end(process_iterator) noexcept {
  return process_iterator{};
}

#endif
