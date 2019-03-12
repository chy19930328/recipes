#pragma once

#include <stdio.h>
#include <memory>
#include <string>
#include "muduo/base/Logging.h"

// Wrappers FILE* from stdio.
class File
{
 public:
  int64_t tell()
  {
    return ::ftell(file_);
  }

  void close()
  {
    if (file_)
      ::fclose(file_);
    file_ = nullptr;
    buffer_.reset();
  }

 protected:
  // https://github.com/coreutils/coreutils/blob/master/src/ioblksize.h
  /* As of May 2014, 128KiB is determined to be the minimium
   * blksize to best minimize system call overhead.
   */
  static const int kBufferSize = 1024 * 1024;

  File(const std::string& filename, const char* mode, int bufsize=kBufferSize)
    : file_(CHECK_NOTNULL(::fopen(filename.c_str(), mode))),
      buffer_(CHECK_NOTNULL(new char[bufsize]))
  {
    ::setbuffer(file_, buffer_.get(), bufsize);
  }

  virtual ~File()
  {
    close();
  }

 protected:
  FILE* file_ = nullptr;

 private:
  std::unique_ptr<char[]> buffer_;

  File(const File&) = delete;
  void operator=(const File&) = delete;
};

class InputFile : public File
{
 public:
  explicit InputFile(const char* filename, int bufsize=kBufferSize)
    : File(filename, "r", bufsize)
  {
  }

  bool getline(std::string* output)
  {
    char buf[1024] = "";
    if (::fgets(buf, sizeof buf, file_))
    {
      *output = buf;
      if (!output->empty() && output->back() == '\n')
      {
        output->resize(output->size()-1);
      }
      return true;
    }
    return false;
  }
};

class OutputFile : public File
{
 public:
  explicit OutputFile(const std::string& filename)
    : File(filename, "w")
  {
  }

  void write(std::string_view s)
  {
    ::fwrite(s.data(), 1, s.size(), file_);
  }

  void appendRecord(std::string_view s)
  {
    assert(s.size() < 255);
    uint8_t len = s.size();
    ::fwrite(&len, 1, sizeof len, file_);
    ::fwrite(s.data(), 1, len, file_);
    ++items_;
  }

  size_t items()
  {
    return items_;
  }

 private:
  size_t items_ = 0;
};

