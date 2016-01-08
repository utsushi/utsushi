//  shell-pipe.hpp -- outsource filtering to a command-line utility
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
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
  : public filter
{
public:
  ~shell_pipe ();

  void mark (traits::int_type c, const context& ctx);
  streamsize write (const octet *data, streamsize n);

protected:
  shell_pipe (const std::string& command);

  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);
  void eos (const context& ctx);
  void eof (const context& ctx);

  virtual void freeze_options ();

  virtual context estimate (const context& ctx);
  virtual context finalize (const context& ctx);

  virtual std::string arguments (const context& ctx);

  virtual void checked_write (octet *data, streamsize n);

private:
  traits::int_type exec_process_(const context& ctx);
  traits::int_type reap_process_();

  streamsize service_pipes_(const octet *data, streamsize n);
  void handle_error_(int ec, int& fd);

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

#endif  /* filters_shell_pipe_hpp_ */
