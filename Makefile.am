AUTOMAKE_OPTIONS = gnu 1.4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libole2-2.0.pc

SUBDIRS = libole2 test

#small hack to get distcheck to work
clean-local:
	rm -f intl/po2tbl.sed

EXTRA_DIST = 		\
    autogen.sh 		\
    libole2.spec	\
    libole2.spec.in	\
    libole2-2.0.pc.in
