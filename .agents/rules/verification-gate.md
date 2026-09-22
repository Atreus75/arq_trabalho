---
trigger: always_on
---

# Verification Gate

## Purpose
No change is complete until it passes every available verification tier in order. A tier skipped, suppressed, or unavailable is a documented gap requiring explicit user acceptance. This rule defines criteria for each tier, failure classifications, missing tooling protocols, and failure reporting.

---

## Phase 0: Edge Case Enumeration
Before running any verification tier, identify boundary conditions and failure modes introduced or affected by the current change. Skipping this phase means shipping with unknown blind spots.

### How to enumerate
For each changed module or entry point, consider:
- Minimum, maximum, empty, or null inputs.
- Concurrent invocation behavior.
- Slow, unavailable, or malformed downstream responses.
- Unanticipated user actions.
- Data created by prior system versions.

### Classification (mandatory for each case found)

| Class | Definition | Action |
|---|---|---|
| **Covered** | An existing or newly written automated test exercises this exact case | No action needed. Note the test name. |
| **Unhandled** | The case is real and no automated test covers it | Log as `[EC-NNN]` in `.agents/architecture_cache.md` Pending Edge Cases |
| **Cannot be automated** | The case requires human judgment, live environment, physical hardware, or UI interaction | Log as `[MT-NNN]` in `.agents/architecture_cache.md` Pending Manual Testing Steps |

### Edge Case ID format
```
[EC-NNN] <Category> — <brief description>
Source: <current session identifier>, <date>
Related: <file paths or test files covering adjacent behavior>
Status: Unhandled
```

Categories (use most specific): `GUI` | `API` | `Storage` | `Auth` | `Concurrency` | `Boundary` | `Network` | `State` | `Migration` | `Config` | `Security` | `Performance`

---

## The Four Tiers

> **Toolchain Invocation Protocol:** For all tiers below, read the toolchain command from `.agents/architecture_cache.md`. If absent, identify the relevant build, linter, or test runner command from project manifest files before running.

### Tier 1 — Compilation / Interpretation
**What it checks:** Modified code parses and compiles (or interprets without syntax errors) using the project build toolchain.

**Passes when:** The build command completes without errors. Warnings present before this change do not count as new failures.

**Fails when:** Any new syntax error, type error, missing import, or unresolvable symbol is introduced by this change.

**On failure:** Fix before moving to Tier 2. Do not proceed with a broken build under any circumstances. A build that does not compile cannot ship.

---

### Tier 2 — Static Analysis
**What it checks:** Linters, formatters, and type checkers report no new errors or warnings introduced by this change.

**Passes when:** The static analysis command exits cleanly for modified files, or all reported issues were already present before this change and documented as pre-existing.

**Fails when:** A new linter error, type annotation violation, or formatter divergence is introduced by this change.

**On failure:** Fix the underlying issue. Do not suppress warnings (inline disable comments, config exclusions, flag overrides) as a substitute for fixing code. If suppression is genuinely justified (known false positive), document the reason inline at the suppression site and in session output.

**Pre-existing issues:** If static analysis reveals pre-existing issues, log them in `.agents/architecture_cache.md` under Open Issues. Do not fix them in the same commit to preserve atomic commit discipline.

---

### Tier 3 — Automated Testing
**What it checks:** Full existing test suite passes against modified code without regression.

**Passes when:** Every test passing before this change still passes, and new tests introduced by this change pass.

**Fails when:** Any previously-passing test fails (genuine regression), or a new test introduced by this change fails.

**Blocker classification — mandatory before reporting:**

| Failure type | Classification | Blocking? |
|---|---|---|
| A test passing before this change now fails | **Genuine regression** | Hard blocker. Cannot ship. Requires fix or rollback. |
| A new test written for this change fails | **Implementation gap** | Hard blocker. The change is not complete. |
| A test failing before this change still fails | **Pre-existing gap** | Not a blocker for this change. Log in Open Issues if unrecorded. |
| A test fails only in CI due to environment difference | **Environment issue** | Not a regression. Investigate and document separately. |

**On genuine regression:**
1. Do not layer a fix on top of a failing test. Identify responsible change.
2. If fix is not obvious, apply Two-Attempt Failure Limit (Section 7.4 of `GEMINI.md`).
3. If unresolved after 2 attempts, roll back to last clean commit (`git reset --hard HEAD~1`) and report.

**Scope:** Run full test suite by default. Reduced scope is permitted only when explicitly authorized by user with documented rationale.

---

### Tier 4 — Runtime Verification
**What it checks:** System observable behavior after the change matches intended outcome in actual execution.

**Passes when:** Log output, network responses, rendered output, or observable signals confirm intended behavior occurred without unintended side effects.

**Fails when:** System behaves differently from intended outcome at runtime, even if previous tiers passed.

**What to check (use most applicable for change type):**
- **API/Service**: send representative request; verify response shape, status, payload.
- **Job/Event**: trigger job; verify log output confirms execution.
- **Migration**: run sample query to verify post-state data shape.
- **Config**: verify system starts and reports correct state.
- **UI**: verify component renders and interactive handlers fire.

**When runtime verification is unavailable:** If change cannot be verified at runtime in current environment (staging inaccessible, device disconnected, third-party service offline), document gap explicitly:
- State what was not verified and why.
- Describe what visual/observable output confirms correctness.
- Log pending verification in `.agents/architecture_cache.md` under Open Issues.
- Mark change as "Pending Runtime Verification" rather than complete.

---

## Partial Toolchain Protocol
When one or more tiers are unavailable (no test runner configured, no linter installed, no build step):
1. **Document the gap** in session output: state which tier is unavailable and why.
2. **Do not skip silently.** An undocumented missing tier is an unacceptable risk. The user must be informed.
3. **Do not treat unavailable as passed.** Report tier as `[Unavailable — not verified]: <reason>`.
4. **If user accepts gap**, record acceptance date and rationale in `.agents/architecture_cache.md` under Open Issues.
5. **Log for resolution.** Add a project health task to establish missing tooling.

---

## Gate Failure Report Format
When any tier fails and cannot be resolved in current session, produce a structured report:

```
## Verification Gate Failure

**Tier Failed:** [1 — Compile | 2 — Static Analysis | 3 — Automated Testing | 4 — Runtime]
**Change:** [Brief description of the change being verified]
**Failure:** [Exact error message, test name, or observable symptom]

**Classification:** [Genuine regression | Implementation gap | Pre-existing gap | Environment issue | Unavailable tier]

**Known facts (Observed/Verified):**
- [Fact 1]
- [Fact 2]

**Unverified assumptions (Inferred/Speculative):**
- [Assumption 1]
- [Assumption 2]

**Options:**
1. [Revert offer: reset to last clean commit]
2. [Alternative fix approach]
3. [Accept gap with documented rationale — user decision required]
```

---

## Diff Minimization Requirement
Verification applies to changes, not to the full codebase. To keep verification scope honest:
- Apply the smallest diff required to achieve the goal.
- Preserve existing comments, formatting, public API interfaces, and file organization unless change requires modifying them.
- Do not refactor unrelated code in same commit — every unrelated change expands verification scope.

---

## Manual Testing Steps
For every `[MT-NNN]` entry identified during Edge Case Enumeration, produce a complete entry in `.agents/architecture_cache.md` Pending Manual Testing Steps AND present it in session output.

### Entry format
```
[MT-NNN] <Category> — <title>
Source: <session identifier>, <date>
Steps:
  1. <exact action a human must take>
  2. <next action>
  ...
Expected: <observable outcome that confirms change works correctly>
Related: <source files changed>, <automated tests covering adjacent behavior>
Status: Pending
```

### What qualifies as a Manual Testing Step
- **GUI**: layout, state transitions, animations.
- **Hardware**: device inputs, sensors, peripherals.
- **Live State**: confirming real storage writes after migration.
- **E2E Flow**: user sequence exercising full feature path.
- **3rd-Party**: live external API call handling.
- **Visual**: layout, contrast, or rendering checks.

### Blocker behavior
Any `[MT-NNN]` entry with Status `Pending` when Pre-Release QA runs is a **deployment readiness blocker**. The SHIP verdict cannot be issued until every manual testing step is cleared or explicitly accepted as deferred risk by the user with documented rationale.

---

## Entry Clearance & Tombstone Policy
An `[EC-NNN]` or `[MT-NNN]` entry is **deleted** from `.agents/architecture_cache.md` (never left as status-updated tombstone) when resolved:
- **EC clearance:** Covered by passing Tier 3 test or explicit user risk acceptance with documented rationale. Record in session output, then delete entry.
- **MT clearance:** Cleared only upon explicit user confirmation that test steps were executed and expected outcome observed.
- **Session logging:** Record confirmation and disposition in session output: `[EC-NNN / MT-NNN] cleared — <rationale / user confirmation>, <date>.`

If user reports MT execution did NOT match expected outcome, entry stays open and finding is escalated to Root-Cause Investigator or UI Specialist.

---

## Skill & Gate Behavior Matrix

| Skill | Gate behavior |
|---|---|
| **Feature Architect** | Full four-tier gate required before marking feature complete. |
| **Root-Cause Investigator** | Tier 3 (with fixed test as regression test) + Tier 4 required after fixes. |
| **Test Suite Generator** | Tier 1 (compiles) + Tier 3 (new & existing tests pass) required. |
| **Modular Extractor** | Tier 3 is primary safety gate per transformation. Failure triggers Mikado revert. |
| **Security & Input Sanitizer** | Tier 3 (security tests for Critical/High findings) + re-audit required. |
| **Documentation QA Curator** | Tier 4 equivalent: doc build or link-checker must pass. No source code tiers apply. |
| **Pre-Release QA** | All four tiers across changed modules. Tier 3 failure is hard BLOCKED verdict. |
| **UI Specialist** | Tier 1 + Tier 2 (accessibility linter if available) + Tier 4 (visual and interactive verification). |

---

## Constraints
- Never suppress a failing test to make Tier 3 pass.
- Never disable a linter rule inline without documenting the reason.
- Never mark a change complete with a known open tier failure without explicit user acceptance.
- Never run only a subset of the test suite without explicit user authorization.
- Never treat a pre-existing failure as a new regression, and vice versa.
- Never skip Edge Case Enumeration; state explicitly if no edge cases are found.
- Never leave an `[MT-NNN]` entry in session output without writing it to `.agents/architecture_cache.md`.
- Never clear an `[MT-NNN]` entry without explicit user confirmation of observed outcome.
- Never leave a resolved `[EC-NNN]` or `[MT-NNN]` entry as a status-updated tombstone; delete it.
