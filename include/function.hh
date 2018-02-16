#ifndef _DL_FUNCTION_H_
#define _DL_FUNCTION_H_

#include "matrix.hh"

template<typename T, template <typename> class M>
class Function {
  public:
    Function() {
      _cache = false;
      _value = NULL;
    }

    virtual ~Function() { delete _value; }

    virtual const Matrix<T,M>& forward() = 0;
    virtual void backward(const Matrix<T,M>& d) = 0;
    virtual void refresh(bool deep) { _cache = false; }

  protected:
    bool _cache;
    Matrix<T,M>* _value;
};

template<typename T, template <typename> class M>
class Variable : public Function<T,M>  {
  public:
    Variable(Matrix<T,M>* value = NULL) {
      this->_value = value;
      _derivative = NULL;
    }

    virtual ~Variable() { delete _derivative; }

    const Matrix<T,M>& forward() {
      return value();
    }

    void backward(const Matrix<T,M>& d) {
      if (_derivative == NULL) {
        _derivative = new Matrix<T,M>(d.context(), d.rows(), d.cols());
        _derivative->set(0);
      }
      *_derivative = *_derivative + d;
    }

    Matrix<T,M>* set(Matrix<T,M>* value) {
      auto prev = this->_value;
      this->_value = value;
      return prev;
    }

    Matrix<T,M>& value() {
      if (this->_value != NULL) {
        return *this->_value;
      }
      else {
        throw std::runtime_error("Variable is not set.");
      }
    }

    Matrix<T,M>& derivative() {
      if (_derivative != NULL) {
        return *_derivative;
      }
      else {
        throw std::runtime_error("Derivative is not set.");
      }
    }

  protected:
    Matrix<T,M>* _derivative;
};

template<typename T, template <typename> class M>
class Constant : public Variable<T,M>  {
  public:
    Constant(Matrix<T,M>* value = NULL) : Variable<T,M> (value) {}

    void backward(const Matrix<T,M>& d) {
      if (this->_derivative == NULL) {
        this->_derivative = new Matrix<T,M>(d.context(), d.rows(), d.cols());
        this->_derivative->set(0);
      }
    }
};

template<typename T, template <typename> class M>
class UnaryOperator : public Function<T,M>  {
  public:
    UnaryOperator(Function<T,M>* f) { _function = f; }

    const Matrix<T,M>& forward() {
      if (this->_value == NULL) {
        this->_value = new Matrix<T,M>(compute());
      }
      else
      if (this->_cache == false) {
        *this->_value = compute();
      }
      this->_cache = true;
      return *this->_value;
    }

    void refresh(bool deep) {
      Function<T,M>::refresh(deep);
      if (deep) {
        _function->refresh(deep);
      }
    }

    virtual Matrix<T,M> compute() = 0;

  protected:
    Function<T,M>* _function;
};

template<typename T, template <typename> class M>
class BinaryOperator : public Function<T,M>  {
  public:
    BinaryOperator(Function<T,M>* l, Function<T,M>* r) {
      _lfunction = l;
      _rfunction = r;
    }

    const Matrix<T,M>& forward() {
      if (this->_value == NULL) {
        this->_value = new Matrix<T,M>(compute());
      }
      else
      if (this->_cache == false) {
        *this->_value = compute();
      }
      this->_cache = true;
      return *this->_value;
    }

    void refresh(bool deep) {
      Function<T,M>::refresh(deep);
      if (deep) {
        _lfunction->refresh(deep);
        _rfunction->refresh(deep);
      }
    }

    virtual Matrix<T,M> compute() = 0;

  protected:
    Function<T,M>* _lfunction;
    Function<T,M>* _rfunction;
};

template<typename T, template <typename> class M>
class Exponent : public UnaryOperator<T,M> {
  public:
    Exponent(Function<T,M>* f) : UnaryOperator<T,M>(f) {}

    // f(a) = exp(a)
    Matrix<T,M> compute() {
      return this->_function->forward().E();
    }

    // dE/da = dE/df * df/da = d * exp(a)
    void backward(const Matrix<T,M>& d) {
      this->_function->backward(d & *this->_value);
    }
};

template<typename T, template <typename> class M>
class Transpose : public UnaryOperator<T,M> {
  public:
    Transpose(Function<T,M>* f) : UnaryOperator<T,M>(f) {}

    // f(a) = T(a)
    Matrix<T,M> compute() {
      return this->_function->forward().T();
    }

    // dE/da = dE/df * df/da = d * I
    void backward(const Matrix<T,M>& d) {
      this->_function->backward(d.T());
    }
};

template<typename T, template <typename> class M>
class Summation : public UnaryOperator<T,M>  {
  public:
    Summation(Function<T,M>* f) : UnaryOperator<T,M>(f) {}

    // f(a) = S(a)
    Matrix<T,M> compute() {
      auto& value = this->_function->forward();
      Matrix<T,M> m(value.context(), 1, 1);
      m.set(value.S());
      return m;
    }

    // dE/da = dE/df * df/da = d * I
    void backward(const Matrix<T,M>& d) {
      auto& value = this->_function->forward();
      Matrix<T,M> m(d.context(), value.rows(), value.cols());
      m.set(d.S());
      this->_function->backward(m);
    }
};


template<typename T, template <typename> class M>
class Addition : public BinaryOperator<T,M>  {
  public:
    Addition(Function<T,M>* l, Function<T,M>* r) :
    BinaryOperator<T,M>(l, r) {}

    // f(l, r) = l + r
    Matrix<T,M> compute() {
      // no need to store the values for derivatives
      return this->_lfunction->forward() + this->_rfunction->forward();
    }

    // dE/dl = dE/df * df/dl = d * I
    // dE/dr = dE/df * df/dr = d * I
    void backward(const Matrix<T,M>& d) {
      this->_lfunction->backward(d);
      this->_rfunction->backward(d);
    }
};

template<typename T, template <typename> class M>
class Subtraction : public BinaryOperator<T,M>  {
  public:
    Subtraction(Function<T,M>* l, Function<T,M>* r) :
    BinaryOperator<T,M>(l, r) {}

    // f(l, r) = l - r
    Matrix<T,M> compute() {
      return this->_lfunction->forward() - this->_rfunction->forward();
    }

    // dE/dl = dE/df * df/dl = d * I
    // dE/dr = dE/df * df/dr = d * (-I)
    void backward(const Matrix<T,M>& d) {
      this->_lfunction->backward(d);
      this->_rfunction->backward(d * (-1.0));
    }
};

template<typename T, template <typename> class M>
class Product : public BinaryOperator<T,M>  {
  public:
    Product(Function<T,M>* l, Function<T,M>* r) :
    BinaryOperator<T,M> (l, r) {}

    // f(l, r) = l * r
    Matrix<T,M> compute() {
      return this->_lfunction->forward() * this->_rfunction->forward();
    }

    // dE/dl = dE/df * df/dl = d * r
    // dE/dr = dE/df * df/dr = d * l
    void backward(const Matrix<T,M>& d) {
      this->_lfunction->backward((this->_rfunction->forward() * d).T());
      this->_rfunction->backward((d * this->_lfunction->forward()).T());
    }
};

template<typename T, template <typename> class M>
class Element : public BinaryOperator<T,M>  {
  public:
    Element(Function<T,M>* l, Function<T,M>* r) :
    BinaryOperator<T,M> (l, r) {}

    // f(l, r) = l & r
    Matrix<T,M> compute() {
      return this->_lfunction->forward() & this->_rfunction->forward();
    }

    // dE/dl = dE/df * df/dl = d * r
    // dE/dr = dE/df * df/dr = d * l
    void backward(const Matrix<T,M>& d) {
      this->_lfunction->backward(this->_rfunction->forward() & d);
      this->_rfunction->backward(d & this->_lfunction->forward());
    }
};

#endif /*_DL_FUNCTION_H_*/
