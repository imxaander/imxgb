# CMake generated Testfile for 
# Source directory: /home/xndr/Documents/imxgameboy
# Build directory: /home/xndr/Documents/imxgameboy/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(check_gbe "/home/xndr/Documents/imxgameboy/build/tests/check_gbe")
set_tests_properties(check_gbe PROPERTIES  _BACKTRACE_TRIPLES "/home/xndr/Documents/imxgameboy/CMakeLists.txt;96;add_test;/home/xndr/Documents/imxgameboy/CMakeLists.txt;0;")
subdirs("lib")
subdirs("gbemu")
subdirs("tests")
