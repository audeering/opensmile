/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#include <catch2/catch.hpp>
#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <core/configManager.hpp>
#include <core/componentManager.hpp>
#include <memory>
#include <random>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <vector>

// 64-bit FNV-1a hash algorithm
// Intended for comparison of two data memory level buffers in unit tests.
class cHasher {
private:
  static constexpr uint64_t OFFSET_BASIS = 14695981039346656037u;
  static constexpr uint64_t PRIME = 1099511628211u;

  uint64_t state_ = OFFSET_BASIS;

public:
  void feed(const void *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
      state_ ^= ((uint8_t *)data)[i];
      state_ *= PRIME;
    }
  }

  void feed(const cVector &vec) {
    feed(vec.data, sizeof(FLOAT_DMEM) * vec.N);
  }

  void feed(const cMatrix &mat) {
    feed(mat.data, sizeof(FLOAT_DMEM) * mat.nT * mat.N);
  }

  void reset() { 
    state_ = OFFSET_BASIS;
  }

  uint64_t hash() const { return state_; }

  static uint64_t hash(const void *data, size_t length) {
    cHasher hasher;
    hasher.feed(data, length);
    return hasher.hash();
  }

  static uint64_t hash(const cMatrix &mat) {
    cHasher hasher;
    hasher.feed(mat);
    return hasher.hash();
  }
};

class cRandom {
private:
  std::mt19937 mt;

public:
  template <typename T, size_t length>
  void fill(T *array) {
    for (size_t i = 0; i < length; i++) {
      array[i] = mt();
    }
  }

  void fill(cVector &vec) {
    for (long i = 0; i < vec.N; i++) {
      vec.data[i] = mt();
    }
  }

  void fill(cMatrix &mat) {
    for (long i = 0; i < mat.nT * mat.N; i++) {
      mat.data[i] = mt();
    }
  }
};

// define string representations for cVector and cMatrix, used by Catch2 in log and error messages

static std::ostream &operator<<(std::ostream &stream, const cVector &vec) {
  if (vec.N == 0) return stream << "[]";
  else if (vec.N == 1) return stream << "[" << vec.data[0] << "]";
  else if (vec.N == 2) return stream << "[" << vec.data[0] << ", " << vec.data[1] << "]";
  else return stream << "[" << vec.data[0] << ", " << vec.data[1] << ", " << vec.N - 2 << " more values...]";
}

static std::ostream &operator<<(std::ostream &stream, const cMatrix &mat) {
  if (mat.nT == 0) return stream << "[]";
  else if (mat.nT == 1) {
    cVector *col0 = mat.getCol(0);
    std::ostream &s = stream << "[" << *col0 << "]";
    delete col0;
    return s;
  } else if (mat.nT == 2) {
    cVector *col0 = mat.getCol(0);
    cVector *col1 = mat.getCol(1);
    std::ostream &s = stream << "[" << *col0 << ", " << *col1 << "]";
    delete col0;
    delete col1;
    return s;
  } else {
    cVector *col0 = mat.getCol(0);
    cVector *col1 = mat.getCol(1);
    std::ostream &s = stream << "[" << *col0 << ", " << *col1 << ", " << mat.nT - 2 << " more columns...]";
    delete col0;
    delete col1;
    return s;
  }
}

class EqualsVectorMatcher : public Catch::MatcherBase<cVector> {
private:
  const cVector &vec;

public:
  EqualsVectorMatcher(const cVector &vec) : vec(vec) {}

  virtual bool match(const cVector& other) const override {
    if (vec.N != other.N) return false;

    return memcmp(vec.data, other.data, sizeof(FLOAT_DMEM) * vec.N) == 0;
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "is equal to " << vec;
    return ss.str();
  }
};

inline EqualsVectorMatcher EqualsVector(const cVector &vec) {
  return EqualsVectorMatcher(vec);
}

// Matcher checking whether two cVector are almost equal 
// (to account for rounding errors)
class WithinAbsVectorMatcher : public Catch::MatcherBase<cVector> {
private:
  FLOAT_DMEM margin;
  const cVector &vec;

public:
  WithinAbsVectorMatcher(const cVector &vec, FLOAT_DMEM margin) : vec(vec), margin(margin) {}

  virtual bool match(const cVector& other) const override {
    if (vec.N != other.N) return false;

    for (int i = 0; i < vec.N; i++)
      if (std::fabs(vec.data[i] - other.data[i]) > margin)
        return false;

    return true;
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "is within " << ::Catch::Detail::stringify(margin) << " to " << vec;
    return ss.str();
  }
};

inline WithinAbsVectorMatcher WithinAbsVector(const cVector &vec, FLOAT_DMEM margin) {
  return WithinAbsVectorMatcher(vec, margin);
}

class EqualsMatrixMatcher : public Catch::MatcherBase<cMatrix> {
private:
  const cMatrix &mat;

public:
  EqualsMatrixMatcher(const cMatrix &mat) : mat(mat) {}

  virtual bool match(const cMatrix& other) const override {
    if (mat.N != other.N) return false;
    if (mat.nT != other.nT) return false;

    return memcmp(mat.data, other.data, sizeof(FLOAT_DMEM) * mat.N * mat.nT) == 0;
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "is equal to " << mat;
    return ss.str();
  }
};

inline EqualsMatrixMatcher EqualsMatrix(const cMatrix &mat) {
  return EqualsMatrixMatcher(mat);
}

// Matcher checking whether two cMatrix are almost equal 
// (to account for rounding errors)
class WithinAbsMatrixMatcher : public Catch::MatcherBase<cMatrix> {
private:
  FLOAT_DMEM margin;
  const cMatrix &mat;

public:
  WithinAbsMatrixMatcher(const cMatrix &mat, FLOAT_DMEM margin) : mat(mat), margin(margin) {}

  virtual bool match(const cMatrix& other) const override {
    if (mat.N != other.N) return false;

    for (int i = 0; i < mat.N * mat.nT; i++)
      if (std::fabs(mat.data[i] - other.data[i]) > margin)
        return false;

    return true;
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "is within " << ::Catch::Detail::stringify(margin) << " to " << mat;
    return ss.str();
  }
};

inline WithinAbsMatrixMatcher WithinAbsMatrix(const cMatrix &mat, FLOAT_DMEM margin) {
  return WithinAbsMatrixMatcher(mat, margin);
}
