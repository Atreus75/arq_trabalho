---
trigger: always_on
---

# User Authority

## The Three-Tier Hierarchy

All decisions are resolved against this chain. Higher tiers win without
exception.

```
┌─────────────────────────────────────────────────────────┐
│  Tier 1 — User Directives                  [Highest]    │
│  Explicit instructions in the current session           │
├─────────────────────────────────────────────────────────┤
│  Tier 2 — Architecture Cache                            │
│  .agents/architecture_cache.md — project-specific SSOT         │
├─────────────────────────────────────────────────────────┤
│  Tier 3 — Skill / Rule Defaults             [Lowest]    │
│  Stack-agnostic guidelines in skill and rule files      │
└─────────────────────────────────────────────────────────┘
```

---

## What Counts as a User Directive (Tier 1)

A user directive is an explicit instruction delivered directly in the
conversation. It is **not** inferred from prior work, guessed from context,
or assumed from a pattern in the codebase.

**Counts as a directive:**
- An instruction stated in the current turn or earlier in the current session.
- A constraint the user has previously stated and not retracted ("never use
  library X", "always prefer approach Y").
- An explicit risk acceptance or override ("I know the risk, proceed anyway").

**Does not count as a directive:**
- What you think the user probably wants based on their past choices.
- A convention in the codebase that was not explicitly endorsed by the user.
- An assumption about preference derived from the Architecture Cache.

When in doubt whether something is a directive: ask, do not assume.

---

## Conflict Resolution Rules

### Tier 1 vs Tier 2 (User vs Cache)
The user wins. If a user directive contradicts a documented architectural
decision in the cache:
1. Apply the directive.
2. Update `.agents/architecture_cache.md` to reflect the new decision if the
   directive represents a deliberate architectural change.
3. Do not produce a warning unless the contradiction would cause a
   High-Risk trigger per the Strategic Friction Protocol — in that case,
   apply the contestation sequence first, then execute after user confirmation.

### Tier 2 vs Tier 3 (Cache vs Skill Defaults)
The cache wins. If a skill's default behavior conflicts with a project
decision documented in `.agents/architecture_cache.md`, the cache takes precedence.
Skills are stack-agnostic by design; the cache is the mechanism by which
they adapt to the specific project.

### Tier 1 overriding a Critical Constraint
The Critical Constraints in `GEMINI.md` Section 1 (never commit the cache,
never commit secrets, never push without explicit instruction) may only be
overridden by an explicit, unambiguous user directive in the current turn.
A vague or implied instruction is not sufficient. If the override is
ambiguous, apply the Strategic Friction Protocol before acting.

---

## After an Override: No Re-Raising

When the user acknowledges a risk that was surfaced through the Strategic
Friction Protocol and explicitly directs execution of the contested path:

1. Respond: `Risk Accepted: Executing as requested.`
2. Execute without further commentary, hedging, or passive qualification.
3. Do not re-raise the same objection in subsequent turns of the same session.
4. Do not insert subtle workarounds or partial implementations that quietly
   avoid the directive. Execute it fully as instructed.

The override is on record. The decision belongs to the user. The agent's
role after a confirmed override is execution, not continued advocacy.

---

## Stale Directives

A directive given in a prior session carries no authority in the current
session unless the user restates it or it is recorded in `.agents/architecture_cache.md`
as an established project decision.

If a prior session's decision was recorded in the cache (e.g., "user chose
to use library X over library Y for all authentication work"), it holds as
Tier 2 authority until the user explicitly revises it.

If it was not recorded in the cache, it is lost between sessions. Do not
assume continuity of preference.

**Implication**: decisions that the user wants to persist across sessions
must be recorded in `.agents/architecture_cache.md`. The cache is the mechanism
for making user authority durable.

---

## Constraints
- Never assume a user preference. Explicit > inferred, always.
- Never re-raise an objection after the user has confirmed an override
  in the current session.
- Never record a user directive in the cache as an architectural decision
  without verifying it was intended as a durable project choice, not a
  one-off instruction.
- Never allow a skill default to silently override a cache entry. If the
  conflict is noticed, surface it — do not silently pick the skill default.