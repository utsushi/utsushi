//  shell-pipe.hpp -- outsource filtering to a command-line utility
//  Copyright (C) 2014  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
//
//  This file is part of the 'Utsushi' package.
//  This package is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License or, at
//  your option, any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//  You ought to have received a copy of the GNU General Public License
//  along with this package.  If not, see <http://www.gnu.org/licenses/>.


#ifndef filters_shell_pipe_hpp_
#define filters_shell_pipe_hpp_

#include <utsushi/filter.hpp>

#include <string>

namespace utsushi {
namespace _flt_ {

class shell_pipe
  : public ofilter
{
public:
  ~shell_pipe ();

  streamsize write (const octet *data, streamsize n);

protected:
  shell_pipe (const std::string& command);

  void boi (const context& ctx);
  void eoi (const context& ctx);
  void eof (const context& ctx);

  virtual context estimate (const context& ctx) const;
  virtual context finalize (const context& ctx) const;

  virtual std::string arguments (const context& ctx);

private:
  void execute_(const std::string& command_line);

  streamsize service_pipes_(const octet *data, streamsize n);

  std::string command_;
  std::string message_;
  pid_t       process_;

  int i_pipe_;
  int o_pipe_;
  int e_pipe_;

  octet  *buffer_;
  ssize_t buffer_size_;
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* !defined (filters_shell_pipe_hpp_) */
