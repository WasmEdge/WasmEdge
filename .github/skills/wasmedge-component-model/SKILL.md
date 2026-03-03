---
name: wasmedge-component-model
description: Development guidelines for WebAssembly Component Model proposal implementation in WasmEdge
license: Apache-2.0
---

# WasmEdge Component Model Development Guidelines

Follow this 4-step workflow for **all** Component Model contributions:

## 1. Check the Task Under Issue #4236

**Start here**: <https://github.com/WasmEdge/WasmEdge/issues/4236>

Issue #4236 tracks the Component Model implementation. All tasks are in this issue or its sub-issues.

### Before Starting Development

- ✅ **Read the issue** - Understand requirements and acceptance criteria
- ✅ **Check sub-issues** - Many tasks are in smaller sub-issues
- ✅ **Identify dependencies** - Does this depend on other incomplete features?
- ✅ **Note spec test requirements** - If spec tests are mentioned, they MUST pass
- ✅ **Review spec** - Read relevant sections at <https://github.com/WebAssembly/component-model>
- ✅ **Claim the task** - Comment on the issue

### Spec Test Requirements (CRITICAL)

**If the issue mentions spec tests → they are MANDATORY.**

Look for:

- "Must pass spec test: `component-model/alias`"
- Links to test suite path on the WasmEdge-spectest repo

## 2. Develop Your Code

Implement following WasmEdge coding standards.

### Coding Style

```cpp
// Naming (enforced by .clang-tidy)
class ComponentInstance {};     // CamelCase
Expect<void> validateTypes();   // camelBack
std::unique_ptr<Module> Module; // CamelCase

// File headers (MANDATORY)
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

// Include order
#include "ast/component/canonical.h"  // 1. Corresponding header
#include "ast/component/type.h"       // 2. Component headers
#include "common/errinfo.h"           // 3. Core headers
#include <memory>                     // 4. Standard library
```

### Error Handling (MANDATORY)

```cpp
// ALWAYS use Expect<T>
Expect<void> validateComponent(const Component &Comp) {
  EXPECTED_TRY(validator.validate(Comp));  // Validate FIRST
  EXPECTED_TRY(executor.instantiate(Comp));
  return {};
}
```

## 3. Follow Build, Test, Lint, and Commit Rules

### 3.1 Build

```bash
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Debug -DWASMEDGE_BUILD_TESTS=ON
cmake --build build
```

**✅ Build must succeed with zero warnings.**

### 3.2 Test

```bash
cd build
ctest
```

**Write comprehensive tests:**

```cpp
TEST(ComponentMyFeatureTest, Input) {
  auto Result = myFeature(validInput);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Result->getValue(), expected);

  Result = myFeature(invalidInput);
  EXPECT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::Expected);
}
```

**Coverage requirements:**

- ✅ Happy path, error cases, edge cases
- ✅ Index validation, type checking, encodings

**✅ All existing tests must pass.**

### 3.3 Lint

**CRITICAL: Lint ALL code, not just component files.**

```bash
# REQUIRED - must pass
bash .github/scripts/clang-format.sh $(which clang-format-18)

# Format all code
find include lib tools plugins examples -type f \( -iname "*.h" -o -iname "*.cpp" \) \
  | grep -v "/thirdparty/" \
  | xargs clang-format-18 -i -style=file
```

**✅ Zero linting errors required.**

### 3.4 Commit

```bash
git commit -s -m "feat(component): add canonical lift operation"
```

**Format:**

- Type: `feat`, `fix`, `refactor`, `test`, `docs`
- Scope: `component`, `validator`, `loader`, `executor`
- **Sign-off: MANDATORY** - use `git commit -s`

## 4. Pass Spec Tests (If Applicable)

**If the issue mentions spec tests → they MUST pass.**

### Run Spec Tests

```bash
cd build/test/component

# Run tests
./componentTests
```

### Expected Output (Passing)

```text
[  PASSED  ] 127 tests.
```

**✅ Spec compliance is MANDATORY when mentioned in issue.**

## Submission Checklist

- [ ] Issue #4236 or sub-issue reviewed
- [ ] Code follows naming conventions
- [ ] Build succeeds: `cmake --build build` ✅
- [ ] All tests pass
- [ ] Spec tests pass (if applicable)
- [ ] Linting passes: `bash .github/scripts/clang-format.sh` ✅
- [ ] Commits signed-off: `git commit -s` ✅
- [ ] Commit message linted

## Resources

- **Main Issue**: <https://github.com/WasmEdge/WasmEdge/issues/4236>
- **Component Spec**: <https://github.com/WebAssembly/component-model>
- **Canonical ABI**: <https://github.com/WebAssembly/component-model/blob/main/design/mvp/CanonicalABI.md>
- **General Guidelines**: `AGENTS.md`
