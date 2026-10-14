# Upstream sync — 2026-10-14

Merge of `upstream/master` (`04c5b664`, 52 commits) into the fork's `master`
(`bbba4a32`, 301 commits ahead). **Not compiled.** No Conan/CMake environment
was available when this was done, so treat the result as unverified until it
builds and the game starts.

## How conflicts were resolved

104 files / 138 hunks conflicted. They were resolved by a deterministic script,
not by hand:

| Kind | Hunks | Rule |
|---|---|---|
| Hook registration lines (`RH_Scoped*Install…`) | 110 | Merge both sides per function. The merged line uses upstream's option syntax. A function counts as **reversed if either side reversed it** (the fork reverses many functions upstream has not). |
| Function bodies | 26 | Take **upstream's** body, unless upstream's is a `NOTSA_UNREACHABLE` stub and the fork's is an implementation (none hit that case). |
| Headers | 2 | Take upstream's: `Formation.h`, `Interior/Interior_c.h`. **Review these.** |

After the merge, two API migrations were applied tree-wide (they affect fork-only
code that did not conflict textually):

* Hook options: `.reversed` → `.Reversed`, `.locked` → `.Locked`,
  `.stackArguments = N` → `.StackArgumentsToPreserve = N, .PreserveRegisters = true`,
  `.jmpCodeSize` dropped, `.enabled = false` → `.Unhooked = true`.
* `CTimer` statics are private upstream: 42 direct uses in fork code were
  rewritten to the new accessors (`GetTimeInMS()`, `GetFrameCounter()`,
  `GetTimeStep()`, `SetTimeStep()`, …).

## Known risks

1. **`Formation.h` / `Formation.cpp`.** The fork documented (raw disassembly)
   that `CFormation` has 24 links per array and lives at fixed game addresses
   (`StaticRef`, `NUM_FORMATION_LINKS = 24`). Upstream finished `CFormation`
   with its own storage and `TOTAL_PED_GROUP_MEMBERS` (8). This merge took
   upstream's. If the fork's finding is right, this regresses it.
2. **Bodies taken from upstream may have dropped fork-only helpers** declared in
   the same hunk; a build will show them as unresolved symbols.
3. **A function the fork reversed but upstream did not** keeps the fork's body
   only if the hunk did not conflict; the registration line says "reversed" in
   either case.
4. Anything else upstream changed in the build system (CMake 4.x, Conan recipe)
   was taken as-is.

## To accept

```
git checkout master && git merge --ff-only merge/upstream-2026-10-14
```

(after it builds). To drop it, delete the branch; `master` was not touched.
