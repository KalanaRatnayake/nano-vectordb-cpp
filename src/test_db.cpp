#include "NanoVectorDB.hpp"
#include "MultiTenantNanoVDB.hpp"
#include <iostream>
#include <random>
#include <cassert>
#include <filesystem>

using namespace nano_vectordb;

Vector random_vector(int dim)
{
  static std::mt19937 gen{ std::random_device{}() };
  static std::uniform_real_distribution<Float> dist(0.0, 1.0);
  Vector v(dim);
  for (int i = 0; i < dim; ++i)
    v[i] = dist(gen);
  return v;
}

void test_init()
{
  std::cerr << "[test_init] START" << std::endl;
  int data_len = 1000;
  int fake_dim = 1024;
  NanoVectorDB a(fake_dim);
  std::cerr << "[test_init] Created NanoVectorDB a, fake_dim=" << fake_dim << std::endl;
  std::vector<Data> fakes_data;
  for (int i = 0; i < data_len; ++i)
  {
    fakes_data.push_back({ std::to_string(i), random_vector(fake_dim) });
  }
  std::cerr << "[test_init] Created fakes_data, size=" << fakes_data.size() << std::endl;
  Vector query_data = fakes_data[data_len / 2].vector;
  std::cerr << "[test_init] Calling a.upsert(fakes_data)" << std::endl;
  a.upsert(fakes_data);
  std::cerr << "[test_init] Calling a.save()" << std::endl;
  a.save();
  NanoVectorDB b(fake_dim);
  std::cerr << "[test_init] Created NanoVectorDB b" << std::endl;
  std::cerr << "[test_init] Calling b.query()" << std::endl;
  auto results = b.query(query_data, 10, 0.01);
  assert(results[0].data.id == std::to_string(data_len / 2));
  assert(results.size() <= 10);
  for (const auto& d : results)
    assert(d.score >= 0.01);
  std::filesystem::remove("nano-vectordb.json");
  std::cerr << "[test_init] END" << std::endl;
}

void test_same_upsert()
{
  std::cerr << "[test_same_upsert] START" << std::endl;
  int data_len = 1000;
  int fake_dim = 1024;
  NanoVectorDB a(fake_dim);
  std::vector<Data> fakes_data;
  for (int i = 0; i < data_len; ++i)
    fakes_data.push_back({ "", random_vector(fake_dim) });
  a.upsert(fakes_data);
  std::vector<Data> fakes_data2;
  for (int i = 0; i < data_len; ++i)
    fakes_data2.push_back({ "", random_vector(fake_dim) });
  a.upsert(fakes_data2);
  std::cerr << "[test_same_upsert] END" << std::endl;
}

void test_get()
{
  std::cerr << "[test_get] START" << std::endl;
  NanoVectorDB a(1024);
  std::vector<Data> fakes_data;
  for (int i = 0; i < 100; ++i)
    fakes_data.push_back({ std::to_string(i), random_vector(1024) });
  a.upsert(fakes_data);
  auto r = a.get({ "0", "1", "2" });
  assert(r.size() == 3);
  std::cerr << "[test_get] END" << std::endl;
}

void test_delete()
{
  std::cerr << "[test_delete] START" << std::endl;
  NanoVectorDB a(1024);
  std::vector<Data> fakes_data;
  for (int i = 0; i < 100; ++i)
    fakes_data.push_back({ std::to_string(i), random_vector(1024) });
  a.upsert(fakes_data);
  a.remove({ "0", "50", "90" });
  auto r = a.get({ "0", "50", "90" });
  assert(r.size() == 0);
  assert(a.size() == 97);
  std::cerr << "[test_delete] END" << std::endl;
}

void test_cond_filter()
{
  std::cerr << "[test_cond_filter] START" << std::endl;
  int data_len = 10;
  int fake_dim = 1024;
  NanoVectorDB a(fake_dim);
  std::vector<Data> fakes_data;
  for (int i = 0; i < data_len; ++i)
    fakes_data.push_back({ std::to_string(i), random_vector(fake_dim) });
  a.upsert(fakes_data);
  auto query_data = fakes_data[data_len / 2].vector;
  auto results = a.query(query_data, 10, 0.01);
  assert(results[0].data.id == std::to_string(data_len / 2));
  auto cond_filter = [](const Data& x) { return x.id == "1"; };
  auto filtered = a.query(query_data, 10, std::nullopt, cond_filter);
  assert(filtered[0].data.id == "1");
  std::cerr << "[test_cond_filter] END" << std::endl;
}

void test_additional_data()
{
  std::cerr << "[test_additional_data] START" << std::endl;
  int fake_dim = 1024;
  NanoVectorDB a(fake_dim);
  nlohmann::json add_data = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
  a.store_additional_data(add_data);
  a.save();
  NanoVectorDB b(fake_dim);
  assert(b.get_additional_data() == add_data);
  std::filesystem::remove("nano-vectordb.json");
  std::cerr << "[test_additional_data] END" << std::endl;
}

void test_multi_tenant()
{
  std::cerr << "[test_multi_tenant] START" << std::endl;
  try
  {
    MultiTenantNanoVDB multi_tenant(1024, "cosine", 0);
    assert(false);  // Should throw
  }
  catch (...)
  {
  }
  MultiTenantNanoVDB multi_tenant(1024);
  std::string tenant_id = multi_tenant.create_tenant();
  auto tenant = multi_tenant.get_tenant(tenant_id);
  nlohmann::json add_data = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
  tenant->store_additional_data(add_data);
  multi_tenant.save();
  MultiTenantNanoVDB multi_tenant2(1024);
  assert(multi_tenant2.contain_tenant(tenant_id));
  auto tenant2 = multi_tenant2.get_tenant(tenant_id);
  assert(tenant2->get_additional_data() == add_data);
  try
  {
    multi_tenant2.get_tenant("1");
    assert(false);
  }
  catch (...)
  {
  }
  MultiTenantNanoVDB multi_tenant3(1024, "cosine", 1);
  multi_tenant3.create_tenant();
  multi_tenant3.get_tenant(tenant_id);
  multi_tenant3.delete_tenant(tenant_id);
  MultiTenantNanoVDB multi_tenant4(1024);
  assert(!multi_tenant4.contain_tenant(tenant_id));
  std::filesystem::remove_all("nano_multi_tenant_storage");
  MultiTenantNanoVDB multi_tenant5(1024, "cosine", 1);
  multi_tenant5.create_tenant();
  assert(!std::filesystem::exists("nano_multi_tenant_storage"));
  multi_tenant5.create_tenant();
  assert(std::filesystem::exists("nano_multi_tenant_storage"));
  std::filesystem::remove_all("nano_multi_tenant_storage");
  std::cerr << "[test_multi_tenant] END" << std::endl;
}

int main()
{
  std::cerr << "[main] START" << std::endl;
  try {
    test_init();
    test_same_upsert();
    test_get();
    test_delete();
    test_cond_filter();
    test_additional_data();
    test_multi_tenant();
    std::cout << "All tests passed!" << std::endl;
    std::cerr << "[main] END SUCCESS" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    std::cerr << "[main] END EXCEPTION" << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception occurred." << std::endl;
    std::cerr << "[main] END UNKNOWN EXCEPTION" << std::endl;
    return 1;
  }
}
