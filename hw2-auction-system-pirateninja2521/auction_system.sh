#!/bin/bash

if [ $# -ne 2 ]; then
    echo "usage: bash auction_system.sh [n_host] [n_player]"
    exit
fi

n_host=$1
n_player=$2

for i in $(seq 1 $n_player); do
    score[$i]=0
done

#Prepare FIFO files for all M hosts.
cnt=0
for i in $(seq 0 $n_host); do
    PIPE[$i]="fifo_${i}.tmp"
    if test -e "${PIPE[$i]}"; then 
        #echo "${PIPE[$i]} exists."
        rm "${PIPE[$i]}"
    fi
    mkfifo ${PIPE[$i]}
    key[$i]=$RANDOM
    key_inv[key[$i]]=$i
    eval exec "$(($i+3))"'<>"${PIPE[$i]}"'
    if [ $i -ne 0 ]; then
        cmd="./host ${i} ${key[$i]} 0"
        $cmd &
        pid[$i]=$!
        cnt=$(($cnt+1))
    fi
done
exec 3<> fifo_0.tmp
# Generate  combinations of N players and assign to hosts via FIFO. Collect rankings from hosts and calculate the scores.
total=0
for a in $(seq 1 $n_player); do
    for b in $(seq $(($a+1)) $n_player); do
        for c in $(seq $(($b+1)) $n_player); do
            for d in $(seq $(($c+1)) $n_player); do
                for e in $(seq $(($d+1)) $n_player); do
                    for f in $(seq $(($e+1)) $n_player); do
                        for g in $(seq $(($f+1)) $n_player); do
                            for h in $(seq $(($g+1)) $n_player); do                              
                                total=$(($total+1))
                                forhost[$total]="$a $b $c $d $e $f $g $h"                               
                            done
                        done
                    done
                done
            done
        done
    done
done

sent_line=0
current_line=0
for i in $(seq 1 $n_host); do
    sent_line=$(($sent_line+1))
    echo ${forhost[$i]} > "${PIPE[$i]}"  
    if [ $i -eq $total ]; then 
        break
    fi
done
mod=0
now_host=""

while read -r line; do
    if [ $mod -eq 0 ]; then
#        echo $line
        now_host=${key_inv[$line]}
#        echo "###"
#        echo "$now_host"
    else
        read -r now_player rank <<< "$line"
#        echo $now_player $rank
        score[$now_player]=$((${score[$now_player]}+8-$rank))
    fi
    if [ $mod -eq 8 ]; then
        if [ $sent_line -lt $total ]; then
            sent_line=$(($sent_line+1))
            echo ${forhost[$sent_line]} > "${PIPE[$now_host]}"
        else
            # After all combinations are hosted, send an ending message to all hosts forked.
            echo "-1 -1 -1 -1 -1 -1 -1 -1" > "${PIPE[$now_host]}"
        fi
        current_line=$(($current_line+1))
        mod=0
        if [ $current_line -eq $total ]; then
            break
        fi
    else   
        mod=$(($mod+1))
    fi

done < "fifo_0.tmp"

# Print the final scores ordered by player id (ascending) to stdout.

for i in $(seq 1 $n_player); do
    echo "$i ${score[$i]}"
done


# Remove FIFO files.

for((i=0; i<=$n_host; i++))
do
    rm "${PIPE[$i]}"
done

# Wait for all forked process to exit.

#echo "waiting"
for i in $(seq 1 $n_host); do
    wait ${pid[$i]}
done
#echo "done"
