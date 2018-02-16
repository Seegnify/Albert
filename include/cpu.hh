#ifndef _DL_CPU_H_
#define _DL_CPU_H_

#include <eigen3/Eigen/Dense>

#include "matrix.hh"

// CPU implementation with Eigen

template<typename T> using CPUMatrix = Eigen::Matrix
<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

template<typename T>
class CPUContext : public Context<T, CPUMatrix> {
  public:
    CPUMatrix<T>*
    create(std::size_t rows, std::size_t cols) const {
      return new CPUMatrix<T>(rows, cols);
    }

    std::size_t
    rows(const CPUMatrix<T>& a) const {
      return a.rows();
    }

    std::size_t
    cols(const CPUMatrix<T>& a) const {
      return a.cols();
    }

    void
    set(CPUMatrix<T>& r, const std::vector<T>& s) const {
      int rows = r.rows();
      int cols = r.cols();
      std::memcpy(r.data(), s.data(), sizeof(T) * rows * cols);
    }

    void
    get(const CPUMatrix<T>& s, std::vector<T>& r) const {
      int rows = s.rows();
      int cols = s.cols();
      r.resize(rows * cols);
      std::memcpy(r.data(), s.data(), sizeof(T) * rows * cols);
    }

    void
    set(CPUMatrix<T>& r, T v) const {
      r.setConstant(v);
    }

    void
    add(const CPUMatrix<T>& a, const CPUMatrix<T>& b, CPUMatrix<T>& r) const {
      if (a.rows() == b.rows() && a.cols() == b.cols()) {
        r.noalias() = a + b;
      }
      else {
        this->on_error("dimension mismatch in matrix addition");
      }
    }

    void
    sub(const CPUMatrix<T>& a, const CPUMatrix<T>& b, CPUMatrix<T>& r) const {
      if (a.rows() == b.rows() && a.cols() == b.cols()) {
        r.noalias() = a - b;
      }
      else {
        this->on_error("dimension mismatch in matrix subtracion");
      }
    }

    void
    prod(const CPUMatrix<T>& a, const CPUMatrix<T>& b, CPUMatrix<T>& r) const {
      if (a.cols() == b.rows()) {
        r.noalias() = a * b;
      }
      else {
        this->on_error("dimension mismatch in matrix-product multiplication");
      }
    }

    void
    mul(const CPUMatrix<T>& a, T s, CPUMatrix<T>& r) const {
      r.noalias() = a * s;
    }

    void
    mul(
    const CPUMatrix<T>& a, const CPUMatrix<T>& b, CPUMatrix<T>& r) const {
      if (a.cols() == b.cols() && a.rows() == b.rows()) {
        r = a.array() * b.array();
      }
      else {
        this->on_error("dimension mismatch in matrix-element multiplication");
      }
    }

    void
    exponent(const CPUMatrix<T>& a, CPUMatrix<T>& r) const {
      r.noalias() = a.array().exp().matrix();
    }

    void
    transpose(const CPUMatrix<T>& a, CPUMatrix<T>& r) const {
      r.noalias() = a.transpose();
    }

    T
    summation(const CPUMatrix<T>& a) const {
      return a.array().sum();
    }
};

#endif /*_DL_MATRIX_H_*/
