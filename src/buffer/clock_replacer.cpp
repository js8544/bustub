//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// clock_replacer.cpp
//
// Identification: src/buffer/clock_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/clock_replacer.h"

#include <vector>

namespace bustub {

ClockReplacer::ClockReplacer(size_t num_pages) {
  num_frame_ = num_pages;
  frame_status_ = std::vector<int>(num_frame_, 0);
  clock_hand_ = 0;
  num_replacer_ = 0;
}

ClockReplacer::~ClockReplacer() = default;

bool ClockReplacer::Victim(frame_id_t *frame_id) {
  mtx_.lock();
  size_t initial_hand = clock_hand_;
  int visit_cnt = 0;  // how many times clock_hand_ visits initial_hand, the hand loops at most twice
  while (frame_status_[clock_hand_] != 2) {
    if (frame_status_[clock_hand_] == 1) {
      frame_status_[clock_hand_] = 2;
    }
    clock_hand_ = (clock_hand_ + 1) % num_frame_;
    if (clock_hand_ == initial_hand) {
      if (visit_cnt == 1) {
        mtx_.unlock();
        return false;
      }
      visit_cnt = 1;
    }
  }
  *frame_id = static_cast<int>(clock_hand_);
  frame_status_[clock_hand_] = 0;
  num_replacer_--;
  mtx_.unlock();
  return true;
}

void ClockReplacer::Pin(frame_id_t frame_id) {
  mtx_.lock();

  if (frame_status_[frame_id] != 0) {
    frame_status_[frame_id] = 0;
    num_replacer_--;
  }
  mtx_.unlock();
}

void ClockReplacer::Unpin(frame_id_t frame_id) {
  mtx_.lock();

  if (frame_status_[frame_id] == 0) {
    num_replacer_++;
  }
  frame_status_[frame_id] = 1;

  mtx_.unlock();
}

size_t ClockReplacer::Size() {
  mtx_.lock();
  size_t val = num_replacer_;
  mtx_.unlock();
  return val;
}

}  // namespace bustub
