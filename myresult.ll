; ModuleID = '-'
source_filename = "-"

@width = dso_local constant i32 1024
@height = dso_local constant i32 1024
@image_in = dso_local global [1048576 x i32] zeroinitializer
@image_out = dso_local global [1048576 x i32] zeroinitializer

declare dso_local void @_sysy_starttime(i32)

declare dso_local void @_sysy_stoptime(i32)

declare dso_local i32 @_sysy_getch()

declare dso_local void @_sysy_putch(i32)

declare dso_local i32 @_sysy_getint()

declare dso_local void @_sysy_putint(i32)

declare dso_local i32 @_sysy_getarray(i32*)

declare dso_local void @_sysy_putarray(i32, i32*)

define dso_local i32 @cutout(i32 %val) {
entry:
  %val.addr = alloca i32, align 4
  store i32 %val, i32* %val.addr, align 4
  %0 = load i32, i32* %val.addr, align 4
  %slttmp = icmp slt i32 %0, 0
  br i1 %slttmp, label %ifthen, label %ifelse

ifthen:                                           ; preds = %entry
  store i32 0, i32* %val.addr, align 4
  br label %ifcont2

ifelse:                                           ; preds = %entry
  %1 = load i32, i32* %val.addr, align 4
  %sgttmp = icmp sgt i32 %1, 255
  br i1 %sgttmp, label %ifthen1, label %ifcont

ifthen1:                                          ; preds = %ifelse
  store i32 255, i32* %val.addr, align 4
  br label %ifcont

ifcont:                                           ; preds = %ifthen1, %ifelse
  br label %ifcont2

ifcont2:                                          ; preds = %ifcont, %ifthen
  %2 = load i32, i32* %val.addr, align 4
  ret i32 %2
}

define dso_local i32 @main() {
entry:
  %val = alloca i32, align 4
  %ip1jp1 = alloca i32, align 4
  %ip1j = alloca i32, align 4
  %ip1jm1 = alloca i32, align 4
  %ijp1 = alloca i32, align 4
  %ij = alloca i32, align 4
  %ijm1 = alloca i32, align 4
  %im1jp1 = alloca i32, align 4
  %im1j = alloca i32, align 4
  %im1jm1 = alloca i32, align 4
  %num = alloca i32, align 4
  %j = alloca i32, align 4
  %i = alloca i32, align 4
  %0 = call i32 @_sysy_getarray(i32* getelementptr inbounds ([1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 0))
  store i32 %0, i32* %num, align 4
  call void @_sysy_starttime(i32 24)
  store i32 1, i32* %j, align 4
  br label %whilecond

whilecond:                                        ; preds = %whileend, %entry
  %1 = load i32, i32* %j, align 4
  %2 = load i32, i32* @width, align 4
  %subtmp = sub nsw i32 %2, 1
  %slttmp = icmp slt i32 %1, %subtmp
  br i1 %slttmp, label %whilebody, label %whileend46

whilebody:                                        ; preds = %whilecond
  store i32 1, i32* %i, align 4
  br label %whilecond1

whilecond1:                                       ; preds = %whilebody4, %whilebody
  %3 = load i32, i32* %i, align 4
  %4 = load i32, i32* @height, align 4
  %subtmp2 = sub nsw i32 %4, 1
  %slttmp3 = icmp slt i32 %3, %subtmp2
  br i1 %slttmp3, label %whilebody4, label %whileend

whilebody4:                                       ; preds = %whilecond1
  %5 = load i32, i32* %i, align 4
  %subtmp5 = sub nsw i32 %5, 1
  %6 = load i32, i32* @width, align 4
  %multmp = mul nsw i32 %subtmp5, %6
  %7 = load i32, i32* %j, align 4
  %addtmp = add nsw i32 %multmp, %7
  %subtmp6 = sub nsw i32 %addtmp, 1
  store i32 %subtmp6, i32* %im1jm1, align 4
  %8 = load i32, i32* %i, align 4
  %subtmp7 = sub nsw i32 %8, 1
  %9 = load i32, i32* @width, align 4
  %multmp8 = mul nsw i32 %subtmp7, %9
  %10 = load i32, i32* %j, align 4
  %addtmp9 = add nsw i32 %multmp8, %10
  store i32 %addtmp9, i32* %im1j, align 4
  %11 = load i32, i32* %i, align 4
  %subtmp10 = sub nsw i32 %11, 1
  %12 = load i32, i32* @width, align 4
  %multmp11 = mul nsw i32 %subtmp10, %12
  %13 = load i32, i32* %j, align 4
  %addtmp12 = add nsw i32 %multmp11, %13
  %addtmp13 = add nsw i32 %addtmp12, 1
  store i32 %addtmp13, i32* %im1jp1, align 4
  %14 = load i32, i32* %i, align 4
  %15 = load i32, i32* @width, align 4
  %multmp14 = mul nsw i32 %14, %15
  %16 = load i32, i32* %j, align 4
  %addtmp15 = add nsw i32 %multmp14, %16
  %subtmp16 = sub nsw i32 %addtmp15, 1
  store i32 %subtmp16, i32* %ijm1, align 4
  %17 = load i32, i32* %i, align 4
  %18 = load i32, i32* @width, align 4
  %multmp17 = mul nsw i32 %17, %18
  %19 = load i32, i32* %j, align 4
  %addtmp18 = add nsw i32 %multmp17, %19
  store i32 %addtmp18, i32* %ij, align 4
  %20 = load i32, i32* %i, align 4
  %21 = load i32, i32* @width, align 4
  %multmp19 = mul nsw i32 %20, %21
  %22 = load i32, i32* %j, align 4
  %addtmp20 = add nsw i32 %multmp19, %22
  %addtmp21 = add nsw i32 %addtmp20, 1
  store i32 %addtmp21, i32* %ijp1, align 4
  %23 = load i32, i32* %i, align 4
  %addtmp22 = add nsw i32 %23, 1
  %24 = load i32, i32* @width, align 4
  %multmp23 = mul nsw i32 %addtmp22, %24
  %25 = load i32, i32* %j, align 4
  %addtmp24 = add nsw i32 %multmp23, %25
  %subtmp25 = sub nsw i32 %addtmp24, 1
  store i32 %subtmp25, i32* %ip1jm1, align 4
  %26 = load i32, i32* %i, align 4
  %addtmp26 = add nsw i32 %26, 1
  %27 = load i32, i32* @width, align 4
  %multmp27 = mul nsw i32 %addtmp26, %27
  %28 = load i32, i32* %j, align 4
  %addtmp28 = add nsw i32 %multmp27, %28
  store i32 %addtmp28, i32* %ip1j, align 4
  %29 = load i32, i32* %i, align 4
  %addtmp29 = add nsw i32 %29, 1
  %30 = load i32, i32* @width, align 4
  %multmp30 = mul nsw i32 %addtmp29, %30
  %31 = load i32, i32* %j, align 4
  %addtmp31 = add nsw i32 %multmp30, %31
  %addtmp32 = add nsw i32 %addtmp31, 1
  store i32 %addtmp32, i32* %ip1jp1, align 4
  %32 = load i32, i32* %ij, align 4
  %33 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %32
  %34 = load i32, i32* %33, align 4
  %multmp33 = mul nsw i32 8, %34
  %35 = load i32, i32* %im1jm1, align 4
  %36 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %35
  %37 = load i32, i32* %36, align 4
  %subtmp34 = sub nsw i32 %multmp33, %37
  %38 = load i32, i32* %im1j, align 4
  %39 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %38
  %40 = load i32, i32* %39, align 4
  %subtmp35 = sub nsw i32 %subtmp34, %40
  %41 = load i32, i32* %im1jp1, align 4
  %42 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %41
  %43 = load i32, i32* %42, align 4
  %subtmp36 = sub nsw i32 %subtmp35, %43
  %44 = load i32, i32* %ijm1, align 4
  %45 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %44
  %46 = load i32, i32* %45, align 4
  %subtmp37 = sub nsw i32 %subtmp36, %46
  %47 = load i32, i32* %ijp1, align 4
  %48 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %47
  %49 = load i32, i32* %48, align 4
  %subtmp38 = sub nsw i32 %subtmp37, %49
  %50 = load i32, i32* %ip1jm1, align 4
  %51 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %50
  %52 = load i32, i32* %51, align 4
  %subtmp39 = sub nsw i32 %subtmp38, %52
  %53 = load i32, i32* %ip1j, align 4
  %54 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %53
  %55 = load i32, i32* %54, align 4
  %subtmp40 = sub nsw i32 %subtmp39, %55
  %56 = load i32, i32* %ip1jp1, align 4
  %57 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %56
  %58 = load i32, i32* %57, align 4
  %subtmp41 = sub nsw i32 %subtmp40, %58
  store i32 %subtmp41, i32* %val, align 4
  %59 = load i32, i32* %i, align 4
  %60 = load i32, i32* @width, align 4
  %multmp42 = mul nsw i32 %59, %60
  %61 = load i32, i32* %j, align 4
  %addtmp43 = add nsw i32 %multmp42, %61
  %62 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_out, i32 0, i32 %addtmp43
  %63 = load i32, i32* %val, align 4
  %64 = call i32 @cutout(i32 %63)
  store i32 %64, i32* %62, align 4
  %65 = load i32, i32* %i, align 4
  %addtmp44 = add nsw i32 %65, 1
  store i32 %addtmp44, i32* %i, align 4
  br label %whilecond1

whileend:                                         ; preds = %whilecond1
  %66 = load i32, i32* %j, align 4
  %addtmp45 = add nsw i32 %66, 1
  store i32 %addtmp45, i32* %j, align 4
  br label %whilecond

whileend46:                                       ; preds = %whilecond
  store i32 0, i32* %i, align 4
  br label %whilecond47

whilecond47:                                      ; preds = %whilebody49, %whileend46
  %67 = load i32, i32* %i, align 4
  %68 = load i32, i32* @height, align 4
  %slttmp48 = icmp slt i32 %67, %68
  br i1 %slttmp48, label %whilebody49, label %whileend59

whilebody49:                                      ; preds = %whilecond47
  %69 = load i32, i32* %i, align 4
  %70 = load i32, i32* @width, align 4
  %multmp50 = mul nsw i32 %69, %70
  %71 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_out, i32 0, i32 %multmp50
  %72 = load i32, i32* %i, align 4
  %73 = load i32, i32* @width, align 4
  %multmp51 = mul nsw i32 %72, %73
  %74 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %multmp51
  %75 = load i32, i32* %74, align 4
  store i32 %75, i32* %71, align 4
  %76 = load i32, i32* %i, align 4
  %77 = load i32, i32* @width, align 4
  %multmp52 = mul nsw i32 %76, %77
  %78 = load i32, i32* @width, align 4
  %addtmp53 = add nsw i32 %multmp52, %78
  %subtmp54 = sub nsw i32 %addtmp53, 1
  %79 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_out, i32 0, i32 %subtmp54
  %80 = load i32, i32* %i, align 4
  %81 = load i32, i32* @width, align 4
  %multmp55 = mul nsw i32 %80, %81
  %82 = load i32, i32* @width, align 4
  %addtmp56 = add nsw i32 %multmp55, %82
  %subtmp57 = sub nsw i32 %addtmp56, 1
  %83 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %subtmp57
  %84 = load i32, i32* %83, align 4
  store i32 %84, i32* %79, align 4
  %85 = load i32, i32* %i, align 4
  %addtmp58 = add nsw i32 %85, 1
  store i32 %addtmp58, i32* %i, align 4
  br label %whilecond47

whileend59:                                       ; preds = %whilecond47
  store i32 0, i32* %j, align 4
  br label %whilecond60

whilecond60:                                      ; preds = %whilebody62, %whileend59
  %86 = load i32, i32* %j, align 4
  %87 = load i32, i32* @width, align 4
  %slttmp61 = icmp slt i32 %86, %87
  br i1 %slttmp61, label %whilebody62, label %whileend70

whilebody62:                                      ; preds = %whilecond60
  %88 = load i32, i32* %j, align 4
  %89 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_out, i32 0, i32 %88
  %90 = load i32, i32* %j, align 4
  %91 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %90
  %92 = load i32, i32* %91, align 4
  store i32 %92, i32* %89, align 4
  %93 = load i32, i32* @height, align 4
  %subtmp63 = sub nsw i32 %93, 1
  %94 = load i32, i32* @width, align 4
  %multmp64 = mul nsw i32 %subtmp63, %94
  %95 = load i32, i32* %j, align 4
  %addtmp65 = add nsw i32 %multmp64, %95
  %96 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_out, i32 0, i32 %addtmp65
  %97 = load i32, i32* @height, align 4
  %subtmp66 = sub nsw i32 %97, 1
  %98 = load i32, i32* @width, align 4
  %multmp67 = mul nsw i32 %subtmp66, %98
  %99 = load i32, i32* %j, align 4
  %addtmp68 = add nsw i32 %multmp67, %99
  %100 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @image_in, i32 0, i32 %addtmp68
  %101 = load i32, i32* %100, align 4
  store i32 %101, i32* %96, align 4
  %102 = load i32, i32* %j, align 4
  %addtmp69 = add nsw i32 %102, 1
  store i32 %addtmp69, i32* %j, align 4
  br label %whilecond60

whileend70:                                       ; preds = %whilecond60
  call void @_sysy_stoptime(i32 60)
  %103 = load i32, i32* @width, align 4
  %104 = load i32, i32* @height, align 4
  %multmp71 = mul nsw i32 %103, %104
  call void @_sysy_putarray(i32 %multmp71, i32* getelementptr inbounds ([1048576 x i32], [1048576 x i32]* @image_out, i32 0, i32 0))
  %105 = load i32, i32* %num, align 4
  ret i32 %105
}
