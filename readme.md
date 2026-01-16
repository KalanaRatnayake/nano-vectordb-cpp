<div align="center">
  <h1>nano-VectorDB</h1>
  <p><strong>A simple, easy-to-hack Vector Database (Python & C++)</strong></p>
  <p>
    <img src="https://img.shields.io/badge/python->=3.9.11-blue">
    <img src="https://img.shields.io/badge/c++->=17-blue">
    <a href="https://pypi.org/project/nano-vectordb/">
      <img src="https://img.shields.io/pypi/v/nano-vectordb.svg">
    </a>
    <a href="https://codecov.io/github/KalanaRatnayake/nano-vectordb" > 
      <img src="https://codecov.io/github/KalanaRatnayake/nano-vectordb/graph/badge.svg"/> 
    </a>
  </p>
</div>


🌬️ A vector database implementation with single-dependency (`numpy`).

🎁 It can handle a query from `100,000` vectors and return in 100 milliseconds.

🏃 It's okay for your prototypes, maybe even more.

🏃 Support naive [multi-tenancy](#Multi-Tenancy).


## [New] C++ Support

### Requirements

- C++17 or later
- [Eigen](https://eigen.tuxfamily.org/) (vector math)
- [OpenSSL](https://www.openssl.org/) (base64 encoding)
- [nlohmann/json](https://github.com/nlohmann/json) (JSON serialization)


### Build & Test

```sh
sudo apt-get update
sudo apt-get install -y g++ libeigen3-dev libssl-dev
wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp -O /usr/local/include/json.hpp
make test
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

## [Original] Python

### Install

**Install from PyPi**

```shell
pip install nano-vectordb
```

**Install from source**

```shell
# clone this repo first
cd nano-vectordb
pip install -e .
```

### Quick Start

**Faking your data**:

```python
from nano_vectordb import NanoVectorDB
import numpy as np

data_len = 100_000
fake_dim = 1024
fake_embeds = np.random.rand(data_len, fake_dim)    

fakes_data = [{"__vector__": fake_embeds[i], **ANYFIELDS} for i in range(data_len)]
```

You can add any fields to a data. But there are two keywords:

- `__id__`: If passed, `NanoVectorDB` will use your id, otherwise a generated id will be used.
- `__vector__`: must pass, your embedding `np.ndarray`.

### Init a DB

```python
vdb = NanoVectorDB(fake_dim, storage_file="fool.json")
```

Next time you init `vdb` from `fool.json`, `NanoVectorDB` will load the index automatically.

### Upsert

```python
r = vdb.upsert(fakes_data)
print(r["update"], r["insert"])
```

### Query

```python
# query with embedding 
vdb.query(np.random.rand(fake_dim))

# arguments:
vdb.query(np.random.rand(fake_dim), top_k=5, better_than_threshold=0.01)
```

#### Conditional filter

```python
vdb.query(np.random.rand(fake_dim), filter_lambda=lambda x: x["any_field"] == "any_value")
```

### Save

```python
# will create/overwrite 'fool.json'
vdb.save()
```

### Get, Delete

```python
# get and delete the inserted data
print(vdb.get(r["insert"]))
vdb.delete(r["insert"])
```

### Additional Data

```python
vdb.store_additional_data(a=1, b=2, c=3)
print(vdb.get_additional_data())
```

## Multi-Tenancy

If you have multiple vectorDB to use, you can use `MultiTenantNanoVDB` to manage:

```python
from nano_vectordb import NanoVectorDB, MultiTenantNanoVDB

multi_tenant = MultiTenantNanoVDB(1024)
tenant_id = multi_tenant.create_tenant()

# tenant is a NanoVectorDB, you can upsert, query, get... on this.
tenant: NanoVectorDB = multi_tenant.get_tenant(tenant_id)

# some chores:
multi_tenant.delete_tenant(tenant_id)
multi_tenant.contain_tenant(tenant_id)

# save it
multi_tenant.save()
```

`MultiTenantNanoVDB` use a queue to manage the total vector dbs in memory, you can adjust the parameter:

```python
# There will be only `max_capacity` NanoVectorDB in the memory.
multi_tenant = MultiTenantNanoVDB(1024, max_capacity=1)
```

## Benchmark

> Embedding Dim: 1024. Device: MacBook M3 Pro

- Save a index with `100,000` vectors will generate a roughly 520M json file.
- Insert `100,000` vectors will cost roughly `2`s
- Query from `100,000` vectors will cost roughly `0.1`s
