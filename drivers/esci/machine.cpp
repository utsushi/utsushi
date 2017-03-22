//  machine.cpp -- ESC/I handshake aware state machine
//  Copyright (C) 2016  SEIKO EPSON CORPORATION
//
//  License: BSL-1.0
//  Author : EPSON AVASYS CORPORATION
//
//  This file is part of the 'Utsushi' package.
//  This file is distributed under the terms of the Boost Software
//  License, Version 1.0.
//
//  You ought to have received a copy of the Boost Software License
//  along along with this package.
//  If not, see <http://www.boost.org/LICENSE_1_0.txt>.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "machine.hpp"

#include "interpreter.hpp"
#include "usb.hpp"

#include <utsushi/cstdint.hpp>

#include <cassert>
#include <iostream>
#include <map>

#define require assert
#define promise assert

#if __cplusplus < 201103L
#define nullptr 0
#else
#endif

#define EOT     "\x04"
#define ACK     "\x06"
#define FF      "\x0c"
#define NAK     "\x15"
#define CAN     "\x18"
#define PF      "\x19"
#define ESC     "\x1b"
#define FS      "\x1c"

extern int (*interpreter_reader_) (void *, int);
extern int (*interpreter_writer_) (void *, int);

struct machine::implementation
{
  implementation (const std::string& udi);
  ~implementation ();

  bool eof () const;

  std::string   reader () const;
  void          writer (const std::string& buf) const;

  usb_handle   *device;
  void        (*processor) (implementation *pimpl, const std::string& data);
  std::string (*responder) (implementation *pimpl);

  std::string   command;
  size_t        reply_size;
  unsigned      line_count;
  unsigned      block_count;
  unsigned      block_size;
  unsigned      last_block_size;

  std::string::value_type ec;
};

static inline uint16_t
to_uint16_t (const std::string s, size_t offset)
{
  typedef std::string::traits_type traits;
  return (traits::to_int_type (s[offset])
          | traits::to_int_type (s[offset+1]) << 8);
}

static inline uint32_t
to_uint32_t (const std::string s, size_t offset)
{
  return (to_uint16_t (s, offset) | to_uint16_t (s, offset+2) << 16);
}

static void process_command (machine::implementation *, const std::string&);

static std::string
respond_unsupported (machine::implementation *pimpl)
{
  pimpl->responder = nullptr;
  pimpl->processor = process_command;
  return NAK;
}

static std::string
respond_last_block (machine::implementation *pimpl)
{
  pimpl->responder = nullptr;
  pimpl->processor = process_command;
  return pimpl->reader ();
}

static std::string
respond_info_block (machine::implementation *pimpl)
{
  std::string rv (pimpl->reader ());

  pimpl->responder = respond_last_block;
  pimpl->reply_size = to_uint16_t (rv, 2);

  return rv;
}

static void
process_parameters (machine::implementation *pimpl, const std::string& parm)
{
  pimpl->processor = nullptr;
  pimpl->responder = respond_last_block;
  pimpl->reply_size = 1;

  // FIXME do not accept ESC C 0x01 and 0x11 because we do not support
  // the page sequence mode handshakes.

  pimpl->writer (parm);

  if (ESC "d" == pimpl->command)
    pimpl->line_count = parm[0];
}

static std::string
respond_get_parameters (machine::implementation *pimpl)
{
  std::string rv (pimpl->reader ());

  pimpl->responder = nullptr;
  pimpl->processor = (NAK != rv.substr (0,1)
                      ? process_parameters
                      : process_command);
  return rv;
}

static void
process_parameters2 (machine::implementation *pimpl, const std::string& parm)
{
  pimpl->responder = nullptr;
  pimpl->processor = process_parameters;

  pimpl->writer (parm);
}

static std::string
respond_get_parameters2 (machine::implementation *pimpl)
{
  std::string rv (pimpl->reader ());

  pimpl->responder = nullptr;
  pimpl->processor = (NAK != rv.substr (0,1)
                      ? process_parameters2
                      : process_command);
  return rv;
}

static void process_std_ack (machine::implementation *, const std::string&);
static std::string respond_get_std_image (machine::implementation *);

static std::string
respond_std_nak (machine::implementation *pimpl)
{
  pimpl->responder = nullptr;
  pimpl->processor = process_std_ack;
  return NAK;
}

static std::string
respond_std_image_data (machine::implementation *pimpl)
{
  pimpl->responder = nullptr;
  pimpl->processor = process_std_ack;
  return pimpl->reader ();
}

static std::string
respond_get_std_image (machine::implementation *pimpl)
{
  pimpl->responder = respond_std_image_data;
  pimpl->reply_size = (pimpl->line_count ? 6 : 4);

  std::string rv (pimpl->reader ());

  if (pimpl->line_count)
    pimpl->reply_size = to_uint16_t (rv, 4);
  else
    pimpl->reply_size = 1;

  pimpl->reply_size *= to_uint16_t (rv, 2);

  return rv;
}

static void
process_std_ack (machine::implementation *pimpl, const std::string& reply)
{
  std::string rep (reply.substr (0,1));

  /**/ if (ACK == rep)
    pimpl->responder = respond_get_std_image;
  else if (CAN == rep)
    pimpl->responder = respond_last_block;
  else
    pimpl->responder = respond_std_nak;

  pimpl->processor = nullptr;
  pimpl->reply_size = 1;

  pimpl->writer (reply);
}

static void process_ext_ack (machine::implementation *, const std::string&);
static std::string respond_get_ext_image (machine::implementation *);

static std::string
respond_ext_nak (machine::implementation *pimpl)
{
  pimpl->responder = nullptr;
  pimpl->processor = process_ext_ack;
  return NAK;
}

static std::string
respond_ext_image_data (machine::implementation *pimpl)
{
  pimpl->responder = nullptr;
  if (pimpl->block_count)
    {
      pimpl->processor = process_ext_ack;
      pimpl->reply_size = pimpl->block_size;
      pimpl->block_count--;
    }
  else
    {
      pimpl->processor = process_command;
      pimpl->reply_size = pimpl->last_block_size;
    }
  std::string rv (pimpl->reader ());

  pimpl->ec = rv[rv.size () - 1];

  return rv;
}

static std::string
respond_get_ext_image (machine::implementation *pimpl)
{
  pimpl->responder = respond_ext_image_data;

  std::string rv (pimpl->reader ());

  pimpl->block_size      = to_uint32_t (rv,  2);
  pimpl->block_count     = to_uint32_t (rv,  6);
  pimpl->last_block_size = to_uint32_t (rv, 10);

  pimpl->block_size++;
  pimpl->last_block_size++;

  return rv;
}

static void
process_ext_ack (machine::implementation *pimpl, const std::string& reply)
{
  std::string rep (reply.substr (0,1));

  /**/ if (ACK == rep)
    pimpl->responder = respond_ext_image_data;
  else if (CAN == rep)
    pimpl->responder = respond_last_block;
  else if (EOT == rep && pimpl->ec & 0x20)
    pimpl->responder = respond_last_block;
  else
    pimpl->responder = respond_ext_nak;

  pimpl->processor = nullptr;
  pimpl->reply_size = 1;

  pimpl->writer (reply);
}

typedef std::map< std::string,
                  std::pair< std::string (*) (machine::implementation *),
                             size_t> > reply_map;
reply_map
initialize ()
{
  reply_map next;

  // This defines handshakes for all commands that are documented
  // in the Utsushi esci driver implementation.  Commands for the
  // ESC/I-2 protocol, FS X, FS Y are explicitly defined as *not*
  // supported.
  // Any commands not listed below are implicitly unsupported.

  next[ CAN     ] = std::make_pair (respond_last_block, 1);
  next[ EOT     ] = std::make_pair (respond_last_block, 1);
  next[ ESC "!" ] = std::make_pair (respond_info_block, 4);
  next[ ESC "(" ] = std::make_pair (respond_last_block, 1);
  next[ ESC ")" ] = std::make_pair (respond_last_block, 1);
  next[ ESC "@" ] = std::make_pair (respond_last_block, 1);
  next[ ESC "A" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "B" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "C" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "D" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "F" ] = std::make_pair (respond_last_block, 4);
  next[ ESC "G" ] = std::make_pair (respond_get_std_image, 4);
  next[ ESC "H" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "I" ] = std::make_pair (respond_info_block, 4);
  next[ ESC "K" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "L" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "M" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "N" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "P" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "Q" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "R" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "S" ] = std::make_pair (respond_info_block, 4);
  next[ ESC "Z" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "[" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "]" ] = std::make_pair (respond_last_block, 1);
  next[ ESC "b" ] = std::make_pair (respond_get_parameters2, 1);
  next[ ESC "d" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "e" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "f" ] = std::make_pair (respond_info_block, 4);
  next[ ESC "g" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "i" ] = std::make_pair (respond_info_block, 4);
  next[ ESC "m" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "p" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "q" ] = std::make_pair (respond_info_block, 4);
  next[ ESC "s" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "t" ] = std::make_pair (respond_get_parameters, 1);
  next[ ESC "w" ] = std::make_pair (respond_last_block, 1);
  next[ ESC "z" ] = std::make_pair (respond_get_parameters, 1);
  next[ FF      ] = std::make_pair (respond_last_block, 1);
  next[ FS "F"  ] = std::make_pair (respond_last_block, 16);
  next[ FS "G"  ] = std::make_pair (respond_get_ext_image, 14);
  next[ FS "I"  ] = std::make_pair (respond_last_block, 80);
  next[ FS "S"  ] = std::make_pair (respond_last_block, 64);
  next[ FS "W"  ] = std::make_pair (respond_get_parameters, 1);
  next[ FS "X"  ] = std::make_pair (respond_unsupported, 1);
  next[ FS "Y"  ] = std::make_pair (respond_unsupported, 1);
  next[ PF      ] = std::make_pair (respond_last_block, 1);

  // The list above is as per combined generic ESC/I specification(s).
  // Now override on a per interpreter basis.

  // FIXME this needs to be done by the interpreter library, somehow,
  // or by means of a configuration file.  We need a list of required
  // and/or supported commands.  Anything not on those list, should
  // respond_unsupported.  Anything on a *required* list that we do
  // not support is also pretty fatal, BTW.

  // The commands below are known to trigger errors from the gt-s650
  // interpreter.

  next[ ESC "[" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "]" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "(" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC ")" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "M" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "m" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "s" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "B" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "b" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "Q" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "L" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "K" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "H" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "P" ] = std::make_pair (respond_unsupported, 1);
  next[ ESC "p" ] = std::make_pair (respond_unsupported, 1);

  return next;
}

static void
process_command (machine::implementation *pimpl, const std::string& cmd)
{
  static reply_map next (initialize ());

  pimpl->command.clear ();
  pimpl->processor = nullptr;
  pimpl->responder = respond_unsupported;
  pimpl->reply_size = 0;

  reply_map::const_iterator entry = next.find (cmd);
  if (next.end () != entry)
    {
      pimpl->command    = cmd;
      pimpl->responder  = entry->second.first;
      pimpl->reply_size = entry->second.second;

      if (pimpl->responder != respond_unsupported)
        pimpl->writer (cmd);
    }
}

machine::implementation::implementation (const std::string& udi)
  : device (new usb_handle (udi))
  , processor (process_command)
  , responder (nullptr)
  , command ()
  , reply_size (0)
{}

machine::implementation::~implementation ()
{
  delete device;
}

bool
machine::implementation::eof () const
{
  return processor;
}

std::string
machine::implementation::reader () const
{
  std::string rv;
  rv.resize (reply_size);
  if (!interpreter_reader_(const_cast< char * > (rv.data ()), rv.size ()))
    {
      std::clog << __func__ << ": oops\n";
      // FIXME now what?
    }
  return rv;
}

void
machine::implementation::writer (const std::string& buf) const
{
  if (!interpreter_writer_(const_cast< char * > (buf.data ()), buf.size ()))
    {
      std::clog << __func__ << ": oops\n";
      // FIXME now what?
    }
}

machine::machine (const std::string& udi)
  : pimpl (new implementation (udi))
{}

machine::~machine ()
{
  delete pimpl;
}

bool
machine::eof () const
{
  return pimpl->eof ();
}

void
machine::process (const std::string& data)
{
  pimpl->processor (pimpl, data);
}

std::string
machine::respond ()
{
  return pimpl->responder (pimpl);
}
