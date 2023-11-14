set(ANGELSCRIPTJIT_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB ANGELSCRIPTJIT_SRC "${ANGELSCRIPTJIT_SOURCES_DIR}/*.cpp")

add_executable(AngelScriptJIT ${ANGELSCRIPTJIT_SRC})
target_link_libraries(AngelScriptJIT --static angelscript asmjit)
