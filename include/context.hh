#ifndef _DL_CONTEXT_
#define _DL_CONTEXT_

#include <memory>
#include <queue>
#include <unordered_map>

template<typename T, template <typename> class M>
class Context {
  public:
    Context() {
      error_handler = NULL;
    }

    // clear all cache
    virtual ~Context() {
      auto it = matrix_cache.begin();
      while (it != matrix_cache.end()) {
        while (it->second->size() > 0) {
          auto top = it->second->front();
          it->second->pop();
          delete top;
        }
        delete it->second;
        it++;
      }
    }

    // get matrix from cache or create one if does not exist
    M<T>* get_matrix(std::size_t rows, std::size_t cols) {
      auto it = matrix_cache.find(pair_hash(rows, cols));
      if (it != matrix_cache.end() && it->second->size() > 0) {
        auto top = it->second->front();
        it->second->pop();
        return top;
      }
      else {
        return create(rows, cols);
      }
    }

    // put matrix to cache
    void put_matrix(M<T>* matrix) {
      std::size_t h = pair_hash(matrix->rows(), matrix->cols());
      auto it = matrix_cache.find(h);
      if (it == matrix_cache.end()) {
        matrix_cache.insert({h, new std::queue<M<T>*>()});
        it = matrix_cache.find(h);
      }
      it->second->push(matrix);
    }

    // get matrix count
    std::size_t get_matrix_count() {
      std::size_t count = 0;
      auto it = matrix_cache.begin();
      while (it != matrix_cache.end()) {
        count += it->second->size();
        it++;
      }
      return count;
    }

    // get matrix count
    std::size_t get_matrix_count(std::size_t rows, std::size_t cols) {
      std::size_t h = pair_hash(rows, cols);
      auto it = matrix_cache.find(h);
      if (it == matrix_cache.end()) {
        return 0;
      } else {
        return it->second->size();
      }
    }

    // print matrix
    void print(const M<T>& a, std::ostream& out) const {
      out << "[" << rows(a) << "x" << cols(a) << "]" << std::endl;
      std::vector<T> v;
      get(a, v);
      for (int r=0; r<rows(a); r++) {
        for (int c=0; c<cols(a); c++) {
          out << v[r * cols(a) + c] << ",";
        }
        out << std::endl;
      }
    }

    // set error handler
    void set_error_handler(void (*handler)(const char* msg)){
      error_handler = handler;
    }

    //
    // matrix interface
    //

    virtual M<T>* create(std::size_t rows, std::size_t cols) const = 0;

    virtual std::size_t rows(const M<T>& a) const = 0;
    virtual std::size_t cols(const M<T>& a) const = 0;

    virtual void set(M<T>& r, const std::vector<T>& s) const = 0;
    virtual void get(const M<T>& s, std::vector<T>& r) const = 0;

    virtual void set(M<T>& r, T v) const = 0;

    virtual void add(const M<T>& a, const M<T>& b, M<T>& r) const = 0;
    virtual void sub(const M<T>& a, const M<T>& b, M<T>& r) const = 0;

    virtual void prod(const M<T>& a, const M<T>& b, M<T>& r) const = 0;
    virtual void mul(const M<T>& a, T b, M<T>& r) const = 0;
    virtual void mul(const M<T>& a, const M<T>& b, M<T>& r) const = 0;

    virtual void exponent(const M<T>& a, M<T>& r) const = 0;
    virtual void transpose(const M<T>& a, M<T>& r) const = 0;

    virtual T summation(const M<T>& a) const = 0;

    //
    // error handler
    //
    void on_error(const std::string& msg) const {
      on_error(msg.c_str());
    }

    void on_error(const char* msg) const {
      if (error_handler == NULL) {
        throw std::runtime_error(msg);
      }
      else {
        error_handler(msg);
      }
    }

  private:
    // get a hash of 2D matrix size
    std::size_t pair_hash(std::size_t rows, std::size_t cols) {
      return (rows << 32) + cols;
    }

    // matrix cache keyed by size hash
    std::unordered_map<std::size_t, std::queue<M<T>*>*> matrix_cache;

    // invalid argument handler
    void (*error_handler)(const char* msg);
};

#endif /*_DL_CONTEXT_*/
