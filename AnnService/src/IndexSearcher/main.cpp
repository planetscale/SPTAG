// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/Helper/VectorSetReader.h"
#include "inc/Helper/SimpleIniReader.h"
#include "inc/Helper/CommonHelper.h"
#include "inc/Helper/StringConvert.h"
#include "inc/Helper/AsyncFileReader.h"
#include "inc/Core/BKT/Index.h"
#include "inc/Core/Common.h"
#include "inc/Core/Common/CommonUtils.h"
#include "inc/Core/Common/DistanceUtils.h"
#include "inc/Core/Common/TruthSet.h"
#include "inc/Core/Common/QueryResultSet.h"
#include "inc/Core/SPANN/Index.h"
#include "inc/Core/ResultIterator.h"
#include "inc/Core/VectorIndex.h"
#include <algorithm>
#include <assert.h>
#include <iomanip>
#include <set>
#include <atomic>
#include <ctime>
#include <thread>
#include <chrono>

#define CHECK(cmd) do { auto err = cmd; if (err != SPTAG::ErrorCode::Success) { printf("%s: err=%d\n", #cmd, (int)err); exit(1); } } while(0)

int main(int argc, const char **argv) {
	std::string disk_path = argc > 1 ? argv[1] : "/tmp/t1.spann";
	float query_vector[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	const int DefaultBatchSize = 128;

	SPTAG::SetLogger(std::make_shared<SPTAG::Helper::SimpleLogger>(SPTAG::Helper::LogLevel::LL_Debug));

	std::shared_ptr<SPTAG::VectorIndex> base;
	printf("disk_path=\"%s\"\n", disk_path.c_str());
	CHECK(SPTAG::SPANN::Index<float>::LoadIndex(disk_path, base));

	auto index = std::dynamic_pointer_cast<SPTAG::SPANN::Index<float>>(base);
	assert(index);

	int dimension = index->GetFeatureDim();
	printf("ready=%s dim=%d\n", index->IsReady() ? "true" : "false", dimension);
	printf("NumSamples=%d NumDeleted=%d\n", index->GetNumSamples(), index->GetNumDeleted());

	SPTAG::QueryResult result;
	result.Init(query_vector, DefaultBatchSize, true, true);
	CHECK(index->SearchIndex(result, false));
	int count = 0;
	for (auto p=result.begin(); p!=result.end() && p->VID >= 0; ++p) {
		++count;
	}
	printf("%d results:\n", count);
	for (auto p=result.begin(); p!=result.end() && p->VID >= 0; ++p) {
		float vec[dimension];
		uint64_t label;
		memcpy(&label, p->Meta.Data(), sizeof(label));
		assert(sizeof(vec) == p->Sample.Length());
		memcpy(vec, p->Sample.Data(), p->Sample.Length());
		printf("VID=%-2d label=%-2lu dist=%-7.3f [", p->VID, label, sqrt(p->Dist));
		for (int i=0; i<dimension; i++) {
			printf(" %3.1f", vec[i]);
		}
		puts(" ]");
	}

	return 0;
}
