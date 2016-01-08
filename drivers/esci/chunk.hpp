//  chunk.hpp -- ESC/I protocol chunks
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_chunk_hpp_
#define drivers_esci_chunk_hpp_

#include <stdexcept>

#include <boost/shared_array.hpp>
#include <boost/throw_exception.hpp>

#include "code-point.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    using std::logic_error;

    class chunk
    {
      boost::shared_array<byte> buffer_;
      streamsize buffer_size_;
      bool error_code_;

    public:
      chunk (streamsize size = 0, bool with_error_code = false)
        : buffer_()
        , buffer_size_(size)
        , error_code_(with_error_code)
      {
        if (0 < buffer_size_
            || error_code_)
          {
            buffer_ = boost::shared_array<byte>
              (new byte[ buffer_size_ + (error_code_ ? 1 : 0) ]);
          }
      }

      streamsize size (bool with_error_code = false) const
      {
        return buffer_size_ + ((with_error_code && error_code_) ? 1 : 0);
      }

      byte error_code () const
      {
        if (error_code_)
          {
            return buffer_[size (true) - 1];
          }

        BOOST_THROW_EXCEPTION (logic_error (""));
      }

      const byte * get () const
      {
        return buffer_.get ();
      }

      operator byte * ()
      {
        return buffer_.get ();
      }

      operator bool () const
      {
        return bool(buffer_);
      }
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_chunk_hpp_ */
