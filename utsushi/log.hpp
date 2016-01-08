//  log.hpp -- formatted messages based on priority and category
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_log_hpp_
#define utsushi_log_hpp_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <boost/throw_exception.hpp>

#include "format.hpp"
#include "thread.hpp"

#ifndef UTSUSHI_LOG_ARGUMENT_COUNT_CHECK_ENABLED
#define UTSUSHI_LOG_ARGUMENT_COUNT_CHECK_ENABLED true
#endif

namespace utsushi {

class log
{
  inline static bool make_noise (int level, int cat = ALL)
  {
    return (threshold >= level && matching & cat);
  }

public:
  template <typename charT, typename traits = std::char_traits<charT> >
  class basic_logger
  {
  public:
    static std::basic_ostream<charT, traits>& os_;
  };

  //!  Formatted, self-outputting log messages
  /*!  Modeled after boost::format, this class provides a convenient,
   *   yet fast, mechanism to add log message support to your code.
   *
   *   \note
   *   An approach based on I/O manipulators has been considered and
   *   rejected.  Such an approach can't short-circuit the work done
   *   by boost::format::operator%() for any messages that are to be
   *   suppressed based on log::threshold and log::matching (because
   *   operator%() has higher precedence than operator<<() has).
   */
  template <typename charT, typename traits = std::char_traits<charT>,
            typename Alloc = std::allocator<charT> >
  class basic_message
  {
  public:
    typedef boost::basic_format<charT, traits, Alloc> format_type;
  private:
    boost::optional<boost::posix_time::ptime> timestamp_;
    boost::optional<thread::id>               thread_id_;
    boost::optional<format_type>              fmt_;
    int arg_;
    int cnt_;
    mutable bool dumped_;

    void clear_exception_bits ()
    {
      if (!arg_count_checking && fmt_)
        fmt_->exceptions (fmt_->exceptions ()
                          ^ (boost::io::too_many_args_bit
                             | boost::io::too_few_args_bit));
    }

    template <typename type>
    void init_args (const type& fmt, int lvl, int cat = ALL)
    {
      if (make_noise (lvl, cat))
        {
          timestamp_ = boost::posix_time::microsec_clock::local_time ();
          thread_id_ = this_thread::get_id ();
          fmt_ = format_type (fmt);
          cnt_ = fmt_->num_args_;
          clear_exception_bits ();
        }
      else
        {
          cnt_ = format_type (fmt).num_args_;
        }
    }

  public:
    typedef charT char_type;
    typedef std::basic_string<charT, traits, Alloc> string_type;

    basic_message ()
      : arg_(0), cnt_(0), dumped_(false)
    {}

    //  Convenience define to cut down on copy-and-paste
#define expand_ctor(type)                                               \
    basic_message (int lvl, const type& fmt)                            \
      : arg_(0), dumped_(false)                                         \
    { init_args (fmt, lvl); }                                           \
    basic_message (int lvl, const int& cat, const type& fmt)            \
      : arg_(0), dumped_(false)                                         \
    { init_args (fmt, lvl, cat); }                                      \
    basic_message (const type& fmt)                                     \
      : timestamp_(boost::posix_time::microsec_clock::local_time ())    \
      , thread_id_(this_thread::get_id ())                              \
      , fmt_(fmt), arg_(0), cnt_(fmt_->num_args_), dumped_(false)       \
    { clear_exception_bits (); }                                        \
    /**/

    //  Declare constructors for variously typed format specs
    expand_ctor (format_type);
    expand_ctor (string_type);
    expand_ctor (char_type *);

    //  Overloads that only perform argument count checking
    basic_message (const format_type& fmt, bool)
      : arg_(fmt.cur_arg_), cnt_(fmt.num_args_), dumped_(false)
    {}
    basic_message (const string_type& fmt, bool)
      : arg_(0), cnt_(format_type (fmt).num_args_), dumped_(false)
    {}
    basic_message (const char_type *& fmt, bool)
      : arg_(0), cnt_(format_type (fmt).num_args_), dumped_(false)
    {}

#undef expand_ctor

    ~basic_message ()
    {
      if (arg_ < cnt_) {
        if (log::arg_count_checking) {
          log::error ("log::message::too_few_args: %1% < %2%") % arg_ % cnt_;
        }
        for (int i = arg_; i < cnt_; /**/) {
          std::basic_ostringstream <charT, traits> os;
          os << "%" << ++i << "%";
          *this % os.str ();
        }
      }
      basic_logger<charT, traits>::os_ << *this;
    }

    //!  Feeds the argument \a t to a message
    template <typename T> basic_message& operator% (const T& t)
    {
      if (dumped_) arg_ = 0;
      ++arg_;
      if (fmt_) {
        *fmt_ % t;
      } else {
        if (arg_count_checking && arg_ > cnt_) {
          BOOST_THROW_EXCEPTION (boost::io::too_many_args (arg_, cnt_));
        }
      }
      return *this;
    }

    operator string_type () const
    {
      string_type rv;

      if (fmt_) {
        std::basic_ostringstream <charT, traits> os;
        os << *timestamp_ << "[" << *thread_id_ << "]: " << *fmt_
           << std::endl;
        rv = os.str ();
      }
      else if (log::arg_count_checking && arg_ < cnt_) {
        BOOST_THROW_EXCEPTION (boost::io::too_few_args (arg_, cnt_));
      }
      dumped_ = true;
      return rv;
    }
  };

  typedef enum {
    FATAL,                      //!<  famous last words
    ALERT,                      //!<  outside intervention required
    ERROR,                      //!<  something went wrong
    BRIEF,                      //!<  short informational notes
    TRACE,                      //!<  more chattery feedback
    DEBUG                       //!<  the gory details
    ,
    QUARK = TRACE,              //!<  stack tracing feedback
  } priority;

  typedef enum {
    NOTHING,
    SANE_BACKEND = 1 << 0,
    ALL = ~0
  } category;

  static const bool
  arg_count_checking = UTSUSHI_LOG_ARGUMENT_COUNT_CHECK_ENABLED;

  //!  The priority at and above which messages may be logged
  static priority threshold;
  //!  The category specification for which messages will be logged
  /*!  Only messages in matching categories are considered for output.
   */
  static category matching;

  //!  Convenience type for character log messages
  typedef basic_message<char>    message;
  //!  Convenience type for wide character log messages
  typedef basic_message<wchar_t> wmessage;

  //!  Prioritized log messages
  /*!  Constructing a log message formatter requires passing of an
   *   explicit priority.  There is nothing wrong with that but it
   *   leads to verbose code like this
   *
   *     \code
   *     log::message (log::ERROR, "error message");
   *     \endcode
   *
   *   Using the prioritised named constructors allows for more
   *   concise code
   *
   *     \code
   *     log::error ("error message");
   *     \endcode
   */
#define expand_convenience_ctors(ctor,level,type,fmt_type)      \
  inline static type                                            \
  ctor (const type::fmt_type& fmt)                              \
  { return ctor (ALL, fmt); }                                   \
  inline static type                                            \
  ctor (const log::category& cat, const type::fmt_type& fmt)    \
  {                                                             \
    if (!arg_count_checking)                                    \
      return (make_noise (level, cat) ? type (fmt) : type ());  \
                                                                \
    return (make_noise (level, cat)                             \
            ? type (fmt) : type (fmt, arg_count_checking));     \
  }                                                             \
  /**/

#define expand_named_format_ctors(ctor,level,type)              \
  expand_convenience_ctors (ctor, level, type, format_type)     \
  expand_convenience_ctors (ctor, level, type, string_type)     \
  expand_convenience_ctors (ctor, level, type, char_type *)     \
  /**/

#define expand_named_ctors(ctor,level)                          \
  expand_named_format_ctors (ctor, log::level, log::message)    \
  expand_named_format_ctors (ctor, log::level, log::wmessage)   \
  /**/

  expand_named_ctors (fatal, FATAL);
  expand_named_ctors (alert, ALERT);
  expand_named_ctors (error, ERROR);
  expand_named_ctors (brief, BRIEF);
  expand_named_ctors (trace, TRACE);
  expand_named_ctors (debug, DEBUG);

#undef expand_named_ctors
#undef expand_named_format_ctors
#undef expand_convenience_ctors

  //! Conveniently trace scope entry and exit
  /*! During development or debugging you may want to see a trail of
   *  which (function) scopes are entered and exited in what order.
   *  Strategically placed log::quark() instantiations let you.  So
   *  you would normally write code like
   *
   *    \code
   *    void do_something ()
   *    {
   *      log::quark ();
   *      // ...
   *    }
   *    \endcode
   *
   *  The log::quark creates two log::messages, one at construction
   *  time and another one when it is destroyed.  The messages will
   *  include the source file name and line number that instatiated
   *  the log::quark as well as the encompassing function's name.
   *
   *  All you have to do is (re)compile the code of interest with \c
   *  ENABLE_LOG_QUARK defined to a true value and run your programs
   *  with the log::priority set to a value of log::QUARK or higher.
   *
   *  When \c ENABLE_LOG_QUARK is set to false at compile time, your
   *  log::quark() calls expand to something that any self-respecting
   *  compiler ought to optimize away unless explicitly told not to.
   */
  class quark_impl
  {
    const char *file_;
    int         line_;
    const char *func_;
    const char *fmt_;
  public:
    quark_impl (const char *file, int line, const char *func)
      : file_(file), line_(line), func_(func)
      , fmt_("%1%:%2%: %3% %4%")
    { log::message (log::QUARK, fmt_) % file_ % line_ % "entered" % func_; }
    ~quark_impl ()
    { log::message (log::QUARK, fmt_) % file_ % line_ % "exiting" % func_; }
  };

  static void quark_noop () {}
};

#ifndef ENABLE_LOG_QUARK
#define ENABLE_LOG_QUARK false
#endif

#if ENABLE_LOG_QUARK
#define quark() quark_impl (__FILE__, __LINE__, __func__)
#else
#define quark() quark_noop ()
#endif

//! Outputs a formatted log message to a stream
template <typename charT, typename traits, typename Alloc>
std::basic_ostream<charT, traits>&
operator<< (std::basic_ostream<charT, traits>& os,
            const log::basic_message<charT, traits, Alloc>& msg)
{
  os << typename log::basic_message<charT, traits, Alloc>::string_type (msg);
  return os;
}

}       // namespace utsushi

#endif  /* utsushi_log_hpp_ */
