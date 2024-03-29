#!/bin/bash
HOME="/home/srsantos/Experiment"
SIM_HOME=$HOME"/OrCS"
PIN_HOME=$SIM_HOME"/trace_generator/pin"
SINUCA_TRACER_HOME=$SIM_HOME"/trace_generator/extras/pinplay/bin/intel64/sinuca_tracer.so"
CODE_HOME=$HOME"/surf/db_op_x86"
COMP_FLAGS="-O2 -DNOINLINE -static -mavx2 -march=native"
SIZES=(1 20 64 80) # 256 8192 1048576)
PROBS=(6 7 9 10)

cd $CODE_HOME

if [ ! -d "exec" ]; then
	mkdir -p "exec"
fi

if [ ! -d "traces" ]; then
	mkdir -p "traces"
fi

for i in *tion*.cpp
do 
    rm exec/${i%.cpp}.out
    g++ $i $COMP_FLAGS -o exec/${i%.cpp}.out
    for j in "${SIZES[@]}";
    do
	#for k in "${PROBS[@]}";
	#do
    		echo "$PIN_HOME -t $SINUCA_TRACER_HOME -trace x86 -orcs_tracing 1 -output $CODE_HOME/traces/${i%.cpp}_${k}_.${j}MB.1t -- $CODE_HOME/exec/${i%.cpp}.out ${j} ${k} &> ${i%.cpp}_${j}_${k}.out &"
	    	nohup $PIN_HOME -t $SINUCA_TRACER_HOME -trace x86 -orcs_tracing 1 -output $CODE_HOME/traces/${i%.cpp}_${k}_.${j}MB.1t -- $CODE_HOME/exec/${i%.cpp}.out ${j} ${k} &> ${i%.cpp}_${j}_${k}.out &
	#done
    done
done
