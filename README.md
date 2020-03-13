# loop_adapt

*loop_adapt* is a runtime system for controlling system parameters and measuring the performance for timestep based applications. It monitors (using timers or [LIKWID](https://github.com/RRZE-HPC/likwid)) the timestep loop with controllable settings like CPU frequency. The measurements are not directly evaluated by loop_adapt but written back to a central control instance like OnSite.

# Description:

Here we describe the main data types used in loop_adapt:
- Parameter: A parameter is a runtime value or a knob for system manipulation. It consists of a name and a value (int, float, double, boolean, character, string). If it is a system knob, everytime we change the parameter value, the knob is triggered with the same value.
- Measurements: Backends to performs measurements like timer or LIKWID
- Configuration: A list of parameters and measurements for a loop. When the specified loop executes, the parameters in the list are applied to the system and the measurements are started. As soon as the measurement is done, the measurement results are written out.

# Components

loop_adapt is assembled out of four components:

- Thread storage: loop_adapt needs to know how many application threads are present and on which CPU they run. As soon as we register a thread, the thread is also pinned to a CPU
- Parameter space: Parameters are a way to store application data or knobs for system state manipulation in loop_adapt. They are arranged in a tree, so it is possible to have different parameter values for each thread/core/socket/NUMA domain or the whole system.
- Measurement system: The loop iterations can be measured using common measurement solutions like timers or LIKWID
- Configuration system: A configuration describes with which parameter setting a loop should be executed and what should be measured. Every time the loop executes, it checks for a new configuration and if any, applys the parameters and triggers the measurements.

# How does it work

The loop_adapt frontend for users consists of a set of macros the user has to add
to the code.

The setup phase requires the user to register loops that should behandled by loop_adapt with their number of iteration used for each measurement (`LA_REGISTER(loopname, num_iterations)`). The loopname identifier is a string.

Moreover, the user has to register all threads with their thread ID (`LA_REGISTER_THREAD(thread_id)` where thread_id is e.g. `omp_get_thread_num()` for OpenMP).

The loops of interest have to be transformed. If you have a for-loop like `for (i = 0; i < MAX_TIMESTEPS; i++)`, you have to rewrite it to use the loop_adapt macro `LA_FOR`: `LA_FOR(loopname, i = 0, i < MAX_TIMESTEPS, i++)`. There are other macros for do..while and while loops.

That's basically everything for a standard run. The loop is executed as normal but loop_adapt checks at each beginning of an iteration whether there is a new configuration provided. A configuration, as explained above, is a set of parameter settings and measurement instructions. As soon as a new configuration is available, the parameters are applied and the measurement system set up and started. After the user-selected amount of loop iterations, the measurements are stopped and the results returned to the configuration system for writing and/or evaluation.

## How does it work (optional stuff)

Optinally, the user can register own application parameters (`LA_NEW_<INT|BOOL|DOUBLE|...>_PARAMETER(parametername, scope, init_value)`). This parameter can also be adjusted by a configuration later. Examples for application parameters are sizes for loop-blocking, loop-tiling or grid cells. The user can get the current value with `LA_GET_<INT|BOOL|DOUBLE|...>_PARAMETER(parametername)` and manually modified with `LA_SET_<INT|BOOL|DOUBLE|...>_PARAMETER(parametername, value)`.

# What's missing

- Own application parameters don't have any background effents. With background effects I mean that they don't call any further functions. Other predefined parameters like `HW_PREFETCHER` call a function in the background that actually manipulates the hardware prefetcher at system level. For application parameters this might be useful as well.

- Already implemented but not yet added anywhere are `Parameter` limits. These limits allow to specify a valid set of values for a parameter. This is e.g. needed for a parameter that reflects a function pointer which points to different code variants. The function pointers to the code variants need to be known and are the only valid values for the parameter.
