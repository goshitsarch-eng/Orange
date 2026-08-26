#ifndef STRAWBERRY_PLAYLISTPLAYED_H
#define STRAWBERRY_PLAYLISTPLAYED_H

#include <algorithm>
#include <numeric>
#include <vector>

namespace PlaylistPlayed {

inline void Push(std::vector<int> *stack, int row) {
  if (!stack || row < 0) {
    return;
  }
  if (!stack->empty() && stack->back() == row) {
    return;
  }
  stack->push_back(row);
}

inline int Pop(std::vector<int> *stack) {
  if (!stack || stack->empty()) {
    return -1;
  }
  const int row = stack->back();
  stack->pop_back();
  return row;
}

inline std::vector<int> AfterRemove(const std::vector<int> &stack, const std::vector<int> &removed) {
  std::vector<int> sorted = removed;
  std::sort(sorted.begin(), sorted.end());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  std::vector<int> out;
  out.reserve(stack.size());
  for (int row : stack) {
    if (std::binary_search(sorted.begin(), sorted.end(), row)) {
      continue;
    }
    int shift = 0;
    for (int gone : sorted) {
      if (gone < row) {
        ++shift;
      }
    }
    out.push_back(row - shift);
  }
  return out;
}

inline std::vector<int> AfterInsert(const std::vector<int> &stack, int at, int count) {
  if (count <= 0) {
    return stack;
  }
  std::vector<int> out = stack;
  for (int &row : out) {
    if (row >= at) {
      row += count;
    }
  }
  return out;
}

inline std::vector<int> MoveMap(int count, const std::vector<int> &from, int to) {
  std::vector<int> order(count > 0 ? static_cast<size_t>(count) : 0);
  std::iota(order.begin(), order.end(), 0);
  std::vector<int> sorted = from;
  std::sort(sorted.begin(), sorted.end());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  std::vector<int> moving;
  for (int row : sorted) {
    if (row < 0 || row >= count) {
      return {};
    }
    moving.push_back(order[static_cast<size_t>(row)]);
  }
  int dest = to;
  if (dest < 0) {
    dest = 0;
  }
  if (dest > count) {
    dest = count;
  }
  for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
    order.erase(order.begin() + *it);
    if (*it < dest) {
      --dest;
    }
  }
  if (dest < 0) {
    dest = 0;
  }
  if (dest > static_cast<int>(order.size())) {
    dest = static_cast<int>(order.size());
  }
  order.insert(order.begin() + dest, moving.begin(), moving.end());
  std::vector<int> map(static_cast<size_t>(count), -1);
  for (int i = 0; i < static_cast<int>(order.size()); ++i) {
    const int old = order[static_cast<size_t>(i)];
    if (old >= 0 && old < count) {
      map[static_cast<size_t>(old)] = i;
    }
  }
  return map;
}

inline std::vector<int> AfterMove(const std::vector<int> &stack, int count, const std::vector<int> &from, int to) {
  const std::vector<int> map = MoveMap(count, from, to);
  if (map.empty()) {
    return stack;
  }
  std::vector<int> out;
  out.reserve(stack.size());
  for (int row : stack) {
    if (row >= 0 && row < static_cast<int>(map.size()) && map[static_cast<size_t>(row)] >= 0) {
      out.push_back(map[static_cast<size_t>(row)]);
    }
  }
  return out;
}

}  // namespace PlaylistPlayed

#endif
