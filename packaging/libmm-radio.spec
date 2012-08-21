Name:       libmm-radio
Summary:    Multimedia Framework Radio Library
Version:    0.2.3
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO BE FILLED IN
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  audio-session-manager-devel
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(mm-ta)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-plugins-base-0.10)

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
./autogen.sh
CFLAGS=" %{optflags}  -DGST_EXT_TIME_ANALYSIS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "; export CFLAGS;
%configure --disable-static --prefix=%{_prefix}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


%post -p /sbin/ldconfig

%postun  -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/libmmfradio.so.*
%{_bindir}/mm_radio_testsuite



%files devel
%defattr(-,root,root,-)
%{_libdir}/libmmfradio.so
%{_libdir}/pkgconfig/mm-radio.pc
%{_includedir}/mmf/mm_radio.h
