/**********
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <math.h>

#include <xcl.h>

static void* smalloc(size_t size) {
	void* ptr;

	ptr = malloc(size);

	if (ptr == NULL) {
		printf("Error: Cannot allocate memory\n");
		exit(EXIT_FAILURE);
	}

	return ptr;
}

static int load_file_to_memory(const char *filename, char **result) {
	unsigned int size;

	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		*result = NULL;
		printf("Error: Could not read file %s\n", filename);
		exit(EXIT_FAILURE);
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	*result = (char *) smalloc(sizeof(char)*(size+1));

	if (size != fread(*result, sizeof(char), size, f)) {
		free(*result);
		printf("Error: read of kernel failed\n");
		exit(EXIT_FAILURE);
	}

	fclose(f);
	(*result)[size] = 0;

	return size;
}

xcl_world xcl_world_single() {
	int err;
	xcl_world world;
	cl_uint num_platforms;

	char *xcl_mode = getenv("XCL_EMULATION_MODE");

	int mode_len;
	if(xcl_mode == NULL) {
		mode_len = strlen("hw");
		world.mode = (char*) malloc(sizeof(char)*mode_len);
		strcpy(world.mode, "hw");
	} else {
		mode_len = strlen(xcl_mode);
		world.mode = (char*) malloc(sizeof(char)*mode_len);
	
		strcpy(world.mode, xcl_mode);

		err = setenv("XCL_EMULATION_MODE", "true", 1);
		if(err != 0) {
			printf("Error: cannot set XCL_EMULATION_MODE\n");
			exit(EXIT_FAILURE);
		}
	}

	err = clGetPlatformIDs(0, NULL, &num_platforms);
	if (err != CL_SUCCESS) {
		printf("Error: no platforms available or OpenCL install broken");
		exit(EXIT_FAILURE);
	}

	cl_platform_id *platform_ids = (cl_platform_id *) malloc(sizeof(cl_platform_id) * num_platforms);

	if (platform_ids == NULL) {
		printf("Error: Out of Memory\n");
		exit(EXIT_FAILURE);
	}

	err = clGetPlatformIDs(num_platforms, platform_ids, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to find an OpenCL platform!\n");
		exit(EXIT_FAILURE);
	}

	int i;
	for(i = 0; i < num_platforms; i++) {
		size_t platform_name_size;
		err = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_NAME, 
		                        0, NULL, &platform_name_size);
		if( err != CL_SUCCESS) {
			printf("Error: Could not determine platform name!\n");
			exit(EXIT_FAILURE);
		}

		char *platform_name = (char*) malloc(sizeof(char)*platform_name_size);
		if(platform_name == NULL) {
			printf("Error: out of memory!\n");
			exit(EXIT_FAILURE);
		}

		err = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_NAME,
		                        platform_name_size, platform_name, NULL);
		if(err != CL_SUCCESS) {
			printf("Error: could not determine platform name!\n");
			exit(EXIT_FAILURE);
		}

		if (!strcmp(platform_name, "Xilinx")) {
			free(platform_name);
			world.platform_id = platform_ids[i];
			break;
		}

		free(platform_name);
	}

	free(platform_ids);

	if (i == num_platforms) {
		printf("Error: Failed to find Xilinx platform\n");
		exit(EXIT_FAILURE);
	}

	err = clGetDeviceIDs(world.platform_id, CL_DEVICE_TYPE_ALL,
	                     1, &world.device_id, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: could not get device ids\n");
		exit(EXIT_FAILURE);
	}

	size_t device_name_size;
	err = clGetDeviceInfo(world.device_id, CL_DEVICE_NAME,
	                      0, NULL, &device_name_size);
	if(err != CL_SUCCESS) {
		printf("Error: could not determine device name\n");
		exit(EXIT_FAILURE);
	}

	world.device_name = (char*) malloc(sizeof(char)*device_name_size);

	if(world.device_name == NULL) {
		printf("Error: Out of Memory!\n");
		exit(EXIT_FAILURE);
	}

	err = clGetDeviceInfo(world.device_id, CL_DEVICE_NAME,
	                      device_name_size, world.device_name, NULL);
	if(err != CL_SUCCESS) {
		printf("Error: could not determine device name\n");
		exit(EXIT_FAILURE);
	}

	world.context = clCreateContext(0, 1, &world.device_id,
	                                NULL, NULL, &err);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to create a compute context!\n");
		exit(EXIT_FAILURE);
	}

	world.command_queue = clCreateCommandQueue(world.context,
	                                           world.device_id,
	                                           CL_QUEUE_PROFILING_ENABLE,
	                                           &err);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to create a command queue!\n");
		exit(EXIT_FAILURE);
	}

	return world;
}

void xcl_release_world(xcl_world world) {
	clReleaseCommandQueue(world.command_queue);
	clReleaseContext(world.context);
	free(world.device_name);
	free(world.mode);
}

cl_program xcl_import_binary(xcl_world world,
                            const char *xclbin_name
) {
	int err;

	size_t xclbin_len = strlen(xclbin_name);
	size_t mode_len = strlen(world.mode);

	size_t device_len = strlen(world.device_name);
	char *device_name = (char*) malloc(sizeof(char)*device_len+1);
	if(device_name == NULL) {
		printf("Error: Out of Memory\n");
		exit(EXIT_FAILURE);
	}

	for(size_t i = 0; i < device_len; i++) {
		char tmp = world.device_name[i];
		if(tmp == ':' || tmp == '.') {
			tmp = '_';
		}
		device_name[i] = tmp;
	}

	device_name[device_len] = '\0';

	size_t xclbin_file_name_len = xclbin_len + mode_len + device_len + 17;
	char *xclbin_file_name = (char*) malloc(sizeof(char)*(xclbin_len+mode_len+device_len+18));
	if(xclbin_file_name == NULL) {
		printf("Error: Out of Memory!\n");
		exit(EXIT_FAILURE);
	}

	err = sprintf(xclbin_file_name, "xclbin/%s.%s.%s.xclbin", xclbin_name, world.mode, device_name);
	if(err != xclbin_file_name_len-1) {
		printf("Error: failed to create xclbin file name\n");
		exit(EXIT_FAILURE);
	}

	free(device_name);

	if(access(xclbin_file_name, R_OK) != 0) {
		printf("ERROR: %s kernel not available please build\n", xclbin_file_name);
		exit(EXIT_FAILURE);
	}

	char *krnl_bin;
	const size_t krnl_size = load_file_to_memory(xclbin_file_name, &krnl_bin);

	cl_program program = clCreateProgramWithBinary(world.context, 1,
	                                    &world.device_id, &krnl_size,
	                                    (const unsigned char**) &krnl_bin,
	                                    NULL, &err);
	if ((!program) || (err!=CL_SUCCESS)) {
		printf("Error: Failed to create compute program from binary %d!\n",
		       err);
		printf("Test failed\n");
		exit(EXIT_FAILURE);
	}

	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		size_t len;
		char buffer[2048];

		clGetProgramBuildInfo(program, world.device_id, CL_PROGRAM_BUILD_LOG,
		                      sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		printf("Error: Failed to build program executable!\n");
		exit(EXIT_FAILURE);
	}

	free(krnl_bin);

	return program;
}

cl_program xcl_import_source(xcl_world world,
                            const char *krnl_file
) {
	int err;

	char *krnl_bin;
	load_file_to_memory(krnl_file, &krnl_bin);

	cl_program program = clCreateProgramWithSource(world.context, 1,
	                                               (const char**) &krnl_bin,
	                                               0, &err);
	if ((err!=CL_SUCCESS) || (!program))  {
		printf("Error: Failed to create compute program from binary %d!\n",
		       err);
		printf("Test failed\n");
		exit(EXIT_FAILURE);
	}

	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		size_t len;
		char buffer[2048];

		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, world.device_id, CL_PROGRAM_BUILD_LOG,
		                      sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		printf("Test failed\n");
		exit(EXIT_FAILURE);
	}

	free(krnl_bin);

	return program;
}

cl_kernel xcl_get_kernel(cl_program program,
                         const char *krnl_name
) {
	int err;

	cl_kernel kernel = clCreateKernel(program, krnl_name, &err);
	if (!kernel || err != CL_SUCCESS) {
		printf("Error: Failed to create kernel for %s: %d\n", krnl_name, err);
		exit(EXIT_FAILURE);
	}

	return kernel;
}

void xcl_set_kernel_arg(cl_kernel krnl,
                        cl_uint num,
                        size_t size,
                        const void *ptr
) {
	int err = clSetKernelArg(krnl, num, size, ptr);

	if(err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arg\n");
		exit(EXIT_FAILURE);
	}
}

cl_mem xcl_malloc(xcl_world world, cl_mem_flags flags, size_t size) {
	cl_mem mem = clCreateBuffer(world.context, flags, size, NULL, NULL);

	if (!mem) {
		printf("Error: Failed to allocate device memory!\n");
		exit(EXIT_FAILURE);
	}

	return mem;
}

void xcl_memcpy_to_device(xcl_world world, cl_mem dest, void* src,
                          size_t size) {
	int err = clEnqueueWriteBuffer(world.command_queue, dest, CL_TRUE, 0, size,
	                               src, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to write to source array a!\n");
		exit(EXIT_FAILURE);
	}
}

void xcl_memcpy_from_device(xcl_world world, void* dest,
                            cl_mem src, size_t size
) {
	int err = clEnqueueReadBuffer(world.command_queue, src, CL_TRUE, 0, size,
	                              dest, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to read output array! %d\n", err);
		exit(EXIT_FAILURE);
	}
}

unsigned long xcl_get_event_duration(cl_event event) {
	unsigned long start, stop;

	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
	                        sizeof(unsigned long), &start, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
	                        sizeof(unsigned long), &stop, NULL);

	return stop - start;
}

unsigned long xcl_run_kernel3d(xcl_world world, cl_kernel krnl,
                               size_t x, size_t y, size_t z
) {
	size_t size[3] = {x, y, z};
	cl_event event;
	unsigned long start, stop;

	int err = clEnqueueNDRangeKernel(world.command_queue, krnl, 3,
	                                 NULL, size, size, 0, NULL, &event);
	if( err != CL_SUCCESS) {
		printf("Error: failed to execute kernel! %d\n", err);
		exit(EXIT_FAILURE);
	}

	clFinish(world.command_queue);

	return xcl_get_event_duration(event);
}
