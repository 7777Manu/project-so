# AI Usage Report - Phase 1

**Tool used:** Google Gemini (Large Language Model)

**Prompts given:**
1. "Generate a C function `int parse_condition(const char *input, char *field, char *op, char *value)` that splits a string like 'severity:>=:2' using ':' as a delimiter."
2. "Generate a C function `match_condition(Report *r, const char *field, const char *op, const char *value)` to compare a struct field against a value based on an operator (==, !=, <, >, etc.)."

**What was generated:**
The AI provided the logic using `strtok` for parsing and a series of `if-else` blocks for matching different data types (strings vs integers).

**Changes made:**
- Adjusted the `match_condition` to handle `time_t` for the timestamp field.
- Added `atol()` conversion for the value string to ensure proper numeric comparison.

**Learning outcome:**
I learned how to decouple the parsing logic from the matching logic, making the filter command easier to extend for multiple conditions.