##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012  EPSON AVASYS CORPORATION
##
##  License: GPL-3.0+
##  Author : Olaf Meeuwissen
##
##  This file is part of the 'Utsushi' package.
##  This package is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License or, at
##  your option, any later version.
##
##    This program is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##  You ought to have received a copy of the GNU General Public License
##  along with this package.  If not, see <http://www.gnu.org/licenses/>.

##  Process this file with automake to make a Makefile.in file.

SUBDIRS  = .
SUBDIRS += tests

EXTRA_DIST  = logo.svg
EXTRA_DIST += logo.png
EXTRA_DIST += icon.svg
EXTRA_DIST += icon32.xpm
EXTRA_DIST += icon16.xpm

logo.png: logo.svg
	rsvg-convert -h 100 -f png $< > $@
icon32.xpm: icon.svg
	rsvg-convert -h 32 -f png $< | $(MAGICK_CONVERT) png:- $@
icon16.xpm: icon.svg
	rsvg-convert -h 16 -f png $< | $(MAGICK_CONVERT) png:- $@

EXTRA_DIST += doxygen.conf
EXTRA_DIST += layout.xml
EXTRA_DIST += filter
EXTRA_DIST += main.dox
EXTRA_DIST += usr-guide.dox
EXTRA_DIST += api-specs.dox
EXTRA_DIST += test-spec.dox
EXTRA_DIST += dev-notes.dox

documentation  =
documentation += usr_guide_doc
documentation += api_specs_doc
documentation += test_spec_doc
documentation += dev_notes_doc

usr_guide_inputs  =
api_specs_inputs  = $(top_srcdir)/utsushi
test_spec_inputs  =
dev_notes_inputs  = $(top_srcdir)/lib
dev_notes_inputs += $(top_srcdir)/src
dev_notes_inputs += $(top_srcdir)/utsushi/test

CLEANFILES  = main.tags
CLEANFILES += `echo $(documentation:_doc=.tags) | sed 's|_|-|g'`
CLEANFILES += config.log

supported_formats  = html-target
if have_pdflatex
supported_formats += pdf-target
endif

clean-local: $(supported_formats:target=clean)
uninstall-local: $(supported_formats:target=uninstall)

.PHONY: $(supported_formats:target=clean)
.PHONY: $(supported_formats:target=uninstall)

##  Output format independent configuration settings

mainpage_title = sed -n 's/.*mainpage \(.*\)/\1/p'

configure = \
	test x != x"$$title" && title=" $$title"; \
	$(DOXYGEN) -g -; \
	cat $(srcdir)/doxygen.conf; \
	echo "PROJECT_NAME = \"$(PACKAGE_NAME)$$title\""; \
	echo 'PROJECT_NUMBER = "$(PACKAGE_VERSION)"'; \
	echo "STRIP_FROM_PATH = `cd $(top_srcdir) && pwd`"; \
	echo "STRIP_FROM_INC_PATH = `cd $(top_srcdir) && pwd`"; \
	echo 'LAYOUT_FILE = $(srcdir)/layout.xml'; \
	echo "GENERATE_TAGFILE = $$output.tags"; \
	echo 'INPUT_FILTER = $(srcdir)/filter'; \
	echo 'PERL_PATH = $(PERL)'; \
	echo 'HAVE_DOT = $(HAVE_DOT)'; \
	echo 'DOT_PATH = $(DOT_PATH)'; \
	if test x != x"$$inputs"; then \
	  for input in $$inputs; do \
	    echo "INPUT += $$input"; \
	  done; \
	fi

##  HTML output specific configuration settings snippet and targets

EXTRA_DIST += style.css
EXTRA_DIST += header.html
EXTRA_DIST += footer.html

.PHONY: $(documentation:_doc=_html)

html_configure = \
	test -d html/$$output || $(MKDIR_P) html/$$output; \
	$(configure); \
	echo "HTML_OUTPUT = html/$$output"; \
	echo 'HTML_HEADER = $(srcdir)/header.html'; \
	echo 'HTML_FOOTER = $(srcdir)/footer.html'; \
	echo 'HTML_STYLESHEET = $(srcdir)/style.css'; \
	echo "GENERATE_HTML = YES"

html-local: $(documentation:_doc=_html)
	inputs="$(srcdir)/main.dox"; \
	title="Documentation"; \
	output="."; \
	($(html_configure); \
	 echo 'GENERATE_TAGFILE = main.tags'; \
	 echo "TAGFILES = `echo $(documentation:_doc=.tags) | sed 's|_|-|g'`") \
	 | $(DOXYGEN) -
	for doc in `echo $(documentation:_doc=) | sed 's|_|-|g'`; do \
	  title="`$(mainpage_title) $(srcdir)/$$doc.dox`"; \
	  sed -i "s|index.html#$$doc\">$$doc|$$doc/index.html\">$$title|" \
	    html/index.html; \
	done

html-clean:
	-rm -rf html

install-html-local:
	test -d $(DESTDIR)$(htmldir) || $(MKDIR_P) $(DESTDIR)$(htmldir)
	cp -al html $(DESTDIR)$(htmldir)
html-uninstall:
	-rm -rf $(DESTDIR)$(htmldir)/html
	-rmdir $(DESTDIR)$(htmldir) 2>/dev/null

$(documentation:_doc=_html):
	mainpage="`echo $(srcdir)/$(@:_html=.dox) | sed 's|_|-|g'`"; \
	inputs="$$mainpage $($(@:_html=_inputs))"; \
	title="`$(mainpage_title) $$mainpage`"; \
	output="`echo $(@:_html=) | sed 's|_|-|g'`"; \
	($(html_configure)) | $(DOXYGEN) -

##  PDF output specific configuration settings snippet and targets

.PHONY: $(documentation:_doc=_pdf)

if have_pdflatex

pdf_configure = \
	test -d latex/$$output || $(MKDIR_P) latex/$$output; \
	$(configure); \
	echo "LATEX_BATCHMODE = YES"; \
	echo "USE_PDFLATEX = YES"; \
	echo "LATEX_OUTPUT = latex/$$output"; \
	echo "GENERATE_LATEX = YES"

pdf-local: $(documentation:_doc=_pdf)

pdf-clean:
	-rm -rf latex
	-rm `echo $(documentation:_doc=.pdf) | sed 's|_|-|g'`

install-pdf-local:
	test -d $(DESTDIR)$(pdfdir) || $(MKDIR_P) $(DESTDIR)$(pdfdir)
	for pdf in `echo $(documentation:_doc=.pdf) | sed 's|_|-|g'`; do \
	  test -e $$pdf || continue; \
	  $(INSTALL_DATA) $$pdf $(DESTDIR)$(pdfdir); \
	done
pdf-uninstall:
	for pdf in `echo $(documentation:_doc=.pdf) | sed 's|_|-|g'`; do \
	  rm $(DESTDIR)$(pdfdir)/$$pdf || true; \
	done
	-rmdir $(DESTDIR)$(pdfdir) 2>/dev/null

endif

##  It seems that automake only parses up to the : in the target and
##  then concludes that this has been defined already (when we wrote
##  the HTML target above) when this is put inside the have_pdflatex
##  conditional (where it logically belongs).  Putting it outside of
##  the if logic works around this issue.

$(documentation:_doc=_pdf):
	mainpage="`echo $(srcdir)/$(@:_pdf=.dox) | sed 's|_|-|g'`"; \
	inputs="$$mainpage $($(@:_pdf=_inputs))"; \
	title="`$(mainpage_title) $$mainpage`"; \
	output="`echo $(@:_pdf=) | sed 's|_|-|g'`"; \
	($(pdf_configure)) | $(DOXYGEN) -; \
	(cd latex/$$output; make) \
	  && mv latex/$$output/refman.pdf `echo $(@:_pdf=.pdf) | sed 's|_|-|g'`
