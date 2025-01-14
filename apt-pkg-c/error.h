#pragma once
#include <apt-pkg/error.h>

#include "oma-apt/src/error.rs"
#include "rust/cxx.h"

#include "types.h"

/// Handle the situation where a string is null and return a result to rust
inline bool pending_error() { return _error->PendingError(); }

inline bool empty() { return _error->empty(); }

Vec<AptError> get_all() noexcept;
