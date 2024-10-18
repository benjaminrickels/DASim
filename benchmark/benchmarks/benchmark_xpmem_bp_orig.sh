vsizes=(4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912)
file=benchmark_xpmem_bp_orig.csv

for vsize in ${vsizes[@]}; do
    echo ${vsize} >> ../results/${file}
    for i in {1..10} ; do
        ../../build/benchmark/benchmark ../../build/benchmark/client 0 1 4096 ${vsize} 0 >> ../results/${file}
        sleep 1
    done
    echo "" >> ../results/${file}
    echo "" >> ../results/${file}
done
