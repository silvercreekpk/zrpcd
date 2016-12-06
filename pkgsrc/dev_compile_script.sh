# Copyright 2016 Tata Consulting
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
#
# - Neither the name of 6WIND S.A. nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

set -eux

ZRPCD_BUILD_FOLDER=${ZRPCD_BUILD_FOLDER:-/tmp}
export_variables (){
    #these are required by quagga
    export ZEROMQ_CFLAGS="-I"$ZRPCD_BUILD_FOLDER"/zeromq4-1/include"
    export ZEROMQ_LIBS="-L"$ZRPCD_BUILD_FOLDER"/zeromq4-1/.libs/ -lzmq"
    export CAPN_C_CFLAGS='-I'$ZRPCD_BUILD_FOLDER'/c-capnproto/ -I'$ZRPCD_BUILD_FOLDER'/'
    export CAPN_C_LIBS='-L'$ZRPCD_BUILD_FOLDER'/c-capnproto/.libs/ -lcapn_c'

    #In addition to the above, zrpcd requires these flags too.
    export QUAGGA_CFLAGS='-I'$ZRPCD_BUILD_FOLDER'/quagga/lib/'
    export QUAGGA_LIBS='-L'$ZRPCD_BUILD_FOLDER'/quagga/lib/.libs/. -lzebra'
    export THRIFT_CFLAGS="-I"$ZRPCD_BUILD_FOLDER"/thrift/lib/c_glib/src/thrift/c_glib/ -I"$ZRPCD_BUILD_FOLDER"/thrift/lib/c_glib/src"
    export THRIFT_LIBS="-L'$ZRPCD_BUILD_FOLDER'/thrift/lib/c_glib/.libs/ -lthrift_c_glib"

}

install_deps() {

    pushd $ZRPCD_BUILD_FOLDER
    export_variables
#Install the required software for building quagga
    apt-get install automake bison flex g++ git libboost1.55-all-dev libevent-dev libssl-dev libtool make pkg-config gawk libreadline-dev libglib2.0-dev wget -y --force-yes

#Clean the directory
    rm -rf c-capnproto thrift zeromq4-1 quagga zrpcd

#Install thrift
    git clone https://git-wip-us.apache.org/repos/asf/thrift.git
    cd thrift

    wget https://issues.apache.org/jira/secure/attachment/12840512/0001-THRIFT-3987-externalise-declaration-of-thrift-server.patch
    patch -p1 < 0001-THRIFT-3987-externalise-declaration-of-thrift-server.patch
    wget https://issues.apache.org/jira/secure/attachment/12840511/0002-THRIFT-3986-using-autoreconf-i-fails-because-of-miss.patch
    patch -p1 < 0002-THRIFT-3986-using-autoreconf-i-fails-because-of-miss.patch

    autoreconf -i
    ./configure --without-qt4 --without-qt6 --without-csharp --without-java\
    --without-erlang --without-nodejs --without-perl --without-python\
    --without-php --without-php_extension --without-dart --without-ruby\
    --without-haskell --without-go --without-haxe --without-d\
    --prefix=/opt/quagga
    make
    make install
    cd ..

#Install ZeroMQ
    git clone https://github.com/zeromq/zeromq4-1.git
    cd zeromq4-1
    git checkout 56b71af22db3
    autoreconf -i
    ./configure --without-libsodium --prefix=/opt/quagga
    make
    make install
    cd ..

#Install C-capnproto
     git clone https://github.com/opensourcerouting/c-capnproto
     cd c-capnproto
     git checkout 332076e52257
     autoreconf -i
     ./configure --prefix=/opt/quagga --without-gtest

     make
     mkdir /opt/quagga/lib -p
     mkdir /opt/quagga/include/c-capnproto -p

    cp capn.h /opt/quagga/include/c-capnproto/.
    cp .libs/libcapn.so.1.0.0 .libs/libcapn_c.so.1.0.0
    ln -sf $ZRPCD_BUILD_FOLDER/c-capnproto/.libs/libcapn_c.so.1.0.0 $ZRPCD_BUILD_FOLDER/c-capnproto/.libs/libcapn_c.so
    cp .libs/libcapn.so.1.0.0 /opt/quagga/lib/libcapn_c.so.1.0.0
    ln -sf /opt/quagga/lib/libcapn_c.so.1.0.0 /opt/quagga/lib/libcapn_c.so
    cd ..

#Install Quagga
    git clone https://github.com/6WIND/quagga.git
    cd quagga
    git checkout quagga_110_mpbgp_capnp
    autoreconf -i
    LIBS='-L'$ZRPCD_BUILD_FOLDER'/zeromq4-1/.libs/ -L'$ZRPCD_BUILD_FOLDER'/c-capnproto/.libs/' \
    ./configure --with-zeromq --with-ccapnproto --prefix=/opt/quagga --enable-user=quagga \
    --enable-group=quagga --enable-vty-group=quagga --localstatedir=/opt/quagga/var/run/quagga \
    --disable-doc --enable-multipath=64
    make
    make install
    cp /opt/quagga/etc/bgpd.conf.sample4 /opt/quagga/etc/bgpd.conf
    mkdir /opt/quagga/var/run/quagga -p
    mkdir /opt/quagga/var/log/quagga -p
    touch /opt/quagga/var/log/quagga/zrpcd.init.log
    addgroup --system quagga 
    addgroup --system quagga
    adduser --system --ingroup quagga --home /opt/quagga/var/run/quagga \
             --gecos "Quagga-BGP routing suite" \
             --shell /bin/false quagga  >/dev/null
    chown -R quagga:quagga /opt/quagga/var/run/quagga
    chown -R quagga:quagga /opt/quagga/var/log/quagga

    cd ..
    popd
}

build_zrpcd (){
#Install ZRPC.
    export_variables

    if [ -z "${BUILD_FROM_DIST}" ]; then
        pushd $ZRPCD_BUILD_FOLDER
        git clone https://github.com/6WIND/zrpcd.git
        cd zrpcd
    elif [ -n "${DIST_ARCHIVE}" ]; then
        tar zxvf $DIST_ARCHIVE
        cd "${DIST_ARCHIVE%.tar.gz}"
    else
        # prepate the dist archive
        touch NEWS README
        autoreconf -i
        LIBS='-L'$ZRPCD_BUILD_FOLDER'/zeromq4-1/.libs/ -L'$ZRPCD_BUILD_FOLDER'/c-capnproto/.libs/ -L'$ZRPCD_BUILD_FOLDER'/quagga/lib/.libs/' \
        ./configure --enable-zrpcd --prefix=/opt/quagga --enable-user=quagga --enable-group=quagga \
        --enable-vty-group=quagga --localstatedir=/opt/quagga/var/run/quagga
        make dist
        DIST_ARCHIVE=$(ls *.tar.gz)
        tar zxvf $DIST_ARCHIVE
        cd "${DIST_ARCHIVE%.tar.gz}"
    fi

    touch NEWS README
    autoreconf -i
    LIBS='-L'$ZRPCD_BUILD_FOLDER'/zeromq4-1/.libs/ -L'$ZRPCD_BUILD_FOLDER'/c-capnproto/.libs/ -L'$ZRPCD_BUILD_FOLDER'/quagga/lib/.libs/' \
	./configure --enable-zrpcd --prefix=/opt/quagga --enable-user=quagga --enable-group=quagga \
    --enable-vty-group=quagga --localstatedir=/opt/quagga/var/run/quagga
    make
    make install
    # Temporarily disable this when using the dist method
    if [ -z "$BUILD_FROM_DIST" ]; then
        mkdir /opt/quagga/etc/init.d -p
        cp pkgsrc/zrpcd.ubuntu /opt/quagga/etc/init.d/zrpcd
        chmod +x /opt/quagga/etc/init.d/zrpcd
    fi

    if [ -z "${BUILD_FROM_DIST}" ]; then
        popd
    fi

     echo "hostname bgpd" >> /opt/quagga/etc/bgpd.conf
     echo "password sdncbgpc" >> /opt/quagga/etc/bgpd.conf
     echo "service advanced-vty" >> /opt/quagga/etc/bgpd.conf
     echo "log stdout" >> /opt/quagga/etc/bgpd.conf
     echo "line vty" >> /opt/quagga/etc/bgpd.conf
     echo " exec-timeout 0 0 " >> /opt/quagga/etc/bgpd.conf
     echo "debug bgp " >> /opt/quagga/etc/bgpd.conf
     echo "debug bgp updates" >> /opt/quagga/etc/bgpd.conf
     echo "debug bgp events" >> /opt/quagga/etc/bgpd.conf
     echo "debug bgp fsm" >> /opt/quagga/etc/bgpd.conf
}

display_usage ()
{
cat << EOF
OPTIONS:
  -b/--build, build zrpcd. By default clones the master of the upstream repository and \
      builds that.
  -t/--from-dist-archive, package zrpcd using make dist and build the sources in the archive. \
      If --build is not used, the flag is ignored.
  -a/--archive [path to archive.tar.gz], give explicitly the source archive to be used instead \
      of producing one with make dist.
  -d/--install-deps, compile and install zrpcd's dependecies.
  -h help, prints this help text

EOF
}


INSTALL_DEPS=""
BUILD_ZRPCD=""
BUILD_FROM_DIST=""
DIST_ARCHIVE=""
parse_cmdline() {
    while [ $# -gt 0 ]
    do
        case "$1" in
            -h|--help)
                display_usage
                exit 0
                ;;
            -b|--build)
                BUILD_ZRPCD="true"
                shift
                ;;
            -d|--install-deps)
                INSTALL_DEPS="true"
                shift
                ;;
            -t|--from-dist-archive)
                BUILD_FROM_DIST="true"
                shift
                ;;
            -a|--archive)
                DIST_ARCHIVE=${2}
                shift 2
                ;;
            *)
                display_usage
                exit 1
                ;;
        esac
    done
}

parse_cmdline $@

if [ -n "$INSTALL_DEPS" ]; then
    install_deps
fi

if [ -n "$BUILD_ZRPCD" ]; then
    build_zrpcd
fi