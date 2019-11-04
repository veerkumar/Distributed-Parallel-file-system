# Parallel-file-system

Setup GRPC
```
sudo apt-get install build-essential autoconf libtool pkg-config
```
 https://github.com/protocolbuffers/protobuf/blob/master/src/README.md
 
 ```
sudo apt-get install autoconf automake libtool curl make g++ unzip
 git clone https://github.com/protocolbuffers/protobuf.git
    cd protobuf
    git submodule update --init --recursive
    ./autogen.sh
      ./configure
     make
     make check
     sudo make install
     sudo ldconfig # refresh shared library cache.
 ```
