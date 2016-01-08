//  reorient.hpp -- images to make text face the right way up
//  Copyright (C) 2015  SEIKO EPSON CORPORATION
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

#ifndef filters_reorient_hpp_
#define filters_reorient_hpp_

#include "shell-pipe.hpp"

#include <deque>

namespace utsushi {
namespace _flt_ {

struct bucket;

class reorient
  : public shell_pipe
{
public:
  reorient ();

  streamsize write (const octet *data, streamsize n);

  void mark (traits::int_type c, const context& ctx);

protected:
  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);
  void eos (const context& ctx);
  void eof (const context& ctx);

  void freeze_options ();

  context estimate (const context& ctx);
  context finalize (const context& ctx);

  std::string arguments (const context& ctx);

  void checked_write (octet *data, streamsize n);

private:
  typedef shell_pipe base;

  value       reorient_;
  std::string engine_;

  std::deque< shared_ptr< bucket > > pool_;
  std::string report_;
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_reorient_hpp_ */
