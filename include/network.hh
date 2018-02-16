#ifndef _DL_NETWORK_
#define _DL_NETWORK_

#include <rapidjson/document.h>

#include "compiler.hh"

template<typename T, template <typename> class M>
class Network : public Function<T,M> {
  public:
    Network() {
      _definition = NULL;
      _time = -1;
    }

    virtual ~Network() {
      clear();
    }

    // clear network content
    void clear() {
      _timeline.clear();
      _dictionary.clear();
      _definition = NULL;
      _time = -1;
    }

    // load network definition from json
    void load(const std::string& json, Resolver& resolver) {
      // start in a clean state
      clear();

      // compile definition from json
      Compiler jsonc(resolver, "", "", "");
      _definition = jsonc.compile(json, _dictionary);

      // create initial runtime
      std::vector<Function<T,M>*> no_args;
      _timeline.add_runtime(0, _dictionary, *_definition, no_args);
      _time = 0;
    }

    // load network wights
    void load_variables(std::istream& is) {
    }

    // save network wights
    void save_variables(std::ostream& os) {
    }

    // get compiled network definition
    const Definition& definition() const {
      if (_definition)
        return *_definition;
      else
        throw std::runtime_error("Undefined Network. No definition available.");
    }

    // get input
    const std::vector<Function<T,M>*>& input() const {
      if (_timeline.time_size() > _time)
        return _timeline.get_runtime(_time, 0).constants();
      else
        throw std::runtime_error("Undefined Network. No input available.");
    }

    // get weights
    std::unordered_map<std::string, Function<T,M>*> variables() const {
      std::unordered_map<std::string, Function<T,M>*> weights;
      if (_timeline.time_size() > 0 && _definition != NULL) {
        std::vector<const char*> path;
        auto& rt = *_timeline.get_runtime(0, 0);
        _timeline.get_variables(rt, *_definition, weights, path);
      }
      return weights;
    }

    // Function::forward()
    const Matrix<T,M>& forward() {
      /*
      if (_time < _timeline.size()) {
        if (_recurrent) {
          return _timeline[_time][_main]["return"]->forward();
        }
        else {
          return _timeline[0][_main]["return"]->forward();
        }
      }

      // compile the main function for the next time step
      Compiler<T,M> c(_ctx);
      c.compile(_definitions[_main], _timeline, _definitions);
      return _timeline[_time++][_main]["return"]->forward();
      */
      Matrix<T,M>* ptr;
      return *ptr;
    }

    // Function::backward()
    void backward(const Matrix<T,M>& d) {
    /*
      if (_recurrent) {
        if (_time > 0) {
          _timeline[_time--][_main]["return"]->backward(d);
        }
        else {
          _ctx->on_error("Backward called at negative time.");
        }
      }
      else {
        _timeline[0][_main]["return"]->backward(d);
      }
      */
    }

    // Function::clear()
    void refresh(bool deep) {
    /*
      Function<T,M>::clear(deep);
      if (deep) {
        _timeline[_time++][_main]["return"]->clear(deep);
      }
      _time = 0;
      */
    }

  private:
    // main function definition
    Definition* _definition;

    // function dictionary
    Dictionary _dictionary;

    // function timeline
    Timeline<T,M> _timeline;

    // time location
    int _time;
};

#endif /*_DL_NETWORK_*/
