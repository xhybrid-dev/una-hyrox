# CMake generated Testfile for 
# Source directory: /home/user/una-hyrox/hybridx-workout/Tests/Host
# Build directory: /home/user/una-hyrox/hybridx-workout/build-asan
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[hybridx-workout-host-tests]=] "/home/user/una-hyrox/hybridx-workout/build-asan/hybridx-workout-host-tests")
set_tests_properties([=[hybridx-workout-host-tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/user/una-hyrox/hybridx-workout/Tests/Host/CMakeLists.txt;61;add_test;/home/user/una-hyrox/hybridx-workout/Tests/Host/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
