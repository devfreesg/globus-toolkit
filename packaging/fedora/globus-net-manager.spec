Name:		globus-net-manager
%global _name %(tr - _ <<< %{name})
Version:	0.8
Release:	1%{?dist}
Summary:	Globus Toolkit - Net Manager Library

Group:		System Environment/Libraries
License:	ASL 2.0
URL:		http://toolkit.globus.org/
Source:	http://toolkit.globus.org/ftppub/gt6/packages/%{_name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:	globus-common%{?_isa} >= 15.27
Requires:	globus-xio%{?_isa} >= 5

BuildRequires:	globus-common-devel >= 15.27
BuildRequires:	globus-xio-devel >= 5
BuildRequires:	doxygen
BuildRequires:	graphviz
%if "%{?rhel}" == "5"
BuildRequires:	graphviz-gd
%endif
%if %{?fedora}%{!?fedora:0} >= 19 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:  automake >= 1.11
BuildRequires:  autoconf >= 2.60
BuildRequires:  libtool >= 2.2
%endif
BuildRequires:  pkgconfig
%if %{?rhel}%{!?rhel:0} == 5
BuildRequires:  python26-devel
%else
BuildRequires:  python-devel
%endif

%package devel
Summary:	Globus Toolkit - Net Manager Library Development Files
Group:		Development/Libraries
Requires:	%{name}%{?_isa} = %{version}-%{release}
Requires:	globus-common-devel%{?_isa} >= 15.27
Requires:	globus-xio-devel%{?_isa} >= 5

%package xio-driver
Summary:	Globus Toolkit - Net Manager Library XIO Driver
Group:		System Environment/Libraries
Requires:	%{name}%{?_isa} = %{version}-%{release}
Requires:	globus-common-devel%{?_isa} >= 15.27
Requires:	globus-xio-devel%{?_isa} >= 5

%package doc
Summary:	Globus Toolkit - Net Manager Library Documentation Files
Group:		Documentation
%if %{?fedora}%{!?fedora:0} >= 10 || %{?rhel}%{!?rhel:0} >= 6
BuildArch:	noarch
%endif
Requires:	%{name} = %{version}-%{release}

%description
The Globus Toolkit is an open source software toolkit used for building Grid
systems and applications. It is being developed by the Globus Alliance and
many others all over the world. A growing number of projects and companies are
using the Globus Toolkit to unlock the potential of grids for their cause.

The %{name} package contains:
Net Manager Library

%description devel
The Globus Toolkit is an open source software toolkit used for building Grid
systems and applications. It is being developed by the Globus Alliance and
many others all over the world. A growing number of projects and companies are
using the Globus Toolkit to unlock the potential of grids for their cause.

The %{name}-devel package contains:
Net Manager Library Development Files

%description xio-driver
The Globus Toolkit is an open source software toolkit used for building Grid
systems and applications. It is being developed by the Globus Alliance and
many others all over the world. A growing number of projects and companies are
using the Globus Toolkit to unlock the potential of grids for their cause.

The %{name}-devel package contains:
Net Manager Library XIO Driver

%description doc
The Globus Toolkit is an open source software toolkit used for building Grid
systems and applications. It is being developed by the Globus Alliance and
many others all over the world. A growing number of projects and companies are
using the Globus Toolkit to unlock the potential of grids for their cause.

The %{name}-doc package contains:
Net Manager Library Documentation Files

%prep
%setup -q -n %{_name}-%{version}

%build
%if %{?fedora}%{!?fedora:0} >= 19 || %{?rhel}%{!?rhel:0} >= 7
# Remove files that should be replaced during bootstrap
rm -rf autom4te.cache

autoreconf -if
%endif

%configure \
           --disable-static \
           --docdir=%{_docdir}/%{name}-%{version} \
           --includedir=%{_includedir}/globus \
           --libexecdir=%{_datadir}/globus \
           --enable-python 

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

# Remove libtool archives (.la files)
find $RPM_BUILD_ROOT%{_libdir} -name 'lib*.la' -exec rm -v '{}' \;

%check
make %{?_smp_mflags} check

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc %{_docdir}/%{name}-%{version}/GLOBUS_LICENSE
%{_libdir}/lib%{_name}.so.*

%files devel
%defattr(-,root,root,-)
%{_includedir}/globus/globus_net_manager*.h
%{_libdir}/libglobus_net_manager*.so
%{_libdir}/pkgconfig/%{name}.pc

%files xio-driver
%defattr(-,root,root,-)
%{_includedir}/globus/globus_xio_net_manager_driver.h
%{_libdir}/libglobus_xio_net_manager_driver.so
%{_libdir}/pkgconfig/globus-xio-net-manager-driver.pc

%files doc
%defattr(-,root,root,-)
%dir %{_docdir}/%{name}-%{version}/html
%{_docdir}/%{name}-%{version}/html/*
%{_mandir}/man3/*

%changelog
* Mon Apr 13 2015 Globus Toolkit <support@globus.org> - 0.8-1
- fix for attr not being used on connect()

* Fri Mar 27 2015 Globus Toolkit <support@globus.org> - 0.7-1
- add file paramter to logging driver to set a file to log to.  use manager=logging;file=/path/to/file;.

* Fri Jan 09 2015 Globus Toolkit <support@globus.org> - 0.6-1
- Fix conflicts with globus-common-doc and globus-xio-doc

* Thu Jan 08 2015 Globus Toolkit <support@globus.org> - 0.5-1
- Fix test link on recent debians

* Wed Jan 07 2015 Globus Toolkit <support@globus.org> - 0.4-1
- Link in ltdl for tests

* Mon Jan 05 2015 Globus Toolkit <support@globus.org> - 0.3-1
- Tests run with static build

* Mon Dec 22 2014 Globus Toolkit <support@globus.org> - 0.2-1
- Fix missing skip test

* Fri Dec 19 2014 Globus Toolkit <support@globus.org> - 0.1-1
- check for python2.6-config

* Wed Dec 17 2014 Globus Toolkit <support@globus.org> - 0.0-1
- Initial package
