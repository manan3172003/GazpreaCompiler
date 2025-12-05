; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

@charFormat = internal constant [3 x i8] c"%c\00"
@charInputFormat = internal constant [3 x i8] c"%c\00"
@intFormat = internal constant [3 x i8] c"%d\00"
@intInputFormat = internal constant [3 x i8] c"%d\00"
@floatFormat = internal constant [3 x i8] c"%g\00"
@floatInputFormat = internal constant [3 x i8] c"%f\00"
@boolInputFormat = internal constant [8 x i8] c" %1[TF]\00"
@stream_state_019ae35e_4e0e_7d02_98f8_6e5abd8135e9 = internal global i32 0

declare void @free(ptr)

declare ptr @malloc(i64)

define { i32, ptr, i1 } @shape(ptr %0, i32 %1) {
  %3 = icmp eq i32 %1, 1
  br i1 %3, label %4, label %19

4:                                                ; preds = %2
  %5 = getelementptr { i32, i32, ptr, i1 }, ptr %0, i32 0, i32 0
  %6 = load i32, ptr %5, align 4
  %7 = alloca i32, align 4
  %8 = getelementptr i32, ptr %7, i32 1
  %9 = ptrtoint ptr %7 to i64
  %10 = ptrtoint ptr %8 to i64
  %11 = sub i64 %10, %9
  %12 = mul i64 1, %11
  %13 = call ptr @malloc(i64 %12)
  store i32 %6, ptr %13, align 4
  %14 = alloca { i32, ptr, i1 }, align 8
  %15 = getelementptr { i32, ptr, i1 }, ptr %14, i32 0, i32 0
  store i32 1, ptr %15, align 4
  %16 = getelementptr { i32, ptr, i1 }, ptr %14, i32 0, i32 1
  store ptr %13, ptr %16, align 8
  %17 = getelementptr { i32, ptr, i1 }, ptr %14, i32 0, i32 2
  store i1 false, ptr %17, align 1
  %18 = load { i32, ptr, i1 }, ptr %14, align 8
  br label %64

19:                                               ; preds = %2
  %20 = getelementptr { i32, ptr, i1 }, ptr %0, i32 0, i32 0
  %21 = load i32, ptr %20, align 4
  %22 = getelementptr { i32, ptr, i1 }, ptr %0, i32 0, i32 1
  %23 = load ptr, ptr %22, align 8
  %24 = getelementptr { i32, ptr, i1 }, ptr %0, i32 0, i32 2
  %25 = load i1, ptr %24, align 1
  br i1 %25, label %26, label %48

26:                                               ; preds = %19
  %27 = icmp eq i32 %21, 0
  br i1 %27, label %28, label %29

28:                                               ; preds = %26
  br label %32

29:                                               ; preds = %26
  %30 = getelementptr { i32, ptr, i1 }, ptr %23, i32 0, i32 0
  %31 = load i32, ptr %30, align 4
  br label %32

32:                                               ; preds = %28, %29
  %33 = phi i32 [ %31, %29 ], [ 0, %28 ]
  br label %34

34:                                               ; preds = %32
  %35 = alloca i32, align 4
  %36 = getelementptr i32, ptr %35, i32 1
  %37 = ptrtoint ptr %35 to i64
  %38 = ptrtoint ptr %36 to i64
  %39 = sub i64 %38, %37
  %40 = mul i64 2, %39
  %41 = call ptr @malloc(i64 %40)
  store i32 %21, ptr %41, align 4
  %42 = getelementptr i32, ptr %41, i32 1
  store i32 %33, ptr %42, align 4
  %43 = alloca { i32, ptr, i1 }, align 8
  %44 = getelementptr { i32, ptr, i1 }, ptr %43, i32 0, i32 0
  store i32 2, ptr %44, align 4
  %45 = getelementptr { i32, ptr, i1 }, ptr %43, i32 0, i32 1
  store ptr %41, ptr %45, align 8
  %46 = getelementptr { i32, ptr, i1 }, ptr %43, i32 0, i32 2
  store i1 false, ptr %46, align 1
  %47 = load { i32, ptr, i1 }, ptr %43, align 8
  br label %61

48:                                               ; preds = %19
  %49 = alloca i32, align 4
  %50 = getelementptr i32, ptr %49, i32 1
  %51 = ptrtoint ptr %49 to i64
  %52 = ptrtoint ptr %50 to i64
  %53 = sub i64 %52, %51
  %54 = mul i64 1, %53
  %55 = call ptr @malloc(i64 %54)
  store i32 %21, ptr %55, align 4
  %56 = alloca { i32, ptr, i1 }, align 8
  %57 = getelementptr { i32, ptr, i1 }, ptr %56, i32 0, i32 0
  store i32 1, ptr %57, align 4
  %58 = getelementptr { i32, ptr, i1 }, ptr %56, i32 0, i32 1
  store ptr %55, ptr %58, align 8
  %59 = getelementptr { i32, ptr, i1 }, ptr %56, i32 0, i32 2
  store i1 false, ptr %59, align 1
  %60 = load { i32, ptr, i1 }, ptr %56, align 8
  br label %61

61:                                               ; preds = %34, %48
  %62 = phi { i32, ptr, i1 } [ %60, %48 ], [ %47, %34 ]
  br label %63

63:                                               ; preds = %61
  br label %64

64:                                               ; preds = %4, %63
  %65 = phi { i32, ptr, i1 } [ %62, %63 ], [ %18, %4 ]
  br label %66

66:                                               ; preds = %64
  ret { i32, ptr, i1 } %65
}

define i32 @length(ptr %0, i32 %1) {
  %3 = icmp eq i32 %1, 1
  br i1 %3, label %4, label %7

4:                                                ; preds = %2
  %5 = getelementptr { i32, i32, ptr, i1 }, ptr %0, i32 0, i32 0
  %6 = load i32, ptr %5, align 4
  br label %10

7:                                                ; preds = %2
  %8 = getelementptr { i32, ptr, i1 }, ptr %0, i32 0, i32 0
  %9 = load i32, ptr %8, align 4
  br label %10

10:                                               ; preds = %4, %7
  %11 = phi i32 [ %9, %7 ], [ %6, %4 ]
  br label %12

12:                                               ; preds = %10
  ret i32 %11
}

declare i32 @printf_019ae38d_3df3_74a3_b276_d9a9f7a8008b(ptr, ...)

declare i32 @scanf_019ae392_2fe0_72fc_ad1e_94bb9c5662c0(ptr, ...)

declare i32 @ipow_019addc8_6352_7de5_8629_b0688522175f(i32, i32)

declare void @throwDivisionByZeroError_019addc8_a29b_740a_9b09_8a712296bc1a()

declare void @throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c()

declare void @throwVectorSizeError_019addc9_1a57_7674_b3dd_79d0624d2029()

declare void @throwArrayIndexError_019ae3a1_54f9_7452_b095_6faaebe8aa2e()

declare void @printArray_019addab_1674_72d4_aa4a_ac782e511e7a(ptr, i32)

define i32 @main() {
  %1 = alloca i32, align 4
  store i32 5, ptr %1, align 4
  %2 = alloca i32, align 4
  store i32 5, ptr %2, align 4
  %3 = alloca { { i32, ptr, i1 }, { i32, ptr, i1 } }, align 8
  %4 = alloca { i32, ptr, i1 }, align 8
  %5 = getelementptr { i32, ptr, i1 }, ptr %4, i32 0, i32 0
  store i32 0, ptr %5, align 4
  %6 = getelementptr { i32, ptr, i1 }, ptr %4, i32 0, i32 1
  store ptr null, ptr %6, align 8
  %7 = getelementptr { i32, ptr, i1 }, ptr %4, i32 0, i32 2
  store i1 false, ptr %7, align 1
  %8 = getelementptr { { i32, ptr, i1 }, { i32, ptr, i1 } }, ptr %3, i32 0, i32 0
  %9 = alloca { i32, ptr, i1 }, align 8
  %10 = load i32, ptr %1, align 4
  %11 = alloca i32, align 4
  %12 = getelementptr i32, ptr %11, i32 1
  %13 = ptrtoint ptr %11 to i64
  %14 = ptrtoint ptr %12 to i64
  %15 = sub i64 %14, %13
  %16 = sext i32 %10 to i64
  %17 = mul i64 %16, %15
  %18 = call ptr @malloc(i64 %17)
  br label %19

19:                                               ; preds = %22, %0
  %20 = phi i32 [ %24, %22 ], [ 0, %0 ]
  %21 = icmp slt i32 %20, %10
  br i1 %21, label %22, label %25

22:                                               ; preds = %19
  %23 = getelementptr i32, ptr %18, i32 %20
  store i32 0, ptr %23, align 4
  %24 = add i32 %20, 1
  br label %19

25:                                               ; preds = %19
  %26 = getelementptr { i32, ptr, i1 }, ptr %9, i32 0, i32 1
  store ptr %18, ptr %26, align 8
  %27 = getelementptr { i32, ptr, i1 }, ptr %9, i32 0, i32 0
  store i32 %10, ptr %27, align 4
  %28 = getelementptr { i32, ptr, i1 }, ptr %9, i32 0, i32 2
  store i1 false, ptr %28, align 1
  %29 = load { i32, ptr, i1 }, ptr %9, align 8
  store { i32, ptr, i1 } %29, ptr %8, align 8
  %30 = alloca { i32, ptr, i1 }, align 8
  %31 = getelementptr { i32, ptr, i1 }, ptr %30, i32 0, i32 0
  store i32 0, ptr %31, align 4
  %32 = getelementptr { i32, ptr, i1 }, ptr %30, i32 0, i32 1
  store ptr null, ptr %32, align 8
  %33 = getelementptr { i32, ptr, i1 }, ptr %30, i32 0, i32 2
  store i1 false, ptr %33, align 1
  %34 = getelementptr { { i32, ptr, i1 }, { i32, ptr, i1 } }, ptr %3, i32 0, i32 1
  %35 = alloca { i32, ptr, i1 }, align 8
  %36 = load i32, ptr %2, align 4
  %37 = alloca i1, align 1
  %38 = getelementptr i1, ptr %37, i32 1
  %39 = ptrtoint ptr %37 to i64
  %40 = ptrtoint ptr %38 to i64
  %41 = sub i64 %40, %39
  %42 = sext i32 %36 to i64
  %43 = mul i64 %42, %41
  %44 = call ptr @malloc(i64 %43)
  br label %45

45:                                               ; preds = %48, %25
  %46 = phi i32 [ %50, %48 ], [ 0, %25 ]
  %47 = icmp slt i32 %46, %36
  br i1 %47, label %48, label %51

48:                                               ; preds = %45
  %49 = getelementptr i1, ptr %44, i32 %46
  store i1 false, ptr %49, align 1
  %50 = add i32 %46, 1
  br label %45

51:                                               ; preds = %45
  %52 = getelementptr { i32, ptr, i1 }, ptr %35, i32 0, i32 1
  store ptr %44, ptr %52, align 8
  %53 = getelementptr { i32, ptr, i1 }, ptr %35, i32 0, i32 0
  store i32 %36, ptr %53, align 4
  %54 = getelementptr { i32, ptr, i1 }, ptr %35, i32 0, i32 2
  store i1 false, ptr %54, align 1
  %55 = load { i32, ptr, i1 }, ptr %35, align 8
  store { i32, ptr, i1 } %55, ptr %34, align 8
  %56 = alloca { { i32, ptr, i1 }, { i32, ptr, i1 } }, align 8
  %57 = getelementptr { { i32, ptr, i1 }, { i32, ptr, i1 } }, ptr %3, i32 0, i32 0
  %58 = getelementptr { { i32, ptr, i1 }, { i32, ptr, i1 } }, ptr %56, i32 0, i32 0
  %59 = alloca { i32, ptr, i1 }, align 8
  %60 = getelementptr { i32, ptr, i1 }, ptr %57, i32 0, i32 0
  %61 = load i32, ptr %60, align 4
  %62 = getelementptr { i32, ptr, i1 }, ptr %57, i32 0, i32 1
  %63 = load ptr, ptr %62, align 8
  %64 = alloca i32, align 4
  %65 = getelementptr i32, ptr %64, i32 1
  %66 = ptrtoint ptr %64 to i64
  %67 = ptrtoint ptr %65 to i64
  %68 = sub i64 %67, %66
  %69 = sext i32 %61 to i64
  %70 = mul i64 %69, %68
  %71 = call ptr @malloc(i64 %70)
  br label %72

72:                                               ; preds = %75, %51
  %73 = phi i32 [ %79, %75 ], [ 0, %51 ]
  %74 = icmp slt i32 %73, %61
  br i1 %74, label %75, label %80

75:                                               ; preds = %72
  %76 = getelementptr i32, ptr %63, i32 %73
  %77 = getelementptr i32, ptr %71, i32 %73
  %78 = load i32, ptr %76, align 4
  store i32 %78, ptr %77, align 4
  %79 = add i32 %73, 1
  br label %72

80:                                               ; preds = %72
  %81 = getelementptr { i32, ptr, i1 }, ptr %59, i32 0, i32 1
  store ptr %71, ptr %81, align 8
  %82 = getelementptr { i32, ptr, i1 }, ptr %59, i32 0, i32 0
  store i32 %61, ptr %82, align 4
  %83 = getelementptr { i32, ptr, i1 }, ptr %57, i32 0, i32 2
  %84 = load i1, ptr %83, align 1
  %85 = getelementptr { i32, ptr, i1 }, ptr %59, i32 0, i32 2
  store i1 %84, ptr %85, align 1
  %86 = load { i32, ptr, i1 }, ptr %59, align 8
  store { i32, ptr, i1 } %86, ptr %58, align 8
  %87 = getelementptr { { i32, ptr, i1 }, { i32, ptr, i1 } }, ptr %3, i32 0, i32 1
  %88 = getelementptr { { i32, ptr, i1 }, { i32, ptr, i1 } }, ptr %56, i32 0, i32 1
  %89 = alloca { i32, ptr, i1 }, align 8
  %90 = getelementptr { i32, ptr, i1 }, ptr %87, i32 0, i32 0
  %91 = load i32, ptr %90, align 4
  %92 = getelementptr { i32, ptr, i1 }, ptr %87, i32 0, i32 1
  %93 = load ptr, ptr %92, align 8
  %94 = alloca i1, align 1
  %95 = getelementptr i1, ptr %94, i32 1
  %96 = ptrtoint ptr %94 to i64
  %97 = ptrtoint ptr %95 to i64
  %98 = sub i64 %97, %96
  %99 = sext i32 %91 to i64
  %100 = mul i64 %99, %98
  %101 = call ptr @malloc(i64 %100)
  br label %102

102:                                              ; preds = %105, %80
  %103 = phi i32 [ %109, %105 ], [ 0, %80 ]
  %104 = icmp slt i32 %103, %91
  br i1 %104, label %105, label %110

105:                                              ; preds = %102
  %106 = getelementptr i1, ptr %93, i32 %103
  %107 = getelementptr i1, ptr %101, i32 %103
  %108 = load i1, ptr %106, align 1
  store i1 %108, ptr %107, align 1
  %109 = add i32 %103, 1
  br label %102

110:                                              ; preds = %102
  %111 = getelementptr { i32, ptr, i1 }, ptr %89, i32 0, i32 1
  store ptr %101, ptr %111, align 8
  %112 = getelementptr { i32, ptr, i1 }, ptr %89, i32 0, i32 0
  store i32 %91, ptr %112, align 4
  %113 = getelementptr { i32, ptr, i1 }, ptr %87, i32 0, i32 2
  %114 = load i1, ptr %113, align 1
  %115 = getelementptr { i32, ptr, i1 }, ptr %89, i32 0, i32 2
  store i1 %114, ptr %115, align 1
  %116 = load { i32, ptr, i1 }, ptr %89, align 8
  store { i32, ptr, i1 } %116, ptr %88, align 8
  %117 = getelementptr { { i32, ptr, i1 }, { i32, ptr, i1 } }, ptr %56, i32 0, i32 0
  call void @printArray_019addab_1674_72d4_aa4a_ac782e511e7a(ptr %117, i32 0)
  %118 = alloca i32, align 4
  store i32 0, ptr %118, align 4
  %119 = getelementptr { i32, ptr, i1 }, ptr %117, i32 0, i32 0
  %120 = load i32, ptr %119, align 4
  %121 = getelementptr { i32, ptr, i1 }, ptr %117, i32 0, i32 1
  %122 = load ptr, ptr %121, align 8
  call void @free(ptr %122)
  store i32 0, ptr %119, align 4
  store ptr null, ptr %121, align 8
  %123 = load i32, ptr %118, align 4
  ret i32 %123
}
