# Define version and release number
%define version 1.0.2
%define release 1

Name:      php-igbinary
Version:   %{version}
Release:   %{release}%{?dist}
Packager:  Mikko Koppanen <mikko@ibuildings.com>
Summary:   PHP igbinary extension
License:   PHP Style License (http://opensource.dynamoid.com/#license)
Group:     Web/Applications
URL:       http://opensource.dynamoid.com/
# pear package creates .tgz file and the original source was .tar.gz
Source:    http://opensource.dynamoid.com/igbinary-%{version}.tgz
Prefix:    %{_prefix}
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: php-devel, make, gcc, /usr/bin/phpize

%description
Igbinary is a drop in replacement for the standard PHP serializer. 
Instead of time and space consuming textual representation, 
igbinary stores PHP data structures in a compact binary form. 

%prep
%setup -q -n igbinary-%{version}

%build
/usr/bin/phpize && %configure && %{__make} %{?_smp_mflags}

# Clean the buildroot so that it does not contain any stuff from previous builds
[ "%{buildroot}" != "/" ] && %{__rm} -rf %{buildroot}

# Install the extension
%{__make} install INSTALL_ROOT=%{buildroot}

# Create the ini location
%{__mkdir} -p %{buildroot}/etc/php.d

# Preliminary extension ini
echo "extension=igbinary.so" > %{buildroot}/%{_sysconfdir}/php.d/igbinary.ini

%clean
[ "%{buildroot}" != "/" ] && %{__rm} -rf %{buildroot}

%files
%{_libdir}/php/modules/igbinary.so
%{_sysconfdir}/php.d/igbinary.ini
%{_includedir}/php/ext/igbinary/igbinary.h

%changelog
* Fri Oct 02 2009 Mikko Koppanen <mikko@ibuildings.com>
 - Initial spec file
