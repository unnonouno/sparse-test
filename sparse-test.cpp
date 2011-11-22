#include "config.h"

#include <vector>
#include <iostream>
#include <stdint.h>

#include <pficommon/data/unordered_map.h>

#include "config.h"
#include "cmdline.h"

using namespace std;

typedef vector<pair<uint32_t, float> > sparse;
typedef vector<float> dense;

void make_vector(size_t dim, double density, sparse& v) {
  vector<uint32_t> inds;
  size_t n = static_cast<size_t>(dim * density);
  for (size_t i = 0; i < n; ++i) {
    inds.push_back(rand() % dim);
  }
  sort(inds.begin(), inds.end());
  inds.erase(unique(inds.begin(), inds.end()));
  for (size_t i = 0; i < inds.size(); ++i) {
    v.push_back(make_pair(inds[i], rand()));
  }
}

void make_weight(dense& w) {
  for (size_t i = 0; i < w.size(); ++i)
    w[i] = rand();
}

struct test {
  virtual double prod() const = 0;
};

void run(const test& t, size_t loop, size_t element_size) {
  double sum = 0;
  clock_t begin = clock();
  for (size_t i = 0; i < loop; ++i)
    sum += t.prod();
  clock_t end = clock();
  
  cout << typeid(t).name() << endl;
  cout << "result: " << sum << endl;
  double sec = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
  cout << sec << " sec." << endl;
  double elem_per_sec = element_size / sec;
  cout << elem_per_sec << " elem/sec." << endl;
  cout << endl;
}


struct std_test : public test {
  std_test(const sparse& v, const dense& w) {
    vec = v;
    weight = w;
  }

  double prod() const {
    float res = 0;
    for (sparse::const_iterator it = vec.begin(), end = vec.end();
         it != end; ++it) {
      res += it->second * weight[it->first];
    }
    return res;
  }

  sparse vec;
  dense weight;
};

struct hash_test : public test {
  hash_test(const sparse& v, const dense& w) {
    for (size_t i = 0; i < v.size(); ++i)
      vec[v[i].first] = v[i].second;
    weight = w;
  }

  double prod() const {
    float res = 0;
    for (pfi::data::unordered_map<uint32_t, float>::const_iterator it = vec.begin(), end = vec.end();
         it != end; ++it)
      res += it->second * weight[it->first];
    return res;
  }
  
  pfi::data::unordered_map<uint32_t, float> vec;
  dense weight;
};

#ifdef HAVE_EIGEN3

#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET
#include <Eigen/Sparse>

struct eigen_test : public test {
  Eigen::SparseVector<float> vec;
  Eigen::VectorXf weight;

  eigen_test(size_t dim, const sparse& v, const dense& w)
      : vec(dim), weight(dim) {
    vec.reserve(v.size());
    for (size_t i = 0; i < v.size(); ++i)
      vec.insert(v[i].first, 0) = v[i].second;

    for (size_t i = 0; i < w.size(); ++i)
      weight(i) = w[i];
  }

  double prod() const {
    return vec.dot(weight);
  }
};
#endif // HAVE_EIGEN3

#ifdef HAVE_EIGEN2

#include <Eigen/Sparse>

struct eigen_test : public test {
  Eigen::SparseVector<float> vec;
  Eigen::VectorXf weight;

  eigen_test(size_t dim, const sparse& v, const dense& w)
      : vec(dim), weight(dim) {
    vec.startFill(v.size());
    for (size_t i = 0; i < v.size(); ++i)
      vec.fill(v[i].first) = v[i].second;

    for (size_t i = 0; i < w.size(); ++i)
      weight(i) = w[i];
  }

  double prod() const {
    return vec.dot(weight);
  }
};
#endif // HAVE_EIGEN2

#ifdef HAVE_BOOST_NUMERIC_UBLAS_VECTOR_HPP

#define NDEBUG

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_sparse.hpp>

typedef boost::numeric::ublas::coordinate_vector<float> ublas_sparse;
typedef boost::numeric::ublas::vector<float> ublas_dense;

struct ublas_test : public test {
  ublas_test(size_t dim, const sparse& v, const dense& w)
      : vec(dim), weight(dim) {
    for (size_t i = 0; i < v.size(); ++i)
      vec.append_element(v[i].first, v[i].second);

    for (size_t i = 0; i < w.size(); ++i)
      weight[i] = w[i];
  }

  ublas_sparse vec;
  ublas_dense weight;
};

struct ublas_dot_test : public ublas_test {
  ublas_dot_test(size_t dim, const sparse& v, const dense& w) : ublas_test(dim, v, w) {}

  double prod() const {
    return boost::numeric::ublas::inner_prod(vec, weight);
  }
};

struct ublas_loop_test : public ublas_test {
  ublas_loop_test(size_t dim, const sparse& v, const dense& w) : ublas_test(dim, v, w) {}

  double prod() const {
    ublas_sparse::index_array_type::const_iterator idx_it = vec.index_data().begin();
    double sum = 0.f;
    for (ublas_sparse::const_iterator it = vec.begin(),
             end = vec.end(); it != end; ++it) {
      sum += (double)*it * weight[*idx_it];
      ++idx_it;
    }
    return sum;
  }
};
#endif


#ifdef HAVE_DAG_DAG_VECTOR_HPP

#include <dag/dag_vector.hpp>

class dag_iterator {
 public:
  dag_iterator(dag::dag_vector::const_iterator index,
               vector<float>::const_iterator value,
               vector<float>::const_iterator end)
      : index_(index), value_(value), end_(end) {}

  size_t index() const {
    return *index_;
  }

  float value() const {
    return *value_;
  }

  void next() {
    ++value_;
    ++index_;
  }

  bool end() const {
    return end_ == value_;
  }

 private:
  dag::dag_vector::const_iterator index_;
  vector<float>::const_iterator value_;
  vector<float>::const_iterator end_;
};

class dag_sparse {
 public:
  void push_back(size_t index, float value) {
    indexes_.push_back(index);
    values_.push_back(value);
  }

  size_t size() const {
    return values_.size();
  }

  size_t get_index(size_t i) const {
    return indexes_[i];
  }

  float get_value(size_t i) const {
    return values_[i];
  }

  dag_iterator iterator() const {
    return dag_iterator(indexes_.begin(), values_.begin(), values_.end());
  }


 private:
  dag::dag_vector indexes_;
  vector<float> values_;
};

struct dag_test : public test {
  dag_sparse vec;
  dense weight;

  dag_test(const sparse& v, const dense& w) {
    for (size_t i = 0; i < v.size(); ++i)
      vec.push_back(v[i].first, v[i].second);

    weight = w;
  }

  double prod() const {
    float res = 0;
    for (dag_iterator it(vec.iterator()); !it.end(); it.next()) {
      res += it.value() * weight[it.index()];
    }
    return res;
  }
};
#endif // HAVE_DAG_DAG_VECTOR_HPP

int main(int argc, char* argv[]) {
  cmdline::parser p;
  p.add<uint32_t>("dimension", 'n', "# of features", false, 1000000);
  p.add<float>   ("density",   'd', "density", false, 0.1);
  p.add<uint32_t>("loop",      'l', "# of loop", false, 1000);
  p.set_program_name("sparse-test");

  p.parse_check(argc, argv);

  cout << "dimension: " << p.get<uint32_t>("dimension") << endl;
  cout << "density:   " << p.get<float>("density") << endl;
  cout << "loop:      " << p.get<uint32_t>("loop") << endl;
  cout << endl;

  uint32_t dim = p.get<uint32_t>("dimension");
  float density = p.get<float>("density");
  size_t loop = p.get<uint32_t>("loop");

  sparse v;
  make_vector(dim, density, v);
  dense w(dim);
  make_weight(w);

  size_t size = v.size();

  run(std_test(v, w), loop, size);
  run(hash_test(v, w), loop, size);

#ifdef HAVE_EIGEN3 or HAVE_EIGEN2
  run(eigen_test(dim, v, w), loop, size);
#endif // HAVE_EIGEN

#ifdef HAVE_BOOST_NUMERIC_UBLAS_VECTOR_HPP
  run(ublas_dot_test(dim, v, w), loop, size);
  run(ublas_loop_test(dim, v, w), loop, size);
#endif // HAVE_BOOST_NUMERIC_UBLAS_VECTOR_HPP

#ifdef HAVE_DAG_DAG_VECTOR_HPP
  run(dag_test(v, w), loop, size);
#endif // HAVE_DAG_DAG_VECTOR_HPP
}
