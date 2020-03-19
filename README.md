# loop_adapt

*loop_adapt* is a runtime system for controlling system parameters and measuring the performance for timestep based applications. It monitors (using timers or [LIKWID](https://github.com/RRZE-HPC/likwid)) the timestep loop with controllable parameters like CPU frequency. The configurations for the timestep loop are not provided by loop_adapt, it is just the application level interface. Configurations can be provided by different backends like input files or through network. A configuration assembles a set of parameter values and a set of measurements. Whenever a configuration was applied and the loop's runtime measurements are performed, the setting is send to an output backend like stdout, files or to the network.

# Description:

Here we describe the main data types used in loop_adapt:
- Parameter: A parameter is a runtime value or a knob for system manipulation. It consists of a name and a value (int, float, double, boolean, character, string). If it is a system knob, everytime we change the parameter value, the system is adjusted accordingly. An example parameter would be the CPU frequency. There are also used defined parameters which can only be changed without effect to the system. This can be runtime paramters like cache blocking factors or grid partition settings.
- Measurements: Backends to performs measurements like timers or LIKWID measurements.
- Configuration: A list of parameters and measurements for a loop. When the specified loop executes, the parameters in the list are applied to the system and the measurements are started. As soon as the measurement is done, the measurement results are written out.

# Components

loop_adapt is assembled out of four components:

- Thread storage: loop_adapt needs to know how many application threads are present and on which CPU they run. As soon as we register a thread, the thread is also pinned to a CPU. The thread information is only used internally, there are no macros or API calls to get information.
- Parameter space: Parameters are a way to store application data or knobs for system state manipulation in loop_adapt. They are arranged in a tree, so it is possible to have different parameter values for each thread/core/socket/NUMA domain or the whole system.
- Measurement system: The loop iterations can be measured using common measurement solutions like timers or LIKWID
- Configuration system: A configuration describes with which parameter setting a loop should be executed and what should be measured. Every time the loop executes, it checks for a new configuration and if any, applys the parameters and triggers the measurements.

# How does it work

The loop_adapt frontend for users consists of a set of macros the user has to add
to the code. At first there is `LA_INIT` to initialize the internal data structures.

Moreover, the user has to register all threads with their thread ID (`LA_REGISTER_THREAD(thread_id)` where thread_id is e.g. `omp_get_thread_num()` for OpenMP or some other integer id).

The setup phase requires the user to register loops that should behandled by loop_adapt with their number of iteration used for each measurement cycle: `LA_REGISTER(loopname, num_iterations)`. The loopname identifier is a string, num_iterations is an integer.

The loops of interest have to be transformed. If you have a for-loop like `for (i = 0; i < MAX_TIMESTEPS; i++)`, you have to rewrite it to use the loop_adapt macro `LA_FOR`: `LA_FOR(loopname, i = 0, i < MAX_TIMESTEPS, i++)`. There are other macros for other loop types.

That's basically everything for a standard run as long as only builtin parameters are used. The loop is executed as normal but loop_adapt checks at each beginning of an iteration whether there is a new configuration provided. A configuration is a set of parameter settings and measurement instructions. As soon as a new configuration is available, the parameters are applied and the measurement system set up and started. After the user-selected amount of loop iterations, the measurements are stopped and the results returned to the configuration system for writing and/or evaluation.

At the end, a call to `LA_FINALIZE` deallocates everything.

Basic example with OpenMP:

```
#include <loop_adapt>
#include <omp.h>

int main(int argc, char* argv)
{
    int t = 0;
    int max_timesteps = 10000;
    LA_INIT;

#pragma omp parallel
{
    LA_REGISTER_THREAD(omp_get_thread_num())
}

    LA_REGISTER("TIMESTEPLOOP", 10);

    LA_FOR("TIMESTEPLOOP", t = 0, t < max_timesteps, t++)
    {
        // do your thing
    }

    LA_FINALIZE;
    return 0;
}
```

## How does it work (optional stuff)

Optinally, the user can register own application parameters (`LA_NEW_<INT|BOOL|DOUBLE|...>_PARAMETER(parametername, scope, init_value)`). This parameter can also be adjusted by a configuration later. Examples for application parameters are sizes for loop-blocking, loop-tiling or grid cells. The user can get the current value with `LA_GET_<INT|BOOL|DOUBLE|...>_PARAMETER(parametername)` and manually modified with `LA_SET_<INT|BOOL|DOUBLE|...>_PARAMETER(parametername, value)`.

# Parameter space
## Introduction
The parameter space contains the parameters for which can be adjusted for loop iterations. Parameters consist of a value, a so-called scope and, optionally, a set of bounds. The value can be most of data types provided by C, see *Parameter Value* section. The scope is a location in the system topology tree. Examples are hardware threads, CPU sockets or NUMA domains. The optional bounds can be a range of parameter values or a list of those.

The scope can be (type `LoopAdaptScope_t`):

- `LOOP_ADAPT_SCOPE_THREAD`: Hardware thread
- `LOOP_ADAPT_SCOPE_CORE`: Phyiscal core
- `LOOP_ADAPT_SCOPE_SOCKET`: CPU socket/package
- `LOOP_ADAPT_SCOPE_NUMANODE`: NUMA domain
- `LOOP_ADAPT_SCOPE_LLCACHE`: Last level cache segment
- `LOOP_ADAPT_SCOPE_SYSTEM`: The whole system

## Builtin system parameters
*loop_adapt* ships with a set of builtin system parameters that can be used by configurations to manipulate the system state for different measurements. Currently the provided parameters are:

|Name | Scope|Description|
|-----|------|-----------|
|`HW_PREFETCHER`|`LOOP_ADAPT_SCOPE_THREAD`| |
|`CL_PREFETCHER`|`LOOP_ADAPT_SCOPE_THREAD`| |
|`DCU_PREFETCHER`|`LOOP_ADAPT_SCOPE_THREAD`| |
|`IP_PREFETCHER`|`LOOP_ADAPT_SCOPE_THREAD`| |
|`CPU_FREQUENCY`|`LOOP_ADAPT_SCOPE_THREAD`| The CPU frequency of a CPU core. |
|`UNCORE_FREQUENCY`|`LOOP_ADAPT_SCOPE_SOCKET`| The Uncore frequency of a  CPU socket. |
|`OMP_NUM_THREADS` |`LOOP_ADAPT_SCOPE_SYSTEM` | The max. number of OpenMP threads.

## User-provided parameters

### Parameter Value
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
```



### Parameter Limit
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


## Application usage of parameters
For the application, there exist the basic three parameter functions:

- `int loop_adapt_new_<TYPE>_parameter(char* name, LoopAdaptScope_t scope, <TYPE> value)`
- `TYPE loop_adapt_new_get_parameter(char* name)`
- `int loop_adapt_set_<TYPE>_parameter(char* name, <TYPE> value)`





# Measurement system

# Configuration system



# What's missing

- Own application parameters don't have any background effents. With background effects I mean that they don't call any further functions. Other predefined parameters like `HW_PREFETCHER` call a function in the background that actually manipulates the hardware prefetcher at system level. For application parameters this might be useful as well.

- Already implemented but not yet added anywhere are `Parameter` limits. These limits allow to specify a valid set of values for a parameter. This is e.g. needed for a parameter that reflects a function pointer which points to different code variants. The function pointers to the code variants need to be known and are the only valid values for the parameter.
