#include <libgpu/context.h>
#include <libgpu/shared_device_buffer.h>
#include <libutils/fast_random.h>
#include <libutils/misc.h>
#include <libutils/timer.h>

#include "cl/matrix_multiplication_cl.h"

#include <iostream>
#include <stdexcept>
#include <vector>

int benchmarkingIters = 1;// TODO пока тестируетесь удобно выставить единицу
unsigned int M = 1024;
unsigned int K = 1024;
unsigned int N = 1024;

const size_t gflops =
        ((size_t) M * K * N * 2) / (1000 * 1000 * 1000);// умножить на два, т.к. операция сложения и умножения


std::vector<float> cpu_compute(std::vector<float> const &as, std::vector<float> const &bs) {
    timer t;
    std::vector<float> cs(M * N, 0);
    for (int iter = 0; iter < benchmarkingIters; ++iter) {
        for (int j = 0; j < M; ++j) {
            for (int i = 0; i < N; ++i) {
                float sum = 0.0f;
                for (int k = 0; k < K; ++k) {
                    sum += as.data()[j * K + k] * bs.data()[k * N + i];
                }
                cs.data()[j * N + i] = sum;
            }
        }
        t.nextLap();
    }
    std::cout << "CPU: " << t.lapAvg() << "+-" << t.lapStd() << " s" << std::endl;
    std::cout << "CPU: " << gflops / t.lapAvg() << " GFlops" << std::endl;
    return cs;
}

void bench_and_test(ocl::Kernel kernel, gpu::WorkSize workSize, gpu::gpu_mem_32f as_gpu, gpu::gpu_mem_32f bs_gpu,
                    gpu::gpu_mem_32f cs_gpu, std::vector<float> const &cs_cpu_reference, std::string test_name) {
    std::cout << "Test " << test_name << ":\n";
    {
        timer t;
        for (int iter = 0; iter < benchmarkingIters; ++iter) {

            kernel.exec(workSize, as_gpu, bs_gpu, cs_gpu, M, K, N);
            t.nextLap();
        }
        std::cout << "\tGPU: " << t.lapAvg() << "+-" << t.lapStd() << " s" << std::endl;
        std::cout << "\tGPU: " << gflops / t.lapAvg() << " GFlops" << std::endl;
    }

    std::vector<float> cs = cs_cpu_reference;
    cs_gpu.readN(cs.data(), M * N);


    // Проверяем корректность результатов
    double diff_sum = 0;
    for (int i = 0; i < M * N; ++i) {
        double a = cs[i];
        double b = cs_cpu_reference[i];
        if (a != 0.0 || b != 0.0) {
            double diff = fabs(a - b) / std::max(fabs(a), fabs(b));
            diff_sum += diff;
        }
    }

    double diff_avg = diff_sum / (M * N);
    std::cout << "\tAverage difference: " << diff_avg * 100.0 << "%" << std::endl;
    if (diff_avg > 0.01) {
        std::cerr << "\tToo big difference!" << std::endl;
        return;
    }
    std::cout << std::endl;
}

int main(int argc, char **argv) {
    gpu::Device device = gpu::chooseGPUDevice(argc, argv);

    gpu::Context context;
    context.init(device.device_id_opencl);
    context.activate();

    std::vector<float> as(M * K, 0);
    std::vector<float> bs(K * N, 0);

    FastRandom r(M + K + N);
    for (unsigned int i = 0; i < as.size(); ++i) {
        as[i] = r.nextf();
    }
    for (unsigned int i = 0; i < bs.size(); ++i) {
        bs[i] = r.nextf();
    }

    std::cout << "Data generated for M=" << M << ", K=" << K << ", N=" << N << std::endl;

    const std::vector<float> cs_cpu_reference = cpu_compute(as, bs);

    gpu::gpu_mem_32f as_gpu, bs_gpu, cs_gpu;
    as_gpu.resizeN(M * K);
    bs_gpu.resizeN(K * N);
    cs_gpu.resizeN(M * N);

    as_gpu.writeN(as.data(), M * K);
    bs_gpu.writeN(bs.data(), K * N);

    {
        for (int work_group_size : {8, 16}) {
            ocl::Kernel matrix_multiplication_kernel(matrix_multiplication, matrix_multiplication_length,
                                                     "matrix_multiplication_naive");
            matrix_multiplication_kernel.compile();
            bench_and_test(matrix_multiplication_kernel, gpu::WorkSize(work_group_size, work_group_size, N, M), as_gpu,
                           bs_gpu, cs_gpu, cs_cpu_reference, "Naive " + std::to_string(work_group_size));
        }
    }

    {
        for (int work_group_size : {8, 16}) {
            ocl::Kernel matrix_multiplication_kernel(matrix_multiplication, matrix_multiplication_length,
                                                     "matrix_multiplication_local_mem",
                                                     "-DTILE_SIZE=" + std::to_string(work_group_size));
            matrix_multiplication_kernel.compile();
            bench_and_test(matrix_multiplication_kernel, gpu::WorkSize(work_group_size, work_group_size, N, M), as_gpu,
                           bs_gpu, cs_gpu, cs_cpu_reference, "Local Mem " + std::to_string(work_group_size));
        }
    }

    {
        for (int work_group_size : {8, 16})
            for (int work_per_item = 4; work_per_item <= work_group_size; work_per_item *= 2) {
                std::string defines = "-DTILE_SIZE=" + std::to_string(work_group_size) +
                                      " -DWORK_PER_ITEM=" + std::to_string(work_per_item);
                ocl::Kernel matrix_multiplication_kernel(matrix_multiplication, matrix_multiplication_length,
                                                         "matrix_multiplication_local_mem_more_work", defines);
                matrix_multiplication_kernel.compile();
                bench_and_test(matrix_multiplication_kernel,
                               gpu::WorkSize(work_group_size, work_group_size / work_per_item,
                                             (N + work_per_item - 1) / work_per_item, M),
                               as_gpu, bs_gpu, cs_gpu, cs_cpu_reference,
                               "More WPI " + std::to_string(work_group_size) + ", " + std::to_string(work_per_item));
            }
    }

    return 0;
}