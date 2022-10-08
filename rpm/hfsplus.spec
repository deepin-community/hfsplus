Summary: user-space HFS+ utilities
Name: hfsplusutils
%define	version 1.0.4
%define location /usr
Version: %{version}
Release: 4
Copyright: GPL
Group:  Applications/File
Source: ftp://penguinppc.org/pub/hfsplus/hfsplus.tz2
# Patch: hfsplus-static-make.patch
BuildRoot: /var/tmp/%{name}-root
Vendor: Klaus Halfmann <klaus.halfmann@t-online.de>
Packager: Klaus Halfmann <klaus.halfmann@t-online.de>

%description
A portable, free implementation of routines for accessing HFS+ volumes.
Currently only reading is supported.

%prep

%setup -q -n hfsplus
# %patch0 -p0

%build
CFLAGS="$RPM_OPT_FLAGS" make -f Makefile.cvs
make


%install
rm -rf $RPM_BUILD_ROOT

install -d $RPM_BUILD_ROOT%{location}/bin
install -d $RPM_BUILD_ROOT%{location}/lib

install -s -m 755 src/.libs/* $RPM_BUILD_ROOT%{location}/bin
install -s -m 644 libhfsp/src/libhfsp.la $RPM_BUILD_ROOT%{location}/lib
install -s -m 755 libhfsp/src/.libs/libhfsp.a $RPM_BUILD_ROOT%{location}/lib
install -s -m 755 libhfsp/src/.libs/libhfsp.so.0.0.0 $RPM_BUILD_ROOT%{location}/lib

# hpfsck should work a bit
# rm  $RPM_BUILD_ROOT%{location}/bin/hpfsck

cd $RPM_BUILD_ROOT%{location}/lib; ln -s libhfsp.so.0.0.0 libhfsp.so
cd $RPM_BUILD_ROOT%{location}/lib; ln -s libhfsp.so.0.0.0 libhfsp.so.0

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{location}/bin/*
%{location}/lib/*
%doc doc/bugs.html doc/faq.html doc/hfsp.html doc/libhfsp.html doc/man/hfsp.sgml
