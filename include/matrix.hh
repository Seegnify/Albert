#ifndef _DL_MATRIX_H_
#define _DL_MATRIX_H_

#include "context.hh"

template <typename B, template <typename B> class M>
class Matrix {
  public:

    // default ctor
    Matrix(Context<B,M>& ctx,
    std::size_t rows, std::size_t cols = 1) : _ctx(ctx) {
      _mtx = _ctx.get_matrix(rows, cols);
    }

    // move ctor
    Matrix(Matrix&& m) : _ctx(m._ctx) {
      _mtx = m._mtx;
      m._mtx = NULL;
    }

    // copy ctor
    Matrix(const Matrix& m) : _ctx(m._ctx) {
      _mtx = _ctx.get_matrix(m.rows(), m.cols());
      *_mtx = *m._mtx;
    }

    // virtual dtor
    virtual ~Matrix() {
      if (_mtx != NULL) {
        _ctx.put_matrix(_mtx);
      }
    };

    // print matrix
    void print(std::ostream& out) const {
      _ctx.print(*_mtx, out);
    }

    // move assignment
    Matrix& operator=(Matrix&& m) {
      _ctx.put_matrix(_mtx);
      _mtx = m._mtx;
      m._mtx = NULL;
    }

    // copy assignment
    Matrix& operator=(const Matrix& m) {
      _ctx.put_matrix(_mtx);
      _mtx = _ctx.get_matrix(m.rows(), m.cols());
      *_mtx = *m._mtx;
    }

    // matrix context
    Context<B,M>& context() const {
      return _ctx;
    }

    // row count
    int rows() const {
      return _ctx.rows(*_mtx);
    }

    // col count
    int cols() const {
      return _ctx.cols(*_mtx);
    }

    // set
    void set(B v) {
      _ctx.set(*_mtx, v);
    }

    // set operator
    Matrix& operator=(B v) {
      set(v);
      return *this;
    }

    // set
    void set(const std::vector<B> &v) {
      _ctx.set(*_mtx, v);
    }

    // set operator
    Matrix& operator=(const std::vector<B>& v) {
      set(v);
      return *this;
    }

    // get
    void get(std::vector<B> &v) const {
      _ctx.get(*_mtx, v);
    }

    // get operator
    operator std::vector<B>() const {
      std::vector<B> v;
      get(v);
      return v;
    }

    // add
    Matrix operator+(const Matrix& m) const {
      Matrix r(_ctx, rows(), m.cols());
      _ctx.add(*_mtx, *m._mtx, *r._mtx);
      return r;
    }

    // subtract
    Matrix operator-(const Matrix& m) const {
      Matrix r(_ctx, rows(), m.cols());
      _ctx.sub(*_mtx, *m._mtx, *r._mtx);
      return r;
    }

    // matrix multiply
    Matrix operator*(const Matrix& m) const {
      if (rows() == 1 && cols() == 1) {
        Matrix r(_ctx, m.rows(), m.cols());
        _ctx.mul(*m._mtx, S(), *r._mtx);
        return r;
      }
      else
      if (m.rows() == 1 && m.cols() == 1) {
        Matrix r(_ctx, rows(), cols());
        _ctx.mul(*_mtx, m.S(), *r._mtx);
        return r;
      }
      else {
        Matrix r(_ctx, rows(), m.cols());
        _ctx.prod(*_mtx, *m._mtx, *r._mtx);
        return r;
      }
    }

    // scalar multiply
    Matrix operator*(B s) const {
      Matrix r(_ctx, rows(), cols());
      _ctx.mul(*_mtx, s, *r._mtx);
      return r;
    }

    // element multiply
    Matrix operator&(const Matrix& m) const {
      if (rows() == 1 && cols() == 1) {
        Matrix r(_ctx, m.rows(), m.cols());
        _ctx.mul(*m._mtx, S(), *r._mtx);
        return r;
      }
      else
      if (m.rows() == 1 && m.cols() == 1) {
        Matrix r(_ctx, rows(), cols());
        _ctx.mul(*_mtx, m.S(), *r._mtx);
        return r;
      }
      else {
        Matrix r(_ctx, rows(), m.cols());
        _ctx.mul(*_mtx, *m._mtx, *r._mtx);
        return r;
      }
    }

    // exponent
    Matrix E() const {
      Matrix r(_ctx, rows(), cols());
      _ctx.exponent(*_mtx, *r._mtx);
      return r;
    }

    // transpose
    Matrix T() const {
      Matrix r(_ctx, cols(), rows());
      _ctx.transpose(*_mtx, *r._mtx);
      return r;
    }

    // summation
    B S() const {
      return _ctx.summation(*_mtx);
    }

  private:
    M<B>* _mtx;
    Context<B,M>& _ctx;
};

#endif /*_DL_MATRIX_H_*/
