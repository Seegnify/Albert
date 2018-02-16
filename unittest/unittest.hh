#ifndef _DL_UNITTEST_H_
#define _DL_UNITTEST_H_

#include <iostream>

class UnitTest {
  private:
    bool done;
    std::ostringstream log;

  public:
    UnitTest(const char* text) {
      done = false;
      std::cout << "test [" << text << "]: ";
    }

    ~UnitTest() {
      if (!done) {
        std::cout << "UNFINISHED" << std::endl;
      }
    }

    void finish() {
      std::string log_str = log.str();
      if (log_str.size() == 0) {
        std::cout << "OK" << std::endl;
      }
      else {
        std::cout << "FAILED" << std::endl;
        std::cout << log_str;
      }
      done = true;
    }

    void assert_true(bool expr, int line, const char* file) {
      if (!expr) {
        log << "test failure: ";
        log << line << ":" << file << std::endl;
      }
    }
};

#define TEST_BEGIN(text) \
  UnitTest _ut(text);

#define TEST_END() \
  _ut.finish();

#define ASSERT(expr) \
  _ut.assert_true((expr), __LINE__, __FILE__);

#endif /*_DL_UNITTEST_H_*/

