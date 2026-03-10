#include "satp/cli/detail/execution/JobFactory.h"

#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/algorithms/LogLog.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/cli/detail/execution/RunReporter.h"

using namespace std;

namespace satp::cli::executor {
    namespace {
        namespace alg = satp::algorithms;

        [[nodiscard]] string resolveHashFunctionName(const optional<string> &configured,
                                                     const string &fallback) {
            return configured.value_or(fallback);
        }

        [[nodiscard]] uint32_t resolveHashSeed(const optional<uint32_t> &configured,
                                               const uint32_t datasetSeed) {
            return configured.value_or(datasetSeed);
        }

        [[nodiscard]] uint32_t resolveK(const optional<uint32_t> &configured,
                                        const uint32_t fallback) {
            return configured.value_or(fallback);
        }

        [[nodiscard]] string hllppParams(const uint32_t k) {
            return "k=" + to_string(k);
        }

        [[nodiscard]] satp::evaluation::MergeValidity classifyHllppCompatibility(
            const satp::evaluation::MergeSketchContext &left,
            const satp::evaluation::MergeSketchContext &right) {
            const bool sameHashSemantics = left.hashName == right.hashName &&
                                           left.hashSeed == right.hashSeed;
            const bool sameParams = left.params == right.params;
            if (sameHashSemantics && sameParams) {
                return satp::evaluation::MergeValidity::Valid;
            }
            if (sameHashSemantics) {
                return satp::evaluation::MergeValidity::Recoverable;
            }
            return satp::evaluation::MergeValidity::Invalid;
        }

        void validateHllppStrategy(const satp::evaluation::MergeValidity validity,
                                   const satp::evaluation::MergeStrategy strategy) {
            using satp::evaluation::MergeStrategy;
            using satp::evaluation::MergeValidity;

            if (strategy == MergeStrategy::Direct && validity != MergeValidity::Valid) {
                throw invalid_argument("HLL++ direct merge richiede un caso valid");
            }
            if (strategy == MergeStrategy::ReduceThenMerge && validity != MergeValidity::Recoverable) {
                throw invalid_argument("HLL++ reduce_then_merge richiede un caso recoverable");
            }
            if (strategy == MergeStrategy::UnsafeNaiveMerge && validity == MergeValidity::Valid) {
                throw invalid_argument("HLL++ unsafe_naive_merge richiede un caso non-valid");
            }
        }

        [[nodiscard]] satp::evaluation::HeterogeneousMergeRunDescriptor makeHllppMergeDescriptor(
            satp::evaluation::EvaluationFramework &bench,
            const DatasetRuntimeContext &ctx,
            const RunConfig &cfg) {
            const string leftHashName = resolveHashFunctionName(cfg.leftHashFunctionName, cfg.hashFunctionName);
            const string rightHashName = resolveHashFunctionName(cfg.rightHashFunctionName, cfg.hashFunctionName);
            const uint32_t leftHashSeed = resolveHashSeed(cfg.leftHashSeed, ctx.seed);
            const uint32_t rightHashSeed = resolveHashSeed(cfg.rightHashSeed, ctx.seed);
            const uint32_t leftK = resolveK(cfg.leftK, cfg.k);
            const uint32_t rightK = resolveK(cfg.rightK, cfg.k);

            satp::evaluation::MergeSketchContext left{
                leftHashName,
                leftHashSeed,
                hllppParams(leftK)
            };
            satp::evaluation::MergeSketchContext right{
                rightHashName,
                rightHashSeed,
                hllppParams(rightK)
            };

            const auto validity = classifyHllppCompatibility(left, right);
            validateHllppStrategy(validity, cfg.mergeStrategy);

            const satp::evaluation::MergeSketchContext baseline{
                left.hashName,
                left.hashSeed,
                hllppParams(min(leftK, rightK))
            };
            const optional<satp::evaluation::MergeSketchContext> serialReference =
                (validity == satp::evaluation::MergeValidity::Recoverable)
                    ? optional<satp::evaluation::MergeSketchContext>{baseline}
                    : optional<satp::evaluation::MergeSketchContext>{left};

            return {
                satp::algorithms::catalog::getNameBy("hllpp"),
                left,
                right,
                cfg.mergeStrategy,
                validity,
                satp::evaluation::MergeTopology::Pairwise,
                bench.metadata(),
                serialReference,
                baseline
            };
        }

        [[nodiscard]] string heterogeneousHashLabel(const satp::evaluation::HeterogeneousMergeRunDescriptor &descriptor) {
            return "left=" + descriptor.left.hashName + "_seed=" + to_string(descriptor.left.hashSeed) +
                   "__right=" + descriptor.right.hashName + "_seed=" + to_string(descriptor.right.hashSeed);
        }

        [[nodiscard]] string heterogeneousParamsLabel(const satp::evaluation::HeterogeneousMergeRunDescriptor &descriptor) {
            return "left_" + descriptor.left.params +
                   "__right_" + descriptor.right.params +
                   "__strategy_" + string(satp::evaluation::toString(descriptor.strategy));
        }

        [[nodiscard]] alg::HyperLogLogPlusPlus buildHllppFromContext(
            const satp::evaluation::MergeSketchContext &sketchContext,
            const satp::hashing::HashFunction &hashFunction) {
            const string prefix = "k=";
            const size_t pos = sketchContext.params.find(prefix);
            if (pos == string::npos) {
                throw invalid_argument("HLL++ merge context richiede params nel formato k=<p>");
            }
            const uint32_t k = static_cast<uint32_t>(stoul(sketchContext.params.substr(pos + prefix.size())));
            return alg::HyperLogLogPlusPlus(k, hashFunction);
        }
    } // namespace

    vector<AlgorithmJob> buildAlgorithmJobs(
        satp::evaluation::EvaluationFramework &bench,
        const DatasetRuntimeContext &ctx,
        const RunConfig &cfg,
        const RunMode mode,
        const string &hashName) {
        const string kParam = "k=" + to_string(cfg.k);
        const string kAndLLogParam = kParam + ",L=" + to_string(cfg.lLog);
        const string lParam = "L=" + to_string(cfg.l);

        vector<AlgorithmJob> jobs;
        jobs.reserve(4);

        addAlgorithmJob<alg::HyperLogLogPlusPlus>(
            jobs,
            bench,
            ctx,
            mode,
            "hllpp",
            kParam,
            hashName,
            rseHll(cfg.k),
            cfg.k);

        addAlgorithmJob<alg::HyperLogLog>(
            jobs,
            bench,
            ctx,
            mode,
            "hll",
            kAndLLogParam,
            hashName,
            rseHll(cfg.k),
            cfg.k,
            cfg.lLog);

        addAlgorithmJob<alg::LogLog>(
            jobs,
            bench,
            ctx,
            mode,
            "ll",
            kAndLLogParam,
            hashName,
            rseLogLog(cfg.k),
            cfg.k,
            cfg.lLog);

        addAlgorithmJob<alg::ProbabilisticCounting>(
            jobs,
            bench,
            ctx,
            mode,
            "pc",
            lParam,
            hashName,
            rseUnknown(),
            cfg.l);

        return jobs;
    }

    vector<AlgorithmJob> buildHeterogeneousMergeJobs(
        satp::evaluation::EvaluationFramework &bench,
        const DatasetRuntimeContext &ctx,
        const RunConfig &cfg) {
        const auto descriptor = makeHllppMergeDescriptor(bench, ctx, cfg);

        vector<AlgorithmJob> jobs;
        jobs.reserve(1);
        addHeterogeneousAlgorithmJob<alg::HyperLogLogPlusPlus>(
            jobs,
            bench,
            ctx,
            "hllpp",
            heterogeneousParamsLabel(descriptor),
            heterogeneousHashLabel(descriptor),
            descriptor,
            buildHllppFromContext);
        return jobs;
    }
} // namespace satp::cli::executor
