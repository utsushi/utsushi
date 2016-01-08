//  verify.cpp -- ESC/I protocol assumptions and specification compliance
//  Copyright (C) 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2012-2014  EPSON AVASYS CORPORATION
//
//  License: GPL-3.0+
//  Author : Olaf Meeuwissen
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/results_reporter.hpp>
#include <boost/test/unit_test.hpp>

#include <utsushi/connexion.hpp>
#include <utsushi/format.hpp>
#include <utsushi/functional.hpp>
#include <utsushi/log.hpp>
#include <utsushi/monitor.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/scanner.hpp>

#include "../../connexions/hexdump.hpp"

#include "compound.hpp"
#include "grammar.hpp"
#include "grammar-information.hpp"
#include "scanner-inquiry.hpp"
#include "verify.hpp"

namespace bpo = boost::program_options;
namespace but = boost::unit_test;

using namespace utsushi;
using namespace _drv_::esci;

using std::basic_string;
using std::ios_base;
using std::make_pair;
using std::pair;
using std::vector;

// Instantiate program specific global variables

boost::program_options::variables_map verify::vm;

utsushi::connexion::ptr verify::cnx;

utsushi::_drv_::esci::information  verify::info;
utsushi::_drv_::esci::capabilities verify::caps;
utsushi::_drv_::esci::parameters   verify::parm;

boost::optional< utsushi::_drv_::esci::capabilities > verify::caps_flip;
boost::optional< utsushi::_drv_::esci::parameters   > verify::parm_flip;

//! Conveniently output selected device replies as is
class devdata_dumper
  : public compound< FS, UPPER_X >
{
public:
  devdata_dumper (const std::string& file)
    : base_type_(false)
    , fb_()
    , os_(&fb_)
  {
    namespace reply = code_token::reply;

    // Override base class hooks
    hook_[reply::INFO] = bind (&devdata_dumper::dump_hook_, this);
    hook_[reply::CAPA] = bind (&devdata_dumper::dump_hook_, this);
    hook_[reply::CAPB] = bind (&devdata_dumper::dump_hook_, this);
    hook_[reply::RESA] = bind (&devdata_dumper::dump_hook_, this);
    hook_[reply::RESB] = bind (&devdata_dumper::dump_hook_, this);

    if (!file.empty ())
      {
        fb_.open (file.c_str (), ios_base::binary | ios_base::out);
      }
    else
      {
        os_.rdbuf (std::cout.rdbuf ());
      }
  }

  virtual ~devdata_dumper ()
  {
    os_.flush ();
    if (fb_.is_open ()) fb_.close ();
  }

  using base_type_::finish;
  using base_type_::get_information;
  using base_type_::get_capabilities;
  using base_type_::get_parameters;

protected:
  void dump_hook_()
  {
    // The base_type_::decode_reply_block_() function destructively
    // parses the reply header.  Encode the reply_ part for dumping
    // and ignore the status_ part.

    hdr_blk_.clear ();
    encode_.header_(std::back_inserter (hdr_blk_), reply_);

    os_.write (hdr_blk_.data (), hdr_blk_.size ());
    os_.write (dat_blk_.data (), reply_.size);
  }

  std::basic_filebuf< byte > fb_;
  std::basic_ostream< byte > os_;
};

//! Convenience define to cut down on copy-and-paste
/*! The functions it generates can be used to check for inconsistencies
 *  between various bits of information related to a document source.
 */
#define document_source_chk_(src,src_,SRC)                              \
  static void                                                           \
  document_source_ ## src_()                                            \
  {                                                                     \
    if (verify::info.src)                                               \
      {                                                                 \
        BOOST_WARN (verify::info.src && verify::caps.src_);             \
        if (verify::caps_flip)                                          \
          BOOST_WARN (verify::info.src && verify::caps_flip->src_);     \
                                                                        \
        basic_string< byte > version (verify::info.version.begin (),    \
                                      verify::info.version.end ());     \
        BOOST_WARN_NE (basic_string< byte >::npos,                      \
                       version.find (SRC));                             \
        BOOST_WARN_EQUAL (0, version.find (SRC) % 4);                   \
      }                                                                 \
    else                                                                \
      {                                                                 \
        BOOST_WARN (!verify::info.src && !verify::caps.src_);           \
        if (verify::caps_flip)                                          \
          BOOST_WARN (!verify::info.src && !verify::caps_flip->src_);   \
      }                                                                 \
                                                                        \
    if (verify::parm.src_)                                              \
      {                                                                 \
        BOOST_WARN (verify::parm.src_ && verify::info.src);             \
        BOOST_WARN (verify::parm.src_ && verify::caps.src_);            \
        if (verify::caps_flip)                                          \
          BOOST_WARN (verify::parm.src_ && verify::caps_flip->src_);    \
        if (verify::parm_flip)                                          \
          BOOST_WARN (verify::parm.src_ && verify::parm_flip->src_);    \
      }                                                                 \
  }                                                                     \
  /**/

// Define consistency checkers for all document sources.

document_source_chk_(adf    , adf, "ADF ")
document_source_chk_(flatbed, fb , "FB  ")
document_source_chk_(tpu    , tpu, "TPU ")

#undef document_source_chk_

//! Check whether a gamma table is linear
static void
linear_gamma_table (const pair< parameters::gamma_table, bool >& data)
{
  parameters::gamma_table gamma = data.first;
  bool flip  = data.second;

  vector< byte > linear (256);
  vector< byte >::size_type i;

  for (i = 0; i < linear.size (); ++i)
    linear[i] = i;

  BOOST_WARN_MESSAGE (linear == gamma.table,
                      str (gamma.component) << " is not linear ("
                      << (flip ? "RESB" : "RESA") << ")");
  // Now show which elements are out of line
  BOOST_WARN_EQUAL_COLLECTIONS (linear.begin (), linear.end (),
                                gamma.table.begin (), gamma.table.end ());
}

//! Fine tune the set of tests to run
bool
init_test_runner ()
{
  but::master_test_suite_t& master (but::framework::master_test_suite ());

  // Always put the refspec compound protocol related members through
  // their paces.  Test cases in this suite shall only issue warnings
  // about things that appear to be inconsistent or incorrect.

  but::test_suite *refspec = BOOST_TEST_SUITE ("refspec");

  refspec->add (BOOST_TEST_CASE (&document_source_adf));
  refspec->add (BOOST_TEST_CASE (&document_source_fb ));
  refspec->add (BOOST_TEST_CASE (&document_source_tpu));

  vector< pair< parameters::gamma_table, bool > > tables;

  if (verify::parm.gmt)
    {
      BOOST_FOREACH (parameters::gamma_table table, *verify::parm.gmt)
        tables.push_back (make_pair (table, false));
    }
  if (verify::parm_flip && verify::parm_flip->gmt)
    {
      BOOST_FOREACH (parameters::gamma_table table, *verify::parm_flip->gmt)
        tables.push_back (make_pair (table, true));
    }

  refspec->add (BOOST_PARAM_TEST_CASE (linear_gamma_table,
                                       tables.begin (), tables.end ()));

  master.add (refspec);

  /*! \todo Configure which device dependent tests to run based on
   *        information in verify:: members
   */
  if (verify::vm.count ("no-test"))
    {
      master.remove (master.get ("protocol"));
      master.remove (master.get ("scanning"));
    }

  return true;
}

int
main (int argc, char *argv[])
{
  std::string devdata;
  std::string refspec;

  run_time (argc, argv);

  // Command-line processing

  bpo::options_description cmd_opts ("Options");
  cmd_opts
    .add_options ()
    ("help"   , "display this help and exit")
    ("hexdump",
     "log device I/O in hexdump format\n"
     "Data is sent to standard error and may be helpful when debugging"
     " test failures.")
    ("devdata", (bpo::value< std::string > (&devdata)
                 -> implicit_value (devdata, "filename")),
     "dump binary device data\n"
     "The dump is meant as a starting point for a known good reference"
     " for device specific protocol default data.  Reference data will"
     " be used to customize later protocol compliance testing.\n"
     "If no explicit filename is given, data will be sent to standard"
     " output and the program terminated once the dump is complete.")
    ("refspec", bpo::value< std::string > (&refspec),
     "use given file to obtain known good reference data\n"
     "If --devdata is given with a filename, that file name will be"
     " used.  If this option is not specified, the filename will be"
     " inferred from the device's product or firmware name.")
    ("no-test",
     "do not run any tests\n"
     "This can be used, for example, to stop the --devdata option from"
     " proceeding with the tests when a filename is given.")
    ;

  std::string udi;

  bpo::options_description pos_opts;
  pos_opts
    .add_options ()
    ("UDI", bpo::value< std::string > (&udi),
     "image acquistion device to use\n"
     "Defaults to the first driver supported device found.")
    ;
  bpo::positional_options_description pos_args;
  pos_args.add ("UDI", 1);

  bpo::options_description cmd_line;
  cmd_line
    .add (cmd_opts)
    .add (pos_opts)
    ;

  bpo::parsed_options cmd (bpo::command_line_parser (argc, argv)
                           .options (cmd_line)
                           .positional (pos_args)
                           .allow_unregistered ()
                           .run ());
  bpo::store (cmd, verify::vm);
  bpo::notify (verify::vm);

  if (verify::vm.count ("help"))
    {
      std::cout << "Usage: "
                << "verify [OPTIONS] [UDI]"
                << "\n\n"
                << cmd_opts
                << "\n"
                << "Boost.Test options are supported as well.\n"
                   "Unknown options are silently ignored.\n";
      return EXIT_SUCCESS;
    }

  // Connexion setup

  monitor mon;
  monitor::const_iterator it;

  if (!udi.empty ())
    {
      it = mon.find (udi);
      if (it != mon.end () && "esci" != it->driver ())
        {
          std::cerr << "driver mismatch"
                    << std::endl;
          return EXIT_FAILURE;
        }
    }
  else
    {
      do
        {
          it = std::find_if (mon.begin (), mon.end (),
                             bind (&scanner::info::is_driver_set, _1));
        }
      while (it != mon.end () && "esci" != it->driver ());
    }

  if (it != mon.end ())
    {
      verify::cnx = connexion::create (it->connexion (), it->path ());

      if (verify::vm.count ("hexdump"))
        {
          verify::cnx = make_shared< _cnx_::hexdump > (verify::cnx);
        }
    }
  else
    {
      std::string msg;

      if (!udi.empty ())
        {
          format fmt ("cannot find '%1%'");
          msg = (fmt % udi).str ();
        }
      else
        {
          msg = "no devices available";
        }
      std::cerr << "verify: " << msg
                << std::endl;
    }

  if (verify::vm.count ("devdata"))
    {
      if (verify::cnx)
        {
          devdata_dumper dd (devdata);

          *verify::cnx << dd.get_information ();
          *verify::cnx << dd.get_capabilities ();
          *verify::cnx << dd.get_capabilities (true);
          *verify::cnx << dd.get_parameters ();
          *verify::cnx << dd.get_parameters (true);

          if (devdata.empty ())
            return EXIT_SUCCESS;

          if (!verify::vm.count ("refspec"))
            refspec = devdata;
        }
      else
        {
          std::cerr << "verify: " << "unable to connect with device"
                    << std::endl;
          return EXIT_FAILURE;
        }
    }

  if (refspec.empty ())
    {
      if (verify::cnx)
        {
          scanner_inquiry cmd;
          information info;

          *verify::cnx << cmd.get (info);
          refspec  = "data/";
          refspec += info.product_name ();
          refspec += ".dat";
        }
      else
        {
          std::cerr << "verify: " << "unable to connect with device"
                    << std::endl;
          return EXIT_FAILURE;
        }
    }

  // Initialize verify:: members from known good reference data

  std::basic_ifstream< byte > fs (refspec.c_str (),
                                  ios_base::binary | ios_base::in);

  if (!fs.is_open ())
    {
      std::cerr << format ("cannot open %1%") % refspec
                << std::endl;
      return EXIT_FAILURE;
    }

  byte_buffer blk;
  header      hdr;

  decoding::grammar           gram;
  decoding::grammar::iterator head;
  decoding::grammar::iterator tail;

  try
    {
      while (std::basic_ifstream< byte >::traits_type::eof ()
             != fs.peek ())
        {
          const streamsize hdr_len = 12;
          blk.resize (hdr_len);
          fs.read (blk.data (), hdr_len);

          head = blk.begin ();
          tail = head + hdr_len;

          if (!gram.header_(head, tail, hdr))
            {
              log::error ("%1%") % gram.trace ();
              fs.close ();
              return EXIT_FAILURE;
            }

          log::trace ("%1%: %2% byte payload")
            % str (hdr.code)
            % hdr.size
            ;

          if (0 == hdr.size) continue;

          blk.resize (hdr.size);
          fs.read (blk.data (), hdr.size);

          head = blk.begin ();
          tail = head + hdr.size;

          bool result = false;
          namespace reply = code_token::reply;
          /**/ if (reply::INFO == hdr.code)
            {
              result = gram.information_(head, tail, verify::info);
            }
          else if (reply::CAPA == hdr.code)
            {
              result = gram.capabilities_(head, tail, verify::caps);
            }
          else if (reply::CAPB == hdr.code)
            {
              capabilities caps;
              result = gram.capabilities_(head, tail, caps);
              verify::caps_flip = caps;
            }
          else if (reply::RESA == hdr.code)
            {
              result = gram.scan_parameters_(head, tail, verify::parm);
            }
          else if (reply::RESB == hdr.code)
            {
              parameters parm;
              result = gram.scan_parameters_(head, tail, parm);
              verify::parm_flip = parm;
            }

          if (!result)
            {
              log::error ("%1%") % gram.trace ();
              fs.close ();
              return EXIT_FAILURE;
            }
        }
    }
  catch (decoding::grammar::expectation_failure& e)
    {
      std::cerr << "\n  " << e.what () << " @ "
                << std::hex << e.first - blk.begin ()
                << "\n  Looking at " << str (hdr.code)
                << "\n  Processing " << std::string (blk.begin (), blk.end ())
                << "\n  Expecting " << e.what_
                << "\n  Got: " << std::string (e.first, e.last);
      return EXIT_FAILURE;
    }

  // Remove the recognized options and arguments from the command
  // line, leaving the remainder for Boost.Test to process.  Note
  // that argv[0] needs to remain untouched.

  std::vector< std::string > more_opts
    (bpo::collect_unrecognized (cmd.options, bpo::exclude_positional));

  argc = 1 + more_opts.size ();
  for (size_t i = 0; i < more_opts.size (); ++i)
    argv[i + 1] = const_cast< char * > (more_opts[i].c_str ());

  // Start the test suite

  std::string test_module = "compound";

  but::framework::master_test_suite ().p_name.value = test_module;

  return but::unit_test_main (init_test_runner, argc, argv);
}
