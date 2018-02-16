#ifndef _DL_LIBRARY_
#define _DL_LIBRARY_

#include <unordered_map>
#include <iostream>

#include "function.hh"

// standar operator types, enum values are persisted - do not change them
enum OperatorType {
  FUNCTION,     // 0) generic user defined function
  VARIABLE,     // 1) variable for weights learning
  CONSTANT,     // 2) constant for function input
  ADDITION,     // 3) function +
  SUBTRACTION,  // 4) function -
  PRODUCT,      // 5) matrix-wise *
  ELEMENT,      // 6) element-wise *
  TRANSPOSE,    // 7) transpose
  EXPONENT,     // 8) exponent
};

class Definition {
  public:
    const std::string& get_name() const { return _name; }

    void set_name(const std::string& name) { _name = name; }

    const std::string& get_name(int id) const { return _names[id]; }

    const std::vector<int>& variables() const { return _variables; }

    const std::vector<int>& constants() const { return _constants; }

    bool recurrent() const { return _recurrent; }

    int id(const std::string& name) const {
      auto&& it = _import_index.find(name);
      if (it != _import_index.end()) {
        return it->second;
      }
      return -1;
    }

    // get sequential record, return size of the read record
    int get_record(int& offset, OperatorType& type, int& variant, int& id,
    std::vector<int>& input, std::vector<int>& times) const {

      // validate offset
      if (offset >= _definition.size()) {
        return -1;
      }

      // TYPE, VARIANT, ID, ARG_NUM
      type = (OperatorType)_definition[offset++];
      variant = _definition[offset++];
      id = _definition[offset++];
      int arg_num = _definition[offset++];

      // [ARG_1,...,ARG_N]
      for (int i=0; i<arg_num; i++, offset++) {
        input.push_back(_definition[offset]);
      }

      // [TIME_1,...,TIME_N]
      for (int i=0; i<arg_num; i++, offset++) {
        times.push_back(_definition[offset]);
      }

      return arg_num + arg_num + 4;
    }

    // Get definition by import variant id
    const Definition* get_import(int id) const {
      if (id >= 0 && id < _import_defs.size()) {
        return _import_defs[id];
      }
      else {
        std::ostringstream error;
        error << "Import definition out of range. ";
        if (id < 0) error << "Negative index.";
        else error << "Index " << id << " >= size " << _import_defs.size();
        error << ".";
        throw std::runtime_error(error.str());
      }
    }

    // Definition pointer is managed outside by Dictionary
    void add_import(const char* name, Definition* import) {
      // check if import is already defined
      auto&& it = _import_index.find(name);
      if (it != _import_index.end()) {
        std::ostringstream error;
        error << "Function '" << name << "' imported multiple times.";
        throw std::runtime_error(error.str());
      }

      // get new import ID
      int id = _import_index.size();

      // update import index
      _import_index[name] = id;

      // set import
      _import_defs.push_back(import);
    }

    void add_expression(const char* name, const char* function,
    const std::vector<const char*>& input, const std::vector<int>& times) {
      std::string fn = function;

      // check import first, then all default functions
      auto&& it = _import_index.find(fn);
      if (it != _import_index.end()) {
        add_record(FUNCTION, it->second, name, input, times);
        // update recurrent flag
        if (_import_defs[it->second]->recurrent()) {
          _recurrent = true;
        }
      }
      else if(fn == "+") {
        add_record(ADDITION, -1, name, input, times);
      }
      else if(fn == "-") {
        add_record(SUBTRACTION, -1, name, input, times);
      }
      else if(fn == "*") {
        add_record(PRODUCT, -1, name, input, times);
      }
      else if(fn == "**") {
        add_record(ELEMENT, -1, name, input, times);
      }
      else if(fn == "T") {
        add_record(TRANSPOSE, -1, name, input, times);
      }
      else if(fn == "E") {
        add_record(EXPONENT, -1, name, input, times);
      }
      else {
        // nothing matches
        std::ostringstream error;
        error << "Undefined function '" << fn << "' ";
        error << "in expression '" << name << "'.";
        throw std::runtime_error(error.str());
      }
    }

    void add_variable(const char* name) {
      std::vector<const char*> input;
      std::vector<int> times;
      _variables.push_back(_index.size());
      add_record(VARIABLE, -1, name, input, times);
    }

    void add_constant(const char* name) {
      std::vector<const char*> input;
      std::vector<int> times;
      _constants.push_back(_index.size());
      add_record(CONSTANT, -1, name, input, times);
    }

  private:
    void add_record(OperatorType type, int variant, const char* name,
    const std::vector<const char*>& input, const std::vector<int> times) {
      // validate input and time vector size
      if (input.size() != times.size()) {
        std::ostringstream error;
        error << "Mismatched input and time arguments in expression '";
        error << name << "'.";
        throw std::runtime_error(error.str());
      }

      // convert name vector to id vector
      std::vector<int> args;
      for (auto&& symbol: input) {
        auto&& it = _index.find(symbol);
        if (it != _index.end()) {
          args.push_back(it->second);
        }
        else {
          std::ostringstream error;
          error << "Undefined symbol '" << symbol << "' referenced as ";
          error << "argument in expression '" << name << "'.";
          throw std::runtime_error(error.str());
        }
      }

      // get next ID
      int id = _index.size();

      // TYPE, VARIANT, ID, ARG_NUM
      _definition.push_back(type);
      _definition.push_back(variant);
      _definition.push_back(id);
      _definition.push_back(args.size());

      // [ARG_1,...,ARG_N]
      for (auto arg: args) {
        _definition.push_back(arg);
      }

      // [TIME_1,...,TIME_N]
      for (auto time: times) {
        _definition.push_back(time);

        // update recurrent flag
        if (time < 0) {
          _recurrent = true;
        }
      }

      // update index references
      _index[name] = id;
      _names.push_back(name);
    }

    // binary function tree definition with the following block format:
    // TYPE, VARIANT, ID, ARG_NUM, [ARG_1,...,ARG_N,TIME_1,...,TIME_N]
    std::vector<int> _definition;

    // expression names: _names[id] -> name
    std::vector<std::string> _names;

    // expression index: _index[name] -> id
    std::unordered_map<std::string, int> _index;

    // import definitions: _import_defs[import id] -> Definition
    std::vector<Definition*> _import_defs;

    // import index: _import_index[import name] -> import id
    std::unordered_map<std::string, int> _import_index;

    // variable instances: _variables[index] -> id
    std::vector<int> _variables;

    // constant instances: _constants[index] -> id
    std::vector<int> _constants;

    // function name
    std::string _name;

    // recurrent function
    bool _recurrent;
};

class Dictionary {
  public:
    // clear definitions from memory
    ~Dictionary() {
      clear();
    }

    // remove all dictionary entries
    void clear() {
      for (auto&& d: _index) {
        delete d.second;
      }
      _index.clear();
    }

    // get ID matching user:library:function
    static int id(
    const std::string& user,
    const std::string& library,
    const std::string& function) {
      std::ostringstream buf;
      buf << user << ":" << library << ":" << function;
      std::string name = buf.str();
      return std::hash<std::string>()(name);
    }

    // add Definition under given user:library:function,
    // the Dictionary will delete the Definition in destructor
    void put(int id, Definition* definition) {
      _index[id] = definition;
    }

    // get Definition under given id,
    // the returned Definition is a pointer from the local index
    Definition* get(int id) {
      auto&& it = _index.find(id);
      return (it != _index.end()) ? it->second : NULL;
    }

  private:
    std::unordered_map<int, Definition*> _index;
};

template<typename T, template <typename T> class M>
class Runtime : public Function<T,M> {
  public:

    Runtime() {
      _main = NULL;
    }

    void add_expression(Function<T,M>* f) {
      _expressions.push_back(f);
      _main = f;
    }

    void add_variable(Function<T,M>* f) {
      _expressions.push_back(f);
      _variables.push_back(f);
    }

    void add_constant(Function<T,M>* f) {
      _expressions.push_back(f);
      _constants.push_back(f);
    }

    const std::vector<Function<T,M>*>& expressions() const {
      return _expressions;
    }

    const std::vector<Function<T,M>*>& variables() const {
      return _variables;
    }

    const std::vector<Function<T,M>*>& constants() const {
      return _constants;
    }

    virtual const Matrix<T,M>& forward() { return _main->forward(); }

    virtual void backward(const Matrix<T,M>& d) { _main->backward(d); }

    virtual void refresh(bool deep) {
      Function<T,M>::refresh(deep);
      if (deep) for (auto f: _expressions) f->refresh(deep);
    }

  private:
    // all runtime expressions: _instances[index] -> function
    std::vector<Function<T,M>*> _expressions;

    // variable instances: _variables[index] -> variable
    std::vector<Function<T,M>*> _variables;

    // constant instances: _constants[index] -> constant
    std::vector<Function<T,M>*> _constants;

    // main function
    Function<T,M>* _main;
};

// runtime frame: index -> runtime
template<typename T, template <typename T> class M>
using RuntimeFrame = std::vector<Runtime<T,M>*>;

template<typename T, template <typename T> class M>
class ZeroFeed : public Function<T,M> {
  public:
    ZeroFeed(Function<T,M>* delegate) {
      _delegate = delegate;
    }

    virtual const Matrix<T,M>& forward() {
      if (this->_value != NULL) {
        return *this->_value;
      }

      auto& m = _delegate->forward();
      this->_value = new Matrix<T,M>(m.context(), m.rows(), m.cols());
      this->_value->set(0);
      return *this->_value;
    }

    virtual void backward(const Matrix<T,M>& d) {}

  private:
    Function<T,M>* _delegate;
};

template<typename T, template <typename T> class M>
class Timeline {
  public:
    ~Timeline() {
      clear();
    }

    // get timeline size in time
    int space_size() const {
      return (_timeline.size() > 0) ? _timeline[0]->size() : 0;
    }

    // get timeline size in time
    int time_size() const { return _timeline.size(); }

    // clear timeline cache
    void refresh() { for (auto f: _expressions) f->refresh(false); }

    // clear all runtimes in time and space
    void clear() {
      // clear timeline
      for (auto f: _timeline) delete f;
      _timeline.clear();

      // clear all expressions
      for (auto f: _expressions) delete f;
      _expressions.clear();
    }

    // get runtime at the given time and space
    Runtime<T,M>* get_runtime(int time, int space) const {
      return (*_timeline[time])[space];
    }

    // insert new runtime at the given time frame, return space index
    int add_runtime(int time, const Dictionary& dict,
    const Definition& def, const std::vector<Function<T,M>*>& constants) {
      // create new runtime index if needed
      while (time >= _timeline.size()) {
        _timeline.push_back(new RuntimeFrame<T,M>());
      }

      // get runtime frame from timeline
      auto& rt_frame = *_timeline[time];
      int rt_index = rt_frame.size();

      // add new rt to runtime frame
      auto rt = new Runtime<T,M>();
      rt_frame.push_back(rt);
      _expressions.push_back(rt);

      // get first runtime frame from timeline
      auto& rt_zero = *_timeline[0];

      // crate runtime operators
      OperatorType type;
      int variant, id, offset = 0, constants_index = 0;
      std::vector<int> input, times;
      std::vector<Function<T,M>*> finput;
      while (def.get_record(offset, type, variant, id, input, times) > 0) {

        // resolve input
        int size = input.size();
        for (int i=0; i<size; i++) {
          // current time
          if (times[i] == 0) {
            finput.push_back(rt->expressions()[input[i]]);
          }
          // available passed time
          else if (time + times[i] >= 0) {
            auto& passed_frame = *_timeline[time + times[i]];
            auto passed_rt = passed_frame[rt_index];
            finput.push_back(passed_rt->expressions()[input[i]]);
          }
          // unavailable passed time
          else {
            auto delegate = rt->expressions()[input[i]];
            _expressions.push_back(new ZeroFeed<T,M>(delegate));
            finput.push_back(_expressions.back());
          }
        }

        // create an instance from the definition record
        switch(type) {
          case FUNCTION: {
            // recursively create user defined function
            auto child_def = def.get_import(variant);
            auto child_id = add_runtime(time, dict, *child_def, finput);
            auto child_rt = get_runtime(time, child_id);
            rt->add_expression(child_rt);
            break;
          }
          case VARIABLE:
            // take weight from timeline if available, or create one
            if (time > 0) {
              rt->add_variable(rt_zero[rt_index]->expressions()[id]);
            }
            else {
              _expressions.push_back(new Variable<T,M>());
              rt->add_variable(_expressions.back());
            }
            break;
          case CONSTANT:
            // take input from arguments if available, or create one
            if (constants.size() > constants_index) {
              rt->add_constant(constants[constants_index++]);
            }
            else {
              _expressions.push_back(new Constant<T,M>());
              rt->add_constant(_expressions.back());
            }
            break;
          case ADDITION:
            _expressions.push_back(new Addition<T,M>(finput[0], finput[1]));
            rt->add_expression(_expressions.back());
            break;
          case SUBTRACTION:
            _expressions.push_back(new Subtraction<T,M>(finput[0], finput[1]));
            rt->add_expression(_expressions.back());
            break;
          case PRODUCT:
            _expressions.push_back(new Product<T,M>(finput[0], finput[1]));
            rt->add_expression(_expressions.back());
            break;
          case ELEMENT:
            _expressions.push_back(new Element<T,M>(finput[0], finput[1]));
            rt->add_expression(_expressions.back());
          case TRANSPOSE:
            _expressions.push_back(new Transpose<T,M>(finput[0]));
            rt->add_expression(_expressions.back());
            break;
          case EXPONENT:
            _expressions.push_back(new Exponent<T,M>(finput[0]));
            rt->add_expression(_expressions.back());
            break;
          default:
            throw std::runtime_error("Unknown operator type in definition.");
          ;
        };

        // clear and get new record
        input.clear();
        times.clear();
        finput.clear();
      }

      return rt_index;
    }

    // get runtime variables
    void get_variables(const Runtime<T,M>& rt, const Definition& def,
    std::unordered_map<std::string, Function<T,M>*>& variables,
    std::vector<const char*>& path) const {

      // get runtime operators
      OperatorType type;
      int variant, id, offset = 0;
      std::vector<int> input, times;
      while (def.get_record(offset, type, variant, id, input, times) > 0) {

        // get an instance info from the definition record
        switch(type) {
          case FUNCTION: {
            // recursively get variables
            path.push_back(def.get_name(id).c_str());
            auto child_rt = static_cast<Runtime<T,M>*>(rt.expressions()[id]);
            auto child_def = def.get_import(variant);
            get_variables(*child_rt, *child_def, variables, path);
            path.pop_back();
            break;
          }
          case VARIABLE:
            // get the variable
            path.push_back(def.get_name(id).c_str());
            variables.emplace(str(path), rt.expressions()[id]);
            path.pop_back();
            break;
        };

        // clear and get new record
        input.clear();
        times.clear();
      }
    }

  protected:

      // convert definition path to string
    std::string str(const std::vector<const char*>& path) const {
      std::ostringstream p;
      for (auto it = path.begin(); it != path.end(); it++) {
        p << ((p.tellp() > 0) ? ".":"") << *it;
      }
      return p.str();
    }

  private:
    // timeline for recurrent networks
    std::vector<RuntimeFrame<T,M>*> _timeline;

    // all expression references
    std::vector<Function<T,M>*> _expressions;
};

#endif /*_DL_LIBRARY_*/
