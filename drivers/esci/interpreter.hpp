//  interpreter.hpp -- API entrypoints to use protocol translators
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

#ifndef drivers_esci_interpreter_hpp_
#define drivers_esci_interpreter_hpp_

#ifdef __cplusplus
extern "C" {
#endif

typedef int callback (void *buffer, int length);

int interpreter_ctor (callback *wire_protocol_reader,
                      callback *wire_protocol_writer);
int interpreter_dtor ();

int interpreter_reader (void *buffer, int length);
int interpreter_writer (void *buffer, int length);

#ifdef __cplusplus
}       // extern "C"
#endif

#endif  /* drivers_esci_interpreter_hpp_ */
