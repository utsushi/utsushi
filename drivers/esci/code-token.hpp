//  code-token.hpp -- set used by the ESC/I "compound" protocol variants
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_code_token_hpp_
#define drivers_esci_code_token_hpp_

#include <iomanip>
#include <locale>
#include <sstream>

#include <utsushi/cstdint.hpp>

#include "code-point.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

//! Protocol "tokens" in byte groups of four
/*! The "compound" variants of the ESC/I protocol got more ambitious
 *  and started using tokens that are almost human intelligible.  It
 *  groups constants from the code_point namespace in chunks of four
 *  and uses these when communicating with the device.
 *
 *  \sa code_token, CODE_TOKEN
 */
typedef uint32_t quad;

//! Conveniently "construct" code_token values
/*! The definition of quad as a native type makes it a bit awkward to
 *  assign values in a way that is convenient and easy to read.  This
 *  macro tries to remedy that a little.
 *
 *  The macro is implemented in big-endian fashion so that ASCII dumps
 *  of protocol traffic buffers will display tokens left-to-right.  If
 *  one were to use little-endian, tokens would be spelt backward.
 */
#define CODE_TOKEN(b1, b2, b3, b4)              \
  (  (0xff & b1) << 24                          \
   | (0xff & b2) << 16                          \
   | (0xff & b3) <<  8                          \
   | (0xff & b4))                               \
  /**/

//! Stringify protocol tokens
/*! The definition of quad as a native type makes them fairly hard to
 *  map back to the almost human intelligible tokens of the "compound"
 *  protocol variant when sent to any kind of output.  This function
 *  provides a concise way to produce something more readable.
 */
inline
std::string
str (const quad& q)
{
  using namespace std;

  byte b1 (0xff & (q >> 24));
  byte b2 (0xff & (q >> 16));
  byte b3 (0xff & (q >>  8));
  byte b4 (0xff & (q));

  ostringstream os;
  os.imbue (locale::classic ());
  os.fill ('0');

  if (isprint (b1, locale::classic ())
      && isprint (b2, locale::classic ())
      && isprint (b3, locale::classic ())
      && isprint (b4, locale::classic ()))
    {
      os << b1 << b2 << b3 << b4;
    }
  else
    {
      os << setw (10) << showbase << hex << q;
    }
  return os.str ();
}

//! Alias for Boost.Spirit provided parser/generator
/*! The Boost.Spirit library provides parsers and generators that deal
 *  with four-byte integral types in a variety of flavours.  Problem
 *  is, their names do not quite match our "token" vocabulary.  This
 *  preprocessor macro fixes that.  It uses a big-endian version to
 *  match our CODE_TOKEN() implementation.
 */
#define token_ boost::spirit::big_dword

//! Documented code tokens of the ESC/I "compound" protocol variants
/*! The ESC/I "compound" protocol variants use a large collection of
 *  tokens.  The code_token namespace defines symbolic constants for
 *  all known tokens.  The token definitions have been grouped into
 *  nested namespaces to indicate their context.  Namespace aliases
 *  and \c using declarations have been used where that made sense.
 *  Several empty namespaces serve mostly as placeholders to latch
 *  documentation onto.
 *
 *  Note that the same symbol may have different definitions in the
 *  different namespaces.  As a striking example, take status::ERR,
 *  reply::info::ERR and reply::info::err:ERR which are defined as
 *   \c "#ERR", \c "#err" and \c "ERR ", respectively.
 *
 *  \rationale
 *  Why bother with this nested namespacing scheme?  Why not use C
 *  style variable names like \c code_token_reply_info_err_ERR or
 *  static member variables in a \c code_token class?  Well, both
 *  approaches \e force one to use the fully qualified name, every
 *  time, everywhere.  With a nested namespacing approach, one has
 *  the option of shortening the name via a \c using declaration to
 *  the desired length for the duration of a scope.  So, a piece of
 *  code that interprets error conditions can say
 *
 *    \code
 *    using namespace code_token::reply::info::err;
 *
 *    if (where == ADF && what == PJ)
 *      log::alert (_("page feeder media jam"));
 *    if (where == TPU && what == LOCK)
 *      log::alert (_("photo unit carriage is locked"));
 *    \endcode
 *
 *  instead of
 *
 *    \code
 *    if (where == code_token::reply_info_err_ADF
 *        && what == code_token::reply_info_err_PJ)
 *      log::alert (_("page feeder media jam"));
 *    if (where == code_token::reply_info_err_TPU
 *        && what == code_token::reply_info_err_LOCK)
 *      log::alert (_("photo unit carriage is locked"));
 *    \endcode
 *
 *  which would result from a static member variable approach.
 *
 *  In code that deals with scan parameters, one can simply "import"
 *  the \c code_token::parameter namespace and write code that refers
 *  to \c adf::DPLX and \c tpu::IR or \c fmt::JPG rather than have to
 *  bother with those unwieldy long identifiers.
 */
namespace code_token {

//! Request code tokens
/*! The commands that are part of the "compound" protocol variant of
 *  the ESC/I protocol are request based.  One can think of a single
 *  request as a kind of sub-command.  Known requests are collected
 *  in this namespace.
 */
namespace request {

  const quad FIN  = CODE_TOKEN (UPPER_F, UPPER_I, UPPER_N, SPACE  );
  const quad CAN  = CODE_TOKEN (UPPER_C, UPPER_A, UPPER_N, SPACE  );
  const quad INFO = CODE_TOKEN (UPPER_I, UPPER_N, UPPER_F, UPPER_O);
  const quad CAPA = CODE_TOKEN (UPPER_C, UPPER_A, UPPER_P, UPPER_A);
  const quad CAPB = CODE_TOKEN (UPPER_C, UPPER_A, UPPER_P, UPPER_B);
  const quad PARA = CODE_TOKEN (UPPER_P, UPPER_A, UPPER_R, UPPER_A);
  const quad PARB = CODE_TOKEN (UPPER_P, UPPER_A, UPPER_R, UPPER_B);
  const quad RESA = CODE_TOKEN (UPPER_R, UPPER_E, UPPER_S, UPPER_A);
  const quad RESB = CODE_TOKEN (UPPER_R, UPPER_E, UPPER_S, UPPER_B);
  const quad STAT = CODE_TOKEN (UPPER_S, UPPER_T, UPPER_A, UPPER_T);
  const quad AFM  = CODE_TOKEN (UPPER_A, UPPER_F, UPPER_M, SPACE  );
  const quad MECH = CODE_TOKEN (UPPER_M, UPPER_E, UPPER_C, UPPER_H);
  const quad TRDT = CODE_TOKEN (UPPER_T, UPPER_R, UPPER_D, UPPER_T);
  const quad IMG  = CODE_TOKEN (UPPER_I, UPPER_M, UPPER_G, SPACE  );
  const quad EXT0 = CODE_TOKEN (UPPER_E, UPPER_X, UPPER_T, DIGIT_0);
  const quad EXT1 = CODE_TOKEN (UPPER_E, UPPER_X, UPPER_T, DIGIT_1);
  const quad EXT2 = CODE_TOKEN (UPPER_E, UPPER_X, UPPER_T, DIGIT_2);

}     // namespace request

//! Reply code tokens
/*! Every reply to a request starts with the request code when all is
 *  well.  If not all is well, the reply may indicate that the device
 *  does not know the request at all via a \c UNKN or that it received
 *  a known request at a bad time with a \c INVD.
 */
namespace reply {

  using namespace request;    // for the "all-is-well" replies

  const quad UNKN = CODE_TOKEN (UPPER_U, UPPER_N, UPPER_K, UPPER_N);
  const quad INVD = CODE_TOKEN (UPPER_I, UPPER_N, UPPER_V, UPPER_D);

  //! Reply info tokens
  /*! The bulk of each reply is made up of additional information that
   *  communicates various types of status.  These types are collected
   *  in a reply::info namespace with the corresponding type dependent
   *  tokens in a nested namespace named after the type token's name.
   *  That is, all the \c ERR type status tokens have been put in an
   *  \c err namespace within the reply::info namespace.
   *
   *  Each type token is followed by variable sized, token dependent
   *  information.  The details for each token are documented with the
   *  corresponding namespace.
   *
   *  Space to send the additional information is limited, at 52 bytes
   *  to be precise.  It may happen that this is not enough to include
   *  an \c END token.  This is not an error.  Whether token dependent
   *  data is subject to truncation is not documented.
   *
   *  Any tokens in a reply that are \c not defined at the top-level
   *  of this namespace should be ignored.  Parsing should restart at
   *  the first correctly aligned token that \e is defined at that
   *  level in this namespace.  How undefined token dependent data is
   *  to be treated is unspecified.
   *
   *  Each token has an associated priority.  From high to low, the
   *  ordering is as follows:
   *
   *   - \c ERR
   *   - \c NRD
   *   - \c PST
   *   - \c PEN
   *   - \c LFT
   *   - \c TYP
   *   - \c ATN
   *   - \c PAR
   *   - \c END
   *
   *  \note It is not clear whether the same type of information may
   *        repeat within a single reply.  Which type of information
   *        is always included is not documented either.  What to do
   *        with information of lesser priority in the face of higher
   *        priority information of a certain kind is undefined.
   */
  namespace info {

    const quad ERR  = CODE_TOKEN (NUMBER , LOWER_E, LOWER_R, LOWER_R);
    const quad NRD  = CODE_TOKEN (NUMBER , LOWER_N, LOWER_R, LOWER_D);
    const quad PST  = CODE_TOKEN (NUMBER , LOWER_P, LOWER_S, LOWER_T);
    const quad PEN  = CODE_TOKEN (NUMBER , LOWER_P, LOWER_E, LOWER_N);
    const quad LFT  = CODE_TOKEN (NUMBER , LOWER_L, LOWER_F, LOWER_T);
    const quad TYP  = CODE_TOKEN (NUMBER , LOWER_T, LOWER_Y, LOWER_P);
    const quad ATN  = CODE_TOKEN (NUMBER , LOWER_A, LOWER_T, LOWER_N);
    const quad PAR  = CODE_TOKEN (NUMBER , LOWER_P, LOWER_A, LOWER_R);
    const quad DOC  = CODE_TOKEN (NUMBER , LOWER_D, LOWER_O, LOWER_C);
    const quad END  = CODE_TOKEN (NUMBER , MINUS  , MINUS  , MINUS  );

    //! Hardware trouble indicators
    namespace err {

      // locations where trouble can occur
      const quad ADF  = CODE_TOKEN (UPPER_A, UPPER_D, UPPER_F, SPACE  );
      const quad TPU  = CODE_TOKEN (UPPER_T, UPPER_P, UPPER_U, SPACE  );
      const quad FB   = CODE_TOKEN (UPPER_F, UPPER_B, SPACE  , SPACE  );

      // kinds of trouble that may occur
      const quad OPN  = CODE_TOKEN (UPPER_O, UPPER_P, UPPER_N, SPACE  );
      const quad PJ   = CODE_TOKEN (UPPER_P, UPPER_J, SPACE  , SPACE  );
      const quad PE   = CODE_TOKEN (UPPER_P, UPPER_E, SPACE  , SPACE  );
      const quad ERR  = CODE_TOKEN (UPPER_E, UPPER_R, UPPER_R, SPACE  );
      const quad LTF  = CODE_TOKEN (UPPER_L, UPPER_T, UPPER_F, SPACE  );
      const quad LOCK = CODE_TOKEN (UPPER_L, UPPER_O, UPPER_C, UPPER_K);
      const quad DFED = CODE_TOKEN (UPPER_D, UPPER_F, UPPER_E, UPPER_D);
      const quad DTCL = CODE_TOKEN (UPPER_D, UPPER_T, UPPER_C, UPPER_L);
      const quad AUTH = CODE_TOKEN (UPPER_A, UPPER_U, UPPER_T, UPPER_H);
      const quad PERM = CODE_TOKEN (UPPER_P, UPPER_E, UPPER_R, UPPER_M);
      const quad BTLO = CODE_TOKEN (UPPER_B, UPPER_T, UPPER_L, UPPER_O);

    }   // namespace err

    //! Not quite ready indicators
    namespace nrd {

      const quad RSVD = CODE_TOKEN (UPPER_R, UPPER_S, UPPER_V, UPPER_D);
      const quad BUSY = CODE_TOKEN (UPPER_B, UPPER_U, UPPER_S, UPPER_Y);
      const quad WUP  = CODE_TOKEN (UPPER_W, UPPER_U, UPPER_D, SPACE  );
      const quad NONE = CODE_TOKEN (UPPER_N, UPPER_O, UPPER_N, UPPER_E);

    }   // namespace nrd

    //! Page start notification
    /*! This kind of notification does not use any prefdefined tokens.
     *  Additional information is given in the form of three integers.
     *  These integers correspond to the image width in pixels, number
     *  of trailing padding bytes per scanline and the image height in
     *  pixels, respectively.
     *
     *  Page start notification is included in the first reply after
     *  the device has detected this condition.  Typically, it will
     *  appear in the reply of an image's first \c IMG request.
     *
     *  \note The use of "page" is unfortunate.  The bulk of the API
     *        documentation tries to stick to the term "media".
     */
    namespace pst {}

    //! Page end notification
    /*! This kind of notification does not use any predefined tokens.
     *  It relays additional information by means of two integers.
     *  These integers correspond to the image width and height in
     *  pixels.
     *
     *  Page end notification is included in the first reply \e after
     *  the device has detected this condition.  It is never combined
     *  with page start notification.
     *
     *  \note The use of "page" is unfortunate.  The bulk of the API
     *        documentation tries to stick to the term "media".
     */
    namespace pen {}

    //! Images left to scan
    /*! The number of images left to scan is transferred as an integer
     *  so this kind of status information does not require any token
     *  definitions.  This information only makes sense if a specific
     *  number of images has been requested via the \c PAG parameter.
     *  The value reported includes the image currently being acquired
     *  and changes as soon as the last chunk of image data has been
     *  sent to the driver.  This means that the earliest one can see
     *  such a change is in the reply of the \e next request.
     *
     *  The information is always combined with page end notification.
     *  If a value of zero was set for the \c PAG parameter, it may be
     *  included after the last image data has been acquired and will
     *  always be zero.
     */
    namespace lft {}

    //! Media side being scanned
    /*! The media's flip-side is signalled by \c IMGB.
     *
     *  \note The terms "front" and "back" are strongly correlated to
     *        point of view.  The user perspective may not correspond
     *        to the device perspective, depending on the orientation
     *        of the originals on the device.  The API documentation
     *        prefers the term "flip-side" for the other side of the
     *        media and does not bother to define terminology for the
     *        flip-side's flip-side.
     */
    namespace typ {

      const quad IMGA = CODE_TOKEN (UPPER_I, UPPER_M, UPPER_G, UPPER_A);
      const quad IMGB = CODE_TOKEN (UPPER_I, UPPER_M, UPPER_G, UPPER_B);

    }   // namespace typ

    //! Device attention indicators
    namespace atn {

      const quad CAN  = CODE_TOKEN (UPPER_C, UPPER_A, UPPER_N, SPACE  );
      const quad NONE = CODE_TOKEN (UPPER_N, UPPER_O, UPPER_N, UPPER_E);

    }   // namespace atn

    //! Scan parameter related notifications
    /*! Information in this context is only expected for the \c PARA,
     *  \c PARB, \c RESA and \c RESB requests.
     */
    namespace par {

      const quad OK   = CODE_TOKEN (UPPER_O, UPPER_K, SPACE  , SPACE  );
      const quad FAIL = CODE_TOKEN (UPPER_F, UPPER_A, UPPER_I, UPPER_L);
      const quad LOST = CODE_TOKEN (UPPER_L, UPPER_O, UPPER_S, UPPER_T);

    }   // namespace par

    //! Document type notifications
    namespace doc {

      const quad CRST = CODE_TOKEN (UPPER_C, UPPER_R, UPPER_S, UPPER_T);

    }   // namespace doc

    //! When there's nothing left to say
    /*! Anything past the \c END marker can and should be ignored.
     */
    namespace end {}

  }     // namespace info

}     // namespace reply

//! Allowed value specifications
/*! Device information and capabilities include lists and ranges of
 *  the values that may be used for certain settings.  The allowed
 *  content of a \c LIST is not explicitly specified.  Examples in
 *  the documentation indicates that at least lists of integers and
 *  lists of tokens may be encountered.  There are no "mixed" value
 *  examples.
 *
 *  A \c RANG is naturally followed by two values.  Given that only
 *  integer coding schemes are supported by the protocol, these are
 *  always of an integral nature (although implicit conversions may
 *  alter that).  Whether different coding schemes for a \e single
 *  range are allowed is not clear.
 */
namespace value {

  const quad LIST = CODE_TOKEN (UPPER_L, UPPER_I, UPPER_S, UPPER_T);
  const quad RANG = CODE_TOKEN (UPPER_R, UPPER_A, UPPER_N, UPPER_G);

}     // namespace value_type

//! Saying when done with a device
namespace finish {}

//! Interrupting whatever a device is doing
namespace cancel {}

//! Discovering the basics of a device
/*! The device's characteristics are reported in reply to the \c INFO
 *  request.  This information is generally necessary in order to do
 *  the right thing in the driver implementation.  Some of it may be
 *  of interest to the user and some of it may interact with device
 *  capabilities.
 */
namespace information {

  const quad ADF  = CODE_TOKEN (NUMBER , UPPER_A, UPPER_D, UPPER_F);
  const quad TPU  = CODE_TOKEN (NUMBER , UPPER_T, UPPER_P, UPPER_U);
  const quad FB   = CODE_TOKEN (NUMBER , UPPER_F, UPPER_B, SPACE  );
  const quad IMX  = CODE_TOKEN (NUMBER , UPPER_I, UPPER_M, UPPER_X);
  const quad PB   = CODE_TOKEN (NUMBER , UPPER_P, UPPER_B, SPACE  );
  const quad PRD  = CODE_TOKEN (NUMBER , UPPER_P, UPPER_R, UPPER_D);
  const quad VER  = CODE_TOKEN (NUMBER , UPPER_V, UPPER_E, UPPER_R);
  const quad DSZ  = CODE_TOKEN (NUMBER , UPPER_D, UPPER_S, UPPER_Z);
  const quad EXT  = CODE_TOKEN (NUMBER , UPPER_E, UPPER_X, UPPER_T);
  const quad DLS  = CODE_TOKEN (NUMBER , UPPER_D, UPPER_L, UPPER_S);
  const quad S_N  = CODE_TOKEN (NUMBER , UPPER_S, SLASH  , UPPER_N);
  const quad ATH  = CODE_TOKEN (NUMBER , UPPER_A, UPPER_T, UPPER_H);
  const quad INI  = CODE_TOKEN (NUMBER , UPPER_I, UPPER_N, UPPER_I);
  const quad AFM  = CODE_TOKEN (NUMBER , UPPER_A, UPPER_F, UPPER_M);
  const quad DFM  = CODE_TOKEN (NUMBER , UPPER_D, UPPER_F, UPPER_M);
  const quad CRR  = CODE_TOKEN (NUMBER , UPPER_C, UPPER_R, UPPER_R);

  //! Automatic document feeder features
  namespace adf {

    // keys and flags
    const quad TYPE = CODE_TOKEN (UPPER_T, UPPER_Y, UPPER_P, UPPER_E);
    const quad DPLX = CODE_TOKEN (UPPER_D, UPPER_P, UPPER_L, UPPER_X);
    const quad FORD = CODE_TOKEN (UPPER_F, UPPER_O, UPPER_R, UPPER_D);
    const quad PREF = CODE_TOKEN (UPPER_P, UPPER_R, UPPER_E, UPPER_F);
    const quad DETX = CODE_TOKEN (UPPER_D, UPPER_E, UPPER_T, UPPER_X);
    const quad DETY = CODE_TOKEN (UPPER_D, UPPER_E, UPPER_T, UPPER_Y);
    const quad ALGN = CODE_TOKEN (UPPER_A, UPPER_L, UPPER_G, UPPER_N);
    const quad ASCN = CODE_TOKEN (UPPER_A, UPPER_S, UPPER_C, UPPER_N);
    const quad AREA = CODE_TOKEN (UPPER_A, UPPER_R, UPPER_E, UPPER_A);
    const quad AMIN = CODE_TOKEN (UPPER_A, UPPER_M, UPPER_I, UPPER_N);
    const quad AMAX = CODE_TOKEN (UPPER_A, UPPER_M, UPPER_A, UPPER_X);
    //! Document source dependent optical resolution
    /*! The optical resolution imposes an upper limit on the value
     *  that the main resolution for this document source can take.
     *  As such it affects both capability::RSM and parameter::RSM.
     *  It also caps the corresponding document source specific RSMS
     *  capability.  Note that it does not affect capability::RSS or
     *  parameter::RSS.
     */
    const quad RESO = CODE_TOKEN (UPPER_R, UPPER_E, UPPER_S, UPPER_O);
    const quad RCVR = CODE_TOKEN (UPPER_R, UPPER_C, UPPER_V, UPPER_R);
    const quad OVSN = CODE_TOKEN (UPPER_O, UPPER_V, UPPER_S, UPPER_N);
    const quad CRST = CODE_TOKEN (UPPER_C, UPPER_R, UPPER_S, UPPER_T);
    const quad CARD = CODE_TOKEN (UPPER_C, UPPER_A, UPPER_R, UPPER_D);

    // token values
    const quad PAGE = CODE_TOKEN (UPPER_P, UPPER_A, UPPER_G, UPPER_E);
    const quad FEED = CODE_TOKEN (UPPER_F, UPPER_E, UPPER_E, UPPER_D);
    const quad SCN1 = CODE_TOKEN (DIGIT_1, UPPER_S, UPPER_C, UPPER_N);
    const quad SCN2 = CODE_TOKEN (DIGIT_2, UPPER_S, UPPER_C, UPPER_N);
    const quad PF1N = CODE_TOKEN (UPPER_P, UPPER_F, DIGIT_1, UPPER_N);
    const quad PFN1 = CODE_TOKEN (UPPER_P, UPPER_F, UPPER_N, DIGIT_1);
    const quad LEFT = CODE_TOKEN (UPPER_L, UPPER_E, UPPER_F, UPPER_T);
    const quad CNTR = CODE_TOKEN (UPPER_C, UPPER_N, UPPER_T, UPPER_R);
    const quad RIGT = CODE_TOKEN (UPPER_R, UPPER_I, UPPER_G, UPPER_T);

  }   // namespace adf

  //! Transparency unit characteristics
  namespace tpu {

    // keys and flags
    const quad ARE1 = CODE_TOKEN (UPPER_A, UPPER_R, UPPER_E, DIGIT_1);
    const quad ARE2 = CODE_TOKEN (UPPER_A, UPPER_R, UPPER_E, DIGIT_2);
    //! \copydoc adf::RESO
    const quad RESO = CODE_TOKEN (UPPER_R, UPPER_E, UPPER_S, UPPER_O);
    const quad OVSN = CODE_TOKEN (UPPER_O, UPPER_V, UPPER_S, UPPER_N);

  }   // namespace tpu

  //! Flatbed traits
  namespace fb {

    // keys and flags
    const quad DETX = CODE_TOKEN (UPPER_D, UPPER_E, UPPER_T, UPPER_X);
    const quad DETY = CODE_TOKEN (UPPER_D, UPPER_E, UPPER_T, UPPER_Y);
    const quad ALGN = CODE_TOKEN (UPPER_A, UPPER_L, UPPER_G, UPPER_N);
    const quad AREA = CODE_TOKEN (UPPER_A, UPPER_R, UPPER_E, UPPER_A);
    //! \copydoc adf::RESO
    const quad RESO = CODE_TOKEN (UPPER_R, UPPER_E, UPPER_S, UPPER_O);
    const quad OVSN = CODE_TOKEN (UPPER_O, UPPER_V, UPPER_S, UPPER_N);

    // token values
    const quad LEFT = CODE_TOKEN (UPPER_L, UPPER_E, UPPER_F, UPPER_T);
    const quad CNTR = CODE_TOKEN (UPPER_C, UPPER_N, UPPER_T, UPPER_R);
    const quad RIGT = CODE_TOKEN (UPPER_R, UPPER_I, UPPER_G, UPPER_T);

  }   // namespace fb

  //! Maximum image pixel dimensions
  namespace imx {}

  //! Push button support
  namespace pb  {}

  //! Product name information
  namespace prd {}

  //! Firmware version facts
  namespace ver {}

  //! Maximum data buffer size the device can handle
  namespace dsz {}

  //! Places where blobs can be obtained or sent
  namespace ext {

    // token values
    const quad EXT0 = CODE_TOKEN (UPPER_E, UPPER_X, UPPER_T, DIGIT_0);
    const quad EXT1 = CODE_TOKEN (UPPER_E, UPPER_X, UPPER_T, DIGIT_1);
    const quad EXT2 = CODE_TOKEN (UPPER_E, UPPER_X, UPPER_T, DIGIT_2);

  }   // namespace ext

}     // namespace information

//! Finding out what a device can and cannot do
/*! Device capabilities determine the choices that can be made for the
 *  scan parameters.  Where the \c CAPA request reports capabilities
 *  that apply to both sides, the \c CAPB request only deals with the
 *  flip-side capabilities.
 */
namespace capability {

  const quad ADF  = CODE_TOKEN (NUMBER , UPPER_A, UPPER_D, UPPER_F);
  const quad TPU  = CODE_TOKEN (NUMBER , UPPER_T, UPPER_P, UPPER_U);
  const quad FB   = CODE_TOKEN (NUMBER , UPPER_F, UPPER_B, SPACE  );
  const quad COL  = CODE_TOKEN (NUMBER , UPPER_C, UPPER_O, UPPER_L);
  const quad FMT  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_M, UPPER_T);
  const quad JPG  = CODE_TOKEN (NUMBER , UPPER_J, UPPER_P, UPPER_G);
  const quad THR  = CODE_TOKEN (NUMBER , UPPER_T, UPPER_H, UPPER_R);
  const quad DTH  = CODE_TOKEN (NUMBER , UPPER_D, UPPER_T, UPPER_H);
  const quad GMM  = CODE_TOKEN (NUMBER , UPPER_G, UPPER_M, UPPER_M);
  const quad GMT  = CODE_TOKEN (NUMBER , UPPER_G, UPPER_M, UPPER_T);
  const quad CMX  = CODE_TOKEN (NUMBER , UPPER_C, UPPER_M, UPPER_X);
  const quad SFL  = CODE_TOKEN (NUMBER , UPPER_S, UPPER_F, UPPER_L);
  const quad MRR  = CODE_TOKEN (NUMBER , UPPER_M, UPPER_R, UPPER_R);
  const quad BSZ  = CODE_TOKEN (NUMBER , UPPER_B, UPPER_S, UPPER_Z);
  const quad PAG  = CODE_TOKEN (NUMBER , UPPER_P, UPPER_A, UPPER_G);
  //! Supported resolutions in the main (horizontal) direction
  /*! The resolution in the main or horizontal direction can take any
   *  of the values within the range or in the list reported by this
   *  capability, up to a maximum determined by the selected document
   *  source's optical resolution.  As different document sources may
   *  use different optical resolutions, the effective capability can
   *  change at run-time.
   *
   *  Let's suppose the RSM capability reports a range from 50dpi to
   *  1200dpi.  Let's further suppose that the flatbed has an optical
   *  resolution of 1200dpi and the ADF of 600dpi.  When the flatbed
   *  is selected, the supported resolutions become 50dpi through
   *  1200dpi.  However, when the ADF is selected as the document
   *  source, only resolutions in the 50dpi through 600dpi range are
   *  supported.
   *
   *  \sa information::adf::RESO, information::tpu::RESO,
   *      information::fb:RESO
   */
  const quad RSM  = CODE_TOKEN (NUMBER , UPPER_R, UPPER_S, UPPER_M);
  //! Supported resolutions in the sub (vertical) direction
  /*! The resolution in the sub or vertical direction can take any of
   *  the values reported by this capability.
   *
   *  Unlike RSM, the RSS capability is not subject to the selected
   *  document source's optical resolution.
   */
  const quad RSS  = CODE_TOKEN (NUMBER , UPPER_R, UPPER_S, UPPER_S);
  const quad CRP  = CODE_TOKEN (NUMBER , UPPER_C, UPPER_R, UPPER_P);
  const quad FCS  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_C, UPPER_S);
  const quad FLC  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_L, UPPER_C);
  const quad FLA  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_L, UPPER_A);
  const quad QIT  = CODE_TOKEN (NUMBER , UPPER_Q, UPPER_I, UPPER_T);
  const quad LAM  = CODE_TOKEN (NUMBER , UPPER_L, UPPER_A, UPPER_M);

  //! Automatic document feeder options
  /*! Note that the \c LOAD and \c EJCT tokens correspond to the same
   *  tokens from the \c MECH request.  They should \e not be used in
   *  a scan parameter context.
   */
  namespace adf {

    const quad DPLX = CODE_TOKEN (UPPER_D, UPPER_P, UPPER_L, UPPER_X);
    const quad PEDT = CODE_TOKEN (UPPER_P, UPPER_E, UPPER_D, UPPER_T);
    const quad DFL1 = CODE_TOKEN (UPPER_D, UPPER_F, UPPER_L, DIGIT_1);
    const quad DFL2 = CODE_TOKEN (UPPER_D, UPPER_F, UPPER_L, DIGIT_2);
    const quad LDF  = CODE_TOKEN (UPPER_L, UPPER_D, UPPER_F, SPACE  );
    const quad FAST = CODE_TOKEN (UPPER_F, UPPER_A, UPPER_S, UPPER_T);
    const quad SLOW = CODE_TOKEN (UPPER_S, UPPER_L, UPPER_O, UPPER_W);
    const quad BGWH = CODE_TOKEN (UPPER_B, UPPER_G, UPPER_W, UPPER_H);
    const quad BGBK = CODE_TOKEN (UPPER_B, UPPER_G, UPPER_B, UPPER_K);
    const quad BGGY = CODE_TOKEN (UPPER_B, UPPER_G, UPPER_G, UPPER_Y);
    const quad LOAD = CODE_TOKEN (UPPER_L, UPPER_O, UPPER_A, UPPER_D);
    const quad EJCT = CODE_TOKEN (UPPER_E, UPPER_J, UPPER_C, UPPER_T);
    const quad CRP  = CODE_TOKEN (UPPER_C, UPPER_R, UPPER_P, SPACE  );
    const quad SKEW = CODE_TOKEN (UPPER_S, UPPER_K, UPPER_E, UPPER_W);
    const quad OVSN = CODE_TOKEN (UPPER_O, UPPER_V, UPPER_S, UPPER_N);
    const quad CLEN = CODE_TOKEN (UPPER_C, UPPER_L, UPPER_E, UPPER_N);
    const quad CALB = CODE_TOKEN (UPPER_C, UPPER_A, UPPER_L, UPPER_B);
    //! Document source dependent recommended resolutions
    /*! Primarily meant for software that cannot handle imagery with
     *  resolutions that differ in the main and sub directions, this
     *  capability is currently not exposed by the driver.
     *
     *  When using resolutions based on this capability both main and
     *  sub resolutions need to be set to the same value.
     *
     *  Note that the list or range reported here should be a proper
     *  subset of the main \e and sub resolution capabilities.  This
     *  capability is also subject to the document source's optical
     *  resolution so all values reported should be less or at most
     *  equal to this optical resolution.
     *
     *  \sa information::adf::RESO,
     *      capability::adf::RSM, capability::adf::RSS
     */
    const quad RSMS = CODE_TOKEN (UPPER_R, UPPER_S, UPPER_M, UPPER_S);

  }   // namespace adf

  //! Transparency unit options
  namespace tpu {

    const quad ARE1 = CODE_TOKEN (UPPER_A, UPPER_R, UPPER_E, DIGIT_1);
    const quad ARE2 = CODE_TOKEN (UPPER_A, UPPER_R, UPPER_E, DIGIT_2);
    const quad NEGL = CODE_TOKEN (UPPER_N, UPPER_E, UPPER_G, UPPER_L);
    const quad IR   = CODE_TOKEN (UPPER_I, UPPER_R, SPACE  , SPACE  );
    const quad MAGC = CODE_TOKEN (UPPER_M, UPPER_A, UPPER_G, UPPER_C);
    const quad FAST = CODE_TOKEN (UPPER_F, UPPER_A, UPPER_S, UPPER_T);
    const quad SLOW = CODE_TOKEN (UPPER_S, UPPER_L, UPPER_O, UPPER_W);
    const quad CRP  = CODE_TOKEN (UPPER_C, UPPER_R, UPPER_P, SPACE  );
    const quad SKEW = CODE_TOKEN (UPPER_S, UPPER_K, UPPER_E, UPPER_W);
    const quad OVSN = CODE_TOKEN (UPPER_O, UPPER_V, UPPER_S, UPPER_N);
    //! \copydoc adf::RSMS
    const quad RSMS = CODE_TOKEN (UPPER_R, UPPER_S, UPPER_M, UPPER_S);

  }   // namespace tpu

  //! Flatbed options
  namespace fb {

    const quad LMP1 = CODE_TOKEN (UPPER_L, UPPER_M, UPPER_P, DIGIT_1);
    const quad LMP2 = CODE_TOKEN (UPPER_L, UPPER_M, UPPER_P, DIGIT_2);
    const quad FAST = CODE_TOKEN (UPPER_F, UPPER_A, UPPER_S, UPPER_T);
    const quad SLOW = CODE_TOKEN (UPPER_S, UPPER_L, UPPER_O, UPPER_W);
    const quad CRP  = CODE_TOKEN (UPPER_C, UPPER_R, UPPER_P, SPACE  );
    const quad SKEW = CODE_TOKEN (UPPER_S, UPPER_K, UPPER_E, UPPER_W);
    const quad OVSN = CODE_TOKEN (UPPER_O, UPPER_V, UPPER_S, UPPER_N);
    //! \copydoc adf::RSMS
    const quad RSMS = CODE_TOKEN (UPPER_R, UPPER_S, UPPER_M, UPPER_S);

  }   // namespace fb

  //! Color space specifiers
  namespace col {

    const quad C003 = CODE_TOKEN (UPPER_C, DIGIT_0, DIGIT_0, DIGIT_3);
    const quad C024 = CODE_TOKEN (UPPER_C, DIGIT_0, DIGIT_2, DIGIT_4);
    const quad C048 = CODE_TOKEN (UPPER_C, DIGIT_0, DIGIT_4, DIGIT_8);
    const quad M001 = CODE_TOKEN (UPPER_M, DIGIT_0, DIGIT_0, DIGIT_1);
    const quad M008 = CODE_TOKEN (UPPER_M, DIGIT_0, DIGIT_0, DIGIT_8);
    const quad M016 = CODE_TOKEN (UPPER_M, DIGIT_0, DIGIT_1, DIGIT_6);
    const quad R001 = CODE_TOKEN (UPPER_R, DIGIT_0, DIGIT_0, DIGIT_1);
    const quad R008 = CODE_TOKEN (UPPER_R, DIGIT_0, DIGIT_0, DIGIT_8);
    const quad R016 = CODE_TOKEN (UPPER_R, DIGIT_0, DIGIT_1, DIGIT_6);
    const quad G001 = CODE_TOKEN (UPPER_G, DIGIT_0, DIGIT_0, DIGIT_1);
    const quad G008 = CODE_TOKEN (UPPER_G, DIGIT_0, DIGIT_0, DIGIT_8);
    const quad G016 = CODE_TOKEN (UPPER_G, DIGIT_0, DIGIT_1, DIGIT_6);
    const quad B001 = CODE_TOKEN (UPPER_B, DIGIT_0, DIGIT_0, DIGIT_1);
    const quad B008 = CODE_TOKEN (UPPER_B, DIGIT_0, DIGIT_0, DIGIT_8);
    const quad B016 = CODE_TOKEN (UPPER_B, DIGIT_0, DIGIT_1, DIGIT_6);

  }   // namespace col

  //! Image format specifiers
  /*! The \c RAW image format corresponds to left-to-right, pixel
   *  oriented scan lines.  For color scans, the color components
   *  are arranged in R-G-B order.
   *
   *  Byte packing for single bit color spaces and endianness for
   *  16-bit ones ought to correspond to the non-"compound" ESC/I
   *  variants.
   */
  namespace fmt {

    const quad RAW  = CODE_TOKEN (UPPER_R, UPPER_A, UPPER_W, SPACE  );
    const quad JPG  = CODE_TOKEN (UPPER_J, UPPER_P, UPPER_G, SPACE  );

  }   // namespace fmt

  //! JPEG compression quality values
  namespace jpg {}

  //! Thresholding values
  /*! Values only have an effect with single bit color spaces.
   */
  namespace thr {}

  //! Dither pattern specifiers
  /*! Values only have an effect with single bit color spaces.
   */
  namespace dth {

    const quad NONE = CODE_TOKEN (UPPER_N, UPPER_O, UPPER_N, UPPER_E);
    const quad MIDA = CODE_TOKEN (UPPER_M, UPPER_I, UPPER_D, UPPER_A);
    const quad MIDB = CODE_TOKEN (UPPER_M, UPPER_I, UPPER_D, UPPER_B);
    const quad MIDC = CODE_TOKEN (UPPER_M, UPPER_I, UPPER_D, UPPER_C);
    const quad DTHA = CODE_TOKEN (UPPER_D, UPPER_T, UPPER_H, UPPER_A);
    const quad DTHB = CODE_TOKEN (UPPER_D, UPPER_T, UPPER_H, UPPER_B);
    const quad DTHC = CODE_TOKEN (UPPER_D, UPPER_T, UPPER_H, UPPER_C);
    const quad DTHD = CODE_TOKEN (UPPER_D, UPPER_T, UPPER_H, UPPER_D);

  }   // namespace dth

  //! Predefined gamma tables
  namespace gmm {

    const quad UG10 = CODE_TOKEN (UPPER_U, UPPER_G, DIGIT_1, DIGIT_0);
    const quad UG18 = CODE_TOKEN (UPPER_U, UPPER_G, DIGIT_1, DIGIT_8);
    const quad UG22 = CODE_TOKEN (UPPER_U, UPPER_G, DIGIT_2, DIGIT_2);

  }   // namespace gmm

  //! Gamma table color component specifiers
  namespace gmt {

    const quad RED  = CODE_TOKEN (UPPER_R, UPPER_E, UPPER_D, SPACE  );
    const quad GRN  = CODE_TOKEN (UPPER_G, UPPER_R, UPPER_N, SPACE  );
    const quad BLU  = CODE_TOKEN (UPPER_B, UPPER_L, UPPER_U, SPACE  );
    const quad MONO = CODE_TOKEN (UPPER_M, UPPER_O, UPPER_N, UPPER_O);

  }   // namespace gmt

  //! Color matrix specifiers
  namespace cmx {

    const quad UNIT = CODE_TOKEN (UPPER_U, UPPER_N, UPPER_I, UPPER_T);
    const quad UM08 = CODE_TOKEN (UPPER_U, UPPER_M, DIGIT_0, DIGIT_8);
    const quad UM16 = CODE_TOKEN (UPPER_U, UPPER_M, DIGIT_1, DIGIT_6);

  }   // namespace cmx

  //! Sharpness specifiers
  namespace sfl {

    const quad SMT2 = CODE_TOKEN (UPPER_S, UPPER_M, UPPER_T, DIGIT_2);
    const quad SMT1 = CODE_TOKEN (UPPER_S, UPPER_M, UPPER_T, DIGIT_1);
    const quad NORM = CODE_TOKEN (UPPER_N, UPPER_O, UPPER_R, UPPER_M);
    const quad SHP1 = CODE_TOKEN (UPPER_S, UPPER_H, UPPER_P, DIGIT_1);
    const quad SHP2 = CODE_TOKEN (UPPER_S, UPPER_H, UPPER_P, DIGIT_2);

  }   // namespace sfl

  //! Supported mirror image settings
  namespace mrr {

    const quad ON   = CODE_TOKEN (UPPER_O, UPPER_N, SPACE  , SPACE  );
    const quad OFF  = CODE_TOKEN (UPPER_O, UPPER_F, UPPER_F, SPACE  );

  }   // namespace mrr

  //! Private protocol extension
  /*! The protocol proper does not include capabilities for the data
   *  buffer size setting but it is convenient implementation-wise to
   *  have access to some information on allowed settings through the
   *  same API as for the rest of the scan settings.
   *
   *  \sa parameter::bsz
   */
  namespace bsz {}

  //! Private protocol extension
  /*! The protocol proper does not include capabilities for the image
   *  count setting but it is convenient implementation-wise to have
   *  access to some information on allowed settings through the same
   *  API as for the rest of the scan settings.
   *
   *  \sa parameter::pag
   */
  namespace pag {}

  //! Usable resolutions in the main scan direction
  namespace rsm {}

  //! Available resolutions for the sub scan direction
  namespace rss {}

  //! Supported crop margins
  /*! Values only have an effect when the \c CRP parameter has been
   *  set for the scan source (one of \c ADF, \c TPU or \c FB) that
   *  is to be used.
   */
  namespace crp {}

  //! Focus capabilities
  /*! This capability is for use with the \c MECH request.
   */
  namespace fcs {

    const quad AUTO = CODE_TOKEN (UPPER_A, UPPER_U, UPPER_T, UPPER_O);

  }   // namespace fcs

  //! Known fill colors
  namespace flc {

    const quad WH   = CODE_TOKEN (UPPER_W, UPPER_H, SPACE  , SPACE  );
    const quad BK   = CODE_TOKEN (UPPER_B, UPPER_K, SPACE  , SPACE  );

  }   // namespace flc

  //! Fill area settings that can be used
  namespace fla {}

  //! Supported quiet mode settings
  namespace qit {

    const quad PREF = CODE_TOKEN (UPPER_P, UPPER_R, UPPER_E, UPPER_F);
    const quad ON   = CODE_TOKEN (UPPER_O, UPPER_N, SPACE  , SPACE  );
    const quad OFF  = CODE_TOKEN (UPPER_O, UPPER_F, UPPER_F, SPACE  );

  }   // namespace qit

  //! Supported laminated paper settings
  namespace lam {

    const quad ON   = CODE_TOKEN (UPPER_O, UPPER_N, SPACE  , SPACE  );
    const quad OFF  = CODE_TOKEN (UPPER_O, UPPER_F, UPPER_F, SPACE  );

  }   // namespace lam

}     // namespace capability

//! Setting and getting scan parameters
/*! Tokens in this namespace cater to the \c PARA, \c PARB, \c RESA
 *  and \c RESB requests.  Because the parameters that can be set or
 *  are set are logically restricted by a device's capabilities, a
 *  large part of the implementation simply imports corresponding
 *  namespaces from the capability namespace.
 *
 *  When trying to set unsupported parameter values, the previous
 *  value is kept \e unchanged.  Whether such attempts are flagged is
 *  not known.
 *
 *  As with the \c CAPA and \c CAPB request, the \c PARA and \c RESA
 *  requests apply to both sides of the medium whereas the \c PARB and
 *  \c RESB ones only cover the flip-side.
 *
 *  It appears that the \c CAPB request may be "optional" in some way.
 *  How that would manifest itself is unclear.  It could signal this
 *  via a reply::UNKN or simply not return any reply data.  Whatever
 *  the means, the \c PARB reply will contain reply::info::par::FAIL
 *  in that case.  The \c RESB reply will not return any data.
 */
namespace parameter {

  const quad ADF  = CODE_TOKEN (NUMBER , UPPER_A, UPPER_D, UPPER_F);
  const quad TPU  = CODE_TOKEN (NUMBER , UPPER_T, UPPER_P, UPPER_U);
  const quad FB   = CODE_TOKEN (NUMBER , UPPER_F, UPPER_B, SPACE  );
  const quad COL  = CODE_TOKEN (NUMBER , UPPER_C, UPPER_O, UPPER_L);
  const quad FMT  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_M, UPPER_T);
  const quad JPG  = CODE_TOKEN (NUMBER , UPPER_J, UPPER_P, UPPER_G);
  const quad THR  = CODE_TOKEN (NUMBER , UPPER_T, UPPER_H, UPPER_R);
  const quad DTH  = CODE_TOKEN (NUMBER , UPPER_D, UPPER_T, UPPER_H);
  const quad GMM  = CODE_TOKEN (NUMBER , UPPER_G, UPPER_M, UPPER_M);
  const quad GMT  = CODE_TOKEN (NUMBER , UPPER_G, UPPER_M, UPPER_T);
  const quad CMX  = CODE_TOKEN (NUMBER , UPPER_C, UPPER_M, UPPER_X);
  const quad SFL  = CODE_TOKEN (NUMBER , UPPER_S, UPPER_F, UPPER_L);
  const quad MRR  = CODE_TOKEN (NUMBER , UPPER_M, UPPER_R, UPPER_R);
  const quad BSZ  = CODE_TOKEN (NUMBER , UPPER_B, UPPER_S, UPPER_Z);
  const quad PAG  = CODE_TOKEN (NUMBER , UPPER_P, UPPER_A, UPPER_G);
  const quad RSM  = CODE_TOKEN (NUMBER , UPPER_R, UPPER_S, UPPER_M);
  const quad RSS  = CODE_TOKEN (NUMBER , UPPER_R, UPPER_S, UPPER_S);
  const quad CRP  = CODE_TOKEN (NUMBER , UPPER_C, UPPER_R, UPPER_P);
  const quad ACQ  = CODE_TOKEN (NUMBER , UPPER_A, UPPER_C, UPPER_Q);
  const quad FLC  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_L, UPPER_C);
  const quad FLA  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_L, UPPER_A);
  const quad QIT  = CODE_TOKEN (NUMBER , UPPER_Q, UPPER_I, UPPER_T);
  const quad LDF  = CODE_TOKEN (NUMBER , UPPER_L, UPPER_D, UPPER_F);
  const quad DFA  = CODE_TOKEN (NUMBER , UPPER_D, UPPER_F, UPPER_A);
  const quad LAM  = CODE_TOKEN (NUMBER , UPPER_L, UPPER_A, UPPER_M);

  namespace adf {
    using namespace capability::adf;
    const quad CARD = CODE_TOKEN (UPPER_C, UPPER_A, UPPER_R, UPPER_D);
  }
  namespace tpu = capability::tpu;
  namespace fb  = capability::fb ;
  namespace col = capability::col;
  namespace fmt = capability::fmt;
  namespace jpg = capability::jpg;
  namespace thr = capability::thr;
  namespace dth = capability::dth;
  namespace gmm = capability::gmm;
  namespace gmt = capability::gmt;
  namespace cmx = capability::cmx;
  namespace sfl = capability::sfl;
  namespace mrr = capability::mrr;

  //! Maximum data buffer size the driver is willing to accept
  /*! The default value is 65536 bytes (64 kib).
   */
  namespace bsz {}

  //! Number of images one wants to acquire
  /*! A value of 0 will acquire images until all the originals have
   *  been processed.  For duplex scans, the value should be even,
   *  indicating that it really refers to images and not "pages", and
   *  odd values should be incremented to the next even integer.  The
   *  device will do so if the driver does not.
   *
   *  If not set by the driver, a value of zero will be used.
   */
  namespace pag {}

  namespace rsm = capability::rsm;
  namespace rss = capability::rss;
  namespace crp = capability::crp;

  //! Area of the original that should be acquired
  /*! The area is given by offsets in the main and sub scan directions
   *  followed by extents in the main and sub scan directions.  Values
   *  are in pixels.  This mimicks the way scan area is specified for
   *  the non-"compound" ESC/I protocol variants.
   */
  namespace acq {}

  namespace flc = capability::flc;
  namespace fla = capability::fla;
  namespace qit = capability::qit;
  namespace lam = capability::lam;

}     // namespace parameter

//! Getting a device status update
/*! The \c STAT request reports what media size was detected where as
 *  well as focus and push button and document separation mode status.
 *  In addition, most of the reply::info::err data is included.
 */
namespace status {

  const quad PSZ  = CODE_TOKEN (NUMBER , UPPER_P, UPPER_S, UPPER_Z);
  const quad ERR  = CODE_TOKEN (NUMBER , UPPER_E, UPPER_R, UPPER_R);
  const quad FCS  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_C, UPPER_S);
  const quad PB   = CODE_TOKEN (NUMBER , UPPER_P, UPPER_B, SPACE  );
  const quad SEP  = CODE_TOKEN (NUMBER , UPPER_S, UPPER_E, UPPER_P);
  const quad BAT  = CODE_TOKEN (NUMBER , UPPER_B, UPPER_A, UPPER_T);

  //! Detected media sizes
  /*! The detectable media sizes expand on those from the media_value
   *  enumeration.  The A6 and B6 sizes have been added and Japanese
   *  postcards (PC, at 100mm x 148mm slightly smaller than A6), King
   *  (or 4R) size photos (KG, 4" x 6") and cheques (CK, 90mm x 225mm)
   *  are covered as well.
   *
   *  \note The B-series of media sizes follow the JIS standard.
   *
   *  \sa http://en.wikipedia.org/wiki/Paper_size
   */
  namespace psz {

    // keys
    const quad ADF  = CODE_TOKEN (UPPER_A, UPPER_D, UPPER_F, SPACE  );
    const quad FB   = CODE_TOKEN (UPPER_F, UPPER_B, SPACE  , SPACE  );

    // token values
    const quad A3V  = CODE_TOKEN (UPPER_A, DIGIT_3, UPPER_V, SPACE  );
    const quad WLT  = CODE_TOKEN (UPPER_W, UPPER_L, UPPER_T, SPACE  );
    const quad B4V  = CODE_TOKEN (UPPER_B, DIGIT_4, UPPER_V, SPACE  );
    const quad LGV  = CODE_TOKEN (UPPER_L, UPPER_G, UPPER_V, SPACE  );
    const quad A4V  = CODE_TOKEN (UPPER_A, DIGIT_4, UPPER_V, SPACE  );
    const quad A4H  = CODE_TOKEN (UPPER_A, DIGIT_4, UPPER_H, SPACE  );
    const quad LTV  = CODE_TOKEN (UPPER_L, UPPER_T, UPPER_V, SPACE  );
    const quad LTH  = CODE_TOKEN (UPPER_L, UPPER_T, UPPER_H, SPACE  );
    const quad B5V  = CODE_TOKEN (UPPER_B, DIGIT_5, UPPER_V, SPACE  );
    const quad B5H  = CODE_TOKEN (UPPER_B, DIGIT_5, UPPER_H, SPACE  );
    const quad A5V  = CODE_TOKEN (UPPER_A, DIGIT_5, UPPER_V, SPACE  );
    const quad A5H  = CODE_TOKEN (UPPER_A, DIGIT_5, UPPER_H, SPACE  );
    const quad B6V  = CODE_TOKEN (UPPER_B, DIGIT_6, UPPER_V, SPACE  );
    const quad B6H  = CODE_TOKEN (UPPER_B, DIGIT_6, UPPER_H, SPACE  );
    const quad A6V  = CODE_TOKEN (UPPER_A, DIGIT_6, UPPER_V, SPACE  );
    const quad A6H  = CODE_TOKEN (UPPER_A, DIGIT_6, UPPER_H, SPACE  );
    const quad EXV  = CODE_TOKEN (UPPER_E, UPPER_X, UPPER_V, SPACE  );
    const quad EXH  = CODE_TOKEN (UPPER_E, UPPER_X, UPPER_H, SPACE  );
    const quad HLTV = CODE_TOKEN (UPPER_H, UPPER_L, UPPER_T, UPPER_V);
    const quad HLTH = CODE_TOKEN (UPPER_H, UPPER_L, UPPER_T, UPPER_H);
    const quad PCV  = CODE_TOKEN (UPPER_P, UPPER_C, UPPER_V, SPACE  );
    const quad PCH  = CODE_TOKEN (UPPER_P, UPPER_C, UPPER_H, SPACE  );
    const quad KGV  = CODE_TOKEN (UPPER_K, UPPER_G, UPPER_V, SPACE  );
    const quad KGH  = CODE_TOKEN (UPPER_K, UPPER_G, UPPER_H, SPACE  );
    const quad CKV  = CODE_TOKEN (UPPER_C, UPPER_K, UPPER_V, SPACE  );
    const quad CKH  = CODE_TOKEN (UPPER_C, UPPER_K, UPPER_H, SPACE  );
    const quad OTHR = CODE_TOKEN (UPPER_O, UPPER_T, UPPER_H, UPPER_R);
    const quad INVD = CODE_TOKEN (UPPER_I, UPPER_N, UPPER_V, UPPER_D);

  }   // namespace psz

  //! System error information
  namespace err {

    // locations where trouble can occur
    const quad ADF  = CODE_TOKEN (UPPER_A, UPPER_D, UPPER_F, SPACE  );
    const quad TPU  = CODE_TOKEN (UPPER_T, UPPER_P, UPPER_U, SPACE  );
    const quad FB   = CODE_TOKEN (UPPER_F, UPPER_B, SPACE  , SPACE  );

    // kinds of trouble that may occur
    const quad OPN  = CODE_TOKEN (UPPER_O, UPPER_P, UPPER_N, SPACE  );
    const quad PJ   = CODE_TOKEN (UPPER_P, UPPER_J, SPACE  , SPACE  );
    const quad PE   = CODE_TOKEN (UPPER_P, UPPER_E, SPACE  , SPACE  );
    const quad ERR  = CODE_TOKEN (UPPER_E, UPPER_R, UPPER_R, SPACE  );
    const quad LTF  = CODE_TOKEN (UPPER_L, UPPER_T, UPPER_F, SPACE  );
    const quad LOCK = CODE_TOKEN (UPPER_L, UPPER_O, UPPER_C, UPPER_K);
    const quad DFED = CODE_TOKEN (UPPER_D, UPPER_F, UPPER_E, UPPER_D);
    const quad DTCL = CODE_TOKEN (UPPER_D, UPPER_T, UPPER_C, UPPER_L);
    const quad BTLO = CODE_TOKEN (UPPER_B, UPPER_T, UPPER_L, UPPER_O);

  }   // namespace err

  //! Focus state feedback
  /*! A \c INVD indicates that auto-focus is calibrating.  The \c VALD
   *  token is accompanied by a value that indicates focus distance on
   *  some arbitrary scale.
   */
  namespace fcs {

    const quad INVD = CODE_TOKEN (UPPER_I, UPPER_N, UPPER_V, UPPER_D);
    const quad VALD = CODE_TOKEN (UPPER_V, UPPER_A, UPPER_L, UPPER_D);

  }   // namespace fcs

  //! Push button state feedback
  /*! The push button state is returned via an integer that is to be
   *  interpreted as a collection of bit fields, weird as that might
   *  sound.  Accessor API is provided via hardware_status.
   *
   *  \sa hardware_status::event(), hardware_status::is_duplex() and
   *      hardware_status::media_size()
   */
  namespace pb {}

  //! Document separation mode state feedback
  namespace sep {

    const quad ON   = CODE_TOKEN (UPPER_O, UPPER_N, SPACE  , SPACE  );
    const quad OFF  = CODE_TOKEN (UPPER_O, UPPER_F, UPPER_F, SPACE  );

  }   // namespace sep

  //! Battery power level reporting
  namespace bat {

    const quad LOW  = CODE_TOKEN (UPPER_L, UPPER_O, UPPER_W, SPACE  );

  }   // namespace sep

}     // namespace status

namespace automatic_feed {

  const quad ON   = CODE_TOKEN (NUMBER , UPPER_O, UPPER_N, SPACE  );
  const quad OFF  = CODE_TOKEN (NUMBER , UPPER_O, UPPER_F, UPPER_F);

}     // namespace automatic_feed

//! Moving bits and pieces
/*! The \c MECH request lets one control the "flow" of media through
 *  the automatic document feeder and the focus position.
 */
namespace mechanic {

  const quad ADF  = CODE_TOKEN (NUMBER , UPPER_A, UPPER_D, UPPER_F);
  const quad FCS  = CODE_TOKEN (NUMBER , UPPER_F, UPPER_C, UPPER_S);
  const quad INI  = CODE_TOKEN (NUMBER , UPPER_I, UPPER_N, UPPER_I);

  //! Automatic document feeder actions
  namespace adf {

    const quad LOAD = CODE_TOKEN (UPPER_L, UPPER_O, UPPER_A, UPPER_D);
    const quad EJCT = CODE_TOKEN (UPPER_E, UPPER_J, UPPER_C, UPPER_T);
    const quad CLEN = CODE_TOKEN (UPPER_C, UPPER_L, UPPER_E, UPPER_N);
    const quad CALB = CODE_TOKEN (UPPER_C, UPPER_A, UPPER_L, UPPER_B);

  }   // namespace adf

  //! Focus control actions
  /*! If supported, one can \c AUTO focus or manually set the focus
   *  position to a value on some arbitrary scale.
   */
  namespace fcs {

    const quad AUTO = CODE_TOKEN (UPPER_A, UPPER_U, UPPER_T, UPPER_O);
    const quad MANU = CODE_TOKEN (UPPER_M, UPPER_A, UPPER_N, UPPER_U);

  }   // namespace fcs

}     // namespace mechanic

//! Initiate image acquisition
/*! The \c TRDT request transitions the device from parameter state to
 *  a so-called data state.  Once in the latter state, only the \c FIN,
 *  \c CAN, \c IMG and the \c EXT# requests may still be sent.  When in
 *  the parameter state the \c CAN and \c IMG requests cannot be sent.
 *
 *  The device leaves the data state when a \c FIN or \c CAN request is
 *  sent.  It also leaves data state when an unrecoverable system error
 *  is encountered (the documentation is ambiguous whether that would
 *  be via reply::info::ERR or reply::info::NRD) or when all requested
 *  images have been acquired.
 */
namespace transition {}

//! Fetch image data
/*! \note Image data is \e not four-byte aligned.
 */
namespace acquire_image {}

//! Transfer blobs
namespace extension {}

}       // namespace code_token

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_code_token_hpp_ */
