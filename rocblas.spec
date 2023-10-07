%global upstreamname rocBLAS

%global toolchain rocm
# hipcc does not support some clang flags
%global build_cxxflags %(echo %{optflags} | sed -e 's/-fstack-protector-strong/-Xarch_host -fstack-protector-strong/' -e 's/-fcf-protection/-Xarch_host -fcf-protection/')

# librocblas is too big to link so split base on gpu family
# The librocblas.so that used to be in /usr/lib64/ is now staged
# in /usr/lib64/rocm/<gpu>/  where gpu is gfx8, gfx9, gfx10 or
# gfx11. %post queries which gpu is running and copies the library.
#
# WARNING: can not run with multiple families of GPU's
#
# doing multiple builds, so need do something custom and not
# in the normal single build dir
%define __cmake_in_source_build 1

# Create a *-test rpm to do testing with
# Limited to gfx11
#
# Because the binaries are linking against a nonstandard libpath
# it is necessary to
# export QA_RPATHS=0xff
%bcond_with test

Name:           rocblas
Version:        %{rocm_version}
Release:        1%{?dist}
Summary:        BLAS implementation for ROCm
Url:            https://github.com/ROCmSoftwarePlatform
License:        MIT

Source0:        %{url}/%{upstreamname}/archive/refs/tags/rocm-%{version}.tar.gz#/%{upstreamname}-%{version}.tar.gz
#Patch0:         0001-prepare-rocblas-cmake-for-fedora.patch

BuildRequires:  rocm-rpm-macros
BuildRequires:  %rocm_buildrequires

%if %{with test}
BuildRequires:  %rocm_buildrequires_test
BuildRequires:  blas-devel
BuildRequires:  libomp-devel
BuildRequires:  python3-pyyaml
BuildRequires:  rocminfo
%endif

Requires:       rocminfo

# Only x86_64 works right now:
ExclusiveArch:  x86_64

%description
rocBLAS is the AMD library for Basic Linear Algebra Subprograms
(BLAS) on the ROCm platform. It is implemented in the HIP
programming language and optimized for AMD GPUs.

%package devel
Summary:        Libraries and headers for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
%{summary}

%if %{with test}
%package test
Summary:        Tests for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description test
%{summary}
%endif

%prep
%autosetup -p1 -n %{upstreamname}-rocm-%{version}

%build

for gpu in %{rocm_gpu_list}
do
    mkdir ${gpu}
    cd ${gpu}
    module load rocm/$gpu
    %cmake .. %rocm_cmake_options \
           -DAMDGPU_TARGETS=$ROCM_GPUS \
%if %{with test}
           %rocm_cmake_test_options \
%endif
           -DBUILD_WITH_TENSILE=OFF \
           -DCMAKE_INSTALL_LIBDIR=$ROCM_LIB \
	   -DCMAKE_INSTALL_BINDIR=$ROCM_BIN \
	   -DHIP_PLATFORM=amd
    %cmake_build
    module purge
    cd ..
done

%install

for gpu in %{rocm_gpu_list}
do
    cd ${gpu}
    %cmake_install
    cd ..
done

%files
%dir %{_libdir}/rocm/
%license LICENSE.md
%exclude %{_docdir}/%{name}/LICENSE.md
%{_libdir}/rocm/*/lib%{name}.so.*

%files devel
%doc README.md
%{_includedir}/%{name}
%{_libdir}/rocm/*/lib%{name}.so
%{_libdir}/cmake/%{name}


%changelog
* Sat Oct 7 2023 Tom Rix <trix@redhat.com> - 5.7.0-1
- Update to 5.7
- Use WIP rocm-rpm-macros
- Convert to environent modules

* Sun Oct 1 2023 Tom Rix <trix@redhat.com> - 5.6.0-2
- Split the build into gpu families

* Sat Sep 23 2023 Tom Rix <trix@redhat.com> - 5.6.0-1
- Update to 5.6

* Tue Jun 6 2023 Tom Rix <trix@redhat.com> - 5.5.1-1
- Initial package
