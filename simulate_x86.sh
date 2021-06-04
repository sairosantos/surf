#!/bin/bash
HOME="/home/srsantos/Experiment"
SIM_HOME=$HOME"/OrCS"
CODE_HOME=$HOME"/surf/db_op"
TRACE_HOME=$CODE_HOME"/traces/x86"
THREADS=(1)
VECTOR_SIZE=(8K)
#CACHE_SIZE=(32 64 128 256)
DATE_TIME=$(date '+%d%m%Y_%H%M%S');

cd $CODE_HOME

if [ ! -d "resultados/x86" ]; then
	mkdir -p "resultados/x86"
fi

for t in "${THREADS[@]}"
do
    echo "THREADS: ${t}"
    cd $TRACE_HOME
    for i in *.${t}t.tid0.stat.out.gz
    do 
        cd $SIM_HOME
        TRACE=${i%.tid0.stat.out.gz}
        COUNTER=0
        COMMAND="./orcs"
        while [ $COUNTER -lt $t ]; do
            COMMAND=${COMMAND}' -t '${TRACE_HOME}/${TRACE}
            let COUNTER=COUNTER+1
        done
       	CONFIG_FILE="${SIM_HOME}/configuration_files/sandy_bridge.cfg"
	echo "nohup ${COMMAND} -c ${CONFIG_FILE} &> ${CODE_HOME}/resultados/x86/${TRACE}_${k}_${DATE_TIME}.txt &"
        nohup ${COMMAND} -c ${CONFIG_FILE} &> ${CODE_HOME}/resultados/x86/${TRACE}_${j}_${k}_${DATE_TIME}.txt &
        #echo "${COMMAND} -c ${CONFIG_FILE} &
    done
done

