; ModuleID = '.example.c_mem2reg.ll'
source_filename = "src/example.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define void @example(i32* %x, i32* %w, i32* %a) #0 {
entry:
  br label %For_Loop

For_Loop:                                         ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %For_Loop, %for.inc
  %tmp.02 = phi i32 [ 0, %For_Loop ], [ %add, %for.inc ]
  %i.01 = phi i32 [ 0, %For_Loop ], [ %inc, %for.inc ]
  %idxprom = sext i32 %i.01 to i64
  %arrayidx = getelementptr inbounds i32, i32* %x, i64 %idxprom
  %0 = load i32, i32* %arrayidx, align 4
  %sub = sub nsw i32 99, %i.01
  %idxprom1 = sext i32 %sub to i64
  %arrayidx2 = getelementptr inbounds i32, i32* %w, i64 %idxprom1
  %1 = load i32, i32* %arrayidx2, align 4
  %mul = mul nsw i32 %0, %1
  %add = add nsw i32 %tmp.02, %mul
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.01, 1
  %cmp = icmp slt i32 %inc, 100
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.inc
  ret void
}

; Function Attrs: noinline nounwind uwtable
define i32 @main() #0 {
entry:
  %feature = alloca [1 x [100 x i32]], align 16
  %weight = alloca [1 x [100 x i32]], align 16
  %hist = alloca [1 x [100 x i32]], align 16
  %n = alloca [1 x i32], align 4
  br label %for.body

for.body:                                         ; preds = %entry, %for.inc28
  %i.03 = phi i32 [ 0, %entry ], [ %inc29, %for.inc28 ]
  %idxprom = sext i32 %i.03 to i64
  %arrayidx = getelementptr inbounds [1 x i32], [1 x i32]* %n, i64 0, i64 %idxprom
  store i32 100, i32* %arrayidx, align 4
  br label %for.body3

for.body3:                                        ; preds = %for.body, %for.inc
  %j.02 = phi i32 [ 0, %for.body ], [ %inc, %for.inc ]
  %call = call i32 @rand() #2
  %rem = srem i32 %call, 50
  %idxprom4 = sext i32 %i.03 to i64
  %arrayidx5 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 %idxprom4
  %idxprom6 = sext i32 %j.02 to i64
  %arrayidx7 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx5, i64 0, i64 %idxprom6
  store i32 %rem, i32* %arrayidx7, align 4
  %call8 = call i32 @rand() #2
  %rem9 = srem i32 %call8, 10
  %idxprom10 = sext i32 %i.03 to i64
  %arrayidx11 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %weight, i64 0, i64 %idxprom10
  %idxprom12 = sext i32 %j.02 to i64
  %arrayidx13 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx11, i64 0, i64 %idxprom12
  store i32 %rem9, i32* %arrayidx13, align 4
  %call14 = call i32 @rand() #2
  %rem15 = srem i32 %call14, 50
  %idxprom16 = sext i32 %i.03 to i64
  %arrayidx17 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %hist, i64 0, i64 %idxprom16
  %idxprom18 = sext i32 %j.02 to i64
  %arrayidx19 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx17, i64 0, i64 %idxprom18
  store i32 %rem15, i32* %arrayidx19, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %inc = add nsw i32 %j.02, 1
  %cmp2 = icmp slt i32 %inc, 100
  br i1 %cmp2, label %for.body3, label %for.end

for.end:                                          ; preds = %for.inc
  %arrayidx20 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0
  %arrayidx21 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx20, i64 0, i64 3
  store i32 5, i32* %arrayidx21, align 4
  %arrayidx22 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0
  %arrayidx23 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx22, i64 0, i64 4
  store i32 5, i32* %arrayidx23, align 16
  %arrayidx24 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0
  %arrayidx25 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx24, i64 0, i64 30
  store i32 5, i32* %arrayidx25, align 8
  %arrayidx26 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 0
  %arrayidx27 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx26, i64 0, i64 31
  store i32 5, i32* %arrayidx27, align 4
  br label %for.inc28

for.inc28:                                        ; preds = %for.end
  %inc29 = add nsw i32 %i.03, 1
  %cmp = icmp slt i32 %inc29, 1
  br i1 %cmp, label %for.body, label %for.end30

for.end30:                                        ; preds = %for.inc28
  br label %for.body34

for.body34:                                       ; preds = %for.end30, %for.inc43
  %i31.01 = phi i32 [ 0, %for.end30 ], [ %inc44, %for.inc43 ]
  %idxprom35 = sext i32 %i31.01 to i64
  %arrayidx36 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %feature, i64 0, i64 %idxprom35
  %arraydecay = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx36, i32 0, i32 0
  %idxprom37 = sext i32 %i31.01 to i64
  %arrayidx38 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %weight, i64 0, i64 %idxprom37
  %arraydecay39 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx38, i32 0, i32 0
  %idxprom40 = sext i32 %i31.01 to i64
  %arrayidx41 = getelementptr inbounds [1 x [100 x i32]], [1 x [100 x i32]]* %hist, i64 0, i64 %idxprom40
  %arraydecay42 = getelementptr inbounds [100 x i32], [100 x i32]* %arrayidx41, i32 0, i32 0
  call void @example(i32* %arraydecay, i32* %arraydecay39, i32* %arraydecay42)
  br label %for.inc43

for.inc43:                                        ; preds = %for.body34
  %inc44 = add nsw i32 %i31.01, 1
  %cmp33 = icmp slt i32 %inc44, 1
  br i1 %cmp33, label %for.body34, label %for.end45

for.end45:                                        ; preds = %for.inc43
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
