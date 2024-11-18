#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <memory>
#include <iostream>
#include <vector>

class MetalDevice {
private:
    MTL::Device* device;
    MTL::CommandQueue* commandQueue;
    
public:
    MetalDevice() {
        device = MTL::CreateSystemDefaultDevice();
        if (!device) {
            throw std::runtime_error("Failed to create Metal device");
        }
        commandQueue = device->newCommandQueue();
        if (!commandQueue) {
            throw std::runtime_error("Failed to create command queue");
        }
    }

    ~MetalDevice() {
        commandQueue->release();
        device->release();
    }

    // Basic test function to verify Metal setup
    void runBasicTest() {
        // Create a simple Metal shader
        const char* shaderSource = R"(
            #include <metal_stdlib>
            using namespace metal;
            
            kernel void add_arrays(device const float* A,
                                 device const float* B,
                                 device float* C,
                                 uint index [[thread_position_in_grid]])
            {
                C[index] = A[index] + B[index];
            }
        )";

        // Create shader library
        NS::Error* error = nullptr;
        MTL::Library* library = device->newLibrary(NS::String::string(shaderSource, 
                                                 NS::StringEncoding::UTF8StringEncoding), 
                                                 nullptr, 
                                                 &error);
        if (!library) {
            throw std::runtime_error("Failed to create shader library");
        }

        // Create compute pipeline
        MTL::Function* addFunction = library->newFunction(NS::String::string("add_arrays", 
                                                        NS::StringEncoding::UTF8StringEncoding));
        MTL::ComputePipelineState* pipelineState = device->newComputePipelineState(addFunction, &error);

        // Test data
        const int arrayLength = 1000;
        std::vector<float> A(arrayLength, 1.0f);
        std::vector<float> B(arrayLength, 2.0f);
        std::vector<float> C(arrayLength, 0.0f);

        // Create buffers
        MTL::Buffer* bufferA = device->newBuffer(A.data(), arrayLength * sizeof(float), MTL::ResourceStorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(B.data(), arrayLength * sizeof(float), MTL::ResourceStorageModeShared);
        MTL::Buffer* bufferC = device->newBuffer(C.data(), arrayLength * sizeof(float), MTL::ResourceStorageModeShared);

        // Create command buffer and encoder
        MTL::CommandBuffer* commandBuffer = commandQueue->commandBuffer();
        MTL::ComputeCommandEncoder* computeEncoder = commandBuffer->computeCommandEncoder();

        // Set pipeline and buffers
        computeEncoder->setComputePipelineState(pipelineState);
        computeEncoder->setBuffer(bufferA, 0, 0);
        computeEncoder->setBuffer(bufferB, 0, 1);
        computeEncoder->setBuffer(bufferC, 0, 2);

        // Dispatch threads
        MTL::Size gridSize = MTL::Size(arrayLength, 1, 1);
        MTL::Size threadgroupSize = MTL::Size(pipelineState->maxTotalThreadsPerThreadgroup(), 1, 1);
        computeEncoder->dispatchThreads(gridSize, threadgroupSize);

        // End encoding and commit
        computeEncoder->endEncoding();
        commandBuffer->commit();
        commandBuffer->waitUntilCompleted();

        // Verify results
        float* result = static_cast<float*>(bufferC->contents());
        for (int i = 0; i < arrayLength; i++) {
            if (result[i] != 3.0f) {
                throw std::runtime_error("Computation error!");
            }
        }

        // Cleanup
        bufferA->release();
        bufferB->release();
        bufferC->release();
        pipelineState->release();
        addFunction->release();
        library->release();
    }
};

int main() {
    try {
        std::cout << "Starting Metal test..." << std::endl;
        MetalDevice device;
        device.runBasicTest();
        std::cout << "Metal test completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Metal test failed: " << e.what() << std::endl;
        return 1;
    }
}
