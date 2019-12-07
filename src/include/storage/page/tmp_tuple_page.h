#pragma once

#include "storage/page/page.h"
#include "storage/table/tmp_tuple.h"
#include "storage/table/tuple.h"

namespace bustub {

// To pass the test cases for this class, you must follow the existing TmpTuplePage format and implement the
// existing functions exactly as they are! It may be helpful to look at TablePage.
// Remember that this task is optional, you get full credit if you finish the next task.

/**
 * TmpTuplePage format:
 *
 * Sizes are in bytes.
 * | PageId (4) | LSN (4) | FreeSpace (4) | (free space) | TupleSize2 | TupleData2 | TupleSize1 | TupleData1 |
 *
 * We choose this format because DeserializeExpression expects to read Size followed by Data.
 */
class TmpTuplePage : public Page {
 public:

  void Init(page_id_t page_id, uint32_t page_size) {
    memcpy(GetData(), &page_id, sizeof(page_id));
    SetFreeSpace(page_size);
  }

  void SetFreeSpace(uint32_t free_size){
    memcpy(GetData() + OFFSET_FREE_SPACE, &free_size, sizeof(uint32_t));
  }

  uint32_t GetFreeSpace(){
    return *reinterpret_cast<uint32_t *>(GetData() + OFFSET_FREE_SPACE);
  }

  page_id_t GetTablePageId() {
    return *reinterpret_cast<page_id_t *>(GetData());
  }

  bool Insert(const Tuple &tuple, TmpTuple *out) {
    uint32_t free_space = GetFreeSpace();
    uint32_t tuple_size = tuple.GetLength();

    if(free_space - OFFSET_HEADER - sizeof(uint32_t) < tuple_size){
      return false;
    }
    free_space -= tuple_size + sizeof(uint32_t);
    // std::cout<<"new free space "<<free_space<<"\n";
    SetFreeSpace(free_space);

    memcpy(GetData() + free_space, &tuple_size, sizeof(uint32_t));
    memcpy(GetData() + free_space + sizeof(uint32_t), tuple.GetData(), tuple_size);

    *out = TmpTuple(GetTablePageId(), free_space);

    return true;
  }

 private:
  static_assert(sizeof(page_id_t) == 4);
  static constexpr size_t OFFSET_FREE_SPACE = 8;
  static constexpr size_t OFFSET_HEADER = 12;
};

}  // namespace bustub
