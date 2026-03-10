#pragma once

#include <cmath>
#include <concepts>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/hashing/HashFactory.h"
#include "satp/simulation/detail/framework/DatasetTraversal.h"
#include "satp/simulation/detail/framework/EvaluationContext.h"
#include "satp/simulation/detail/framework/SketchFactory.h"
#include "satp/simulation/detail/merge/HeterogeneousMergeTypes.h"

using namespace std;

namespace satp::evaluation::modes::merge_heterogeneous {
    namespace {
        template<typename Algo, typename Builder>
        concept HeterogeneousSketchBuilder = requires(
            Builder builder,
            const MergeSketchContext &sketchContext,
            const satp::hashing::HashFunction &hashFunction) {
            { builder(sketchContext, hashFunction) } -> same_as<Algo>;
        };

        [[nodiscard]] inline double nanValue() {
            return numeric_limits<double>::quiet_NaN();
        }

        [[nodiscard]] inline double computeAbsoluteError(const double estimate,
                                                         const double exactUnion) {
            if (!isfinite(estimate)) return nanValue();
            return abs(estimate - exactUnion);
        }

        [[nodiscard]] inline double computeRelativeError(const double estimate,
                                                         const double exactUnion) {
            if (!isfinite(estimate) || exactUnion == 0.0) return nanValue();
            return abs(estimate - exactUnion) / exactUnion;
        }

        [[nodiscard]] inline double computeExactUnion(const vector<uint32_t> &partA,
                                                      const vector<uint32_t> &partB) {
            unordered_set<uint32_t> values;
            values.reserve(partA.size() + partB.size());
            values.insert(partA.begin(), partA.end());
            values.insert(partB.begin(), partB.end());
            return static_cast<double>(values.size());
        }

        [[nodiscard]] inline const MergeSketchContext &serialReferenceOf(
            const HeterogeneousMergeRunDescriptor &descriptor) {
            if (descriptor.serialReference.has_value()) return *descriptor.serialReference;
            return descriptor.left;
        }

        [[nodiscard]] inline const optional<MergeSketchContext> &homogeneousBaselineOf(
            const HeterogeneousMergeRunDescriptor &descriptor) {
            return descriptor.homogeneousBaseline;
        }

        [[nodiscard]] inline uint32_t parseSingleUnsignedParam(const string &params,
                                                               const string &key) {
            const string prefix = key + "=";
            const size_t pos = params.find(prefix);
            if (pos == string::npos) {
                throw invalid_argument("Missing merge param: " + key);
            }

            size_t end = params.find(',', pos);
            if (end == string::npos) end = params.size();
            return static_cast<uint32_t>(stoul(params.substr(
                pos + prefix.size(),
                end - (pos + prefix.size()))));
        }

        [[nodiscard]] inline uint32_t reductionTargetKOf(
            const HeterogeneousMergeRunDescriptor &descriptor) {
            if (descriptor.homogeneousBaseline.has_value()) {
                return parseSingleUnsignedParam(descriptor.homogeneousBaseline->params, "k");
            }
            const uint32_t leftK = parseSingleUnsignedParam(descriptor.left.params, "k");
            const uint32_t rightK = parseSingleUnsignedParam(descriptor.right.params, "k");
            return min(leftK, rightK);
        }
    } // namespace

    template<typename Algo, typename Builder>
    vector<HeterogeneousMergePoint> evaluate(const detail::EvaluationContext &context,
                                             const HeterogeneousMergeRunDescriptor &descriptor,
                                             Builder buildAlgo) {
        static_assert(detail::MergeableAlgorithm<Algo>,
                      "merge_heterogeneous::evaluate requires Algo::merge(const Algo&)");
        static_assert(HeterogeneousSketchBuilder<Algo, Builder>,
                      "builder must be callable as Algo(const MergeSketchContext&, const HashFunction&)");

        if (context.metadata.runs < 2u || hasEmptyDataset(context.metadata)) return {};

        const size_t pairCount = context.metadata.runs / 2u;
        const bool hasBaseline = homogeneousBaselineOf(descriptor).has_value();
        const size_t ticksPerPair = context.metadata.sampleSize * (hasBaseline ? 6u : 4u);
        detail::startProgress(context.progress, pairCount * ticksPerPair);

        satp::dataset::PartitionReader reader(context.binaryDataset);
        vector<uint32_t> partA;
        vector<uint32_t> partB;
        vector<HeterogeneousMergePoint> points;
        points.reserve(pairCount);

        for (size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
            const size_t idxA = 2u * pairIndex;
            const size_t idxB = idxA + 1u;
            reader.load(idxA, partA);
            reader.load(idxB, partB);

            const double exactUnion = computeExactUnion(partA, partB);

            auto leftHash = satp::hashing::getHashFunctionBy(descriptor.left.hashName, descriptor.left.hashSeed);
            auto rightHash = satp::hashing::getHashFunctionBy(descriptor.right.hashName, descriptor.right.hashSeed);
            Algo sketchA = buildAlgo(descriptor.left, *leftHash);
            Algo sketchB = buildAlgo(descriptor.right, *rightHash);
            detail::processValues(sketchA, partA, context.progress);
            detail::processValues(sketchB, partB, context.progress);

            double estimateMerge = nanValue();
            switch (descriptor.strategy) {
                case MergeStrategy::Reject:
                    break;
                case MergeStrategy::Direct:
                case MergeStrategy::UnsafeNaiveMerge: {
                    Algo merged = sketchA;
                    if constexpr (is_same_v<Algo, satp::algorithms::HyperLogLogPlusPlus>) {
                        if (descriptor.validity == MergeValidity::Recoverable &&
                            descriptor.strategy == MergeStrategy::UnsafeNaiveMerge) {
                            const uint32_t targetK = reductionTargetKOf(descriptor);
                            merged = sketchA.reducedToNaive(targetK);
                            const Algo reducedB = sketchB.reducedToNaive(targetK);
                            merged.merge(reducedB);
                        } else {
                            merged.merge(sketchB);
                        }
                    } else {
                        merged.merge(sketchB);
                    }
                    estimateMerge = static_cast<double>(merged.count());
                    break;
                }
                case MergeStrategy::ReduceThenMerge: {
                    if constexpr (!is_same_v<Algo, satp::algorithms::HyperLogLogPlusPlus>) {
                        throw logic_error("reduce_then_merge currently supports only HyperLogLogPlusPlus");
                    } else {
                        const uint32_t targetK = reductionTargetKOf(descriptor);
                        Algo merged = sketchA.reducedTo(targetK);
                        const Algo reducedB = sketchB.reducedTo(targetK);
                        merged.merge(reducedB);
                        estimateMerge = static_cast<double>(merged.count());
                    }
                    break;
                }
            }

            const MergeSketchContext &serialContext = serialReferenceOf(descriptor);
            auto serialHash = satp::hashing::getHashFunctionBy(serialContext.hashName, serialContext.hashSeed);
            Algo serial = buildAlgo(serialContext, *serialHash);
            detail::processValues(serial, partA, context.progress);
            detail::processValues(serial, partB, context.progress);
            const double estimateSerial = static_cast<double>(serial.count());

            double baselineHomogeneous = nanValue();
            if (hasBaseline) {
                const MergeSketchContext &baselineContext = *homogeneousBaselineOf(descriptor);
                auto baselineHash = satp::hashing::getHashFunctionBy(
                    baselineContext.hashName,
                    baselineContext.hashSeed);
                Algo baselineA = buildAlgo(baselineContext, *baselineHash);
                Algo baselineB = buildAlgo(baselineContext, *baselineHash);
                detail::processValues(baselineA, partA, context.progress);
                detail::processValues(baselineB, partB, context.progress);
                baselineA.merge(baselineB);
                baselineHomogeneous = static_cast<double>(baselineA.count());
            }

            const double deltaVsBaseline = (isfinite(estimateMerge) && isfinite(baselineHomogeneous))
                                               ? abs(estimateMerge - baselineHomogeneous)
                                               : nanValue();

            points.push_back({
                pairIndex,
                exactUnion,
                estimateMerge,
                estimateSerial,
                computeAbsoluteError(estimateMerge, exactUnion),
                computeRelativeError(estimateMerge, exactUnion),
                computeAbsoluteError(estimateSerial, exactUnion),
                computeRelativeError(estimateSerial, exactUnion),
                baselineHomogeneous,
                deltaVsBaseline
            });
        }

        detail::finishProgress(context.progress);
        return points;
    }
} // namespace satp::evaluation::modes::merge_heterogeneous
