set(ANGELSCRIPTJIT_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB ANGELSCRIPTJIT_SRC "${ANGELSCRIPTJIT_SOURCES_DIR}/*.cpp")

add_executable(AngelScriptJIT ${ANGELSCRIPTJIT_SRC})



if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(AngelScriptJIT PRIVATE "USING_GCC_COMPILER=0")
    target_compile_definitions(AngelScriptJIT PRIVATE "USING_MSVC_COMPILER=1")
else()
    target_compile_definitions(AngelScriptJIT PRIVATE "USING_GCC_COMPILER=1")
    target_compile_definitions(AngelScriptJIT PRIVATE "USING_MSVC_COMPILER=0")
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(AngelScriptJIT PRIVATE "-O3")
        target_compile_options(AngelScriptJIT PRIVATE "-Os")
        target_compile_options(AngelScriptJIT PRIVATE "-finline-functions")
        target_compile_options(AngelScriptJIT PRIVATE "-funroll-loops")
        target_compile_options(AngelScriptJIT PRIVATE "-fomit-frame-pointer")
    else()
        target_compile_options(AngelScriptJIT PRIVATE "-g")
    endif()
endif()


if(ANDROID)
    target_link_libraries(AngelScriptJIT --static angelscript asmjit)
else()
    target_link_libraries(AngelScriptJIT angelscript asmjit)
endif()
