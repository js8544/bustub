//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include <list>
#include <unordered_map>
#include <vector>

#include "common/logger.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // We allocate a consecutive memory space for the buffer pool.
  pages_ = new Page[pool_size_];
  replacer_ = new ClockReplacer(pool_size);
  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
  // LOG_INFO("Buffer size: %zd", pool_size);
}

BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  delete replacer_;
}

Page *BufferPoolManager::FetchPageImpl(page_id_t page_id) {
  // 1.     Search the page table for the requested page (P).
  // 1.1    If P exists, pin it and return it immediately.
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  // 2.     If R is dirty, write it back to the disk.
  // 3.     Delete R from the page table and insert P.
  // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.
  latch_.lock();
  // LOG_INFO("Fetching page: %d", page_id);
  if (page_table_.find(page_id) != page_table_.end()) {  // Page is in page table
    frame_id_t frame_id = page_table_[page_id];
    // LOG_INFO("In page table: %d", frame_id);
    Page *page = &pages_[frame_id];
    page->pin_count_++;
    replacer_->Pin(frame_id);
    latch_.unlock();
    return page;
  }
  // Need to find replacement
  frame_id_t replace_id = -1;

  if (!free_list_.empty()) {  // Pop from free list
    replace_id = free_list_.front();
    free_list_.pop_front();
  } else {  // Free list is empty, find from replacer
    replacer_->Victim(&replace_id);
  }
  // LOG_INFO("Victim: %d", replace_id);

  if (replace_id == -1) {  // No replacement found
    latch_.unlock();
    return nullptr;
  }

  Page *rep_page = &pages_[replace_id];  // Page to be replaced

  if (rep_page->IsDirty()) {  // Write page back to disk
    // LOG_INFO("Is Dirty");
    disk_manager_->WritePage(rep_page->GetPageId(), rep_page->GetData());
  }

  // Update page table
  page_table_.erase(rep_page->GetPageId());
  page_table_[page_id] = replace_id;

  rep_page->ResetMemory();
  rep_page->page_id_ = page_id;
  disk_manager_->ReadPage(page_id, rep_page->data_);
  rep_page->pin_count_ = 1;
  rep_page->is_dirty_ = false;
  replacer_->Pin(replace_id);
  latch_.unlock();
  return rep_page;
}

bool BufferPoolManager::UnpinPageImpl(page_id_t page_id, bool is_dirty) {
  latch_.lock();
  // LOG_INFO("Unpin: %d", page_id);
  frame_id_t frame_id = page_table_[page_id];
  Page *page = &pages_[frame_id];
  if (page == nullptr) {
    latch_.unlock();
    return false;
  }
  if (page->GetPinCount() <= 0) {
    latch_.unlock();
    return false;
  }
  page->pin_count_--;
  if (page->GetPinCount() == 0) {
    replacer_->Unpin(frame_id);
  }
  if (is_dirty) {
    // LOG_INFO("Is Dirty");
    page->is_dirty_ = true;
  }
  latch_.unlock();
  return true;
}

bool BufferPoolManager::FlushPageImpl(page_id_t page_id) {
  // Make sure you call DiskManager::WritePage!
  latch_.lock();
  // LOG_INFO("Flush: %d", page_id);

  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  frame_id_t frame_id = page_table_[page_id];
  Page *page = &pages_[frame_id];
  if (page == nullptr) {
    latch_.unlock();
    return false;
  }
  disk_manager_->WritePage(page_id, page->GetData());

  latch_.unlock();
  return true;
}

Page *BufferPoolManager::NewPageImpl(page_id_t *page_id) {
  // 0.   Make sure you call DiskManager::AllocatePage!
  // 1.   If all the pages in the buffer pool are pinned, return nullptr.
  // 2.   Pick a victim page P from either the free list or the replacer. Always pick from the free list first.
  // 3.   Update P's metadata, zero out memory and add P to the page table.
  // 4.   Set the page ID output parameter. Return a pointer to P.
  latch_.lock();

  frame_id_t replace_id = -1;

  if (!free_list_.empty()) {  // Pop from free list
    replace_id = free_list_.front();
    free_list_.pop_front();
  } else {  // Free list is empty, find from replacer
    replacer_->Victim(&replace_id);
  }

  if (replace_id == -1) {  // No replacement found
    latch_.unlock();
    return nullptr;
  }

  Page *rep_page = &pages_[replace_id];  // Page to be replaced

  if (rep_page->IsDirty()) {  // Write page back to disk
    disk_manager_->WritePage(rep_page->GetPageId(), rep_page->GetData());
  }

  page_id_t new_page_id = disk_manager_->AllocatePage();

  page_table_.erase(rep_page->GetPageId());
  page_table_[new_page_id] = replace_id;
  rep_page->ResetMemory();
  rep_page->page_id_ = new_page_id;
  rep_page->pin_count_ = 1;
  rep_page->is_dirty_ = false;
  replacer_->Pin(replace_id);
  *page_id = new_page_id;
  // LOG_INFO("New page: %d", new_page_id);
  // LOG_INFO("Victim: %d", replace_id);

  latch_.unlock();
  return rep_page;
}

bool BufferPoolManager::DeletePageImpl(page_id_t page_id) {
  // 0.   Make sure you call DiskManager::DeallocatePage!
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  // 2.   If P exists, but has a non-zero pin-count, return false. Someone is using the page.
  // 3.   Otherwise, P can be deleted. Remove P from the page table, reset its metadata and return it to the free list.
  latch_.lock();
  // LOG_INFO("Delete: %d", page_id);
  if (page_table_.find(page_id) != page_table_.end()) {
    frame_id_t frame_id = page_table_[page_id];
    Page *page = &pages_[frame_id];
    if (page->GetPinCount() > 0) {
      latch_.unlock();
      return false;
    }
    disk_manager_->DeallocatePage(page_id);
    page_table_.erase(page_id);
    page->ResetMemory();
    page->pin_count_ = 0;
    page->is_dirty_ = false;
    free_list_.push_back(frame_id);
    latch_.unlock();
    return true;
  }
  latch_.unlock();
  return true;
}

void BufferPoolManager::FlushAllPagesImpl() {
  // You can do it!
  latch_.lock();
  // LOG_INFO("Flush all");

  for (size_t i = 0; i < pool_size_; ++i) {
    Page *page = &pages_[i];
    if (page == nullptr) {
      continue;
    }
    disk_manager_->WritePage(page->GetPageId(), page->GetData());
  }
  latch_.unlock();
}

}  // namespace bustub
