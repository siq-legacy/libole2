%define name libole2
%define version 2.2.8
%define release 12
%define prefix /usr/local/libole2

Summary: Structured Storage OLE2 library

Name: %{name}
Version: %{version}
Release: %{release}
Group: System Environment/Libraries
License: LGPL

Source: ftp://ftp.gnome.org/pub/GNOME/unstable/sources/libole2/libole2-%{version}.tar.gz
Buildroot: /var/tmp/%{name}-%{version}-%{release}-root

%description
A library containing functionality to manipulate OLE2 Structured Storage files. It is used by Gnumeric from Gnome, AbiWord from AbiSuite and by other programs.

%package devel
Summary: Libraries, includes, etc to develop libole2 applications
Group: Development/Libraries
Requires: libole2

%description devel
Libraries, include files, etc you can use to develop libole2 applications.

%prep

%setup

%build
%ifarch alpha
  MYARCH_FLAGS="--host=alpha-redhat-linux"
%endif

if [ ! -f configure ]; then
CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=%{prefix}
else
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix}
fi

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -r $RPM_BUILD_ROOT; fi
mkdir -p $RPM_BUILD_ROOT%{prefix}
make prefix=$RPM_BUILD_ROOT%{prefix} install

%files
%{prefix}
# %{prefix}/lib/lib*.so*
# %{prefix}/lib/*.a

%clean
rm -r $RPM_BUILD_ROOT

%changelog
* Sun Oct 22 2000 John Gotts <jgotts@linuxsavvy.com>
- Minor updates.
* Wed Jun 28 2000 Arturo Tena <arturo@directmail.org>
- Updated summary and description.
* Sun May 23 2000 John Gotts <jgotts@linuxsavvy.com>
- New SPEC file.
