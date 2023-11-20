#include <libgpu/context.h>
#include <libgpu/shared_device_buffer.h>
#include <libutils/fast_random.h>
#include <libutils/misc.h>
#include <libutils/timer.h>
#include <gtest/gtest.h>

// Этот файл будет сгенерирован автоматически в момент сборки - см. convertIntoHeader в CMakeLists.txt:18
#include "cl/radix_cl.h"

#include <iostream>
#include <stdexcept>
#include <vector>




int main(int argc, char **argv) {
    gpu::Device device = gpu::chooseGPUDevice(0);

    EXPECT_GT(device.compute_units, 0);

    gpu::Context context;
    context.init(device.device_id_opencl);
    context.activate();

//    int benchmarkingIters = 10;
//    unsigned int n = 32 * 1024 * 1024;
//    std::vector<unsigned int> as(n, 0);
//    FastRandom r(n);
//    for (unsigned int i = 0; i < n; ++i) {
//        as[i] = (unsigned int) r.next(0, std::numeric_limits<int>::max());
//    }
//    std::cout << "Data generated for n=" << n << "!" << std::endl;
//
//    std::vector<unsigned int> cpu_sorted;
//    {
//        timer t;
//        for (int iter = 0; iter < benchmarkingIters; ++iter) {
//            cpu_sorted = as;
//            std::sort(cpu_sorted.begin(), cpu_sorted.end());
//            t.nextLap();
//        }
//        std::cout << "CPU: " << t.lapAvg() << "+-" << t.lapStd() << " s" << std::endl;
//        std::cout << "CPU: " << (n / 1000 / 1000) / t.lapAvg() << " millions/s" << std::endl;
//    }
    /*
    gpu::gpu_mem_32u as_gpu;
    as_gpu.resizeN(n);

    {
        ocl::Kernel radix(radix_kernel, radix_kernel_length, "radix");
        radix.compile();

        timer t;
        for (int iter = 0; iter < benchmarkingIters; ++iter) {
            as_gpu.writeN(as.data(), n);

            t.restart();// Запускаем секундомер после прогрузки данных, чтобы замерять время работы кернела, а не трансфер данных

            // TODO
        }
        std::cout << "GPU: " << t.lapAvg() << "+-" << t.lapStd() << " s" << std::endl;
        std::cout << "GPU: " << (n / 1000 / 1000) / t.lapAvg() << " millions/s" << std::endl;

        as_gpu.readN(as.data(), n);
    }

    // Проверяем корректность результатов
    for (int i = 0; i < n; ++i) {
        EXPECT_THE_SAME(as[i], cpu_sorted[i], "GPU results should be equal to CPU results!");
    }
*/
    return 0;
}