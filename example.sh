export PATH=~/sysu/bin:$PATH   CPATH=~/sysu/include:$CPATH   LIBRARY_PATH=~/sysu/lib:$LIBRARY_PATH   LD_LIBRARY_PATH=~/sysu/lib:$LD_LIBRARY_PATH && clang -emit-llvm -S tester/wuhlan.c