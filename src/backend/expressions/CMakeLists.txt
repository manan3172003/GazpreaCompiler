set(
        gazprea_expressions_src
        "${CMAKE_CURRENT_SOURCE_DIR}/Arg.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Array.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Binary.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Bool.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Cast.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Char.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/DomainExpr.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Generator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Range.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/StructFuncCallRouter.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/FuncProcCall.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Identifier.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Integer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Real.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tuple.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Struct.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleAccess.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/StructAccess.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ArrayAccess.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Unary.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/SingularIndex.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RangedIndexExpr.cpp"
)

target_sources(gazc PRIVATE ${gazprea_expressions_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")
