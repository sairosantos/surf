#!/bin/bash
HOME="/home/srsantos/Experiment"
SIM_HOME=$HOME"/OrCS"
PIN_HOME=$SIM_HOME"/trace_generator/pin"
SINUCA_TRACER_HOME=$SIM_HOME"/trace_generator/extras/pinplay/bin/intel64/sinuca_tracer.so"
CODE_HOME=$HOME"/surf/db_op"
COMP_FLAGS="-O2 -DNOINLINE -static"
SIZES=(1 20 40) # 256 8192 1048576)

cd $CODE_HOME

if [ ! -d "exec" ]; then
	mkdir -p "exec"
fi

if [ ! -d "traces" ]; then
	mkdir -p "traces"
fi

for i in *.cpp
do 
    rm exec/${i%.cpp}.out
    g++ $i $COMP_FLAGS -o exec/${i%.cpp}.out
    for j in "${SIZES[@]}";
    do
    		echo "$PIN_HOME -t $SINUCA_TRACER_HOME -trace iVIM -orcs_tracing 1 -output $CODE_HOME/traces/${i%.cpp}.${j}MB.1t -- $CODE_HOME/exec/${i%.cpp}.out ${j} &> ${i%.cpp}.${j}MB.out &"
	    	nohup $PIN_HOME -t $SINUCA_TRACER_HOME -trace iVIM -orcs_tracing 1 -output $CODE_HOME/traces/${i%.cpp}.${j}MB.1t -- $CODE_HOME/exec/${i%.cpp}.out ${j} &> ${i%.cpp}.${j}MB.out &
    done
done
