---
name: dwmblocksc-upstream-sync
description: Port new commits from the user's dwmblocks fork (~/.config/dwmblocks, the C dwmblocks that dwmblocksr and dwmblocksc derive from) into dwmblocksc, one at a time and in order - but only those that are applicable and worth having, following the decision dwmblocksr's sync already made for each commit (dwmblocksr is dwmblocksc's specification) and deciding alone only where dwmblocksr skipped for a Rust-only or build-only reason. Every dwmblocks commit after the baseline has a row in UPSTREAM_SYNC.md - pending until looked at - with its origin, kind, dwmblocksr's commit, status, handling and the dwmblocksc commit, and upstream_info.txt names the dwmblocks commit dwmblocksc is synced through. Ports each commit following the dwmc-port skill's dwmblocksc rules, verifies with make, commits each with the dwmblocks/dwmblocksr subject and dwmblocks-commit/dwmblocksr-commit trailers, followed by a ledger commit. Asks before anything that would make dwmblocksc differ from dwmblocksr.
disable-model-invocation: true
argument-hint: "[empty means every remaining commit within the run budget, or: setup, status, rebuild, <dwmblocks-commit>, skip <dwmblocks-commit> <reason>, replay <dwmblocks-commit>]"
---

# Sync dwmblocks commits into dwmblocksc

dwmblocksc (`~/Code2/C/dwmblocksc`, branch `main`, remote `origin` =
`https://github.com/ornfelt/dwmblocksc`) is the C port of dwmblocksr, which is itself the Rust port
of the user's dwmblocks fork. The **source** of this sync is that dwmblocks, and dwmblocksr is the
**specification** dwmblocksc follows:

| | |
| --- | --- |
| Source repo | `~/.config/dwmblocks` (branch `master`, `https://github.com/ornfelt/dwmblocks`) - torrinfail/dwmblocks plus the user's changes and blocks |
| How it gets upstream fixes | its own `dwmblocks-upstream-sync` skill lands `https://github.com/torrinfail/dwmblocks` commits there, with their upstream subject |
| Specification | dwmblocksr, `~/Code2/Rust/dwmblocksr`: its `UPSTREAM_SYNC.md` decides each dwmblocks commit first, and its commit for that dwmblocks commit is what dwmblocksc ports |
| Read with | `git -C ~/.config/dwmblocks ...` and `git -C ~/Code2/Rust/dwmblocksr ...` - no remotes are added |

**Read before porting anything:**

- `~/Code2/C/dwmc/.claude/skills/dwmc-port/SKILL.md` - the port skill for both dwmc and
  dwmblocksc (it lives in dwmc). It is the authority on how dwmblocksr becomes C: the priorities
  (never crash, never leak, only async-signal-safe calls after `fork()`, no allocation per update,
  minimal suckless C, fidelity to dwmblocksr), the layout, `blocks.def.h` / `config/blocks.h`, the
  Rust-to-C translation rules ("dwmblocksr's representations, back to dwmblocks'"), the scope
  guard, the verification and the commit rules.
- `~/Code2/C/dwmc/PORT_STATUS.md`, section "dwmblocksc decisions" - how dwmblocksc was laid out
  (static arrays sized `LENGTH(blocks)`, startup checks instead of `parse()`, `Block.battery`,
  the `ASYNC` define, `sigplus`). New code follows these decisions.
- `~/Code2/Rust/dwmblocksr/.claude/skills/dwmblocksr-dev/SKILL.md` - how dwmblocksr represents
  dwmblocks, so a dwmblocksr commit can be read back into dwmblocks' C.
- `~/Code2/Rust/dwmblocksr/.claude/skills/dwmblocksr-upstream-sync/SKILL.md` - how dwmblocksr
  decides whether a dwmblocks commit belongs in the port (its section 4 and "dwmblocksr's
  deliberate differences"), the ledger format, and the skip reasons this skill reads.

This skill only adds *which* dwmblocks commits dwmblocksc takes, in which order, and how they are
tracked.

dwmblocksc must behave exactly like dwmblocksr (dwmc-port: "the Rust wins"): the same status text,
signal bytes, click protocol and `BLOCK_BUTTON`. So dwmblocksc **does not re-decide** a dwmblocks
commit dwmblocksr has decided: it ports what dwmblocksr ported and skips what dwmblocksr skipped -
with one exception, the commits dwmblocksr skipped because Rust cannot have the problem or Cargo
replaces the build files; C can, and dwmblocksc has a C `Makefile`/`config.mk`, so dwmblocksc
decides those itself (see section 4). Every dwmblocks commit still gets a row, so
`UPSTREAM_SYNC.md` always shows what was ported and why the rest was not.

**Scope guard** (dwmc-port's, plus the user's config copy): write only inside
`~/Code2/C/dwmblocksc`, plus `~/.config/dwmblocksc/blocks.h` under the rule in section 5.
**Never edit** `~/.config/dwmblocks`, `~/Code2/Rust/dwmblocksr`, `~/.config/dwmblocksr` or
`~/Code2/C/dwmc` (including its `PORT_STATUS.md` - this skill keeps its own ledger). If dwmblocksr
or dwmblocks looks wrong, port it right for dwmblocksc when it is a crash, leak or unsafe call,
otherwise faithfully, and flag it (see "Flagged").

## Arguments

- **none** - port the remaining dwmblocks commits, oldest first, within the per-run budget below.
- `setup` - or the ledger does not exist yet: create `UPSTREAM_SYNC.md` with the baseline and a
  row for every dwmblocks commit after it, filled per "Picking up the current state", update
  `upstream_info.txt`, commit both (see "Committing the ledger"), then continue as **none**.
- `status` - only report the baseline, the high-water mark, the remaining list (and which rows
  wait on dwmblocksr), the open questions and the open "Flagged" entries. When the ledger
  disagrees with the tree, report what "Picking up the current state" would change. Write nothing.
- `rebuild` - redo the ledger per "Picking up the current state", keeping the notes, Special
  handling entries, flags and open questions of rows the rebuild confirms. A ledger-only run.
- `<dwmblocks-commit>` - port just that dwmblocks commit, even out of order (it still needs
  dwmblocksr's decision).
- `skip <dwmblocks-commit> <reason>` - record the commit as `skipped` (reason `declined`) without
  porting it, Handling `requested`, with the reason under "Special handling" in the user's words.
- `replay <dwmblocks-commit>` - redo a commit that already has a row, because the earlier port was
  wrong or incomplete, or because dwmblocksr's row changed since (a dwmblocksr replay). See
  "Replay".

## 1. Make sure the trees are ready

```bash
cd ~/Code2/C/dwmblocksc
git status --short             # must be clean before starting - otherwise stop and ask
git -C ~/.config/dwmblocks log -1 --format='%h %cs %s' master    # the source's current HEAD
git -C ~/Code2/Rust/dwmblocksr status --short                     # report if dirty; read only committed state
```

Only **committed** history counts, in both `~/.config/dwmblocks` (local `master`) and dwmblocksr
(local `main`). dwmblocksr's ledger is read from its commit:
`git -C ~/Code2/Rust/dwmblocksr show main:UPSTREAM_SYNC.md`. If dwmblocksr has no ledger yet,
every row here waits on dwmblocksr: say so and suggest running `/dwmblocksr-upstream-sync` in
`~/Code2/Rust/dwmblocksr` first.

## 2. The baseline

The baseline is **`7c2db2f`** (`7c2db2fe9bd6cb6eae0851705ff494dbec9a5e8a`) - "Blocks without
icons, two space delimiter" (2026-10-31), ported as dwmblocksr `148ca90` and dwmblocksc `7321ad4`.
Everything at or before it was mirrored into dwmblocksc before this skill existed (the port from
dwmblocksr on 2026-09-25, then one dwmblocksc commit per dwmblocksr change, with the same subject)
and never comes up again. It is the same baseline as dwmblocksr's, so the two ledgers have the
same rows. The baseline is fixed and recorded in the ledger header; `upstream_info.txt` started
there but moves on with the high-water mark.

```bash
BASE=7c2db2fe9bd6cb6eae0851705ff494dbec9a5e8a
git -C ~/.config/dwmblocks log --reverse --date-order --format='%h %cs %p | %s' "$BASE..master"
```

If `$BASE` is ever not an ancestor of dwmblocks' `master` (history rewritten there), stop and ask.

## 3. The ledger

`UPSTREAM_SYNC.md` in the repo root (tracked) is the progress file. Every run reads it first and
updates it last. It is a hint that makes orientation cheap, not the source of truth - the code is.

Shape:

```markdown
# dwmblocksc upstream sync ledger

Maintained by the `dwmblocksc-upstream-sync` skill. A hint, not the source of truth - the code is.

Source: the user's dwmblocks fork, `~/.config/dwmblocks` `master` (<https://github.com/ornfelt/dwmblocks>).
Specification: dwmblocksr, `~/Code2/Rust/dwmblocksr` - its `UPSTREAM_SYNC.md` decides each row first.
Baseline: `7c2db2f` - Blocks without icons, two space delimiter (2026-10-31), dwmblocksr `148ca90`, dwmblocksc `7321ad4`.
Everything up to and including the baseline was ported before this ledger existed.
High-water mark: `<sha>` `<subject>` (every row up to and including this one is decided)
dwmblocks HEAD when last checked: `<sha>` `<subject>` (<date of the run>)
Remaining: `<n>` (`pending` and `asked` rows; `<m>` of them wait on dwmblocksr)

Statuses: `pending` (not looked at yet, or waiting on dwmblocksr), `ported` (the change carried
over 1:1), `adapted` (ported, but reshaped - by dwmblocksr, or for dwmblocksc), `partial` (part
ported, part skipped - the notes say which), `skipped` (not ported - the notes start with the
reason), `asked` (waiting on an answer from the user, nothing written).

Origin: `own` (the user's own dwmblocks commit), `upstream` (a torrinfail/dwmblocks commit landed
in the fork by `dwmblocks-upstream-sync`), `merge` (a merge commit). Kind: `code`, `config`,
`docs`, `build`, `housekeeping`. Both are copied from dwmblocksr's row.

Handling, measured against dwmblocksr's row: `as-is` (dwmblocksc does what dwmblocksr did - ported
the same change, or skipped for the same reason), `needed` (dwmblocksc differs from dwmblocksr
because C needs it: a fix dwmblocksr skipped as not applicable to Rust, a build change, a C-only
safety detail), `requested` (the user asked). Anything but `as-is` has an entry under Special
handling.

Skip reasons: dwmblocksr's (`housekeeping`, `already in dwmblocksr`, `not applicable (rust)`,
`not applicable (build)`, `no counterpart`, `deviation`, `declined`, `merge, changes nothing`),
marked `(follows dwmblocksr)` when taken over, plus `already in dwmblocksc` for the commits
dwmblocksc decided itself.

| # | Date | dwmblocks commit | Origin | Kind | Subject | dwmblocksr | Status | Handling | dwmblocksc commit | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

## Special handling

<one bullet per row whose Handling is not `as-is`: the row and commit, `needed` or `requested`,
how dwmblocksc differs from dwmblocksr's handling of it, and why - the C reason, or the user's
request in their own words>

## Flagged

<one bullet per evident bug found while porting - in the dwmblocks commit's change (for dwmblocks)
or in dwmblocksr's port of it (for dwmblocksr) - see "Flagged">

## Open questions

<one bullet per `asked` row: the question put to the user and what it is waiting on>
```

Rules for the ledger:

- **One row per dwmblocks commit after the baseline, every one of them**, in the same order and
  with the same `#` as dwmblocksr's ledger (both append in dwmblocks' `--date-order` order from the
  same baseline). Each run first adds `pending` rows for the commits that have none yet. Rows
  never move. `Date` is the dwmblocks commit date.
- `dwmblocksr` is dwmblocksr's decision for the row, as `<status> <dwmblocksr commit>`
  (`ported 1a2b3c4`, `skipped -`), or `pending` while dwmblocksr's row is `pending`/`asked`.
  Refresh it every run.
- A `skipped` row's notes **start with the skip reason** from the list, then the specifics.
- `dwmblocksc commit` is the short sha of the dwmblocksc commit carrying the `dwmblocks-commit:`
  trailer, or `-`.
- **Handling** as above; `requested` wins over `needed`.
- The **high-water mark** is the last row before the first row that is still `pending` or `asked`
  (a row waiting on dwmblocksr counts as `pending`). A commit ported out of order does not move it.
- `Remaining` is the `pending` and `asked` rows, recomputed each run.
- The `Notes` column says which dwmblocksc functions changed, what was translated differently from
  the dwmblocksr commit and why, what went into `blocks.def.h`, `config/blocks.h` and the user's
  blocks, and `flagged` when the row has an entry under "Flagged".
- If the ledger disagrees with dwmblocksc's `main`, **the tree wins** and the rows concerned are
  rebuilt per "Picking up the current state" in the same run.

### upstream_info.txt

`upstream_info.txt` in the repo root (tracked) names the dwmblocks commit dwmblocksc is synced
through - the high-water mark's full hash (the baseline while no row after it is decided):

```text
dwmblocks
  commit <full hash of the high-water mark>
```

It changes whenever the high-water mark moves, and is committed with the ledger. It is never ahead
of dwmblocksr's.

### Picking up the current state

For each dwmblocks commit after the baseline, in ledger order:

1. **An earlier sync**: `git log --format='%h %s' --grep="^dwmblocks-commit: <full sha>" main`
   finds the dwmblocksc commit that carries it (fall back to the subject with the name swaps of
   section 5). Found: `ported`, `adapted` or `partial` as dwmblocksr's row (or as the dwmblocksc
   diff shows), naming it.
2. **dwmblocksr's row**: copy origin, kind and the `dwmblocksr` column from dwmblocksr's ledger. A
   dwmblocksr skip whose reason dwmblocksc follows (section 4) becomes the same skip,
   `(follows dwmblocksr)`.
3. **Already in dwmblocksc**: for a dwmblocksr skip dwmblocksc decides itself, check dwmblocksc's
   code by intent: `skipped (already in dwmblocksc ...)` when it is there.
4. Otherwise `pending`.

Recompute the high-water mark, `Remaining` and `upstream_info.txt` from the rows. Say in the report
how many rows this added or changed.

## 4. Deciding, from dwmblocksr's row

Read dwmblocksr's row for the commit, then:

| dwmblocksr's row | dwmblocksc |
| --- | --- |
| `pending` or `asked` | **wait**: stop the run here, before this row (later rows wait too, to keep the order). Report that dwmblocksr has to decide it first. |
| `ported`, `adapted`, `partial` | **port dwmblocksr's commit** to C (section 5). Same status as dwmblocksr; Handling `as-is` unless C needs more. A `partial` leaves out what dwmblocksr left out. |
| `skipped (housekeeping)`, `(merge, changes nothing)`, `(no counterpart)`, `(deviation)`, `(declined)` | **skip, follows dwmblocksr**, Handling `as-is`. For `no counterpart`, check that dwmblocksc has no counterpart either (it dropped what dwmblocksr dropped: `NO_X`, the OpenBSD paths). |
| `skipped (already in dwmblocksr)` | **check dwmblocksc has it too**: yes - `skipped (already in dwmblocksr) (follows dwmblocksr)`; no - a parity gap: port the dwmblocks change into dwmblocksc, Handling `needed`, and say so in the report. |
| `skipped (not applicable (rust))` | **decide alone**: the bug may well exist in C. Check dwmblocksc's code (it mirrors dwmblocksr's logic, but in C: buffers and `CMDLENGTH`, UTF-8 truncation, prototypes and `signal()` declarations, C standard compatibility, leaks, async-signal-safety). dwmblocksc has the bug - port the dwmblocks fix, which is C already and often fits dwmblocksc.c almost verbatim, Handling `needed`. dwmblocksc's code already rules it out (dwmc-port's rules: bounded copies, `truncate_utf8`, the static arrays, envp built before `fork()`) - `skipped (already in dwmblocksc ...)`, naming the code. |
| `skipped (not applicable (build))` | **decide alone**: dwmblocksc has a C `Makefile` and `config.mk`. Take the change when it applies to dwmblocksc's build (a flag, a library, an install step, a C standard), Handling `needed`; else `skipped (not applicable (build)) (follows dwmblocksr)`. compile.sh's build steps have no counterpart in dwmblocksc either; its block selection follows dwmblocksr (the `battery` field, `hasbattery()`, `selectblocks()`). |

dwmblocksc never ports a dwmblocks change dwmblocksr skipped for a behavioural reason, and never
skips one dwmblocksr ported. If following dwmblocksr would be wrong for dwmblocksc - a C crash,
leak or unsafe call in dwmblocksr's logic, a C-only problem - fix it in dwmblocksc, record
`needed`, and flag it for dwmblocksr. If dwmblocksc and dwmblocksr would end up behaving
differently (any byte of the status text, signal handling, clicks, exit), **ask** (section 5,
step 6).

### The file map

| dwmblocks (`~/.config/dwmblocks`) | dwmblocksr (the spec) | dwmblocksc | Kind |
| --- | --- | --- | --- |
| `dwmblocks.c` | `src/dwmblocks.rs`, `src/main.rs` | `dwmblocksc.c` | code |
| `blocks.def.h` | `Config::default()` in `src/config.rs` and `config/config.toml` | `blocks.def.h` and `config/blocks.h`, and the user's `~/.config/dwmblocksc/blocks.h` (section 5); a new `parse()` validation becomes a startup check in `main()` | config |
| `compile.sh` | `battery` key / `hasbattery()` / `selectblocks()` in `src/config.rs` | `Block.battery`, `hasbattery()`, `selectblocks()` in `dwmblocksc.c` | config / build |
| `Makefile` | `Cargo.toml`, `Makefile` | `Makefile`, `config.mk` | build |
| `README.md` | `README.md`, `dwmblocksr.1` | `README.md`, `dwmblocksc.1` | docs |
| `.claude/`, `*.diff`, `UPSTREAM_SYNC.md`, `upstream_info.txt`, `.gitignore`, `LICENSE` | nothing | nothing | housekeeping |

Tests: a dwmblocksr `#[cfg(test)]` change that is not about the TOML loader goes into `test.c`
(e.g. `icons_match_blocks_h`).

## 5. Port each commit, in order

The work list is the ledger's `pending` rows, oldest first, up to the first one that waits on
dwmblocksr (for `<dwmblocks-commit>`, just that one). If nothing is left, say dwmblocksc is caught
up (or caught up with dwmblocksr, naming what dwmblocksr still has to decide) and stop.

For each commit in the work list:

1. **Decide** per section 4. A skip needs no dwmblocksc commit: fill in the row, then move on (it
   is committed with the next ledger commit).
2. **Read both diffs**: dwmblocksr's commit (`git -C ~/Code2/Rust/dwmblocksr show <commit>`) is
   *what* to do - its logic, its deviations from the dwmblocks diff, its block values, its docs and
   tests. The dwmblocks commit (`git -C ~/.config/dwmblocks show <sha>`) is *how it looks in C* -
   dwmblocksc.c reads like dwmblocks.c, so take dwmblocks' hunks as the starting point and bring
   them to what dwmblocksr does. Where they disagree, **dwmblocksr wins**, and the deviation gets
   dwmblocksr's comment.
3. **Port it** per the dwmc-port skill and the "dwmblocksc decisions" in `PORT_STATUS.md`:
   functions in dwmblocksr's (dwmblocks') order, dwmblocks' naming and comment style, no allocation
   per update (static arrays sized `LENGTH(blocks)`), only async-signal-safe calls after `fork()`
   and in handlers, bounds checks where the Rust had them, unsigned arithmetic where the Rust
   wrapped, both `ASYNC` branches kept in step with `getcmd_async`/`getcmd_sync`.
4. **Config** - dwmblocksr's `Config::default()` / `config/config.toml` change goes into
   `blocks.def.h` **and** `config/blocks.h` (layout from `~/.config/dwmblocks/blocks.def.h`, values
   from the TOML, icons as UTF-8 characters or `\x` escapes as blocks.h writes them), with the
   name swap `dwmblocksr` -> `dwmblocksc`. Then the **user's copy**,
   `~/.config/dwmblocksc/blocks.h`: if it was identical to the shipped `config/blocks.h` before this
   change (`git show HEAD:config/blocks.h | diff - ~/.config/dwmblocksc/blocks.h`), apply the same
   change to it so the two stay identical; otherwise leave it alone and say in the report exactly
   what to add. This standing rule is the user's request and overrides dwmc-port's "never
   overwrite an existing one". The user's copy is never part of a commit, and no backup copy is
   made.
5. **Docs and tests** - mirror dwmblocksr's `dwmblocksr.1` / `README.md` changes into
   `dwmblocksc.1` / `README.md` (names swapped), and its non-loader unit tests into `test.c`.
6. **Ask first** - stop before committing when dwmblocksc would have to behave differently from
   dwmblocksr, when following dwmblocksr looks wrong for C beyond a plain crash/leak/unsafe-call
   fix, or when a decision dwmblocksc makes alone (section 4) is not clear-cut. Explain what the
   commit does, what dwmblocksr did, what would change in dwmblocksc and why, with a recommended
   path, and wait. Leave the tree as it was (`git reset --hard HEAD && git clean -fd` after showing
   the planned change - the tree was clean at the start; restore the user's blocks copy if it was
   touched), record the row as `asked` with the question under "Open questions", and do not
   continue with later commits. Porting it after a "yes" is `requested`.
7. **Verify** (see below). Fix failures caused by this port; report pre-existing ones.
8. **Commit** - one dwmblocksc commit per dwmblocks commit, never squashed:

   ```bash
   git commit -m "<subject>" -m "<body>" -m "dwmblocks-commit: <full dwmblocks sha>
   dwmblocksr-commit: <full dwmblocksr sha>"
   ```

   - **Subject**: dwmblocksr's commit subject with `dwmblocksr` -> `dwmblocksc` (and
     `config.toml` -> `blocks.h` where it names the config file), otherwise unchanged - which is
     normally the dwmblocks subject itself (`Blocks without icons, two space delimiter` in all three
     repos). For a commit dwmblocksc ports without a dwmblocksr commit (a `needed` row), the
     dwmblocks subject with the same swaps. No `dwmblocksc:` prefix - that prefix belongs to the
     dwmc-port skill's porting commits.
   - **Body**: what the change does in dwmblocksc (dwmblocksr's body is the starting point, names
     and files swapped: "the icon fields are empty in blocks.def.h and config/blocks.h"), what was
     translated differently from dwmblocksr and why, and what was tested. Wrap at 72.
   - **Trailers**: `dwmblocks-commit: <full dwmblocks sha>`, then
     `dwmblocksr-commit: <full dwmblocksr sha>` when dwmblocksr has a commit for it, as the last
     lines.
   - **No `Co-Authored-By`** or any other attribution line, on this commit or the ledger commit.
   - The ledger is **not** part of this commit: edit `UPSTREAM_SYNC.md` and `upstream_info.txt`
     only after it exists.
9. **Record** the row, its Special handling / Flagged entries, add pending rows for new dwmblocks
   commits, move the high-water mark, update `upstream_info.txt`, and commit the ledger:

   ```bash
   git add UPSTREAM_SYNC.md upstream_info.txt && git commit -m "update upstream sync ledger"
   ```

Then move on to the next commit.

### Flagged

While porting, check the dwmblocks commit's own change and dwmblocksr's port of it for an evident
bug: a crash, an out-of-bounds access, a leak, a wrong condition, a value used before it is set, a
non-async-signal-safe call after `fork()` or in a handler, behaviour that contradicts the commit
message, or dwmblocksr doing something different from the dwmblocks commit without a comment
saying it is deliberate. Style is not a bug. dwmblocksc gets the commit without the crash, leak or
unsafe call (Handling `needed`); a behavioural difference is ported as dwmblocksr has it. Add an
entry under "Flagged": the row and commit, where the bug is (dwmblocks or dwmblocksr, file and
function), what goes wrong, a repro, the suggested fix, and the decision, `open` until the user
decides. Put `flagged` in the row's notes. Never edit dwmblocks or dwmblocksr.

### Committing the ledger

The ledger and `upstream_info.txt` are committed in the `update upstream sync ledger` commit after
each ported commit. A run that changes them without porting a commit - skips, `asked`, `skip`,
`setup`, `rebuild`, refreshing the `dwmblocksr` column - commits them the same way. When such
decisions come before a ported commit in the same run, fold them into that commit's ledger commit
instead. No attribution trailer.

### Replay

`replay <dwmblocks-commit>` never rewrites a commit already in `main`. For a ported commit, the
correction - the rest of a `partial` port, a wrong translation, catching up with a dwmblocksr
replay (a dwmblocksr follow-up commit carrying the same `dwmblocks-commit:` trailer) - is a new
dwmblocksc commit on top, named after the dwmblocksr follow-up when there is one, or for what it
does (`<function>: <what was fixed> (follow-up to dwmblocks <short sha>)`), with the same trailers
(the `dwmblocksr-commit:` of the follow-up), verified, followed by the ledger commit. The row is
updated in place. For a commit not ported yet (`asked`, `skipped`), replay ports it per this
section.

To find dwmblocksr follow-ups dwmblocksc has not caught up with:
`git -C ~/Code2/Rust/dwmblocksr log --format='%h %s' --grep='^dwmblocks-commit: ' main` lists
every dwmblocksr sync commit; one whose sha is not in any dwmblocksc `dwmblocksr-commit:` trailer
is missing here.

## How much per run

One dwmblocks commit at a time - one dwmblocksc commit (plus its ledger commit) per ported
dwmblocks commit - until roughly **500 changed lines** of dwmblocksr diff have been ported in the
run (`git -C ~/Code2/Rust/dwmblocksr show --numstat <commit>`, not counting `UPSTREAM_SYNC.md`), or
stop earlier at a logical point: a question for the user, a row that waits on dwmblocksr, a large
commit better done in its own run, or a group of related commits just completed. Do not stop in
the middle of such a group because the budget is reached. Skips cost nothing against the budget.

Do not push. The user pushes. Do not run `sudo make install`; that is the user's call.

## Verification

dwmc-port's "Verification" for dwmblocksc, every time code or config changed - gcc and clang, both
configs, both `ASYNC` variants, the sanitized unit tests and the static analyzer:

```sh
cd ~/Code2/C/dwmblocksc
make clean && make                              # gcc, warning-free
make clean && make CC=clang                     # clang, warning-free
make clean && make CC='cc -DASYNC=0'            # the sync variant
make clean && make CONFDIR=/nonexistent         # built with blocks.def.h
make test                                       # ASan + UBSan: blocks.def.h and config/blocks.h, ASYNC 1 and 0
make clean && scan-build-19 make                # no reports in dwmblocksc.c
```

`dwmblocksc` and `blocks.h` in the tree may be owned by root from `sudo make install`; `make clean`
can still remove them (the directory is the user's). If anything in the tree is in the way, build a
throwaway copy instead:
`TMP=$(mktemp -d) && git ls-files -z | xargs -0 cp --parents -t "$TMP" && make -C "$TMP" ...`,
then `rm -rf "$TMP"`.

For a `code` commit that changes behaviour, also run dwmc-port's dwmblocksc functional test
(`make debug` in a local Xvfb): the root name read with a tiny `XFetchName` program and `od -c`,
compared byte for byte against `dwmblocksr` built from the dwmblocksr commit with the same blocks,
in both `ASYNC` modes and with `-p`; update signals (`kill -$((34+n))`), clicks via `sigqueue`
with `BLOCK_BUTTON`, SIGTERM exits 0 with an empty root name, no zombies, no sanitizer report - and
exercise what the commit changed. A config-only commit needs `make test` passing with the new
`config/blocks.h`. If a check cannot run, say so; never report it as passed.

## Report

At the end, list per dwmblocks commit: sha, date, origin, kind, subject, dwmblocksr's decision,
dwmblocksc's status and handling with the skip reason or what was translated differently, or the
question or dwmblocksr row it stopped on. Give any "Flagged" entry with its repro and suggested
fix, any special handling with its reason, and what was done to `~/.config/dwmblocksc/blocks.h`
(updated, or what the user must add). Say how many `pending` rows were added and which rows a
rebuild changed. Include the checks run and their results, the dwmblocksc commits created, the new
high-water mark and `upstream_info.txt`, how many rows remain and how many of those wait on
dwmblocksr.
