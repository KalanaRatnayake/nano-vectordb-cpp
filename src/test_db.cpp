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
  int data_len = 1000;
  int fake_dim = 1024;
  NanoVectorDB a(fake_dim);
  std::vector<Data> fakes_data;
  for (int i = 0; i < data_len; ++i)
  {
    fakes_data.push_back({ std::to_string(i), random_vector(fake_dim) });
  }
  Vector query_data = fakes_data[data_len / 2].vector;
  a.upsert(fakes_data);
  a.save();
  NanoVectorDB b(fake_dim);
  auto results = b.query(query_data, 10, 0.01);
  assert(results[0].data.id == std::to_string(data_len / 2));
  assert(results.size() <= 10);
  for (const auto& d : results)
    assert(d.score >= 0.01);
  std::filesystem::remove("nano-vectordb.json");
}

void test_same_upsert()
{
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
}

void test_get()
{
  NanoVectorDB a(1024);
  std::vector<Data> fakes_data;
  for (int i = 0; i < 100; ++i)
    fakes_data.push_back({ std::to_string(i), random_vector(1024) });
  a.upsert(fakes_data);
  auto r = a.get({ "0", "1", "2" });
  assert(r.size() == 3);
}

void test_delete()
{
  NanoVectorDB a(1024);
  std::vector<Data> fakes_data;
  for (int i = 0; i < 100; ++i)
    fakes_data.push_back({ std::to_string(i), random_vector(1024) });
  a.upsert(fakes_data);
  a.remove({ "0", "50", "90" });
  auto r = a.get({ "0", "50", "90" });
  assert(r.size() == 0);
  assert(a.size() == 97);
}

void test_cond_filter()
{
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
}

void test_additional_data()
{
  int fake_dim = 1024;
  NanoVectorDB a(fake_dim);
  nlohmann::json add_data = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
  a.store_additional_data(add_data);
  a.save();
  NanoVectorDB b(fake_dim);
  assert(b.get_additional_data() == add_data);
  std::filesystem::remove("nano-vectordb.json");
}

void test_multi_tenant()
{
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
}

int main()
{
  test_init();
  test_same_upsert();
  test_get();
  test_delete();
  test_cond_filter();
  test_additional_data();
  test_multi_tenant();
  std::cout << "All tests passed!" << std::endl;
  return 0;
}
