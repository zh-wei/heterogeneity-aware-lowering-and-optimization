// clang-format off

// REQUIRES: odla_dnnl

// RUN: %cxx -DCG_TEST %s -o %t.gen %flags %include %link -DOUTPUT=%t.cc
// RUN: %t.gen
// RUN: %cxx -O2 %t.cc %t.cc.bin %odla_link %s -I%odla_path/include -o %t.dnnl.exe -lodla_dnnl
// RUN: %t.dnnl.exe 2>&1| FileCheck %s

// clang-format on

#ifdef CG_TEST
#include "halo/lib/ir/ir_builder.h"
#include "halo/lib/ir/values.h"
#include "halo/lib/pass/pass_manager.h"
#include "halo/lib/target/generic_cxx/generic_cxx_codegen.h"
#include "halo/lib/target/cpu/x86/binary/x86_llvmir_codegen.h"
#include "halo/lib/transforms/dce.h"
#include "halo/lib/transforms/inst_simplify.h"
#include "halo/lib/transforms/tfextension_legalizer.h"
#include "halo/lib/transforms/type_legalizer.h"
#include <fstream>

#define to_str(s) std::string(#s)
#define xstr(s) to_str(s)

using namespace halo;

static void build() {
  GlobalContext ctx;
  Module m(ctx, "test_module");

  FunctionBuilder func_builder(&m);

  Function* func = func_builder.CreateFunction("func");
  BasicBlockBuilder bb_builder(func);
  BasicBlock* bb = bb_builder.CreateBasicBlock("bb0");

  std::vector<float> w0(12);
  for (int i = 0, e = w0.size(); i < e; ++i) {
    w0[i] = 1.0 + i;
  }

  std::vector<float> w2{1, 3, 9, 11, 2, 4, 10, 12, 5, 7, 13, 15, 6, 8, 14, 16};
  std::vector<float> w3{0, 1, 3, 0, 9,  11, 0, 2, 4, 0, 10, 12,
                        0, 5, 7, 0, 13, 15, 0, 6, 8, 0, 14, 16};

  ConstantBuilder c_builder(func);
  auto x0 = c_builder.CreateConstant(
      "input0", Type(DataType::FLOAT32, {4, 1, 1, 1}), w0.data());

  auto x1 = c_builder.CreateConstant(
      "input1", Type(DataType::FLOAT32, {4, 1, 1, 3}), w0.data());

  auto x2 = c_builder.CreateConstant(
      "input2", Type(DataType::FLOAT32, {4, 2, 2, 1}), w2.data());

  auto x3 = c_builder.CreateConstant(
      "input3", Type(DataType::FLOAT32, {8, 1, 3, 1}), w3.data());

  auto bs = c_builder.CreateConstant("bs", Type(DataType::INT32, {2}),
                                     (const int[]){2, 2});

  auto crops0 = c_builder.CreateConstant("bs", Type(DataType::INT32, {2, 2}),
                                         (const int[]){0, 0, 0, 0});

  auto crops1 = c_builder.CreateConstant("bs", Type(DataType::INT32, {2, 2}),
                                         (const int[]){0, 0, 2, 0});
  IRBuilder ir_builder(bb);

  Instruction* inst0 = ir_builder.CreateTFExtension("ext0", {*x0, *bs, *crops0},
                                                    1, "BatchToSpaceND");

  Instruction* inst1 = ir_builder.CreateTFExtension("ext1", {*x1, *bs, *crops0},
                                                    1, "BatchToSpaceND");

  Instruction* inst2 = ir_builder.CreateTFExtension("ext2", {*x2, *bs, *crops0},
                                                    1, "BatchToSpaceND");

  Instruction* inst3 = ir_builder.CreateTFExtension("ext3", {*x3, *bs, *crops1},
                                                    1, "BatchToSpaceND");

  ir_builder.CreateReturn("ret", {*inst0, *inst1, *inst2, *inst3});

  std::ofstream of_code;
  std::ofstream of_constants;

  of_code.open(xstr(OUTPUT), std::ofstream::binary);
  of_constants.open(xstr(OUTPUT) + ".bin", std::ofstream::binary);
  std::ostringstream os_code;
  std::ostringstream os_header;
  PassManager pm(ctx);
  pm.AddTFExtensionLegalizerPass(true);
  pm.AddDCEPass();
  pm.AddTypeLegalizerPass(true);
  pm.AddInstSimplifyPass();
  auto cg = pm.AddGenericCXXCodeGenPass(os_code, os_header);
  pm.AddX86ConstantWriterPass(of_constants);
  pm.Run(&m);
  of_code << os_code.str();
}

int main() { build(); }

#else

#include <iostream>

extern "C" {
extern void func(float* out0, float* out1, float* out2, float* out3);
}

int main() {
  float out0[4], out1[12], out2[16], out3[16];
  func(out0, out1, out2, out3);
  for (const float& x : out0) {
    std::cout << x << " ";
  }
  std::cout << std::endl;

  for (const float& x : out1) {
    std::cout << x << " ";
  }
  std::cout << std::endl;
  for (const float& x : out2) {
    std::cout << x << " ";
  }
  std::cout << std::endl;
  for (const float& x : out3) {
    std::cout << x << " ";
  }
  std::cout << std::endl;

  // CHECK: 1 2 3 4
  // CHECK: 1 2 3 4 5 6 7 8 9 10 11 12
  // CHECK: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
  // CHECK: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
}

#endif
