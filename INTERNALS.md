# Introduction

This is the documentation of the internal components of loop_adapt and can be used to extend it by new parameters, new measurement backends or new configuration input/output backends.

# Representation of values and limits

For simpler handling in the library, C data types are put into a container called `ParameterValue`. In order to represent specific values or ranges of values there is the `ParameterValueLimit`.

## Parameter Value

Parameter values are containers for C data types to simplify the handling internally, the application API consists of a range of functions/macros for each data type.

The supported types are:

- `boolean`: `unsigned int` single bit
- `int`
- `uint`: `unsigned int`
- `long`
- `ulong`: `unsigned long`
- `double`
- `float`
- `char`
- `string`: `char *`
- `ptr` : `void *`

You can declare an integer parameter like:

```
ParameterValue v = DEC_NEW_INT_PARAM_VALUE(2);
[...]
loop_adapt_destroy_param_value(v);
```



## Parameter Limit

A parameter limit defines bounds for parameters used in *loop_adapt*. There are two types of parameter limits:

- ranges defined by start, end and optionally step parameter value. At range checks the start is inclusive while the end is exclusive. If there is a step value, loop_adapt checks at range checks whether the value is a valid one respecting the step factor. Ranges can be declared like:
```
ParameterValueLimit limit = DEC_NEW_INTRANGE_PARAM_LIMIT(start, end, step)
[...]
loop_adapt_destroy_param_limit(limit);
```

- lists of parameter values. Lists are created empty and can be filled with by function calls:
```
ParameterValueLimit limit = DEC_NEW_LIST_PARAM_LIMIT();
for (i = 0; i < 5; i++)
{
    ParameterValue v = DEC_NEW_DOUBLE_PARAM_VALUE(i);
    loop_adapt_add_param_limit_list(&limit, v);
    loop_adapt_destroy_param_value(v);
}
[...]
loop_adapt_destroy_param_limit(limit);
```
