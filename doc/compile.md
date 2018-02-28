# depend libs

## basic requirement

Only for Ubuntu 14.04 now

# step 1, change ubuntu source to ali
```
deb http://mirrors.aliyun.com/ubuntu/ trusty main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ trusty-security main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ trusty-updates main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ trusty-proposed main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ trusty-backports main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ trusty main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ trusty-security main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ trusty-updates main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ trusty-proposed main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ trusty-backports main restricted universe multiverse
```

# step 2, install necessary tools
```
sudo apt-get install -y cmake git g++ protobuf-compiler python-dev
```

install lib dependcies
```
sudo apt-get install -y libprotobuf-dev libssl-dev liblz4-dev zlib1g-dev
```

get boost 1.58, build install and
```
export BOOST_ROOT=root dir of boost
```

# step 3, install soci library
```
sudo apt-get install -y libmysqlclient-dev libsqlite3-dev
cd libs/soci
mkdir build && cd build
cmake ../
make && sudo make install
```

# step 4 install rocksdb library
```
sudo apt-get install libsnappy-dev zlib1g-dev libbz2-dev
cd libs/rocksdb
mkdir build && cd build
cmake ..
make && sudo make install
```

# step 5, install websocketpp library
```
cd libs/websocketpp
mkdir build && cd build
cmake ..
make && sudo make install
```

# step6, build skywelld
```
mkdir build && cd build
cmake .. && make
```

