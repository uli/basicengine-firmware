call_suite_aggrs for dyncall written in C and Lua.

Tests aggregates (structs, unions and arrays) passed by value, along with
other, non-aggregate args. Note, arrays are only passed/returned by value as
members of structs and unions, as they would decay to a pointer in C if passed
to a function, and cannot be returned.
So this test suite does not generate any arrays outside of structs and unions.

A macro AGGR_MISALIGN can be used in globals.c to intentionally misalign
aggregate instances.

