( export PATH=~/sysu/bin:$PATH \
CPATH=~/sysu/include:$CPATH \
LIBRARY_PATH=~/sysu/lib:$LIBRARY_PATH \
LD_LIBRARY_PATH=~/sysu/lib:$LD_LIBRARY_PATH &&
# sysu-compiler --unittest=benchmark_generator_and_optimizer_1 "**/*.sysu.c" )
# sysu-compiler --unittest=benchmark_generator_and_optimizer_1 "functional/*.sysu.c" )
sysu-compiler --unittest=benchmark_generator_and_optimizer_1 \
/home/wuhlan3/lab3-new/SYsU-lang/tester/wuhlan.c )