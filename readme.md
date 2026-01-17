<div align="center">
  <h1>nano-VectorDB</h1>
  <p><strong>A simple, easy-to-hack Vector Database (C++)</strong></p>
  <p>
    <img src="https://img.shields.io/badge/c++->=17-blue">
    <a href="https://pypi.org/project/nano-vectordb/">
      <img src="https://img.shields.io/pypi/v/nano-vectordb.svg">
    </a>
    <a href="https://codecov.io/github/KalanaRatnayake/nano-vectordb" > 
      <img src="https://codecov.io/github/KalanaRatnayake/nano-vectordb/graph/badge.svg"/> 
    </a>
  </p>
</div>


## [New] C++ Support

### Requirements

- C++17 or later
- [Eigen](https://eigen.tuxfamily.org/) (vector math)
- [OpenSSL](https://www.openssl.org/) (base64 encoding)
- [nlohmann/json](https://github.com/nlohmann/json) (JSON serialization)



### Build & Test (CMake)

```shbut i
sudo apt-get update
sudo apt-get install -y g++ cmake libeigen3-dev libssl-dev

# Build using CMake
mkdir build
cd build
cmake ..
cd ..
cmake --build build

# Run the test executable
./build/test_db
```

### Usage

```cpp
#include "NanoVectorDB.hpp"
#include <vector>
using namespace nano_vectordb;

NanoVectorDB vdb(1024);
std::vector<Data> data;
// Fill data with your vectors
vdb.upsert(data);
auto results = vdb.query(data[0].vector, 10);
vdb.save();
```

See src/test_db.cpp for full API usage and tests.