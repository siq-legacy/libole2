INCLUDES =			\
	-I$(top_srcdir)/	\
	-I$(srcdir)/		\
	$(LIBOLE2_CFLAGS)

if WITH_GNOME_VFS
libole2_gnomevfs =		\
	ms-ole-gnomevfs.c	\
	ms-ole-gnomevfs.h
else
libole2_gnomevfs =
endif

# include these just in case we are building a tarball with out gnomevfs
# support.
EXTRA_DIST =			\
	ms-ole-gnomevfs.c	\
	ms-ole-gnomevfs.h

libole2_sources =		\
	ms-ole.c		\
	ms-ole.h		\
	ms-ole-summary.c	\
	ms-ole-summary.h	\
	ms-ole-vba.c		\
	ms-ole-vba.h

lib_LTLIBRARIES = libgnomeole2.la

LDADD = $(LIBOLE2_LIBS)

libgnomeole2_la_LDFLAGS = \
	-version-info $(VERSION_INFO)

libgnomeole2_la_SOURCES =	\
	$(libole2_sources)	\
	version.c		\
	libole2.h

libgnomeole2includedir      = $(includedir)/libole2-2.0/libole2
libgnomeole2include_HEADERS =	\
	libole2.h		\
	ms-ole.h		\
	ms-ole-summary.h	\
	ms-ole-vba.h

