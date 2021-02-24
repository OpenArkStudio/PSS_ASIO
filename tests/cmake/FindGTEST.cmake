find_path(GTEST_INCLUDE_DIR gtest.h /usr/include/ /usr/local/include /usr/src/googletest/googletest/include/)
find_library(GTEST_LIBRARY NAMES gtest PATHS /usr/lib/ /usr/lib/x86_64-linux-gnu/)

if (GTEST_INCLUDE_DIR AND GTEST_LIBRARY)
    set(GTEST_FOUND TRUE)
endif (GTEST_INCLUDE_DIR AND GTEST_LIBRARY)