include_directories(${PROJECT_SOURCE_DIR}/test)

set(TEST_LINKS)
list(APPEND TEST_LINKS
        "gtest::gtest_main"
        "abel::testing"
        "gtest::gtest"
        "gmock::gmock"
        "abel::abel"
        )
list(APPEND TEST_LINKS ${ABEL_DYLINKS})

add_subdirectory(filesystem)
add_subdirectory(atomic)
add_subdirectory(fiber)
add_subdirectory(base)
add_subdirectory(chrono)
add_subdirectory(config)
add_subdirectory(debugging)
add_subdirectory(digest)
add_subdirectory(math)
add_subdirectory(memory)
add_subdirectory(metrics)
add_subdirectory(numeric)
add_subdirectory(rapidjson)
add_subdirectory(random)
add_subdirectory(strings)
add_subdirectory(system)
add_subdirectory(thread)
add_subdirectory(utility)
add_subdirectory(functional)
add_subdirectory(meta)
add_subdirectory(container)
add_subdirectory(hash)
add_subdirectory(algorithm)



