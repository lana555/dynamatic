; ModuleID = '.example.c_mem2reg_constprop_simplifycfg.ll'
source_filename = "src/example.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define void @example(i32* %x, i32* %w, i32* %a) #0 {
entry:
  br label %for.body

for.body:                                         ; preds = %for.body, %entry
  %i.01 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %inc = add nuw nsw i32 %i.01, 1
  %cmp = icmp ult i32 %inc, 100
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
}

; Function Attrs: noinline nounwind uwtable
define i32 @main() #0 {
entry:
  %feature = alloca [1 x [100 x i32]], align 16
  %weight = alloca [1 x [100 x i32]], align 16
  %hist = alloca [1 x [100 x i32]], align 16
  br label %for.body

for.body:                                         ; preds = %for.end, %entry
  br label %for.body3

for.body3:                                        ; preds = %for.body3, %for.body
  %j.02 = phi i32 [ 0, %for.body ], [ %inc, %for.body3 ]
  %call = call i32 @rand() #2
  %rem = srem i32 %call, 50
  %0 = zext i32 %j.02 to i64
  %1 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0, i64 %0
  store i32 %rem, i32* %1, align 4
  %call8 = call i32 @rand() #2
  %rem9 = srem i32 %call8, 10
  %2 = zext i32 %j.02 to i64
  %3 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %weight, i64 0, i64 0, i64 %2
  store i32 %rem9, i32* %3, align 4
  %call14 = call i32 @rand() #2
  %rem15 = srem i32 %call14, 50
  %4 = zext i32 %j.02 to i64
  %5 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %hist, i64 0, i64 0, i64 %4
  store i32 %rem15, i32* %5, align 4
  %inc = add nuw nsw i32 %j.02, 1
  %cmp2 = icmp ult i32 %inc, 100
  br i1 %cmp2, label %for.body3, label %for.end

for.end:                                          ; preds = %for.body3
  %arrayidx21 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0, i64 3
  store i32 5, i32* %arrayidx21, align 4
  %arrayidx23 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0, i64 4
  store i32 5, i32* %arrayidx23, align 16
  %arrayidx25 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0, i64 30
  store i32 5, i32* %arrayidx25, align 8
  %arrayidx27 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0, i64 31
  store i32 5, i32* %arrayidx27, align 4
  br i1 false, label %for.body, label %for.end30

for.end30:                                        ; preds = %for.end
  br label %for.body34

for.body34:                                       ; preds = %for.body34, %for.end30
  %i31.01 = phi i32 [ 0, %for.end30 ], [ %inc44, %for.body34 ]
  %6 = zext i32 %i31.01 to i64
  %arraydecay = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 %6, i64 0
  %7 = zext i32 %i31.01 to i64
  %arraydecay39 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %weight, i64 0, i64 %7, i64 0
  %8 = zext i32 %i31.01 to i64
  %arraydecay42 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %hist, i64 0, i64 %8, i64 0
  call void @example(i32* nonnull %arraydecay, i32* nonnull %arraydecay39, i32* nonnull %arraydecay42)
  %inc44 = add nuw nsw i32 %i31.01, 1
  br i1 false, label %for.body34, label %for.end45

for.end45:                                        ; preds = %for.body34
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @rand() #1

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (http://llvm.org/git/clang.git 2f27999df400d17b33cdd412fdd606a88208dfcc) (http://llvm.org/git/llvm.git 5136df4d089a086b70d452160ad5451861269498)"}
