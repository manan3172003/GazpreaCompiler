set(
        gazprea_prototypes_src
        "${CMAKE_CURRENT_SOURCE_DIR}/FunctionAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/PrototypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/FunctionParamAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/FunctionParamAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ProcedureAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ProcedureParamAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_prototypes_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")