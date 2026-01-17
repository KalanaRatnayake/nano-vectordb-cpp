# Metric

This document outlines distance metrics in nano-VectorDB and how to select or extend them.

## Built-in Metrics

- L2: squared Euclidean distance $\|a - b\|^2$.
- Cosine: $1 - \frac{a \cdot b}{\|a\|\,\|b\|}$.

Implementations are header-only:
- Interface: [include/metric/base.hpp](../include/metric/base.hpp)
- L2: [include/metric/l2.hpp](../include/metric/l2.hpp)
- Cosine: [include/metric/cosine.hpp](../include/metric/cosine.hpp)

## Selecting Metrics via Enums

Use the factory and enum to choose a metric:
- Factory: [include/metric/factory.hpp](include/metric/factory.hpp)
- Enum: `nano_vectordb::metric { L2, Cosine }`

### Using in NanoVectorDB

- Set metric by enum:
  - initialize_metric: `db.initialize_metric(nano_vectordb::metric::Cosine)`
- This also updates internal preprocessing (row normalization for cosine).

### Using in MultiTenant

- Set default metric for new tenants:
  - `multi.set_default_metric(nano_vectordb::metric::Cosine)`

## Adding New Metrics

1. Create a header in `include/metric/your_metric.hpp`:
	- Derive from `nano_vectordb::IMetric` and implement `distance(a, b)`.
2. Update the enum and factory:
	- Add a new value to `nano_vectordb::metric` in [include/metric/factory.hpp](include/metric/factory.hpp).
	- Add a `make()` case that returns `std::make_shared<YourMetric>()`.
3. Use it:
	- `db.initialize_metric(nano_vectordb::metric::YourMetric)`.

