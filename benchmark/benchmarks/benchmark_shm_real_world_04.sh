stepsizes=(1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072)
file=benchmark_shm_real_world_04.csv

for stepsize in ${stepsizes[@]}; do
    echo ${stepsize} >> ../results/${file}
    for i in {1..10} ; do
        ../../build/benchmark/benchmark ../../build/benchmark/client 1 0 ${stepsize} 4194304 0 >> ../results/${file}
        sleep 1
    done
    echo "" >> ../results/${file}
    echo "" >> ../results/${file}
done
