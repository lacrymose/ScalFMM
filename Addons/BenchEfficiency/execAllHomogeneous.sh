#!/bin/bash

echo "Perform the computation for :"
echo "$SCALFMM_NB particles"
echo "$SCALFMM_H tree height"
echo "Up to $SCALFMM_MAX_NB_CPU CPUs"
echo ""
echo "Using granularities:"
echo "$SCALFMM_BS_CPU_SEQ and $SCALFMM_BS_CPU_PAR"

# only in seq with the seq bs
cpu=1
STARPU_NCPUS=$cpu
STARPU_NCUDA=0

logoutput=`./Tests/Release/testBlockedUnifCudaBench -nb $SCALFMM_NB -h $SCALFMM_H -bs $SCALFMM_BS_CPU_SEQ`
if [[ $VERBOSE ]] ; then
    echo $logoutput
fi
$SCALFMM_STARPU_DIR/bin/starpu_fxt_tool -i "/tmp/prof_file_"$USER"_0"
rec_name="$SCALFMM_RES_DIR/trace-nb_$SCALFMM_NB-h_$SCALFMM_H-bs_$SCALFMM_BS_CPU_SEQ-CPU_$cpu.rec"
mv trace.rec $rec_name
python $SCALFMM_STARPU_DIR/bin/starpu_trace_state_stats.py -t $rec_name > $rec_name.time


for (( cpu=1 ; cpu<=$SCALFMM_MAX_NB_CPU ; cpu++)) ; do
    echo ">> CPU = $cpu"

    STARPU_NCPUS=$cpu
    STARPU_NCUDA=0

    logoutput=`./Tests/Release/testBlockedUnifCudaBench -nb $SCALFMM_NB -h $SCALFMM_H -bs $SCALFMM_BS_CPU_PAR`
    if [[ $VERBOSE ]] ; then
        echo $logoutput
    fi
    $SCALFMM_STARPU_DIR/bin/starpu_fxt_tool -i "/tmp/prof_file_"$USER"_0"
    rec_name="$SCALFMM_RES_DIR/trace-nb_$SCALFMM_NB-h_$SCALFMM_H-bs_$SCALFMM_BS_CPU_PAR-CPU_$cpu.rec"
    mv trace.rec $rec_name
    python $SCALFMM_STARPU_DIR/bin/starpu_trace_state_stats.py -t $rec_name > $rec_name.time
done

