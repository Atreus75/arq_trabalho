---
trigger: always_on
---

# Architecture Cache

## What It Is
`.agents/architecture_cache.md` is a developer-local, agent-maintained file that stores
the concrete, project-specific structural knowledge of the active codebase. It
exists so that agents can navigate, modify, and verify the project without
performing redundant directory scans, dependency reads, or test-runner discovery
on every session.

It is **not** a design document, a feature spec, or a user-facing artifact. It
is an operational map — the equivalent of a project's wiring diagram, maintained
by agents and read by agents.

---

## What It Is Not
- It is **not** committed to version control. It is a gitignored, developer-local
  file. No skill, rule, or workflow may stage or commit it.
- It is **not** a substitute for the actual source code. It is a navigational
  index. When the cache and the source code disagree, the source code is correct
  and the cache must be updated.
- It is **not** immutable. It must be updated after every session that changes
  the project's structure, dependencies, entry points, or layer relationships.

---

## Canonical File Location
The cache lives at `.agents/architecture_cache.md` — inside the project's
agents directory alongside rules, workflows, and skills.

If the file does not exist, run the `cache-init` workflow to create it.

---

## Gitignore Requirement
On first use in any project, verify that `.agents/architecture_cache.md` is excluded
from version control:

1. Check whether `.agents/architecture_cache.md`
   is covered by an existing `.gitignore` pattern.
2. If not covered, append the appropriate entry to `.gitignore`:
   ```
   .agents/architecture_cache.md
   ```
3. Commit the `.gitignore` update alone with:
   `chore: exclude architecture cache from version control`
4. Never skip this step. A committed cache exposes project topology and
   developer workflow details to the public repository history.

---

## Canonical Format
The cache must maintain the following sections. Sections may be omitted only
if they are genuinely not applicable to the project (e.g., a project with no
test suite omits the Testing Infrastructure section). Empty sections must not
be left as placeholders — either populate them or remove them.

```markdown
# Architecture Cache
Last updated: YYYY-MM-DD — <brief description of what changed>

## Stack
- Language(s) and version(s)
- Runtime and version
- Primary framework(s) and version(s)
- Package manager and lock file name
- Build toolchain and relevant commands

## Module Map
- Directory tree of key source directories (not exhaustive; focus on
  layer boundaries and entry points)
- For each key module: file path, responsibility, and layer classification
  (entry handler / business logic / data access / model / presentation / utility)

## Entry Points
- All external-facing entry points: HTTP endpoints, CLI commands, event
  handlers, background job triggers, message consumers, scheduled tasks
- For each: path or command, input shape, authentication requirement

## Dependency Graph
- Key inter-module dependencies (which modules import which)
- External service dependencies (APIs, data stores, message brokers, caches)
  with their connection mechanism (environment variable name, config key)

## Data Layer
- Data store type(s) and access pattern (ORM, query builder, raw driver,
  in-memory, file-based)
- Schema or model file locations
- Migration tool and pending migration status

## Testing Infrastructure
- Test runner command(s)
- Test directory structure and file naming convention
- Existing test helpers, fixtures, and shared utilities (file paths)
- Coverage tool and last known coverage summary (optional)

## Documentation
- Documentation directory structure
- Documentation build command (if applicable)
- Known documentation gaps (brief list, updated after each Docs Curator session)
- Last drift baseline: <date> — <N> gaps found, <M> resolved

## Open Issues
- Known bugs that have been flagged but not yet fixed (brief description,
  date flagged, skill that flagged it)
- Deferred hotspots from Modular Extractor sessions
- Deferred security findings from Security & Input Sanitizer sessions
- Escalation packets received but not yet actioned (link to skill and date)

## Pending Edge Cases
<!-- Edge cases identified during implementation that are not yet covered
     by automated tests. An entry is REMOVED from this section (not just
     status-updated) once one of the following is confirmed:
       (a) an automated test that exercises the case has been written and passes, OR
       (b) the user has explicitly accepted the risk with a stated rationale.
     Do not leave resolved entries as "Accepted risk" tombstones — delete them
     and record the disposition in the session output only. -->
- [EC-NNN] <Category> — <brief description>
  Source: <commit, branch, or skill session that identified this>, <date>
  Related: <file paths or test files that cover adjacent behavior>
  Status: Unhandled | Accepted risk (reason) ← transitional only; entry is deleted once resolved

## Pending Manual Testing Steps
<!-- Verification steps that require human execution. An entry is REMOVED
     from this section (not just status-updated) once the user confirms
     they executed the steps and the expected outcome was observed.
     Do not leave completed entries as "Verified" tombstones — delete them
     and record the confirmation in the session output only.
     Uncleared entries block the Pre-Release QA SHIP verdict. -->
- [MT-NNN] <Category> — <title>
  Source: <commit, branch, or skill session>, <date>
  Steps:
    1. <exact action a human must take>
    2. <next action>
  Expected: <observable outcome that confirms correctness>
  Related: <relevant source files, automated tests covering adjacent behavior>
  Status: Pending ← transitional only; entry is deleted once user confirms outcome

## Release Baseline
- Last released version or commit tag
- Modules changed since last release (updated by Pre-Release QA after each gate)
- Performance baselines recorded at last release (optional)
```

---

## Authority Hierarchy
The cache occupies the second tier of the three-tier authority system:

1. **User Directives (Highest)**: explicit instructions in the current session
   override anything in the cache.
2. **Architecture Cache (SSOT)**: all project-specific decisions — stack,
   module layout, naming conventions, entry points, dependency patterns — are
   authoritative when no user directive contradicts them. Agents must comply
   with the decisions recorded here.
3. **Skill Rules (Lowest)**: the stack-agnostic operational guidelines in each
   skill defer to the cache for all project-specific details.

When a skill's default behavior conflicts with a pattern documented in the
cache, the cache wins. When the cache conflicts with a user directive, the
user wins.

---

## Read-First Contract
Every skill, workflow, and ad-hoc agent session must read the cache before
touching any project file. The read serves three purposes:

1. **Navigation**: locate files, modules, and entry points without scanning.
2. **Compliance**: understand the project's established patterns before
   proposing or applying any change.
3. **Conflict detection**: identify whether the planned change contradicts any
   documented architectural decision.

If the cache is absent, the agent must run the Initialization Protocol before
proceeding with any other work.

---

## Update-After Contract
The cache must be updated after any session that changes:

- A module's file path, name, or responsibility.
- The dependency graph (new import relationship, removed module, new external
  service dependency).
- An entry point (new endpoint, removed command, changed trigger).
- The data layer (new migration applied, schema change, new data store added).
- The testing infrastructure (new test helper, new coverage tool, runner
  command changed).
- The documentation directory structure.
- The open issues list (new finding flagged, existing finding resolved).
- The release baseline (new version shipped, new performance baseline recorded).

Cache updates are written to the local file immediately after the relevant
commit is made. They are not staged or committed.

---

## Initialization Protocol
When `.agents/architecture_cache.md` does not exist:

1. **Scan the project root**: read the manifest or dependency file to identify
   the language, runtime, framework(s), and package manager.
2. **Map the directory structure**: identify the key source directories, test
   directories, and documentation directories.
3. **Identify entry points**: find the main executable, HTTP server
   registration, CLI command definitions, and background job registrations.
4. **Identify the test runner**: locate the test configuration file and
   extract the runner command.
5. **Write the cache**: populate all applicable sections of the canonical
   format using only Observed data (see `evidence-taxonomy.md`). Do not
   infer or speculate about sections that require reading more files.
6. **Mark incomplete sections**: if a section cannot be populated without
   deeper investigation, write `[Pending: requires further investigation]`
   rather than leaving it empty or guessing.
7. **Gitignore the file** (see Gitignore Requirement above).

The initialization is itself an observable action. Record the date and a
one-line description in the `Last updated` field.

---

## Staleness Detection
A cache entry is stale when it describes a file, module, or configuration
that no longer matches the actual project state. Agents must flag stale
entries as they are discovered during any session, even if the current task
does not require updating them.

Stale entries are not silently ignored. They are either:
- Corrected immediately if the correction is within the current session's scope.
- Logged in the Open Issues section with the date and a description if
  correction is out of scope.

---

## Constraints
- Never commit `.agents/architecture_cache.md` to version control.
- Never populate a cache section with Speculative or Inferred data without
  labeling it as such (see `evidence-taxonomy.md`).
- Never use the cache as a substitute for reading the actual source when a
  decision requires confirmed current state. The cache accelerates navigation;
  it does not replace verification.
- Never leave the cache in a state that is known to be inaccurate. If a
  session ends with known stale entries, log them in Open Issues.