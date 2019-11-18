//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.h
//
// Identification: src/include/execution/executors/insert_executor.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/insert_plan.h"
#include "storage/table/tuple.h"

namespace bustub {
/**
 * InsertExecutor executes an insert into a table.
 * Inserted values can either be embedded in the plan itself ("raw insert") or come from a child executor.
 */
class InsertExecutor : public AbstractExecutor {
 public:
  /**
   * Creates a new insert executor.
   * @param exec_ctx the executor context
   * @param plan the insert plan to be executed
   * @param child_executor the child executor to obtain insert values from, can be nullptr
   */
  InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                 std::unique_ptr<AbstractExecutor> &&child_executor)
      : AbstractExecutor(exec_ctx),
        result_(true),
        plan_(plan),
        catalog_(exec_ctx_->GetCatalog()),
        oid_(plan_->TableOid()),
        table_(catalog_->GetTable(oid_)->table_.get()),
        trans_(exec_ctx_->GetTransaction()) {
    RID rid;
    if (plan_->IsRawInsert()) {
      std::vector<std::vector<Value>> valuesv = plan_->RawValues();
      for (const auto &values : valuesv) {
        // std::cout<<"i1\n";
        result_ &= table_->InsertTuple(Tuple(values, &(catalog_->GetTable(oid_)->schema_)), &rid, trans_);
        // std::cout<<"i2\n";
        if (!result_) {
          return;
        }
      }
    } else {
      Tuple tuple;
      // std::cout<<"i3\n";

      while (child_executor->Next(&tuple)) {
        // std::cout<<"i4\n";

        result_ &= table_->InsertTuple(tuple, &rid, trans_);
        // std::cout<<"i5\n";

        if (!result_) {
          return;
        }
      }
    }
  }

  const Schema *GetOutputSchema() override { return plan_->OutputSchema(); }

  void Init() override {}

  // Note that Insert does not make use of the tuple pointer being passed in.
  // We return false if the insert failed for any reason, and return true if all inserts succeeded.
  bool Next([[maybe_unused]] Tuple *tuple) override { return result_; }

 private:
  /** The insert plan node to be executed. */
  bool result_;

  const InsertPlanNode *plan_;
  SimpleCatalog *catalog_;
  table_oid_t oid_;
  TableHeap *table_;
  Transaction *trans_;
};
}  // namespace bustub
