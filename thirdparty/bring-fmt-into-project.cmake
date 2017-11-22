
####################################
# Bring fmt into the project.

# fmtlib appears to have MASTER_PROJECT option in its CMakeLists.txt
# it also appears to control MASTER_PROJECT based on
# CMAKE_CURRENT_SOURCE_DIR and CMAKE_SOURCE_DIR
# So, it should auto-detect that it is a sub project
ADD_SUBDIRECTORY("fmt")

# Bring fmt into the project.
####################################

