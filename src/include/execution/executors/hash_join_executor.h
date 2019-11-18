//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.h
//
// Identification: src/include/execution/executors/hash_join_executor.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "container/hash/hash_function.h"
#include "container/hash/linear_probe_hash_table.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/hash_join_plan.h"
#include "storage/index/hash_comparator.h"
#include "storage/table/tmp_tuple.h"
#include "storage/table/tuple.h"

namespace bustub {
/**
 * IdentityHashFunction hashes everything to itself, i.e. h(x) = x.
 */
class IdentityHashFunction : public HashFunction<hash_t> {
 public:
  /**
   * Hashes the key.
   * @param key the key to be hashed
   * @return the hashed value
   */
  uint64_t GetHash(size_t key) override { return key; }
};

/**
 * A simple hash table that supports hash joins.
 */
class SimpleHashJoinHashTable {
 public:
  /** Creates a new simple hash join hash table. */
  SimpleHashJoinHashTable(const std::string &name, BufferPoolManager *bpm, HashComparator cmp, uint32_t buckets,
                          const IdentityHashFunction &hash_fn) {}

  /**
   * Inserts a (hash key, tuple) pair into the hash table.
   * @param txn the transaction that we execute in
   * @param h the hash key
   * @param t the tuple to associate with the key
   * @return true if the insert succeeded
   */
  bool Insert(Transaction *txn, hash_t h, const Tuple &t) {
    hash_table_[h].emplace_back(t);
    return true;
  }

  /**
   * Gets the values in the hash table that match the given hash key.
   * @param txn the transaction that we execute in
   * @param h the hash key
   * @param[out] t the list of tuples that matched the key
   */
  void GetValue(Transaction *txn, hash_t h, std::vector<Tuple> *t) { *t = hash_table_[h]; }

 private:
  std::unordered_map<hash_t, std::vector<Tuple>> hash_table_;
};

// TODO(student): when you are ready to attempt task 3, replace the using declaration!
using HT = SimpleHashJoinHashTable;

// using HashJoinKeyType = ???;
// using HashJoinValType = ???;
// using HT = LinearProbeHashTable<HashJoinKeyType, HashJoinValType, HashComparator>;

/**
 * HashJoinExecutor executes hash join operations.
 */
class HashJoinExecutor : public AbstractExecutor {
 public:
  /**
   * Creates a new hash join executor.
   * @param exec_ctx the context that the hash join should be performed in
   * @param plan the hash join plan node
   * @param left the left child, used by convention to build the hash table
   * @param right the right child, used by convention to probe the hash table
   */
  HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan, std::unique_ptr<AbstractExecutor> &&left,
                   std::unique_ptr<AbstractExecutor> &&right)
      : AbstractExecutor(exec_ctx),
        plan_(plan),
        predicate_(plan_->Predicate()),
        left_(std::move(left)),
        right_(std::move(right)),
        jht_(HT("SHT", nullptr, jht_comp_, jht_num_buckets_, jht_hash_fn_)) {}

  /** @return the JHT in use. Do not modify this function, otherwise you will get a zero. */
  const HT *GetJHT() const { return &jht_; }

  const Schema *GetOutputSchema() override { return plan_->OutputSchema(); }

  void Init() override {
    left_->Init();
    right_->Init();
    Tuple tuple;
    while (left_->Next(&tuple)) {
      assert(tuple.IsAllocated());
      assert(tuple.GetData() != nullptr);

      hash_t hash = HashValues(&tuple, left_->GetOutputSchema(), plan_->GetLeftKeys());
      jht_.Insert(nullptr, hash, tuple);
    }
  }

  bool Next(Tuple *tuple) override {
    if (!tuples_.empty()) {
      *tuple = tuples_.back();
      tuples_.pop_back();
      return true;
    }
    Tuple tup;
    // std::cout<<"h1\n";
    while (right_->Next(&tup)) {
      assert(tup.IsAllocated());
      assert(tup.GetData() != nullptr);

      // std::cout<<"h2\n";
      hash_t hash = HashValues(&tup, right_->GetOutputSchema(), plan_->GetRightKeys());
      std::vector<Tuple> t;
      jht_.GetValue(nullptr, hash, &t);
      // std::cout<<"h3\n";

      for (Tuple &left_tup : t) {
        // std::cout<<"h4\n";

        if (predicate_->EvaluateJoin(&left_tup, left_->GetOutputSchema(), &tup, right_->GetOutputSchema())
                .GetAs<bool>()) {
          std::vector<Value> result;
          std::vector<Column> columns = plan_->OutputSchema()->GetColumns();
          for (Column &col : columns) {
            const AbstractExpression *expr = col.GetExpr();
            result.push_back(expr->EvaluateJoin(&left_tup, left_->GetOutputSchema(), &tup, right_->GetOutputSchema()));
          }
          tuples_.emplace_back(Tuple(result, plan_->OutputSchema()));
        }
      }
      if (!tuples_.empty()) {
        *tuple = tuples_.back();
        tuples_.pop_back();
        return true;
      }
    }
    // std::cout<<"h100\n";

    return false;
  }

  /**
   * Hashes a tuple by evaluating it against every expression on the given schema, combining all non-null hashes.
   * @param tuple tuple to be hashed
   * @param schema schema to evaluate the tuple on
   * @param exprs expressions to evaluate the tuple with
   * @return the hashed tuple
   */
  hash_t HashValues(const Tuple *tuple, const Schema *schema, const std::vector<const AbstractExpression *> &exprs) {
    hash_t curr_hash = 0;
    // For every expression,
    for (const auto &expr : exprs) {
      // We evaluate the tuple on the expression and schema.
      Value val = expr->Evaluate(tuple, schema);
      // If this produces a value,
      if (!val.IsNull()) {
        // We combine the hash of that value into our current hash.
        curr_hash = HashUtil::CombineHashes(curr_hash, HashUtil::HashValue(&val));
      }
    }
    return curr_hash;
  }

 private:
  /** The hash join plan node. */
  const HashJoinPlanNode *plan_;

  const AbstractExpression *predicate_;
  /** The comparator is used to compare hashes. */
  [[maybe_unused]] HashComparator jht_comp_{};
  /** The identity hash function. */
  IdentityHashFunction jht_hash_fn_{};

  std::unique_ptr<AbstractExecutor> left_;
  std::unique_ptr<AbstractExecutor> right_;

  std::vector<Tuple> tuples_;
  /** The hash table that we are using. */
  HT jht_;
  /** The number of buckets in the hash table. */
  static constexpr uint32_t jht_num_buckets_ = 2;
};
}  // namespace bustub
