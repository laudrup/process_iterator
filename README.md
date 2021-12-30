# process_iterator
C++17 iterator for listing the processes running on the system with an interface somewhat similar to [std::directory_iterator](https://en.cppreference.com/w/cpp/filesystem/directory_iterator).

## Requirements

A working C++ 17 compiler and either a Un*x like system with a proc directory or a relatively recent version of Windows. Only tested on Linux and on 64-bit Windows 10, but ought to work elsewhere. If that is not the case, please do let me know.

## Usage

For simplicity everything has been implemented in a single header file (**process_iterator.h**) so simply include that where needed.

## Examples

List the process identifier of all running processes:

```cpp
for(auto& entry: process_iterator()) {
  std::cout << entry << "\n";
}
```

Find a running process named firefox:
```cpp
auto it = std::find_if(process_iterator::begin(), process_iterator::end(), [](const auto& e) {
  std::error_code ec;
  const auto proc = e.exe(ec);
  if (ec) {
    // Ignore errors...
    return false;
  }
  return proc.stem() == "firefox";
});
```

## API

### process_iterator

#### Member types
| Member type | Definition |
| --- | --- |
| iterator_category	| [std::input_iterator_tag](https://en.cppreference.com/w/cpp/iterator/iterator_tags) |
| value_type | [process_iterator::entry](#process_iteratorentry) |
| difference_type	| [std::ptrdiff_t](https://en.cppreference.com/w/cpp/types/ptrdiff_t) |
| pointer	| const [process_iterator::entry*](#process_iteratorentry) |
| reference	| const [process_iterator::entry&](#process_iteratorentry) |

#### Member functions
| Member function | Return value | Description |
| --- | --- | --- |
| (constructor) |  | constructs a process iterator |
| (destructor) | | default destructor |
| operator* | const [process_iterator::entry&](#process_iteratorentry) | accesses the pointed-to entry |
| operator-> | const [process_iterator::entry*](#process_iteratorentry) | accesses the pointed-to entry |
| operator++ | const [process_iterator::entry&](#process_iteratorentry) | advances to the next entry |

#### Static member functions
| Function | Return value | Description |
| --- | --- | --- |
| begin(process_iterator) | [process_iterator](#process_iterator) | returns an iterator to the first process running on the system |
| end(process_iterator) | [process_iterator](#process_iterator) | returns a default constructed process iterator |

### process_iterator::entry

#### Member types
| Member type | Definition |
| --- | --- |
| pid_type	| the type of a process identifier |

#### Member functions
| Member function | Return value | Description |
| --- | --- | --- |
| (constructor) |  | constructs a directory iterator entry |
| (destructor) | | default destructor |
| pid() const | pid_type | returns the process identifier of the current entry |
| exe() const | [std::filesystem::path](https://en.cppreference.com/w/cpp/filesystem/directory_entry/path) | returns the path to the executable of the process entry. Throws [std::system_error](https://en.cppreference.com/w/cpp/error/system_error) on failure |
| exe(std::error_code& ec) const noexcept | [std::filesystem::path](https://en.cppreference.com/w/cpp/filesystem/directory_entry/path) | returns the path to the executable of the process entry |

#### Non-member functions
| Member function | Return value | Description |
| --- | --- | --- |
template<class CharT, class Traits> operator<<(std::basic_ostream<CharT, Traits>&, const entry&) | [std::basic_ostream<CharT, Traits>&](https://en.cppreference.com/w/cpp/io/basic_ostream) | writes the process identifier of the entry to the stream |

## License

<img align="right" src="https://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">

The code is licensed under the [MIT License](https://opensource.org/licenses/MIT):

Copyright &copy; 2021 [Kasper Laudrup](https://github.com/laudrup)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
