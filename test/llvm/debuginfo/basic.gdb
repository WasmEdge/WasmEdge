set pagination off
set confirm off
set breakpoint pending on
source @HELPER@
break dwarf_basic.c:@BREAK_SUMLIST@
run
echo @@BT1\n
bt 3
echo @@LOCALS\n
info locals
echo @@HEAD_RAW\n
print -raw-values -- Head
echo @@HEAD\n
print Head
echo @@WASM_VALUE\n
print $wasm(N)->Value
echo @@COUNTER\n
print Counter
echo @@NODE1\n
print Nodes[1].Value
echo @@GLOBAL\n
print $wasm_global("Counter")
delete
break dwarf_basic.c:@BREAK_FACT@
continue
echo @@BT2\n
bt 3
echo @@FACT_N\n
print N
kill
quit
