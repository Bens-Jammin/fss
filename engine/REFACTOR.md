# Engine Restructure — Todo List

Goal: split the `fss.hpp` god header and its flat `.cpp` files into per-module
compile units, each with its own header, without breaking the build or tests.

## Current layout (completed)

```
engine/
├── include/fss/
│   ├── fss.hpp             # shim: just #includes types.hpp + indexer.hpp
│   ├── types.hpp           # FileEntry, FSS_STATUS, FSSException, FSS_RESULT, stos()
│   └── indexer.hpp         # class FSSIndexer declaration
├── src/
│   ├── crawler.hpp/.cpp    # FSCrawl (both overloads)
│   ├── db.hpp/.cpp         # db + metadata free functions
│   ├── sqlite_guard.hpp    # DbGuard, StmtGuard
│   ├── detail.hpp          # internal shared helpers (update() internals)
│   ├── indexer.cpp         # ctor(s), done(), build_index(), metadata()
│   ├── update.cpp          # update() + DBisEmpty/getIDs/getMTimes/currentFileMTime/result/updateDir
│   ├── query.cpp           # queryExtension/Exact/Substr (+ queryFuzzy stub — remove)
│   ├── utils.hpp/.cpp      # DBPath, epoch_now
│   ├── ffi.h/.cpp          # unchanged: C ABI
│   └── main.cpp            # demo driver (still holds time_update_speed)
├── benchmark/              # created but EMPTY — move time_update_speed here
└── tests/
    ├── doctest.h
    └── main.cpp
```

## Remaining work

### P1 — build is broken

- [ ] `FSSIndexer()` default ctor: declared (`indexer.hpp:20`) but no longer defined
      (`indexer.cpp` only has the two explicit ctors). `ffi.cpp` calls it 4× →
      `make build` fails with undefined reference. Either re-add a default ctor
      (previously used `TEST_ROOT_DIRECTORY`, which is gone) or change `ffi.cpp`
      to pass an explicit root.
- [ ] `make lib` and confirm the Rust `build.rs` still links (blocked by the above)

### P2 — tests won't compile

- [ ] `tests/main.cpp:2-4` still includes deleted files `../src/fss.hpp` and
      `../src/exception.hpp` → switch to `fss/fss.hpp` (+ add `utils.hpp`, `db.hpp`, `ffi.h`)
- [ ] `tests/main.cpp:53-56` calls `queryFor()` → renamed to `queryExact()`
- [ ] `make test` passes

### P3 — cleanup

- [ ] Move `time_update_speed` from `main.cpp:7,32` into `engine/benchmark/`
- [ ] Remove `queryFuzzy` stub (dead code): declare (`indexer.hpp:30`) + impl (`query.cpp:117`)
- [ ] (hygiene) `update.cpp` uses `detail.hpp` helpers but doesn't `#include "detail.hpp"`
      (compiles today via transitive includes); `indexer.cpp:2,7` double-includes `fss/fss.hpp`;
      `crawler.hpp` could declare the recursive `FSCrawl` overload, though it's only used internally

## Done (no longer tracked)

- [x] `include/fss/` created; `types.hpp`, `indexer.hpp`, shim `fss.hpp` in place
- [x] `src/db.hpp`, `sqlite_guard.hpp`, `detail.hpp`, `utils.hpp`, `crawler.hpp` created
- [x] `DbGuard`/`StmtGuard` moved into `sqlite_guard.hpp`
- [x] `DBPath`/`epoch_now` in `utils.cpp`, `utils.cpp` includes `utils.hpp`
- [x] `update()` + `DBisEmpty/getIDs/getMTimes/currentFileMTime/result/updateDir` moved to
      `update.cpp` and declared in `detail.hpp`; duplicate `make_result()` consolidated into `result()`
- [x] `queryExtension/queryFor/queryLike/queryFuzzy` moved to `query.cpp`
- [x] `queryFor` → `queryExact`, `queryLike` → `querySubstr` (API + impl; tests not yet updated)
- [x] `ChildEntry` and `TEST_ROOT_DIRECTORY` removed
- [x] `Makefile` include flags (`-Iengine/include`)

## Verification (after P1/P2)

- [ ] Run `make build`
- [ ] Run `make test`
- [ ] Run `make lib` and confirm the Rust `build.rs` still links