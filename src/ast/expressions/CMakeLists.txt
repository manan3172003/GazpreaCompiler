set(
        gazprea_expressions_src
        "${CMAKE_CURRENT_SOURCE_DIR}/IntegerLiteralAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RealLiteralAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/UnaryAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/IdentifierAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleAccessAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BinaryAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/CastAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ArgAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/CharLiteralAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BoolLiteralAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/FuncProcCallAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleLiteralAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ArrayLiteralAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/StructLiteralAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/StructAccessAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ArrayAccessAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/SingularIndexExprAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RangedIndexExprAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/DomainExprAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/GeneratorAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RangeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/StructFuncCallRouterAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_expressions_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")