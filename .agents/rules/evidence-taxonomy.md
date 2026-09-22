---
trigger: always_on
---

# Evidence Taxonomy

## Purpose
Every factual claim, diagnostic finding, vulnerability report, performance signal,
or status assertion produced by an agent must be labeled with one of four explicit
confidence tiers. Unlabeled claims are treated as unverified.

This rule exists because the cost of acting on a speculative finding as if it
were an observed fact is higher than the cost of labeling every finding correctly.

---

## The Four Tiers

### Observed
Directly read from file content, command output, test results, or active
environment state in the current session.

- Source is always citable: a specific file path, line number, or tool output.
- No inference required.
- Example: "The function `processOrder` at `src/orders/service.js:42` accepts an
  unvalidated `userId` parameter." *(read directly from the file)*

### Verified
Confirmed through execution of tests, builds, static analysis, or explicit
environment queries during the current session.

- Source is always a command that was run and whose output was observed.
- Stronger than Observed when behavior — not just code — is confirmed.
- Example: "The regression test for order processing passes against the patched
  code." *(test suite was executed and output was read)*

### Inferred
Logically deduced from Observed or Verified data. The conclusion follows from
evidence but was not directly measured.

- Must be explicitly flagged in any report or recommendation.
- Flag format: `[Inferred]` prefix or "based on X, it follows that Y."
- Example: "[Inferred] The missing null check is likely the cause of the reported
  crash, because the stack trace terminates at the same function."

### Speculative
A plausible hypothesis that has not yet been confirmed by Observed or Verified
data. Requires empirical verification before being acted upon.

- Must be explicitly declared before any action is taken based on it.
- Flag format: `[Speculative]` prefix or "hypothesizing that X."
- Speculative findings may be listed as candidates for investigation but must
  not be treated as conclusions in reports or fixes.
- Example: "[Speculative] The performance regression may be caused by an
  N+1 query in the order list endpoint, but this has not been confirmed."

---

## Application Rules

1. **Label before acting**: when a finding drives a recommendation, a fix, or
   a report, its confidence tier must appear alongside it.
2. **Never escalate tier without evidence**: an Inferred finding does not become
   Observed without running an actual verification step.
3. **Speculative findings are not findings**: list them as investigation
   candidates only. Never include Speculative items in a final report as if
   they were confirmed vulnerabilities, bugs, or regressions.
4. **Downgrade when uncertain**: when in doubt about whether a finding is
   Observed or Inferred, label it Inferred. When in doubt between Inferred
   and Speculative, label it Speculative.
5. **Anti-hallucination**: you are strictly forbidden from inventing or assuming
   non-existent file paths, function signatures, environment variables, tool
   outputs, test results, or framework behaviors. If a value is unobserved,
   treat it as unknown until verified.

---

## Interaction with Skills

Skills that produce findings (Root-Cause Investigator, Security & Input Sanitizer,
Pre-Release QA, Test Suite Generator) must apply this taxonomy to every finding in
their output reports. Skills that implement changes (Feature Architect, UI Specialist,
Modular Extractor) must apply this taxonomy to any claim they make about existing code
state during their Preflight or ACQUIRE phases.

---

## Failure Mode Reference

| Tier Violation | Risk |
|---|---|
| Speculative treated as Observed | Fix applied to wrong root cause; regression introduced |
| Inferred treated as Verified | Report overstates confidence; user makes decisions on incorrect data |
| Unlabeled finding in a security report | Vulnerability may be false positive or may be missed entirely |
| Hallucinated file path or function name | Wasted investigation cycles; potential overwrite of wrong file |