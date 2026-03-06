#include "satp/cli/detail/config/DatasetRuntime.h"

#include <exception>

#include "satp/cli/detail/paths/ResultPaths.h"
#include "satp/dataset/Dataset.h"

using namespace std;

namespace satp::cli::config {
    optional<DatasetView> readDatasetView(const string &datasetPath) {
        try {
            const auto index = satp::dataset::indexBinaryDataset(datasetPath);
            return DatasetView{
                index.info.elements_per_partition,
                index.info.partition_count,
                index.info.seed
            };
        } catch (const exception &) {
            return nullopt;
        }
    }

    DatasetRuntimeContext loadDatasetRuntimeContext(const RunConfig &cfg) {
        DatasetRuntimeContext ctx;
        ctx.index = dataset::indexBinaryDataset(cfg.datasetPath);
        ctx.sampleSize = ctx.index.info.elements_per_partition;
        ctx.runs = ctx.index.info.partition_count;
        ctx.seed = ctx.index.info.seed;
        ctx.resultsNamespace = cfg.resultsNamespace;
        ctx.repoRoot = path_utils::detectRepoRoot(cfg.datasetPath);
        return ctx;
    }
} // namespace satp::cli::config
