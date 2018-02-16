#include "cpu.hh"
#include "function.hh"
#include "network.hh"
#include "unittest.hh"
#include "utils.hh"

#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

// CPU DL config
namespace dl {
  typedef float                           base_t;
  typedef std::vector<base_t>             vector;
  typedef CPUContext<base_t>              context;
  typedef Matrix<base_t,CPUMatrix>        matrix;
  typedef Function<base_t,CPUMatrix>      function;
  typedef Variable<base_t,CPUMatrix>      variable;
  typedef Constant<base_t,CPUMatrix>      constant;
  typedef Addition<base_t,CPUMatrix>      addition;
  typedef Subtraction<base_t,CPUMatrix>   subtract;
  typedef Product<base_t,CPUMatrix>       product;
  typedef Element<base_t,CPUMatrix>       element;
  typedef Summation<base_t,CPUMatrix>     summation;
  typedef Transpose<base_t,CPUMatrix>     transpose;
  typedef Exponent<base_t,CPUMatrix>      exponent;
  typedef Network<base_t,CPUMatrix>       network;
  typedef Resolver                        resolver;
}

const dl::base_t EPS = (sizeof(dl::base_t) < 8) ? 1e-3:1e-8;

void print(const dl::vector& vec, int rows, int cols) {
  std::cout << "[" << rows << " x " << cols << "]" << std::endl;
  for (int r=0; r<rows; r++) {
    for (int c=0; c<cols; c++) {
      std::cout << vec[cols * r + c] << ", ";
    }
    std::cout << std::endl;
  }
}

std::string load(const char* filename) {
  // get currend dir
  char current_dir[FILENAME_MAX];
  if (!getcwd(current_dir, sizeof(current_dir))) {
    throw std::runtime_error("Failed to get current working directory.");
  }

  std::ostringstream path;
  path << current_dir << "/" << filename;

  // load the data
  std::ostringstream data;
  read_file(path.str().data(), data);

  return data.str();
}

dl::base_t dfdx(dl::function& f, int fr, int fc, dl::matrix& x, int xr, int xc) {
  dl::base_t eps = 1e-2;
  dl::vector xv=x, f1v, f2v;

  // caclulate f values
  int i = xr * x.cols() + xc;
  dl::base_t bak = xv[i];

  // f2 = f(x+eps)
  f.refresh(true);
  xv[i] = bak + eps;
  x=xv;
  auto f2 = f.forward();

  // f1 = f(x-eps)
  f.refresh(true);
  xv[i] = bak - eps;
  x=xv;
  auto f1 = f.forward();

  // reset x
  xv[i] = bak;
  x.set(xv);

  // calculate f derivative
  int j = fr * f1.cols() + fc;

  f2v = f2;
  f1v = f1;

  // df/dx = (f2 - f1) / (2 * eps)
  return (f2v[j] - f1v[j]) / (2 * eps);
}

dl::matrix dfdx(dl::function& f, int fr, int fc, dl::matrix& x) {
  dl::vector v = x;
  int rows = x.rows();
  int cols = x.cols();
  for (int r=0; r<rows; r++) {
    for (int c=0; c<cols; c++) {
      v[r*cols + c] = dfdx(f, fr, fc, x, r, c);
    }
  }
  dl::matrix ret(x.context(), rows, cols);
  ret = v;
  return ret;
}

bool operator==(const dl::matrix& a, const dl::matrix& b) {
  if (a.rows() != b.rows() || a.cols() != b.cols()) {
    return false;
  }
  dl::vector va, vb;
  a.get(va);
  b.get(vb);
  for (int i=0; i<va.size(); i++) {
    if (std::abs(va[i] - vb[i]) > EPS) {
      return false;
    }
  }
  return true;
}

void test_matrix_context(dl::context& ctx) {
  TEST_BEGIN("matrix Context")

  auto m1 = ctx.get_matrix(2, 3);
  auto m2 = ctx.get_matrix(2, 3);
  auto m3 = ctx.get_matrix(3, 2);
  ctx.put_matrix(m1);
  ctx.put_matrix(m2);
  ctx.put_matrix(m3);

  ASSERT(ctx.get_matrix_count() == 3);
  ASSERT(ctx.get_matrix_count(2,3) == 2);
  ASSERT(ctx.get_matrix_count(3,2) == 1);
  TEST_END()
}

void test_matrix_rows_cols(dl::context& ctx) {
  TEST_BEGIN("matrix Rows/Cols")

  dl::matrix m(ctx, 2, 3);

  ASSERT(m.rows() == 2);
  ASSERT(m.cols() == 3);
  TEST_END()
}

void test_matrix_set_get(dl::context& ctx) {
  TEST_BEGIN("matrix Set/Get")

  dl::matrix A(ctx, 2, 3);
  A.set({1,2,3,10,11,12});

  dl::vector a;
  A.get(a);

  ASSERT(a == dl::vector({1,2,3,10,11,12}))

  dl::matrix B(ctx, 2, 3);
  B.set(9);

  dl::vector b;
  B.get(b);

  ASSERT(b == dl::vector({9,9,9,9,9,9}))

  dl::matrix C(ctx, 1, 1);
  C.set(8);

  dl::vector c;
  C.get(c);

  ASSERT(c == dl::vector({8}))

  TEST_END()
}

void test_matrix_addition(dl::context& ctx) {
  TEST_BEGIN("matrix Addition")

  dl::matrix A(ctx, 2, 3);
  dl::matrix B(ctx, 2, 3);
  dl::matrix C(ctx, 2, 3);
  A.set({
    1,2,3,
    10,11,12
  });
  B.set({
    7,8,9,
    20,21,22
  });
  C.set({
    1+7, 2+8, 3+9,
    10+20, 11+21, 12+22
  });

  ASSERT(A + B == C)
  TEST_END()
}

void test_matrix_subtract(dl::context& ctx) {
  TEST_BEGIN("matrix Subtract")

  dl::matrix A(ctx, 2, 3);
  dl::matrix B(ctx, 2, 3);
  dl::matrix C(ctx, 2, 3);
  A.set({
    1,2,3,
    10,11,12
  });
  B.set({
    7,8,9,
    20,21,22
  });
  C.set({
    1-7, 2-8, 3-9,
    10-20, 11-21, 12-22
  });

  ASSERT(A - B == C)
  TEST_END()
}

void test_matrix_product(dl::context& ctx) {
  TEST_BEGIN("matrix Product")

  dl::matrix A(ctx, 2, 3);
  dl::matrix B(ctx, 3, 2);
  dl::matrix C(ctx, 2, 2);
  A.set({
    1,2,3,
    4,5,6
  });
  B.set({
    2,3,
    4,5,
    6,7
  });
  C.set({
    1*2+2*4+3*6, 1*3+2*5+3*7,
    4*2+5*4+6*6, 4*3+5*5+6*7,
  });

  ASSERT(A * B == C)
  TEST_END()
}

void test_matrix_element(dl::context& ctx) {
  TEST_BEGIN("matrix Element")

  dl::matrix A(ctx, 2, 3);
  dl::matrix B(ctx, 2, 3);
  dl::matrix C(ctx, 2, 3);
  A.set({
    1,2,3,
    4,5,6
  });
  B.set({
    2,3,4,
    5,6,7
  });
  C.set({
    1*2, 2*3, 3*4,
    4*5, 5*6, 6*7
  });

  ASSERT((A & B) == C)
  TEST_END()
}

void test_matrix_transpose(dl::context& ctx) {
  TEST_BEGIN("matrix Transpose")

  dl::matrix A(ctx, 2, 3);
  dl::matrix T(ctx, 3, 2);
  A.set({1,2,3,4,5,6});
  T.set({1,4,2,5,3,6});

  ASSERT(A.T() == T)
  TEST_END()
}

void test_matrix_exponent(dl::context& ctx) {
  TEST_BEGIN("matrix Exponent")

  dl::matrix A(ctx, 2, 3);
  dl::matrix E(ctx, 2, 3);
  A.set({1,2,3,4,5,6});
  E.set({
    std::exp(1),std::exp(2),std::exp(3),
    std::exp(4),std::exp(5),std::exp(6)
  });

  ASSERT(A.E() == E)
  TEST_END()
}

void test_matrix_summation(dl::context& ctx) {
  TEST_BEGIN("matrix Summation")

  dl::matrix A(ctx, 2, 3);
  A.set({1,2,3,4,5,6});

  ASSERT(A.S() == (1+2+3+4+5+6))
  TEST_END()
}

void test_function_derivative(dl::context& ctx) {
  TEST_BEGIN("function Derivative")
  // matrix variables
  auto ma = new dl::matrix(ctx, 2, 3);
  auto mb = new dl::matrix(ctx, 3, 2);

  // function variables
  std::unique_ptr<dl::variable> fa(new dl::variable(ma));
  std::unique_ptr<dl::variable> fb(new dl::variable(mb));

  // set input values
  *ma = {1,2,3,4,5,6};
  *mb = {7,7,8,8,9,9};

  // function: f (a, b) = a * b
  std::unique_ptr<dl::product> f(new dl::product(fa.get(), fb.get()));

  // df/da [0,0] numerical
  auto dfda_00_num = dfdx(*f, 0, 0, *ma);

  // df/db [0,0] numerical
  auto dfdb_00_num = dfdx(*f, 0, 0, *mb);

  // df/da [0,0] analitical
  dl::matrix dfda_00_ana(*ma);
  dfda_00_ana = {7,8,9,0,0,0};

  // df/db [0,0] analitical
  dl::matrix dfdb_00_ana(*mb);
  dfdb_00_ana = {1,0,2,0,3,0};

  ASSERT(dfda_00_num == dfda_00_ana)
  ASSERT(dfdb_00_num == dfdb_00_ana)

  TEST_END()
}

void test_function_constant(dl::context& ctx) {
  TEST_BEGIN("function Constant")

  // test value
  auto mc = new dl::matrix(ctx, 2, 3);
  std::unique_ptr<dl::constant> fc(new dl::constant(mc));
  mc->set({1,2,3,4,5,7});

  ASSERT(fc->forward() == *mc)

  // test derivative
  dl::matrix md(ctx, 2, 3);
  dl::matrix mr(ctx, 2, 3);
  md.set({1,1,1,1,1,1});
  mr.set({0,0,0,0,0,0});
  fc->backward(md);

  ASSERT(fc->derivative() == mr)
  TEST_END()
}

void test_function_variable(dl::context& ctx) {
  TEST_BEGIN("function Variable")

  // test value
  auto mc = new dl::matrix(ctx, 2, 3);
  std::unique_ptr<dl::variable> fc(new dl::variable(mc));
  mc->set({1,2,3,4,5,8});

  ASSERT(fc->forward() == *mc)

  // derivative seed
  dl::matrix md(ctx, 2, 3);
  md.set({1,1,1,1,1,1});
  fc->backward(md);

  // numerival derivative

  ASSERT(fc->derivative() == md)
  TEST_END()
}

void test_function_addition(dl::context& ctx) {
  TEST_BEGIN("function Addition")

  // matrix variables
  auto ma = new dl::matrix(ctx, 2, 3);
  auto mb = new dl::matrix(ctx, 2, 3);
  auto mc = new dl::matrix(ctx, 2, 3);
  auto md = new dl::matrix(ctx, 2, 3);

  // function variables
  std::unique_ptr<dl::variable> fa(new dl::variable(ma));
  std::unique_ptr<dl::variable> fb(new dl::variable(mb));
  std::unique_ptr<dl::constant> fc(new dl::constant(mc));
  std::unique_ptr<dl::variable> fd(new dl::variable(md));

  // set input values
  *ma = {1,2,3,4,5,6};
  *mb = {6,5,4,3,2,1};
  *mc = {1,2,3,1,2,3};
  *md = {4,5,6,6,7,8};

  // function: f (a, b, d) = (a + b) + (c + d)
  std::unique_ptr<dl::addition> ab(new dl::addition(fa.get(), fb.get()));
  std::unique_ptr<dl::addition> cd(new dl::addition(fc.get(), fd.get()));
  std::unique_ptr<dl::addition> f(new dl::addition(ab.get(), cd.get()));

  // function value
  dl::matrix mf(ctx, 2, 3);
  mf.set({12,14,16,14,16,18});

  ASSERT(f->forward() == mf)

  // numerical derivative [0,0]
  dl::matrix dfda = dfdx(*f, 0,0, *ma); // variable
  dl::matrix dfdb = dfdx(*f, 0,0, *mb); // variable
  dl::matrix dfdc(ctx, 2, 3); dfdc = 0; // constant
  dl::matrix dfdd = dfdx(*f, 0,0, *md); // variable

  // derivative seed
  dl::matrix df_seed(ctx, 2, 3);
  df_seed = {1,0,0,0,0,0};

  // function derivative: df/da = 1, df/db = 1, df/dc = 0, df/dd = 1
  f->backward(df_seed);

  ASSERT(fa->derivative() == dfda)
  ASSERT(fb->derivative() == dfdb)
  ASSERT(fc->derivative() == dfdc)
  ASSERT(fd->derivative() == dfdd)
  TEST_END()
}

void test_function_subtract(dl::context& ctx) {
  TEST_BEGIN("function Subtract")

  // matrix variables
  auto ma = new dl::matrix(ctx, 2, 3);
  auto mb = new dl::matrix(ctx, 2, 3);

  // function variables
  std::unique_ptr<dl::variable> fa(new dl::variable(ma));
  std::unique_ptr<dl::variable> fb(new dl::variable(mb));

  // set input values
  ma->set({1,2,3,4,5,6});
  mb->set({6,5,4,3,2,1});

  // function: f (a, b) = a - b
  std::unique_ptr<dl::subtract> f(new dl::subtract(fa.get(), fb.get()));

  // function value
  dl::matrix mf(ctx, 2, 3);
  mf.set({-5,-3,-1,1,3,5});

  ASSERT(f->forward() == mf)

  // numerical derivative [0,0]
  dl::matrix dfda = dfdx(*f, 0,0, *ma);
  dl::matrix dfdb = dfdx(*f, 0,0, *mb);

  // derivative seed [0,0]
  dl::matrix df_seed(ctx, 2, 3);
  df_seed = {1,0,0,0,0,0};

  // function derivative: df/da = 1, df/db = 1, df/dc = 0, df/dd = 1
  f->backward(df_seed);

  ASSERT(fa->derivative() == dfda)
  ASSERT(fb->derivative() == dfdb)
  TEST_END()
}

void test_function_product(dl::context& ctx) {
  TEST_BEGIN("function Product")

  // matrix variables
  auto ma = new dl::matrix(ctx, 2, 3);
  auto mb = new dl::matrix(ctx, 3, 2);

  // function variables
  std::unique_ptr<dl::variable> fa(new dl::variable(ma));
  std::unique_ptr<dl::variable> fb(new dl::variable(mb));

  // set input values
  ma->set({1,2,3,4,5,6});
  mb->set({7,7,8,8,9,9});

  // function: f (a, b) = a * b
  std::unique_ptr<dl::product> f(new dl::product(fa.get(), fb.get()));

  // function value
  dl::matrix mf(ctx, 2, 2);
  mf.set({50,50,122,122});

  ASSERT(f->forward() == mf)

  // derivative index [0,0]
  dl::matrix dv(ctx, 2, 2);
  dv.set({1,0,0,0});

  // function derivative: df/da = mb, df/db = ma
  f->backward(dv);

  // df/da [0,0] numerical
  dl::matrix dfda_00_num = dfdx(*f, 0, 0, *ma);

  // df/db [0,0] numerical
  dl::matrix dfdb_00_num = dfdx(*f, 0, 0, *mb);

  ASSERT(fa->derivative() == dfda_00_num)
  ASSERT(fb->derivative() == dfdb_00_num)
  TEST_END()
}

void test_function_element(dl::context& ctx) {
  TEST_BEGIN("function Element")

  // matrix variables
  auto ma = new dl::matrix(ctx, 2, 3);
  auto mb = new dl::matrix(ctx, 2, 3);

  // function variables
  std::unique_ptr<dl::variable> fa(new dl::variable(ma));
  std::unique_ptr<dl::variable> fb(new dl::variable(mb));

  // set input values
  ma->set({1,2,3,4,5,6});
  mb->set({7,7,8,8,9,9});

  // function: f (a, b) = a & b
  std::unique_ptr<dl::element> f(new dl::element(fa.get(), fb.get()));

  // function value
  dl::matrix mf(ctx, 2, 3);
  mf.set({1*7,2*7,3*8,4*8,5*9,6*9});

  ASSERT(f->forward() == mf)

  // derivative index [0,0]
  dl::matrix dv(ctx, 2, 3);
  dv.set({1,0,0,0,0,0});

  // function derivative: df/da = mb, df/db = ma
  f->backward(dv);

  // df/da [0,0] numerical
  dl::matrix dfda_00_num = dfdx(*f, 0, 0, *ma);

  // df/db [0,0] numerical
  dl::matrix dfdb_00_num = dfdx(*f, 0, 0, *mb);

  ASSERT(fa->derivative() == dfda_00_num)
  ASSERT(fb->derivative() == dfdb_00_num)
  TEST_END()
}

void test_function_transpose(dl::context& ctx) {
  TEST_BEGIN("function Transpose")

  // matrix variables
  auto ma = new dl::matrix(ctx, 2, 3);

  // function variables
  std::unique_ptr<dl::variable> fa(new dl::variable(ma));

  // set input values
  *ma = {7,2,3,4,5,6};

  // function: f (a) = a.T()
  std::unique_ptr<dl::transpose> f(new dl::transpose(fa.get()));

  ASSERT(f->forward() == ma->T())

  // numerical derivative: df/da [0,0]
  auto dfda = dfdx(*f, 0,0, *ma);

  // derivative seed [0,0]
  dl::matrix dv(ctx, 3, 2);
  dv = {1,0,0,0,0,0};

  // function derivative: df/da = 1 [2 x 3]
  f->backward(dv);

  ASSERT(fa->derivative() == dfda)

  TEST_END()
}

void test_function_exponent(dl::context& ctx) {
  TEST_BEGIN("function Exponent")

  // matrix variables
  auto ma = new dl::matrix(ctx, 2, 3);

  // function variables
  std::unique_ptr<dl::variable> fa(new dl::variable(ma));

  // set input values
  *ma = {1.1,1.2,1.3,1.4,1.5,1.6};

  // function: f (a) = a.E)
  std::unique_ptr<dl::exponent> f(new dl::exponent(fa.get()));

  ASSERT(f->forward() == ma->E())

  // numerical derivative: df/da [0,0]
  auto dfda = dfdx(*f, 0,0, *ma);

  // derivative seed [0,0]
  dl::matrix dv(ctx, 2, 3);
  dv = {1,0,0,0,0,0};

  // function derivative: df/da = 1 [2 x 3]
  f->backward(dv);

  ASSERT(fa->derivative() == dfda)

  TEST_END()
}

void test_function_summation(dl::context& ctx) {
  TEST_BEGIN("function Summation")

  // matrix variables
  auto ma = new dl::matrix(ctx, 2, 3);

  // function variables
  std::unique_ptr<dl::variable> fa(new dl::variable(ma));

  // set input values
  ma->set({1,2,3,4,5,6});

  // function: f (a) = a[0]+...+a[n]
  std::unique_ptr<dl::summation> f(new dl::summation(fa.get()));

  // function value
  dl::matrix mf(ctx, 1, 1);
  mf.set(ma->S());

  ASSERT(f->forward() == mf)

  // numerical derivative [0,0]
  dl::matrix dfda_00_num = dfdx(*f, 0,0, *ma);

  // derivative seed
  dl::matrix dv(ctx, 1, 1);
  dv.set(1);

  // function derivative: df/da = 1 [2 x 3]
  f->backward(dv);
  auto& dfda_00_ana = fa->derivative();

  ASSERT(dfda_00_ana == dfda_00_num)
  TEST_END()
}

void test_json_error(dl::context& ctx) {
  TEST_BEGIN("json Validate")

  auto json = load("object-1.json");

  rapidjson::Document doc;
  rapidjson::ParseResult res = doc.Parse(json.data());

  ASSERT(res == false)
  ASSERT(res.Offset() == 123)

  std::string err_msg = rapidjson::GetParseError_En(res.Code());
  ASSERT(err_msg == "Missing a comma or ']' after an array element.")
  TEST_END()
}

void test_json_read(dl::context& ctx) {
  TEST_BEGIN("json Read")

  auto json = load("object-2.json");
  std::vector<char> vjson(json.size()+1);
  std::strcpy(vjson.data(), json.data());

  rapidjson::Document doc;

  // In-situ parsing, decode strings directly in the source string.
  // Source must be string.
  rapidjson::ParseResult res = doc.ParseInsitu(vjson.data());

  ASSERT(res == true)
  TEST_END()
}

void test_json_file(dl::context& ctx) {
  TEST_BEGIN("json Stream")

  auto json = load("object-2.json");

  std::stringstream ifs(json);
  rapidjson::IStreamWrapper isw(ifs);

  rapidjson::Document doc;
  rapidjson::ParseResult res = doc.ParseStream(isw);

  ASSERT(res == true)
  ASSERT(doc.IsObject())

  TEST_END()
}

void test_network_load(dl::context& ctx, dl::resolver& r) {
  TEST_BEGIN("network Load")

  // read the file
  auto json = load("network-1.json");

  // create network
  dl::network net;
  net.load(json, r);

  ASSERT(net.definition().get_name() == "foo");
  TEST_END()
}

void test_network_save(dl::context& ctx, dl::resolver& r) {
  TEST_BEGIN("network Save")

  ASSERT(false);
  TEST_END()
}

void test_network_variables(dl::context& ctx, dl::resolver& r) {
  TEST_BEGIN("network Variables")

  // read the file
  auto json = load("network-1.json");

  // create network
  dl::network net;
  net.load(json, r);

  auto vars = net.variables();

  ASSERT(vars.size() == 5);
  ASSERT(vars.find("x") != vars.end());
  ASSERT(vars.find("x") != vars.end());
  ASSERT(vars.find("x") != vars.end());
  ASSERT(vars.find("e3.x") != vars.end());
  ASSERT(vars.find("e3.y") != vars.end());
  TEST_END()
}

void test_network_subnet(dl::context& ctx, dl::resolver& r) {
  TEST_BEGIN("network Subnet")

  ASSERT(false);
  TEST_END()
}

void test_network_forward(dl::context& ctx, dl::resolver& r) {
  TEST_BEGIN("network Forward")

  ASSERT(false);
  TEST_END()
}

void test_network_backward(dl::context& ctx, dl::resolver& r) {
  TEST_BEGIN("network Backward")

  ASSERT(false);
  TEST_END()
}

void test_network_update(dl::context& ctx, dl::resolver& r) {
  TEST_BEGIN("network Update")

  ASSERT(false);
  TEST_END()
}

void test_network_gpu(dl::context& ctx, dl::resolver& r) {
  TEST_BEGIN("network GPU")

  ASSERT(false);
  TEST_END()
}

void test_matrix(dl::context& ctx) {
  test_matrix_context(ctx);
  test_matrix_rows_cols(ctx);
  test_matrix_set_get(ctx);
  test_matrix_addition(ctx);
  test_matrix_subtract(ctx);
  test_matrix_product(ctx);
  test_matrix_element(ctx);
  test_matrix_transpose(ctx);
  test_matrix_exponent(ctx);
  test_matrix_summation(ctx);
}

void test_function(dl::context& ctx) {
  test_function_derivative(ctx);
  test_function_variable(ctx);
  test_function_constant(ctx);
  test_function_addition(ctx);
  test_function_subtract(ctx);
  test_function_product(ctx);
  test_function_element(ctx);
  test_function_transpose(ctx);
  test_function_exponent(ctx);
  test_function_summation(ctx);
}

void test_json(dl::context& ctx) {
  test_json_error(ctx);
  test_json_read(ctx);
  test_json_file(ctx);
}

void test_network(dl::context& ctx) {
  class R : public dl::resolver {
    std::string resolve(const char* usr, const char* lib, const char* fun) {
      return load("network-2.json");
    }
  };

  R res;
  test_network_load(ctx, res);
  test_network_save(ctx, res);
  test_network_variables(ctx, res);
  test_network_subnet(ctx, res);
  test_network_forward(ctx, res);
  test_network_backward(ctx, res);
  test_network_update(ctx, res);
  test_network_gpu(ctx, res);
}

void context_error(const char* msg) {
  throw std::runtime_error(std::string(msg) + "\n" + stacktrace());
}

int main(int argn, char** args) {
  try {
    dl::context ctx;
    ctx.set_error_handler(context_error);

    test_matrix(ctx);
    test_function(ctx);
    test_json(ctx);
    test_network(ctx);
  }
  catch (std::runtime_error e) {
    std::cout << "Runtime exception:" << std::endl;
    std::cout << e.what() << std::endl;
  }
  catch (std::exception e) {
    std::cout << "Generic exception:" << std::endl;
    std::cout << e.what() << std::endl;
  }
  return 0;
}
