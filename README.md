# BECOS - baidu ecosystem

#### 版本
```
gflags   v2.2.1
leveldb  v1.22
protobuf v3.11.4
brpc     v0.9.6
braft    v1.1.0
```
#### 依赖关系
```
braft
└── brpc
    ├── gflags
    ├── leveldb
    └── protobuf

```
#### 构建leveldb
```
在原CMakeLists.txt中新增 (本repo已增加)

target_compile_options(leveldb
  PRIVATE
    -fPIC
)

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
sudo make install
```
#### 构建gflags
```
在原CMakeLists.txt中新增 (本repo已增加)

if ("^${TYPE}$" STREQUAL "^STATIC$")
  target_compile_options(${target_name}
    PRIVATE
      -fPIC
)

mkdir build && cd build
cmake ..
make
sudo make install
```

#### 构建protobuf
```
缺什么装什么
sudo apt-get install autoconf automake libtool curl make g++ unzip

./autogen.sh 
./configure
make
sudo make install
export LD_LIBRARY_PATH=/usr/local/lib
```

#### 构建brpc
```
将原CMakeLists.txt中的 'git rev-parse --short HEAD' 删除 (本repo已注释掉)

缺什么装什么 主要是ssl
sudo apt-get install -y git g++ make libssl-dev libgflags-dev libprotobuf-dev libprotoc-dev protobuf-compiler libleveldb-dev 

mkdir bld && cd bld && cmake .. && make
sudo make install
```


#### 构建braft
```
mkdir bld && cd bld && cmake .. && make
sudo make install
```