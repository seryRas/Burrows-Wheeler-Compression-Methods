# BWT Implementation — Code Review

## Overview

The project implements the Burrows-Wheeler Transform with two backends:
1. **SA-IS (Suffix Array Induced Sorting)** — O(n) time, the main implementation in [BWT-SAIS.c](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-SAIS.c)
2. **Radix sort** — naive recursive MSD radix sort in [BWT-radix_sort.c](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-radix_sort.c)

All 13 unit tests pass. No memory errors under AddressSanitizer + UBSan. The core algorithm is **functionally correct** for the tested inputs. Below are bugs, risks, and quality issues I found.

---

## 🐛 Bugs & Correctness Issues

### 1. `bwtRetransform` — infinite loop when `next_row == initialIndex`

**Severity: High** — This is a logical bug.

In [BWT-SAIS.c lines 610–618](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-SAIS.c#L610-L618):

```c
for (int i = (int)inputSize - 1; i >= 0; i--) {
    output[i] = transformed[curr_packed];
    unsigned int next_row = LF[curr_packed];
    
    if (next_row < initialIndex) {
        curr_packed = next_row;
    } else if (next_row > initialIndex) {
        curr_packed = next_row - 1;
    }
    // When next_row == initialIndex: curr_packed is NEVER updated!
}
```

When `next_row == initialIndex`, the code falls through both `if`/`else if` branches without updating `curr_packed`. This means `curr_packed` retains its previous value, causing the same character to be emitted repeatedly for every remaining iteration. For most inputs this path isn't hit during the loop body (it would naturally be the terminating step), but for certain inputs it **can** produce incorrect output. The `else if` should be `else` so that `next_row >= initialIndex` always maps to `next_row - 1`.

---

### 2. `BWT_HEADER_SIZE` type mismatch between SAIS and radix sort

**Severity: High** — data corruption on 64-bit systems when using the radix sort backend.

In [BWT.h line 12](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT.h#L12):
```c
#define BWT_HEADER_SIZE (1 + sizeof(unsigned int))  // = 5 bytes
```

But [BWT-radix_sort.c line 62](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-radix_sort.c#L62) writes:
```c
memcpy(output + 1, &initialIndex, sizeof(size_t));  // 8 bytes on 64-bit!
```

The radix sort version uses `size_t` for `initialIndex` and writes `sizeof(size_t)` (8 bytes on 64-bit) into a header slot sized for `sizeof(unsigned int)` (4 bytes). This **overwrites 4 bytes of BWT payload data**. The retransform function reads `sizeof(size_t)` on line 73, so it might round-trip correctly on the *same* build, but the header size constant and actual I/O are fundamentally inconsistent.

---

### 3. `unsigned int` limits input to ~4 GB; silently truncates larger files

**Severity: Medium**

[main.c line 33](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/main.c#L33):
```c
unsigned int fileSize = (unsigned int)toldSize;
```

If `toldSize` exceeds `UINT_MAX` (~4 GB), this silently truncates. More importantly, the SA-IS algorithm uses `unsigned int` throughout and the sentinel value `EMPTY_IDX = UINT_MAX`, so the effective maximum input is actually `UINT_MAX - 1` bytes. There's no bounds check.

---

### 4. Stripping `\n`/`\r` from binary input is wrong

**Severity: Medium**

[main.c lines 46–48](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/main.c#L46-L48):
```c
while (bytesRead > 0 && (arr[bytesRead - 1] == '\n' || arr[bytesRead - 1] == '\r')) {
    arr[--bytesRead] = '\0';
}
```

The file is opened in `"rb"` mode, suggesting it's meant to handle binary data. Stripping trailing newline/carriage-return bytes from binary data corrupts it. This also means a legitimate file consisting entirely of `\n` bytes will be treated as empty.

---

### 5. `bwtRetransform` validates `initialIndex > inputSize` — off-by-one

**Severity: Low**

[BWT-SAIS.c line 589](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-SAIS.c#L589):
```c
if (initialIndex > inputSize) return generalError;
```

The valid range of `initialIndex` is `[0, inputSize]`, so this check is correct for the upper bound. However, the code then uses `initialIndex` to skip a row in the LF mapping, and the exact value `inputSize` as an initial index is unusual. Might want `>= inputSize` to be safe, depending on what values `bwtTransform` actually produces.

---

### 6. Radix sort: unbounded recursion for pathological input

**Severity: Medium**

[BWT-radix_sort.c line 34](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-radix_sort.c#L34):
```c
sortTable(input, currentValueEnd[i], valueBegin[i], current, swap,
          j + 1, inputSize, false);
```

The recursion depth is bounded by `inputSize` (worst case: all characters identical except one). For a 900 KB chunk (`MAX_CHUNK`), this means up to ~900,000 recursive stack frames, each allocating 3 × 256 × `sizeof(size_t)` = 6 KB on the stack. That's a **guaranteed stack overflow** for non-trivial inputs. The `count[i] == inputSize` early exit on line 16 only catches the *all-identical* case at the top level, not at recursive levels.

---

## ⚠️ Risky Patterns

### 7. `memset(suffixArr, -1, ...)` — technically implementation-defined

Using `memset` with `-1` to set all bytes to `0xFF` works for `unsigned int` because `UINT_MAX` is `0xFFFFFFFF` on all mainstream platforms, and you rely on `EMPTY_IDX = UINT_MAX`. This is a well-known idiom but is technically non-portable (the C standard doesn't guarantee 2's complement representation for the `int` argument to `memset`). Fine in practice.

### 8. `output[0]` is never set (except in the all-same-input path)

In [`bwtTransform`](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-SAIS.c#L501-L579), the first byte of `output` (`output[0]`) is never written in the normal code path. The header is `output[0]` + 4 bytes of `initialIndex`. The all-same-input path sets `output[0]` implicitly via pointer arithmetic but the main path doesn't. This means `output[0]` contains whatever the caller's `malloc` gave it. The retransform function ignores `output[0]`, so it works, but it's an uninitialized byte in the output buffer.

### 9. No `#pragma once` / include guard in [BWT.h](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT.h)

Multiple inclusion of `BWT.h` would cause redefinitions of the enums and structs.

---

## 🧹 Code Quality & Maintainability

### Duplicated code between `uc*` and `sizet*` functions

The SA-IS file contains **two near-identical copies** of every major function:
- `ucSortTypes` / `sizetSortTypes`
- `ucFillLMS` / `sizetFillLMS`
- `ucLinductionSort` / `sizetLinductionSort`
- `ucSinductionSort` / `sizetSinductionSort`
- `ucCompareLmsSubstrings` / `sizetCompareLmsSubstrings`
- `ucFindSameSubstrings` / `sizetFindSameSubstrings`
- `ucFinalLMSFill` / `sizetFinalLMSFill`

The only difference is the input type (`unsigned char*` vs `unsigned int*`). This is ~300 lines of near-clone code. Any bug fix must be applied in two places. Consider using a macro-based generic approach or a common template.

### Error handling boilerplate

The deeply nested `free()` chains in [`sizetSAIS`](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-SAIS.c#L202-L313) and [`bwtTransform`](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-SAIS.c#L501-L579) are error-prone and hard to maintain. Consider using a `goto cleanup` pattern, which is the standard C idiom for this:

```c
errors result = success;
// allocations...
if (!ptr) { result = mallocErr; goto cleanup; }
// work...
cleanup:
    free(a); free(b); free(c);
    return result;
```

### Variable shadowing

The compiler reports shadowed variables `i` and `index` inside [`sizetFindSameSubstrings`](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-SAIS.c#L123) (lines 164→182, 126→183) and [`ucFindSameSubstrings`](file:///home/rastik/Documents/Projekty/Burrows-Wheeler-Compression-Methods/BWT-SAIS.c#L413) (line 460→479). These aren't bugs today but are a maintenance hazard.

---

## 📊 Performance Improvement Opportunities

| Area | Current | Potential Improvement |
|---|---|---|
| **Radix sort backend** | Unbounded recursion with huge stack usage | Iterative MSD radix sort, or just drop it in favor of SA-IS |
| **SA-IS memory** | 7+ separate `malloc` calls per invocation | Single arena/slab allocation — one big buffer, carve out sub-arrays |
| **Bitvector** | Byte-at-a-time access | Use `uint64_t` words for bulk operations |
| **LMS bucket filling** | Copies `END` array each time | Inline the copy into the fill loop, or use a single workspace buffer |
| **Retransform** | O(n) with decent cache behavior | Already good; could consider in-place retransform to save memory |
| **`MAX_CHUNK` size** | 900 KB | Increasing to several MB would reduce overhead from repeated SA-IS setup |

---

## 🧪 Test Coverage Gaps

The existing 13 tests are a good start but miss several important cases:

- **Binary data** with bytes 0x00–0xFF (the sentinel is at position `n`, not in the data — but do bytes like `\0` work?)
- **Large inputs** — all tests use strings ≤ 6 characters
- **Inputs near `MAX_CHUNK` boundary** — chunked transform/retransform logic in `main.c`
- **Adversarial inputs** — long runs of the same character with one different character (worst case for radix sort)
- **Two-character alphabet** — triggers deeper SA-IS recursion
- **Stress round-trip** — random data of various sizes, verified with `memcmp`

---

## 📝 Summary

| Aspect | Rating | Notes |
|---|---|---|
| **Correctness** | ⭐⭐⭐ | Core SA-IS is correct for tested inputs. The retransform has an edge-case bug (§1). Radix sort has a header size mismatch (§2). |
| **Robustness** | ⭐⭐ | No input validation for large files, binary data corruption from newline stripping, unbounded recursion in radix sort. |
| **Code quality** | ⭐⭐⭐ | Clean formatting, consistent naming, good test harness. Dragged down by massive code duplication and manual free chains. |
| **Performance** | ⭐⭐⭐⭐ | SA-IS is the right algorithmic choice (linear time). Memory allocation overhead and bitvector access are the main bottlenecks. |
| **Test coverage** | ⭐⭐ | Good variety of small cases, but no large/binary/stress tests. |

> [!TIP]
> The most impactful improvements would be: (1) fix the retransform `next_row == initialIndex` bug, (2) unify the `uc*/sizet*` code duplication with macros, and (3) add large-input round-trip tests to catch edge cases.
