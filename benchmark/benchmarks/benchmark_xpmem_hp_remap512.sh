vsizes=(2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912)
file=benchmark_xpmem_hp_remap512.csv

for vsize in ${vsizes[@]}; do
    echo ${vsize} >> ../results/${file}
    for i in {1..20} ; do
        ../../build/benchmark/benchmark ../../build/benchmark/client 1 1 2097152 ${vsize} 0 >> ../results/${file}
        sleep 1
    done
    echo "" >> ../results/${file}
    echo "" >> ../results/${file}
done
