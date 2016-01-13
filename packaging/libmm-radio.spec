Name:       libmm-radio
Summary:    Multimedia Framework Radio Library
Version:    0.2.4
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	libmm-radio.manifest
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)

%description
Description: Multimedia Framework Radio Library


%package devel
Summary:    Multimedia Framework Radio Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Description: Multimedia Framework Radio Library (DEV)

%package test
Summary:    Multimedia Framework Radio Library (TEST)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description test
Description: Multimedia Framework Radio Library (TEST)

%prep
%setup -q
cp %{SOURCE1001} .

%build
./autogen.sh

#export GSTREAMER_API="-DGST_API_VERSION_1=1"
#export use_gstreamer_1=1

#CFLAGS=" %{optflags} -Wall -DGST_EXT_TIME_ANALYSIS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" $GSTREAMER_API"; export CFLAGS;
CFLAGS=" %{optflags} -Wall -DGST_EXT_TIME_ANALYSIS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "; export CFLAGS;

%configure \
--enable-emulator \
--disable-static --prefix=%{_prefix}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}
%make_install


%post -p /sbin/ldconfig

%postun  -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libmmfradio.so.*
/usr/share/license/%{name}


%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libmmfradio.so
%{_libdir}/pkgconfig/mm-radio.pc
%{_includedir}/mmf/mm_radio.h

%files test
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_bindir}/mm_radio_testsuite
