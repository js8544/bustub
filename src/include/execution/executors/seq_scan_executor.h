//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.h
//
// Identification: src/include/execution/executors/seq_scan_executor.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/seq_scan_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * SeqScanExecutor executes a sequential scan over a table.
 */
class SeqScanExecutor : public AbstractExecutor {
 public:
  /**
   * Creates a new sequential scan executor.
   * @param exec_ctx the executor context
   * @param plan the sequential scan plan to be executed
   */
  SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
      : AbstractExecutor(exec_ctx),
        plan_(plan),
        catalog_(exec_ctx_->GetCatalog()),
        oid_(plan->GetTableOid()),
        table_(catalog_->GetTable(oid_)->table_.get()),
        it_(table_->Begin(exec_ctx_->GetTransaction())) {}

  void Init() override {}

  bool Next(Tuple *tuple) override {
    // if(it_ == table_->End()){
    //   tuple = nullptr;
    //   return false;
    // }

    // if(!(plan_->GetPredicate()->Evaluate(&(*it_), plan_->OutputSchema())).GetAs<bool>()){
    //   ++it_;
    //   return false;
    // }
    // *tuple = *it_;
    // ++it_;
    // return true;
    while (it_ != table_->End()) {
      // std::cout<<"s1\n";
      const AbstractExpression *pred = plan_->GetPredicate();
      if (pred == nullptr || pred->Evaluate(&(*it_), plan_->OutputSchema()).GetAs<bool>()) {
        // std::cout<<"s2\n";
        *tuple = *it_;
        ++it_;
        return true;
      }
      ++it_;
    }
    return false;
  }

  const Schema *GetOutputSchema() override { return plan_->OutputSchema(); }

 private:
  /** The sequential scan plan node to be executed. */
  const SeqScanPlanNode *plan_;
  SimpleCatalog *catalog_;
  table_oid_t oid_;
  TableHeap *table_;
  TableIterator it_;
};
}  // namespace bustub
