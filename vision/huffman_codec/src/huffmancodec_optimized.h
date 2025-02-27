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
#ifndef HUFFMANAPP_H_
#define HUFFMANAPP_H_

#include <string>
#include <vector>
#include "bit_io.h"
#include "xcl.h"
#include "huffmancodec_naive.h"

#define COMPUTE_UNITS 1

using namespace std;

namespace sda {
namespace cl {

/*!
 *
 */
class HuffmanOptimized : public ICodec {
public:
	HuffmanOptimized();
	HuffmanOptimized(const string& vendor_name,
		   const string& device_name,
		   int selected_device,
		   const string& strKernelFP,
		   const string& strBitmapFP);
	virtual ~HuffmanOptimized();

	enum EvBreakDown {evtHostWrite = 0, evtKernelExec = 1, evtHostRead = 2, evtCount = 3};

	//overide interface funcs
	int enc(const vector<u8>& in_data, vector<u8>& out_data);
	int dec(const vector<u8>& in_data, vector<u8>& out_data);

	bool run(int idevice, int nruns);
	bool invoke_kernel(cl_kernel krnl, const vector<u8>& vec_input, vector<u8>& vec_output, cl_event events[evtCount]);


	static double timestamp();
	static double computeEventDurationInMS(const cl_event& event);


protected:
	void cleanup();
	bool releaseMemObject(cl_mem &obj);

private:
	string m_strBitmapFP;

	xcl_world m_world;
	cl_kernel m_clKernelHuffmanEncoder;
	cl_kernel m_clKernelHuffmanDecoder;
};

}
}

#endif /* AESAPP_H_ */
