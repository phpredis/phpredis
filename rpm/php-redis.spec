%global php_apiver  %((echo 0; php -i 2>/dev/null | sed -n 's/^PHP API => //p') | tail -1)
%global php_extdir  %(php-config --extension-dir 2>/dev/null || echo "undefined")
%global php_version %(php-config --version 2>/dev/null || echo 0)

Name:           php-redis
Version:        2.2.4
Release:        1%{?dist}
Summary:        The phpredis extension provides an API for communicating with the Redis key-value store.

Group:          Development/Languages
License:        PHP
URL:            https://github.com/nicolasff/phpredis
Source0:        https://github.com/nicolasff/phpredis/tarball/master
Source1:	redis.ini
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  php-devel
Requires:       php(zend-abi) = %{php_zend_api}
Requires:       php(api) = %{php_apiver}

%description
The phpredis extension provides an API for communicating with the Redis key-value store.

%prep
%setup -q -n nicolasff-phpredis-43bc590

%build
%{_bindir}/phpize
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install INSTALL_ROOT=$RPM_BUILD_ROOT

# install configuration
%{__mkdir} -p $RPM_BUILD_ROOT%{_sysconfdir}/php.d
%{__cp} %{SOURCE1} $RPM_BUILD_ROOT%{_sysconfdir}/php.d/redis.ini

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc CREDITS
%config(noreplace) %{_sysconfdir}/php.d/redis.ini
%{php_extdir}/redis.so

