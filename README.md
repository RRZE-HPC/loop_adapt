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

Before you start the application, you configure the configuration system with environment variables. Each backend might require additional environment variables:

- `LA_CONFIG_INPUT_TYPE`: Specifiy the input backend for configurations.
  - `0`: Text file input reading line by line of format `<parameter0>:<id0>:<value0>;<parameter1>:<id1>:<value1>;...|<measurement0>:<config0>:<metrics0>;<measurement1>:<config1>:<metrics1>;...`. The filename is `<loopname>.txt` like `TIMESTEPLOOP.txt` for loop in above example.
  - `1`: (Not usable!) Example backend to a C++ class
  - `2`: (Work in progress) Receiving configurations over a TCP socket
- `LA_CONFIG_OUTPUT_TYPE`: Specifiy the output backend for configurations.
  - `0`: Test file output writing line by line of format `<parameter0>:<id0>:<value0>;<parameter1>:<id1>:<value1>;...|<measurement0>:<config0>:<metrics0>:<value0>;<measurement1>:<config1>:<metrics1>:<value1>,...`. The output folder is defined through the environment variable `LA_CONFIG_TXT_OUTPUT` and creates files name `<loopname>.txt` like `TIMESTEPLOOP.txt` for loop in above example.
  - `1`: Output to stdout or stderr line by line of format `<parameter0>:<id0>:<value0>;<parameter1>:<id1>:<value1>;...|<measurement0>:<config0>:<metrics0>:<value0>;<measurement1>:<config1>:<metrics1>:<value1>;...`. The default is output to stdout. The output can be explicitly set with `LA_CONFIG_STDOUT_OUTPUT=(stdout|stderr)` environment variable.
  - `2`: (Not usable!) Example backend to a C++ class
  - `3`: (Work in progress) Sending configuration results over a TCP socket


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

|  Name  |  Scope  |  Type  | Description |
|:------:|:-------:|:------:|-------------|
|`HW_PREFETCHER`|`LOOP_ADAPT_SCOPE_THREAD`| `boolean` ||
|`CL_PREFETCHER`|`LOOP_ADAPT_SCOPE_THREAD`| `boolean` ||
|`DCU_PREFETCHER`|`LOOP_ADAPT_SCOPE_THREAD`| `boolean`||
|`IP_PREFETCHER`|`LOOP_ADAPT_SCOPE_THREAD`| `boolean` ||
|`CPU_FREQUENCY`|`LOOP_ADAPT_SCOPE_THREAD`| `int` |The CPU frequency of a CPU core. |
|`UNCORE_FREQUENCY`|`LOOP_ADAPT_SCOPE_SOCKET`| `int` |The Uncore frequency of a  CPU socket. |
|`OMP_NUM_THREADS` |`LOOP_ADAPT_SCOPE_SYSTEM` | `int` |The max. number of OpenMP threads.|

With `boolean` = `unsigned int:1`.

## User defined parameters

Users can register their own parameters in the system topology tree. Like the builtin parameters, they are identified by a name and attached to the topology tree according to the given scope. The parameter is initialized with a given value.

There exists one distinct creation macro/function per supported data type:

- `int LA_NEW_BOOL_PARAMETER(name, scope, boolean value)` =
- `int LA_NEW_CHAR_PARAMETER(name, scope, char value)`
- `int LA_NEW_INT_PARAMETER(name, scope, int value)`
- `int LA_NEW_UINT_PARAMETER(name, scope, unsigned int value)`
- `int LA_NEW_LONG_PARAMETER(name, scope, long value)`
- `int LA_NEW_ULONG_PARAMETER(name, scope, unsigned long value)`
- `int LA_NEW_DOUBLE_PARAMETER(name, scope, double value)`
- `int LA_NEW_FLOAT_PARAMETER(name, scope, float value)`
- `int LA_NEW_STRING_PARAMETER(name, scope, char* value)`
- `int LA_NEW_PTR_PARAMETER(name, scope, void* value)`

## Getting and setting of parameters
For retrieval of the current parameter's value and its manipulation exist getter and setter macros/functions:

- `LA_GET_<TYPE>_PARAMETER(name, <TYPE> variable)`
- `LA_SET_<TYPE>_PARAMETER(name, <TYPE> value)`

With `<TYPE>` like in above section.

If it is builtin system parameter, the setter macro also triggers the manipulation of the system state like activating deallocating a hardware prefetcher. User defined parameters don't have such a functionality, the state is only kept internally.


# Measurement system
There are currently two measurement systems available. There are only builtin measurement systems, it is not possible for users to add their own.

- `TIMER`: A simple timer for runtime measurements. The `configuration` selects the used timer:
  - `LIKWID`: LIKWID' rdtsc based timer
  - `REALTIME`: Uses `clock_gettime` with `TIMER_MEASUREMENT_REALTIME`
  - `MONOTONIC`: Uses `clock_gettime` with `TIMER_MEASUREMENT_MONOTONIC`
  - `PROCESS_CPUTIME`: Uses `clock_gettime` with `TIMER_MEASUREMENT_PROCESS_CPUTIME`
  - `THREAD_CPUTIME`: Uses `clock_gettime` with `TIMER_MEASUREMENT_THREAD_CPUTIME`
- `LIKWID`: The `configuration` is a LIKWID eventset of performance group like `L3`. The `metrics` value specifies a match for a metric in that group. In order to get the read and write `L3 bandwidth [MByte/s]`, it is enough to write `L3 bandwidth` (first match get selected).

# Documentation of internals
The documentation of the internals can be found [here](INTERNALS.md).


# What's missing

- Own application parameters don't have any background effents. With background effects I mean that they don't call any further functions. Other predefined parameters like `HW_PREFETCHER` call a function in the background that actually manipulates the hardware prefetcher at system level. For application parameters this might be useful as well.

- Already implemented but not yet added anywhere are `Parameter` limits. These limits allow to specify a valid set of values for a parameter. This is e.g. needed for a parameter that reflects a function pointer which points to different code variants. The function pointers to the code variants need to be known and are the only valid values for the parameter.

- Multiplexing user defined parameters with configurations received from the backend

- Announing of loops, parameters and measurement backends to configuration output backend. If there is some application producing configurations like a network daemon, it needs to know the available backends and knobs to work like what's the range of a user defined paramters, its scope and what type the parameters value is.
