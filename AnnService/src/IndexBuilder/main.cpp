// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/Helper/VectorSetReader.h"
#include "inc/Core/VectorIndex.h"
#include "inc/Core/Common.h"
#include "inc/Helper/SimpleIniReader.h"
#include "inc/Core/BKT/Index.h"
#include "inc/Core/Common/DistanceUtils.h"
#include "inc/Core/SPANN/Index.h"
#include "inc/Core/ResultIterator.h"

#include <assert.h>
#include <math.h>
#include <memory>
#include <sys/time.h>

// error codes: AnnService/inc/Core/DefinitionList.h
#define CHECK(cmd) do { auto err = cmd; if (err != SPTAG::ErrorCode::Success) { printf("%s: err=%d\n", #cmd, (int)err); exit(1); } } while(0)

int main(int argc, char **argv) {
	std::string disk_path = "/tmp/t1.spann";
	std::string distance_func = "L2";
	float vectors[] = {
		5.6, 8.2, 2.5, 8.1, 1.6,
		9.3, 5.8, 7.7, 7.4, 8.8,
		9.7, 5.3, 0.5, 6.3, 6.9,
		2.0, 9.6, 6.6, 7.0, 9.7,
		4.1, 7.6, 5.1, 4.3, 3.8,
		4.3, 9.9, 0.6, 5.4, 5.4,
		6.7, 2.0, 7.6, 6.6, 3.9,
		7.7, 8.3, 9.5, 1.8, 6.5,
		4.9, 1.2, 0.8, 3.4, 4.6,
		9.9, 2.2, 8.2, 1.9, 8.8,
		2.4, 2.9, 4.0, 6.1, 3.5,
		7.2, 0.6, 2.4, 6.6, 2.4,
		5.5, 6.9, 6.6, 2.7, 1.8,
		0.2, 4.3, 7.3, 6.2, 4.0,
		7.2, 8.7, 7.8, 4.9, 5.5,
		0.9, 0.7, 2.4, 5.1, 8.4,
		9.4, 7.6, 6.9, 5.9, 0.4,
		7.6, 2.6, 5.4, 8.0, 4.5,
		7.1, 0.3, 0.3, 6.7, 5.9,
		4.3, 6.5, 6.3, 5.2, 3.3,
	};
	const int dimension = 5;
	const size_t nvectors = sizeof(vectors)/sizeof(vectors[0])/dimension;

	SPTAG::SetLogger(std::make_shared<SPTAG::Helper::SimpleLogger>(SPTAG::Helper::LogLevel::LL_Debug));

	// configure the index
	auto index = new SPTAG::SPANN::Index<float>();
	CHECK(index->SetParameter("DistCalcMethod", distance_func, "Base"));
	auto options = index->GetOptions();
	options->m_valueType = SPTAG::VectorValueType::Float;
	options->m_useKV = true;
	options->m_enableSSD = true;
	options->m_indexAlgoType = SPTAG::IndexAlgoType::BKT;
	options->m_selectHead = true;
	options->m_buildHead = true;
	options->m_buildSsdIndex = true;
	options->m_update = true;
	options->m_indexDirectory = disk_path;
	options->m_KVPath = disk_path + "/rocks";
	options->m_deleteIDFile = disk_path + "/DeletedIDs.bin";
	options->m_ssdInfoFile = disk_path + "/SSDInfo.bin";

	// set up metadata
	auto metadata = new SPTAG::MemMetadataSet(index->m_iDataBlockSize, index->m_iDataCapacity, sizeof(uint64_t));
	index->SetMetadata(metadata);

	// add the metadata
	for (uint64_t label=1; label<=nvectors; label++) {
		metadata->Add(SPTAG::ByteArray((uint8_t *)&label, sizeof(label), false));
	}

	// first build
	printf("ready=%s\n", index->IsReady() ? "true" : "false");
	printf("BuildIndex: count=%d dimension=%d\n", metadata->Count(), dimension);
	CHECK(index->BuildIndex(vectors, metadata->Count(), dimension, false, true));
	printf("ready=%s dim=%d\n", index->IsReady() ? "true" : "false", index->GetOptions()->m_dim);
	printf("NumSamples=%d NumDeleted=%d\n", index->GetNumSamples(), index->GetNumDeleted());

	// appending if m_spann->IsReady()
	//CHECK(index->AddIndex(vectors, metadata->Count(), dimension, std::shared_ptr<SPTAG::MemMetadataSet>(metadata)));

	CHECK(index->SaveIndex(index->GetParameter("IndexDirectory", "Base")));



	// query the in-memory database

	float query_vector[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	const int DefaultBatchSize = 128;

	printf("QUERY!\n");
	printf("ready=%s dim=%d\n", index->IsReady() ? "true" : "false", index->GetFeatureDim());
	printf("NumSamples=%d NumDeleted=%d\n", index->GetNumSamples(), index->GetNumDeleted());

	for (int repeat=0; repeat<100; repeat++) {
		SPTAG::QueryResult result;
		result.Init(query_vector, DefaultBatchSize, true, true);
		CHECK(index->SearchIndex(result, false));
		int count = 0;
		for (auto p=result.begin(); p!=result.end(); ++p) {
			if (p->VID >= 0) ++count;
		}
		struct timeval tv;
		gettimeofday(&tv, NULL);
		printf("%ld.%06ld: %d results\n", tv.tv_sec, tv.tv_usec, count);
#if 0
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
#endif
	}

	return 0;
}
