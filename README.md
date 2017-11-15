### How to build LOOP\_ADAPT? ###

* git clone https://github.com/RRZE-HPC/loop_adapt.git
* cd loop\_adapt && mkdir build && cd build
* CC=$(C\_COMPILER) CXX=$(CXX\_COMPILER) cmake ..
* Configure the library using ccmake . (if needed)
* make
* make install
* Library Dependencies : hwloc (will be cloned and installed if not found)
* Use CMAKE find\_package to get the proper linking flags for the library

### Want to try LOOP\_ADAPT? ###
To try out examples:

* cd loop\_adapt/examples
* mkdir build && cd build
* CC=$(C\_COMPILER) CXX=$(CXX\_COMPILER) cmake .. -DloopAdapt\_DIR=$(folder containing libloopAdapt.so file)
* make
