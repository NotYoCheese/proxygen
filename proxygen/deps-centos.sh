#!/bin/bash

## Run this script to build proxygen and run the tests. If you want to
## install proxygen to use in another C++ project on this machine, run
## the sibling file `reinstall.sh`.

set -e
start_dir=`pwd`
trap "cd $start_dir" EXIT

# Must execute from the directory containing this script
cd "$(dirname "$0")"

# Some extra dependencies for Ubuntu 13.10 and 14.04
sudo yum install -y \
	flex \
	bison \
	krb5-devel \
	libgsasl-devel \
	numactl-devel \
	numactl-libs \
	pkgconfig \
	openssl-devel \
	openssl-libs \
	libcap-devel \
	ruby \
	gperf \
	autoconf-archive \
	libevent-devel \
	libevent \
	snappy \
    subversion \
	wget

# Adding support for Ubuntu 12.04.x
# Needs libdouble-conversion-dev, google-gflags and double-conversion
# deps-centos.sh in fbthrift and folly builds anyways (no trap there)
# centos RPM for glog & gflags is missing calls for folly
# remove rpm and install from SVN
sudo yum remove gflags # this will take glog out with it
#if ! sudo yum install glog-devel; 
if [ ! -e google-glog ]; then
    echo "fetching glog from svn"
    svn checkout https://google-glog.googlecode.com/svn/trunk/ google-glog
    cd google-glog
    ./configure
    make
    sudo make install
cd ..
fi

if [ ! -e google-gflags ]; then
    echo "Fetching gflags from svn (yum failed)"
    svn checkout https://google-gflags.googlecode.com/svn/trunk/ google-gflags
    cd google-gflags
    ./configure
    make
    sudo make install
    cd ..
fi

# no rpm for this?
if [ ! -e double-conversion ]; then
echo "Fetching double-conversion from git (yum failed)"
	git clone https://github.com/floitsch/double-conversion.git double-conversion
	cd double-conversion
	cmake . -DBUILD_SHARED_LIBS=ON
	sudo make install
	cd ..
fi

git clone https://github.com/NotYoCheese/fbthrift.git || true
cd fbthrift/thrift

# Rebase in case we've already downloaded thrift and folly
git fetch && git rebase origin/master
if [ -e folly/folly ]; then
    # We have folly already downloaded
    cd folly/folly
    git fetch && git rebase origin/master
    cd ../..
fi

# Build folly and fbthrift
if ! ./deps-centos.sh; then
	echo "fatal: fbthrift+folly build failed"
	exit -1
fi

# Build proxygen
cd ../..
autoreconf -ivf
CPPFLAGS=" -I`pwd`/fbthrift/thrift/folly/ -I`pwd`/fbthrift/" \
    LDFLAGS="-L`pwd`/fbthrift/thrift/lib/cpp/.libs/ -L`pwd`/fbthrift/thrift/lib/cpp2/.libs/ -L`pwd`/fbthrift/thrift/folly/folly/.libs/" ./configure
make -j8

# Run tests
make check
