# Parallel-file-system

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

Setup GRPC
```
git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
cd grpc
git submodule update --init
cd grpc/third_party/protobuf
sudo make install
cd ../../
make
sudo make install
 ```
