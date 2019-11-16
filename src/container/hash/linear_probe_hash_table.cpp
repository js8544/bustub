//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// linear_probe_hash_table.cpp
//
// Identification: src/container/hash/linear_probe_hash_table.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/exception.h"
// #include "common/logger.h"
#include "common/rid.h"
#include "container/hash/linear_probe_hash_table.h"


namespace bustub {

template <typename KeyType, typename ValueType, typename KeyComparator>
HASH_TABLE_TYPE::LinearProbeHashTable(const std::string &name, BufferPoolManager *buffer_pool_manager,
                                      const KeyComparator &comparator, size_t num_buckets,
                                      HashFunction<KeyType> hash_fn)
    : buffer_pool_manager_(buffer_pool_manager), comparator_(comparator), hash_fn_(std::move(hash_fn)), num_buckets_(num_buckets) {

      // LOG_INFO("Starting Hash Table with size %zu", num_buckets);

      Page *header_page_holder = buffer_pool_manager_->NewPage(&header_page_id_);
      auto *header_page = reinterpret_cast<HashTableHeaderPage*>(header_page_holder->GetData());
      header_page->SetPageId(header_page_id_);
      header_page->SetSize(num_buckets_);

      int64_t bucket_count = num_buckets_;
      while(bucket_count > 0){
        page_id_t block_page_holder_id;
        // Page *block_page_holder = buffer_pool_manager_->NewPage(&block_page_holder_id);
        buffer_pool_manager_->NewPage(&block_page_holder_id);
        // HASH_TABLE_BLOCK_TYPE *block_page = reinterpret_cast<HASH_TABLE_BLOCK_TYPE*>(block_page_holder->GetData());
        header_page->AddBlockPageId(block_page_holder_id);
        buffer_pool_manager_->UnpinPage(block_page_holder_id, false);
        bucket_count -= BLOCK_ARRAY_SIZE;
      }
      
      buffer_pool_manager_->UnpinPage(header_page_id_, true); 
    }

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::GetValue(Transaction *transaction, const KeyType &key, std::vector<ValueType> *result) {
  // LOG_INFO("Get value");
  table_latch_.RLock();

  uint64_t bucket_index =  hash_fn_.GetHash(key) % num_buckets_;
  size_t num_checked = 0;
  int status;
  MappingType res;
  int found = 0;
  while(num_checked++ < num_buckets_){
    res = GetBucket(bucket_index, &status);
    if(status == 0){
      if(comparator_(res.first, key) == 0){
        found++;
        result->push_back(res.second);
      }
    } else if(status == 1) {}
    else {
      // LOG_INFO("Get value found %d", found);
      table_latch_.RUnlock();

      return (found>0);
    }
    bucket_index = (bucket_index + 1) % num_buckets_;
  }

  table_latch_.RUnlock();


  return (found>0);
}
/*****************************************************************************
 * INSERTION
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::Insert(Transaction *transaction, const KeyType &key, const ValueType &value) {
  table_latch_.WLock();
  // LOG_INFO("Insert");
  if(num_buckets_ <= 0) {
    table_latch_.WUnlock();
    return false;
  }
  uint64_t bucket_index =  hash_fn_.GetHash(key) % num_buckets_;
  size_t num_checked = 0;
  int status;
  MappingType res;
  while(num_checked++ < num_buckets_){
    res = GetBucket(bucket_index, &status);
    if(status == 0){
      if(comparator_(res.first, key) == 0){
        if(value == res.second){
          table_latch_.WUnlock();
          return false;
        }
      }
    } else{
      SetBucket(bucket_index, key, value, 0);
      table_latch_.WUnlock();
      return true;
    }
    bucket_index = (bucket_index + 1) % num_buckets_;
  }
  table_latch_.WUnlock();
  Resize(num_buckets_);
  return Insert(transaction, key, value);
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::Remove(Transaction *transaction, const KeyType &key, const ValueType &value) {
  // LOG_INFO("Remove");
  table_latch_.WLock();
  uint64_t bucket_index =  hash_fn_.GetHash(key) % num_buckets_;
  size_t num_checked = 0;
  int status;
  MappingType res;
  while(num_checked++ < num_buckets_){
    res = GetBucket(bucket_index, &status);
    if(status == 0){
      if(comparator_(res.first, key) == 0){
        if(value == res.second){
          SetBucket(bucket_index, key, value, 1);
          table_latch_.WUnlock();
          return true;
        }
      }
    } else if(status == 1){}
    else{
      table_latch_.WUnlock();
      return false;
    }
    bucket_index = (bucket_index + 1) % num_buckets_;
  }

  table_latch_.WUnlock();

  return false;
}

/*****************************************************************************
 * RESIZE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::Resize(size_t initial_size) {
  // LOG_INFO("Resize");
  table_latch_.WLock();
  std::vector<MappingType> entries;
  for(size_t i=0; i<num_buckets_; i++){
    int status;
    MappingType res;
    res = GetBucket(i, &status);
    if(status == 0){
      entries.push_back(res);
    }
  }

  Page *header_page_holder = buffer_pool_manager_->FetchPage(header_page_id_);

  auto *header_page = reinterpret_cast<HashTableHeaderPage*>(header_page_holder->GetData());
  size_t num_blocks = header_page->NumBlocks();
  for(size_t i=0; i< num_blocks; i++){
    page_id_t page_id = header_page->GetBlockPageId(i);
    buffer_pool_manager_->DeletePage(page_id);
  }
  buffer_pool_manager_->UnpinPage(header_page_id_, false);
  buffer_pool_manager_->DeletePage(header_page_id_);

  num_buckets_ *= 2;
  header_page_holder = buffer_pool_manager_->NewPage(&header_page_id_);
  header_page = reinterpret_cast<HashTableHeaderPage*>(header_page_holder->GetData());
  header_page->SetPageId(header_page_id_);
  header_page->SetSize(num_buckets_);

  int64_t bucket_count = num_buckets_;
  while(bucket_count > 0){
    page_id_t block_page_holder_id;
    buffer_pool_manager_->NewPage(&block_page_holder_id);
    header_page->AddBlockPageId(block_page_holder_id);
    bucket_count -= BLOCK_ARRAY_SIZE;
    buffer_pool_manager_->UnpinPage(block_page_holder_id, false);

  }
  
  buffer_pool_manager_->UnpinPage(header_page_id_, true); 
  
  for(MappingType &pair: entries){
    if(num_buckets_ <= 0) {
      return;
    }
    uint64_t bucket_index =  hash_fn_.GetHash(pair.first) % num_buckets_;
    size_t num_checked = 0;
    int status;
    MappingType res;
    while(num_checked++ < num_buckets_){
      res = GetBucket(bucket_index, &status);
      if(status != 0){
        SetBucket(bucket_index, pair.first, pair.second, 0);
        break;        
      }
      bucket_index = (bucket_index + 1) % num_buckets_;
    }
  }
  
  table_latch_.WUnlock();

  
  
}

/*****************************************************************************
 * GETSIZE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
size_t HASH_TABLE_TYPE::GetSize() {
  return num_buckets_;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
MappingType HASH_TABLE_TYPE::GetBucket(size_t bucket_index, int* status) {
  // LOG_INFO("Get bucket %zu", bucket_index);
  page_id_t page_index = bucket_index/BLOCK_ARRAY_SIZE;

  Page *header_page_holder = buffer_pool_manager_->FetchPage(header_page_id_);

  // header_page_holder->RLatch();

  auto *header_page = reinterpret_cast<HashTableHeaderPage*>(header_page_holder->GetData());
  page_id_t block_page_id = header_page->GetBlockPageId(page_index);

  // header_page_holder->RUnlatch();

  buffer_pool_manager_->UnpinPage(header_page_id_, false);

  Page *block_page_holder = buffer_pool_manager_->FetchPage(block_page_id);

  // block_page_holder->RLatch();

  auto *block_page = reinterpret_cast<HASH_TABLE_BLOCK_TYPE*>(block_page_holder->GetData());

  MappingType result;
  slot_offset_t bucket_offset = bucket_index % BLOCK_ARRAY_SIZE;
  if(block_page->IsReadable(bucket_offset)) {
    result.first = block_page->KeyAt(bucket_offset);
    result.second = block_page->ValueAt(bucket_offset);
    // LOG_INFO("0");
    *status = 0;
  } else{
    if(block_page->IsOccupied(bucket_offset)) {
      // LOG_INFO("1");
      *status = 1;
    }else{
      // LOG_INFO("2");
      *status = 2;
    }
  }

  // block_page_holder->RUnlatch();
  buffer_pool_manager_->UnpinPage(block_page_id, false);

  return result;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::SetBucket(size_t bucket_index, const KeyType &key, const ValueType &value, int option) {
  // LOG_INFO("Set Bucket %zu, %d", bucket_index, option);
  page_id_t page_index = bucket_index/BLOCK_ARRAY_SIZE;

  Page *header_page_holder = buffer_pool_manager_->FetchPage(header_page_id_);

  // header_page_holder->RLatch();

  auto *header_page = reinterpret_cast<HashTableHeaderPage*>(header_page_holder->GetData());

  page_id_t block_page_id = header_page->GetBlockPageId(page_index);

  buffer_pool_manager_->UnpinPage(header_page_id_, false);
  // header_page_holder->RUnlatch();

  Page *block_page_holder = buffer_pool_manager_->FetchPage(block_page_id);

  // block_page_holder->WLatch();

  auto *block_page = reinterpret_cast<HASH_TABLE_BLOCK_TYPE*>(block_page_holder->GetData());

  slot_offset_t bucket_offset = bucket_index % BLOCK_ARRAY_SIZE; 
  if(option == 0) {
    block_page->Insert(bucket_offset, key, value);
  }
  else {
    block_page->Remove(bucket_offset);
  }

  // block_page_holder->WUnlatch();
  buffer_pool_manager_->UnpinPage(block_page_id, true);
}

template class LinearProbeHashTable<int, int, IntComparator>;

template class LinearProbeHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class LinearProbeHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class LinearProbeHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class LinearProbeHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class LinearProbeHashTable<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
