#include "baldr/double_bucket_queue.h"
#include <algorithm>
#include <iostream>

namespace {

void show(uint32_t label, const valhalla::baldr::bucket_t& prevbucket, const valhalla::baldr::DoubleBucketQueue& q) {
  std::cout << " removing " << label << " with cost " << q.labelcost_(label) << " from bucket " << (&prevbucket - &q.buckets_.front()) << std::endl;
  std::cout << "the current cost is " << q.currentcost_ << " the min cost is " << q.mincost_ << " the max cost is " << q.maxcost_ << std::endl;
  for(const auto& b : q.buckets_) {
    if(b.empty())
      continue;
    std::cout << "bucket " << (&b - &q.buckets_.front()) << ": ";
    for(auto i : b)
      std::cout << i << ",";
    std::cout << std::endl;
  }
  if(q.overflowbucket_.size()){
    std::cout << "overflow: ";
    for(auto i : q.overflowbucket_)
      std::cout << i << ",";
  }
  throw std::runtime_error("label was not found in bucket");
}

}

namespace valhalla {
namespace baldr {

// Constructor given a minimum cost, a range of costs held within the
// bucket sort, and a bucket size. All costs above mincost + range are
// stored in an "overflow" bucket.
DoubleBucketQueue::DoubleBucketQueue(const float mincost, const float range,
          const uint32_t bucketsize, const LabelCost& labelcost) {
  // Adjust min cost to be the start of a bucket
  uint32_t c = static_cast<uint32_t>(mincost);
  currentcost_ = (c - (c % bucketsize));
  mincost_ = currentcost_;
  bucketrange_ = range;
  bucketsize_ = static_cast<float>(bucketsize);
  inv_ = 1.0f / bucketsize_;

  // Set the maximum cost (above this goes into the overflow bucket)
  maxcost_ = mincost + bucketrange_;

  // Allocate the low-level buckets
  bucketcount_ = (range / bucketsize_) + 1;
  buckets_.resize(bucketcount_);

  // Set the current bucket to the lowest cost low level bucket
  currentbucket_ = buckets_.begin();

  // Set the cost function.
  labelcost_ = labelcost;
}

// Destructor
DoubleBucketQueue::~DoubleBucketQueue() {
  clear();
}

// Clear all labels from the low-level buckets and the overflow buckets.
void DoubleBucketQueue::clear() {
  // Empty the overflow bucket and each bucket
  overflowbucket_.clear();
  while (currentbucket_ != buckets_.end()) {
    currentbucket_->clear();
    currentbucket_++;
  }

  // Reset current bucket and cost
  currentcost_ = mincost_;
  currentbucket_ = buckets_.begin();
}

// The specified label now has a smaller cost. Reorders it in the sorted list.
// Uses the labelcost_ function to get the bucket that the label is currently
// within.
void DoubleBucketQueue::decrease(const uint32_t label, const float newcost) {
  // Get the buckets of the previous and new costs. Nothing needs to be done
  // if old cost and the new cost are in the same buckets.
  bucket_t& prevbucket = get_bucket(labelcost_(label));
  bucket_t& newbucket  = get_bucket(newcost);

  int found = 0;
  for(const auto& b : buckets_) {
    if(b.size()) {
      for(auto i : b) {
        found += i == label;
      }
    }
  }
  if(found != 1) {
    show(label, prevbucket, *this);
    throw std::runtime_error("Found label " + std::to_string(label) + " in " + std::to_string(found) + " buckets");
  }

  if (prevbucket != newbucket) {
    // Add label to newbucket and remove from previous bucket
    newbucket.push_back(label);
    auto itr = std::remove(prevbucket.begin(), prevbucket.end(), label);
    if(itr != prevbucket.end())
      prevbucket.erase(itr);
    else
      show(label, prevbucket, *this);
  }
}

// Remove the label with the lowest cost
uint32_t DoubleBucketQueue::pop()  {
  if (empty()) {
    // No labels found in the low-level buckets.
    if (overflowbucket_.empty()) {
      // Return an invalid label if no labels are in the overflow buckets.
      // Reset currentbucket to the last bucket - in case another access of
      // adjacency list is done.
      currentbucket_--;
      return kInvalidLabel;
    } else {
      // Move labels from the overflow bucket to the low level buckets.
      // Return invalid label if still empty.
      empty_overflow();
      if (empty()) {
        return kInvalidLabel;
      }
    }
  }

  // Return label from lowest non-empty bucket
  uint32_t label = currentbucket_->back();
  currentbucket_->pop_back();
  return label;
}

// Empties the overflow bucket by placing the labels into the
// low level buckets.
void DoubleBucketQueue::empty_overflow()  {
  bool found = false;
  while (!found && !overflowbucket_.empty()) {
    // Adjust cost range
    mincost_ += bucketrange_;
    maxcost_ += bucketrange_;

    bucket_t tmp;
    for (const auto& label : overflowbucket_) {
      // Get the cost (using the label cost function)
      float cost = labelcost_(label);
      if (cost < maxcost_) {
        buckets_[static_cast<uint32_t>((cost-mincost_)*inv_)].push_back(label);
        found = true;
      } else {
        tmp.push_back(label);
      }
    }

    // Add any labels that lie outside the new range back to overflow bucket
    overflowbucket_ = tmp;
  }

  // Reset current cost and bucket to beginning of low level buckets
  currentcost_ = mincost_;
  currentbucket_ = buckets_.begin();
}

}
}

