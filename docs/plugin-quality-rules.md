# Plugin Quality Rules

Quality expectations:

- fail soft and never block host startup
- validate settings and inputs defensively
- avoid direct host internals dependencies
- provide actionable diagnostics
- keep commands and settings deterministic

Operational rules:

- manifest must validate
- compatibility should be declared explicitly
- unsafe behavior should be rejected during review
