set(
        gazprea_expressions_src
        "${CMAKE_CURRENT_SOURCE_DIR}/IntegerAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RealAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/UnaryAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/IdentifierAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleAccessAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BinaryAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/CastAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ArgAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/CharAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BoolAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/FuncProcCallAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleLiteralAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_expressions_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")