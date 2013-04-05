Name:       libmm-radio
Summary:    Multimedia Framework Radio Library
Version:    0.2.1
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  audio-session-manager-devel
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(mm-ta)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(mm-sound)
%if %{defined with_Gstreamer0.10}
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-plugins-base-0.10)
%else
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
%endif

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

%if %{defined with_Gstreamer0.10}
export GSTREAMER_API=""
%else
export GSTREAMER_API="-DGST_API_VERSION_1=1"
export use_gstreamer_1=1
%endif

CFLAGS=" %{optflags}  -DGST_EXT_TIME_ANALYSIS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" $GSTREAMER_API"; export CFLAGS;
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
%defattr(-,root,root,-)
%{_libdir}/libmmfradio.so
%{_libdir}/pkgconfig/mm-radio.pc
%{_includedir}/mmf/mm_radio.h
