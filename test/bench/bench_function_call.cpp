#include <vector>
#include <benchmark/benchmark.h>

using DataBlock = std::vector<int>;
using DataType = int;
using Context = int;


class Expression {
public:
    virtual void update(Context* context,DataType &input) = 0;
};

class SumExpression: public Expression {
public:
	void update(Context* context,DataType &input) {
		(*(DataType*)context) += input;
	}
};

class SumExpressionV2 final: public Expression {
public:
	void update(Context* context,DataType &input) {
		(*(DataType*)context) += input;
	}
};

class SumExpressionNoVirtualCall {
public:
	void update(Context* context,DataType &input) {
		(*(DataType*)context) += input;
	}
};

static void VirtualCall(benchmark::State& state) {
  DataBlock input_block;
  for(int i = 0; i < 1024;++i) {
    input_block.push_back(i);
  }
  Expression *expression = new SumExpression();
  Context context;

  for (auto _ : state) {
    for(int i = 0;i < input_block.size();++i) {
      expression->update(&context, input_block[i]); // update
    }
  }
}
BENCHMARK(VirtualCall);

static void VirtualCall2(benchmark::State& state) {
  DataBlock input_block;
  for(int i = 0; i < 1024;++i) {
    input_block.push_back(i);
  }
  Expression *expression = new SumExpression();
  Context context;

  for (auto _ : state) {
    for(int i = 0;i < input_block.size();++i) {
      ((SumExpressionV2*)(expression))->update(&context, input_block[i]); // update
    }
  }
}
BENCHMARK(VirtualCall2);


static void FunctionCall(benchmark::State& state) {
  DataBlock input_block;
  for(int i = 0; i < 1024;++i) {
    input_block.push_back(i);
  }
  SumExpressionNoVirtualCall *expression = new SumExpressionNoVirtualCall();
  Context context;
  for (auto _ : state) {
      for(int i = 0;i < input_block.size();++i) {
        expression->update(&context, input_block[i]); // update
      }
  }
}
BENCHMARK(FunctionCall);

BENCHMARK_MAIN();
