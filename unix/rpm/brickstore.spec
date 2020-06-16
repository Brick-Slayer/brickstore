# Header Stanza begins here
#
# Short description
Summary: Offline tool for BrickLink.

# the application name
Name: brickstore

# the version of the application
Version: %{_brickstore_version}

# the packaging revision of this particular version
Release: 1

# this software is licensed under the GPL
License: GPL

# this software is an application used with text files
Group: Applications/Internet

# this software source file; not the use of variables
Source: %{name}-%{version}.tar.bz2

# temporary dir where the software should be compiled
Buildroot: %{_tmppath}/%{name}-root

# non-obvious software required to build the software
BuildRequires: %{_brickstore_buildreq}

# long description
%description
BrickStore is an offline tool to manage your online store on
https://www.bricklink.com

# Prep Stanza begins here
#
%prep

# unpack the source and cd into the source directory
%setup -q

# Build Stanza begins here
#
%build

# run qmake to produce a Makefile
qmake-qt4 CONFIG+=release CONFIG-=debug RELEASE=%{_brickstore_version} PREFIX=%{_prefix}

# compile the software
make %{_smp_mflags}

# Install Stanza begins here
#
%install

# as sanity protection, make sure the Buildroot is empty
rm -rf $RPM_BUILD_ROOT

# install software into the Buildroot
make install INSTALL_ROOT=$RPM_BUILD_ROOT
strip $RPM_BUILD_ROOT/usr/bin/brickstore
cp -aH $RPM_SOURCE_DIR/share $RPM_BUILD_ROOT/usr

# define a clean-up script to run after the software in Buildroot is pkg'ed
%clean

# the actual script -- just delete all files within the Buildroot
rm -rf $RPM_BUILD_ROOT

%post
touch /usr/share/icons/hicolor
if which update-desktop-database >/dev/null 2>&1 ; then
        update-desktop-database -q
fi
if which update-mime-database >/dev/null 2>&1 ; then
        update-mime-database /usr/share/mime
fi

%postun
touch /usr/share/icons/hicolor
if which update-desktop-database >/dev/null 2>&1 ; then
        update-desktop-database -q
fi
if which update-mime-database >/dev/null 2>&1 ; then
        update-mime-database /usr/share/mime
fi

# Files Stanza begins here
#
%files

# set perms and ownerships of packaged files
# the - indicates that the current permissions on the files should be used
%defattr(-,root,root)

# package all files within the $RPM_BUILD_ROOT/usr directory
/usr/bin/brickstore
/usr/share/*
