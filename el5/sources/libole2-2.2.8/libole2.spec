%define name libole2
%define version 2.2.8
%define release 1
%define prefix /usr

Summary: Structured Storage OLE2 library

Name: %{name}
Version: %{version}
Release: %{release}
Group: System Environment/Libraries
Copyright: GPL

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
%defattr(-,root,root)
%doc AUTHORS COPYING README
%{prefix}/lib/lib*.so.*
%{prefix}/share/aclocal/*.m4
%{prefix}/share/libole2/html/libole2/*.html
%{prefix}/share/libole2/html/*.html
%{prefix}/share/libole2/html/*.txt

%files devel
%defattr(-,root,root)
%attr(755,root,root) %{prefix}/bin/libole2-config
%{prefix}/lib/lib*.so
%{prefix}/lib/*a
%{prefix}/lib/*.sh
%{prefix}/include/libole2/*

%clean
rm -r $RPM_BUILD_ROOT

%changelog
* Sun Oct 22 2000 John Gotts <jgotts@linuxsavvy.com>
- Minor updates.
* Wed Jun 28 2000 Arturo Tena <arturo@directmail.org>
- Updated summary and description.
* Sun May 23 2000 John Gotts <jgotts@linuxsavvy.com>
- New SPEC file.
