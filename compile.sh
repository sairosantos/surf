#!/bin/bash
HOME="/home/srsantos/Experiment"
SIM_HOME=$HOME"/OrCS"
PIN_HOME=$SIM_HOME"/trace_generator/pin"
SINUCA_TRACER_HOME=$SIM_HOME"/trace_generator/extras/pinplay/bin/intel64/sinuca_tracer.so"
CODE_HOME=$HOME"/surf/db_op"
COMP_FLAGS="-O2 -static"
SIZES=(64) # 256 8192 1048576)

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
    g++-7 $i $COMP_FLAGS -o exec/${i%.cpp}.out
    for j in "${SIZES[@]}";
    do
    		echo "$PIN_HOME -t $SINUCA_TRACER_HOME -trace iVIM -output $CODE_HOME/traces/${i%.cpp}.${j}KB.1t -- $CODE_HOME/exec/${i%.cpp}.out ${j} &> nohup.out &"
	    	nohup $PIN_HOME -t $SINUCA_TRACER_HOME -trace iVIM -output $CODE_HOME/traces/${i%.cpp}.${j}KB.1t -- $CODE_HOME/exec/${i%.cpp}.out ${j} &> nohup.out &
    done
done
