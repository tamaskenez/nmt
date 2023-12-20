#pragma once

#ifdef NDEBUG
#    include <absl/container/flat_hash_map.h>
#    include <absl/container/flat_hash_set.h>
#    include <absl/container/node_hash_map.h>
#    include <absl/container/node_hash_set.h>
#else
#    include <unordered_map>
#    include <unordered_set>
#endif

// Use stl containers if !NDEBUG because they're better for debugging.
#ifdef NDEBUG
template<class K, class V, class H = absl::flat_hash_map<K, V>::hasher>
using flat_hash_map = absl::flat_hash_map<K, V, H>;
template<class K, class H = absl::flat_hash_set<K>::hasher>
using flat_hash_set = absl::flat_hash_set<K, H>;
template<class K, class V, class H = absl::node_hash_map<K, V>::hasher>
using node_hash_map = absl::node_hash_map<K, V, H>;
template<class K, class H = absl::node_hash_set<K>::hasher>
using node_hash_set = absl::node_hash_set<K, H>;
#else
template<class K, class V, class H = std::unordered_map<K, V>::hasher>
using flat_hash_map = std::unordered_map<K, V, H>;
template<class K, class H = std::unordered_set<K>::hasher>
using flat_hash_set = std::unordered_set<K, H>;
template<class K, class V, class H = std::unordered_map<K, V>::hasher>
using node_hash_map = std::unordered_map<K, V, H>;
template<class K, class H = std::unordered_set<K>::hasher>
using node_hash_set = std::unordered_set<K, H>;
#endif
