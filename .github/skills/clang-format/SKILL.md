---
name: clang-format
description: "Use when formatting C++ files with clang-format. Covers formatting a single file, all files, or only changed files. Formatting is mandatory — CI enforces it on push."
---

# Clang Format

## Policy

Formatting is **mandatory**, not optional. A CI check runs on every push and will fail if any `.cpp` or `.hpp` file is not correctly formatted. Always format before committing.

## Finding clang-format

`clang-format` is **not** on PATH in a plain PowerShell. It ships inside the Visual Studio installation. Locate it with:

```powershell
$clangFormat = (Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio" -Filter "clang-format.exe" -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.DirectoryName -match 'Llvm\\x64\\bin' } | Select-Object -First 1).FullName
```

Then use `& $clangFormat` in place of bare `clang-format` in all commands below. Alternatively, open a **Developer PowerShell for VS** where it is already on PATH.

## Configuration

The project uses the `.clang-format` file at the repository root. **Never use local or personal clang-format settings** — always use the repo's configuration (`--style=file`), which is the default when `.clang-format` exists in a parent directory.

## Format a Single File

```powershell
clang-format -i <file>
```

The `-i` flag formats in-place.

## Format All C++ Files

```powershell
Get-ChildItem -Recurse -Include *.cpp,*.hpp -Path include,source,tests,benchmarks | ForEach-Object { clang-format -i $_.FullName }
```

## Format Only Changed Files

Format files with uncommitted changes:

```powershell
git diff --name-only --diff-filter=d -- '*.cpp' '*.hpp' | ForEach-Object { clang-format -i $_ }
```

Format files changed since a branch point (e.g. vs `main`):

```powershell
git diff --name-only --diff-filter=d main -- '*.cpp' '*.hpp' | ForEach-Object { clang-format -i $_ }
```

## Check Without Modifying

To check if files are correctly formatted without changing them (dry run):

```powershell
clang-format --dry-run --Werror <file>
```

A non-zero exit code means the file needs formatting.

## When to Format

- After writing or modifying any `.cpp` or `.hpp` file
- Before every commit
- If CI fails the clang-format check
