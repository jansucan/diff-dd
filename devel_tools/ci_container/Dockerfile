FROM ubuntu:22.04

RUN <<EOF
    apt-get update
    apt-get install -y g++ python3 pip git clang-format
    python3 -m pip install pre-commit
EOF

RUN <<EOF
    apt-get install -y wget cmake libpcre3-dev

    cd /tmp
    wget https://github.com/danmar/cppcheck/archive/2.14.0.tar.gz
    tar xvf 2.14.0.tar.gz
    cd cppcheck-2.14.0

    mkdir build
    cd build
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_GUI=Off \
        -DUSE_MATCHCOMPILER=On \
        -DHAVE_RULES=On \
        ..
    make install/strip

    rm -r /tmp/2.14.0.tar.gz /tmp/cppcheck-2.14.0
    apt-get remove -y wget cmake libpcre3-dev
    apt-get autoremove -y
    apt-get clean
EOF

WORKDIR /diff-dd

COPY ./devel_tools/ci_container/entry.sh /entry.sh
ENTRYPOINT ["/entry.sh"]
