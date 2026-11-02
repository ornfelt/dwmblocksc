# dwmblocksc upstream sync ledger

Maintained by the `dwmblocksc-upstream-sync` skill. A hint, not the source of truth - the code is.

Source: the user's dwmblocks fork, `~/.config/dwmblocks` `master` (<https://github.com/ornfelt/dwmblocks>).
Specification: dwmblocksr, `~/Code2/Rust/dwmblocksr` - its `UPSTREAM_SYNC.md` decides each row first.
Baseline: `7c2db2f` - Blocks without icons, two space delimiter (2026-10-31), dwmblocksr `148ca90`, dwmblocksc `7321ad4`.
Everything up to and including the baseline was ported before this ledger existed.
High-water mark: `7c2db2f` `Blocks without icons, two space delimiter` (the baseline; no rows yet)
dwmblocks HEAD when last checked: `7c2db2f` `Blocks without icons, two space delimiter` (2026-11-02)
Remaining: `0` (`pending` and `asked` rows; `0` of them wait on dwmblocksr)

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

None yet.

## Flagged

None yet.

## Open questions

None yet.
