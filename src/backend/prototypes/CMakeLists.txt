set(
        gazprea_prototypes_src
        "${CMAKE_CURRENT_SOURCE_DIR}/Function.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/FunctionParam.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Procedure.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ProcedureParams.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Prototype.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/MemberFunc.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BuiltinFunc.cpp"
)

target_sources(gazc PRIVATE ${gazprea_prototypes_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")