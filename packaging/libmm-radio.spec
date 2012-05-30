Name:       libmm-radio
Summary:    Multimedia Framework Radio Library
Version:    0.1.2
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/libmm-radio.manifest 
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  audio-session-manager-devel
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(mm-ta)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(avsystem)


%description
Descirption: Multimedia Framework Radio Library


%package devel
Summary:    Multimedia Framework Radio Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Description: Multimedia Framework Radio Library (DEV)

%prep
%setup -q

%build
cp %{SOURCE1001} .
./autogen.sh
CFLAGS=" %{optflags}  -DGST_EXT_TIME_ANALYSIS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "; export CFLAGS;
%configure --disable-static --prefix=%{_prefix}

make %{?jobs:-j%jobs}

%install
%make_install


%post -p /sbin/ldconfig

%postun  -p /sbin/ldconfig


%files
%manifest libmm-radio.manifest
%defattr(-,root,root,-)
%{_libdir}/libmmfradio.so.*
%{_bindir}/mm_radio_testsuite



%files devel
%manifest libmm-radio.manifest
%defattr(-,root,root,-)
%{_libdir}/libmmfradio.so
%{_libdir}/pkgconfig/mm-radio.pc
%{_includedir}/mmf/mm_radio.h


