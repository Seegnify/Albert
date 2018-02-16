#ifndef _DL_COMPILER_
#define _DL_COMPILER_

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include "library.hh"

/**
 * An example of a typical network definition that this compiler can compile.
 *

  {
    "network" : {
      "name" : "foo",
      "variables" : ["x", y", "z"],
      "constants" : ["a", "b"],
      "imports" : {
        "bar" : { "user" : "joe19", "library" : "default" }
      },
      "body" : {
        "e1" : ["*", "a", "x"],
        "e2" : ["**", "b", "y"],
        "e3" : ["bar", "z"],
        "e4" : ["*", { "e2" : -1 }, "e3"],
        "return" : ["+", "e1", "e2", "e3", "e4"]
      }
    }
  }

 */

// function resolver
struct Resolver {
    virtual std::string resolve(
      const char* user,
      const char* library,
      const char* function
    ) = 0;
};

// function compiler
class Compiler {
  public:

    // create compiler
    Compiler(Resolver& resolver,
    const char* user, const char* library, const char* function) :
    _resolver(resolver) {
      _types = {
        "Null", "False", "True", "Object", "Array", "String", "Number"
      };
      _user = user;
      _library = library;
      _function = function;
    }

    // compile function from definition and function key
    Definition* compile(const std::string& json, Dictionary& dict) {
      // parse json definition
      std::stringstream jstream(json);
      rapidjson::Document doc;
      read_json(jstream, doc);

      // create empty definition
      std::unique_ptr<Definition> def(new Definition());

      // create empty JSON path
      std::vector<const char*> path;

      // compile function definition and add the runtime to the timeline
      compile_definition(doc, *def, dict, path);

      // add definition to dictionary
      int id = Dictionary::id(_user, _library, def->get_name());
      dict.put(id, def.get());

      // return compiled definition
      return def.release();
    }

  private:
  
    // parse json object from stream
    void read_json(std::istream& json, rapidjson::Document& doc) {
      rapidjson::IStreamWrapper istream(json);
      rapidjson::ParseResult res = doc.ParseStream(istream);

      if (res != true) {
        std::ostringstream error;
        error << "JSON error at offset " << res.Offset() << " ";
        error << "(0x" << std::hex << res.Offset() << std::dec << "). ";
        error << rapidjson::GetParseError_En(res.Code());
        throw std::runtime_error(error.str());
      }

      // verify JSON object
      if (!doc.IsObject()) {
        throw std::runtime_error("JSON not an object.");
      }
    }
  
    // convert definition path to string
    std::string str(const std::vector<const char*>& path) {
      std::ostringstream p;
      p << "/" << _user << "/" << _library << "/" << _function;
      for (auto it = path.begin(); it != path.end(); it++) {
        p << "/" << *it;
      }
      return p.str();
    }

    void assert_type(const rapidjson::Value& node, rapidjson::Type type,
    const std::vector<const char*>& path) {
      if(node.GetType() != type) {
        std::ostringstream error;
        error << "Unexpected JSON type '" << _types[node.GetType()];
        error << "' at '" << str(path) << "'. ";
        error << "Expected '" << _types[type] << "' type.";
        throw std::runtime_error(error.str());
      }
    }

    void assert_value(const char* value, const char* key,
    const std::vector<const char*>& path) {
      if (strlen(value) == 0) {
        std::ostringstream error;
        error << "Undefined value '"<< key << "' at '" << str(path) << "'.";
        throw std::runtime_error(error.str());
      }
    }

    void assert_element(bool present, const char* key,
    const std::vector<const char*>& path) {
      if (present == false) {
        std::ostringstream error;
        error << "Missing element '"<< key << "' at '" << str(path) << "'.";
        throw std::runtime_error(error.str());
      }
    }

    void assert_unique(const char* key, const Definition& def,
    const std::vector<const char*>& path) {
      if (def.id(key) >= 0) {
        std::ostringstream error;
        error << "Symbol '"<< key << "' at '" << str(path) << "' ";
        error << "multiply defined.";
        throw std::runtime_error(error.str());
      }
    }

    void unexpected_element(const char* key,
    const std::vector<const char*>& path) {
      std::ostringstream error;
      error << "Unexpected element '" << key << "' ";
      error << "at '" << str(path) << "'.";
      throw std::runtime_error(error.str());
    }

    void compile_definition(
    const rapidjson::Document& doc, Definition& def,
    Dictionary& dict, std::vector<const char*>& path) {
      assert_type(doc, rapidjson::kObjectType, path);

      // expected elements
      const rapidjson::Value *function = NULL;

      for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); m++) {
        // append current element to path
        const char *key = m->name.GetString();
        path.push_back(key);

        if (function == false && strcmp(key, "network") == 0) {
          // compile root element
          compile_root(m->value, def, dict, path);
          function = &m->value;
        }
        else {
          // report unexpected element
          unexpected_element(key, path);
        }

        // remove current element from path
        path.pop_back();
      }

      // assert function element
      assert_element(function, "network", path);
    }

    void compile_root(
    const rapidjson::Value& node, Definition& def,
    Dictionary& dict, std::vector<const char*>& path) {
      assert_type(node, rapidjson::kObjectType, path);

      // exptected elements
      const rapidjson::Value* name = NULL;
      const rapidjson::Value *variables = NULL;
      const rapidjson::Value *constants = NULL;
      const rapidjson::Value *imports = NULL;
      const rapidjson::Value *body = NULL;

      for (auto m = node.MemberBegin(); m != node.MemberEnd(); m++) {
        // append current element to path
        const char *key = m->name.GetString();
        path.push_back(key);

        if (name == NULL && strcmp(key, "name") == 0) {
          // get function name
          name = &m->value;
          assert_value(name->GetString(), "name", path);
          def.set_name(name->GetString());
        }
        else
        if (variables == NULL && strcmp(key, "variables") == 0) {
          // get variables
          compile_variables(m->value, def, dict, path);
          variables = &m->value;
        }
        else
        if (constants == NULL && strcmp(key, "constants") == 0) {
          // get constants
          compile_constants(m->value, def, dict, path);
          constants = &m->value;
        }
        else
        if (imports == NULL && strcmp(key, "imports") == 0) {
          // compile imports
          compile_imports(m->value, def, dict, path);
          imports = &m->value;
        }
        else
        if (body == NULL && strcmp(key, "body") == 0) {
          // get body
          body = &m->value;
        }
        else {
          // report unexpected element
          unexpected_element(key, path);
        }
        // remove current element from path
        path.pop_back();
      }

      // validate name, body
      // imports, variables and constatns are optional
      assert_element(name, "name", path);
      assert_element(body, "body", path);

      // compile body
      path.push_back("body");
      compile_body(*body, def, dict, path);
      path.pop_back();
    }

    void compile_imports(
    const rapidjson::Value& node, Definition& def,
    Dictionary& dict, std::vector<const char*>& path) {
      assert_type(node, rapidjson::kObjectType, path);

      for (auto m = node.MemberBegin(); m != node.MemberEnd(); m++) {
        // append current element to path
        path.push_back(m->name.GetString());
        compile_import(m->value, def, dict, path);
        path.pop_back();
      }
    }

    void compile_import(
    const rapidjson::Value& node, Definition& def,
    Dictionary& dict, std::vector<const char*>& path) {
      assert_type(node, rapidjson::kObjectType, path);

      // skip if imported already
      const char* function = path.back();
      if (dict.get(Dictionary::id(_user, _library, function)) != NULL) {
        return;
      }

      // exptected elements
      const char *user = NULL;
      const char *library = NULL;

      for (auto m = node.MemberBegin(); m != node.MemberEnd(); m++) {
        // append current element to path
        const char *key = m->name.GetString();
        path.push_back(key);
        assert_type(m->value, rapidjson::kStringType, path);

        if (user == NULL && strcmp(key, "user") == 0) {
          user = m->value.GetString();
        }
        else
        if (library == NULL && strcmp(key, "library") == 0) {
          library = m->value.GetString();
        }
        else {
          // report unexpected element
          unexpected_element(key, path);
        }
        path.pop_back();
      }

      // validate elements
      assert_element(user, "user", path);
      assert_element(library, "library", path);

      // resolve and compile the import
      std::string json = _resolver.resolve(user, library, function);
      Compiler import(_resolver, user, library, function);
      def.add_import(function, import.compile(json, dict));
    }

    void compile_variables(
    const rapidjson::Value& node, Definition& def,
    Dictionary& dict, std::vector<const char*>& path) {
      assert_type(node, rapidjson::kArrayType, path);
      char buf[256];
      int i = 0;
      for (auto m = node.Begin(); m != node.End(); m++, i++) {
        // updated variable path
        snprintf(buf, sizeof(buf), "[%d]", i);
        path.push_back(buf);

        // validate variable
        assert_type(*m, rapidjson::kStringType, path);
        const char* name = m->GetString();

        // check if the variable already exists
        if (def.id(name) != -1) {
          unexpected_element(name, path);
        }

        // create named variable
        def.add_variable(name);
        path.pop_back();
      }
    }

    void compile_constants(
    const rapidjson::Value& node, Definition& def,
    Dictionary& dict, std::vector<const char*>& path) {
      assert_type(node, rapidjson::kArrayType, path);
      char buf[256];
      int i = 0;
      for (auto m = node.Begin(); m != node.End(); m++, i++) {
        // updated constant path
        snprintf(buf, sizeof(buf), "[%d]", i);
        path.push_back(buf);

        // validate constant
        assert_type(*m, rapidjson::kStringType, path);
        const char* name = m->GetString();

        // check if the constant already exists
        if (def.id(name) != -1) {
          unexpected_element(name, path);
        }

        // create named constant
        def.add_constant(name);
        path.pop_back();
      }
    }

    void compile_body(
    const rapidjson::Value& node, Definition& def,
    Dictionary& dict, std::vector<const char*>& path) {
      assert_type(node, rapidjson::kObjectType, path);

      // exptected elements
      const rapidjson::Value *ret = NULL;

      for (auto m = node.MemberBegin(); m != node.MemberEnd(); m++) {
        // append current element to path
        const char *key = m->name.GetString();
        path.push_back(key);

        // remember return
        if (ret == NULL && strcmp(key, "return") == 0) {
          ret = &m->value;
        }

        // compile expression
        compile_expression(m->value, def, dict, path);

        path.pop_back();
      }

      // validate elements
      assert_element(ret, "return", path);
    }

    void compile_expression(
    const rapidjson::Value& node, Definition& def,
    Dictionary& dict, std::vector<const char*>& path) {
      assert_type(node, rapidjson::kArrayType, path);
      const char* expression = path.back();

      // exptected elements
      const rapidjson::Value *op = NULL;
      std::vector<const char*> args;
      std::vector<int> times;

      char buf[256];
      int i = 0;
      for (auto m = node.Begin(); m != node.End(); m++, i++) {
        // updated expression path
        snprintf(buf, sizeof(buf), "[%d]", i);
        path.push_back(buf);

        // create operator
        if (op == NULL && m->IsString()) {
          op = m;
          assert_value(op->GetString(), "operator", path);
          // check if the operator exists
        }
        else
        // resolve argument from string
        if (op != NULL && m->IsString()) {
          assert_value(m->GetString(), "argument", path);
          args.push_back(m->GetString());
          times.push_back(0);
          // check if the argument exists
        }
        else
        // resolve argument from object
        if (op != NULL && m->IsObject()) {
          compile_argument(*m, def, args, times, path);
        }
        // report error
        else {
          unexpected_element(buf, path);
        }

        path.pop_back();
      }

      // validate elements
      assert_element(op, "operator", path);

      // create named expression
      assert_unique(expression, def, path);
      def.add_expression(expression, op->GetString(), args, times);
    }

    void compile_argument(
    const rapidjson::Value& node, Definition& def,
    std::vector<const char*>& args, std::vector<int>& times,
    std::vector<const char*>& path) {
      assert_type(node, rapidjson::kObjectType, path);

      // exptected elements
      int count = 0;

      for (auto m = node.MemberBegin(); m != node.MemberEnd(); m++, count++) {
        // append current element to path
        const char *name = m->name.GetString();
        path.push_back(name);

        // append argument and time
        if (count == 0) {
          assert_value(name, "argument", path);
          assert_type(m->value, rapidjson::kNumberType, path);
          int time = m->value.GetInt();
          if (time > 0) {
            std::ostringstream error;
            error << "Argument '"<< name << "' at '" << str(path) << "' ";
            error << "refers to future values.";
            throw std::runtime_error(error.str());
          }

          args.push_back(name);
          times.push_back(time);
        }
        else {
          unexpected_element(name, path);
        }

        path.pop_back();
      }
    }

    Resolver& _resolver;
    std::vector<const char*> _types;
    std::string _user;
    std::string _library;
    std::string _function;
};

#endif /*_DL_COMPILER_*/
