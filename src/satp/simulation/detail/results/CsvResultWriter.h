#pragma once

#include <filesystem>
#include <vector>

#include "satp/simulation/detail/merge/HeterogeneousMergeTypes.h"
#include "satp/simulation/detail/metrics/Statistics.h"
#include "satp/simulation/detail/results/csv/CsvField.h"
#include "satp/simulation/detail/results/csv/CsvFile.h"
#include "satp/simulation/detail/results/csv/CsvRunDescriptor.h"
#include "satp/simulation/detail/results/csv/HeterogeneousMergeCsvDescriptor.h"

using namespace std;

namespace satp::evaluation {
    class CsvResultWriter {
    public:
        static constexpr const char *STREAMING_HEADER =
            "algorithm,params,mode,runs,sample_size,number_of_elements_processed,f0,seed,"
            "f0_mean_t,f0_hat_mean_t,"
            "variance,stddev,rse_theoretical,rse_observed,bias,absolute_bias,relative_bias,"
            "mean_relative_error,rmse,mae";
        static constexpr const char *MERGE_HEADER =
            "algorithm,params,mode,pairs,sample_size,pair_index,seed,"
            "estimate_merge,estimate_serial,delta_merge_serial_abs,delta_merge_serial_rel";
        static constexpr const char *HETEROGENEOUS_MERGE_HEADER =
            "algorithm,mode,pairs,sample_size,pair_index,dataset_seed,"
            "left_hash,right_hash,left_hash_seed,right_hash_seed,left_params,right_params,"
            "strategy,validity,topology,"
            "exact_union,estimate_merge,estimate_serial,"
            "error_merge_abs_exact,error_merge_rel_exact,"
            "error_serial_abs_exact,error_serial_rel_exact,"
            "baseline_homogeneous,delta_vs_baseline";

        static void appendStreaming(const filesystem::path &csvPath,
                                    const CsvRunDescriptor &descriptor,
                                    const vector<StreamingPointStats> &series) {
            ofstream out = csv::openAppend(csvPath, STREAMING_HEADER, "Impossibile aprire il file CSV");
            for (const auto &point : series) {
                writeSummaryRecord(out, descriptor, "streaming", point.number_of_elements_processed,
                                   point.truth_mean, point);
            }
        }

        static void appendMergePairs(const filesystem::path &csvPath,
                                     const CsvRunDescriptor &descriptor,
                                     const vector<MergePairPoint> &points) {
            ofstream out = csv::openAppend(csvPath, MERGE_HEADER, "Impossibile aprire il file CSV merge");
            const size_t pairCount = points.size();
            for (const auto &point : points) {
                writeMergeRecord(out, descriptor, pairCount, point);
            }
        }

        static void appendHeterogeneousMergePairs(const filesystem::path &csvPath,
                                                  const HeterogeneousMergeCsvDescriptor &descriptor,
                                                  const vector<HeterogeneousMergePoint> &points) {
            ofstream out = csv::openAppend(
                csvPath,
                HETEROGENEOUS_MERGE_HEADER,
                "Impossibile aprire il file CSV merge eterogeneo");
            const size_t pairCount = points.size();
            for (const auto &point : points) {
                writeHeterogeneousMergeRecord(out, descriptor, pairCount, point);
            }
        }

    private:
        template<typename Point>
        static void writeSummaryRecord(ofstream &out,
                                       const CsvRunDescriptor &descriptor,
                                       const char *mode,
                                       const size_t elementIndex,
                                       const double truthMean,
                                       const Point &point) {
            out << csv::escapeCsvField(descriptor.algorithmName) << ','
                << csv::escapeCsvField(descriptor.algorithmParams) << ','
                << mode << ','
                << descriptor.metadata.runs << ','
                << descriptor.metadata.sampleSize << ','
                << elementIndex << ','
                << descriptor.metadata.distinctCount << ','
                << descriptor.metadata.seed << ','
                << truthMean << ','
                << point.mean << ','
                << point.variance << ','
                << point.stddev << ','
                << descriptor.rseTheoretical << ','
                << point.rse_observed << ','
                << point.bias << ','
                << point.absolute_bias << ','
                << point.relative_bias << ','
                << point.mean_relative_error << ','
                << point.rmse << ','
                << point.mae << '\n';
        }

        static void writeMergeRecord(ofstream &out,
                                     const CsvRunDescriptor &descriptor,
                                     const size_t pairCount,
                                     const MergePairPoint &point) {
            out << csv::escapeCsvField(descriptor.algorithmName) << ','
                << csv::escapeCsvField(descriptor.algorithmParams) << ','
                << "merge,"
                << pairCount << ','
                << descriptor.metadata.sampleSize << ','
                << point.pair_index << ','
                << descriptor.metadata.seed << ','
                << point.estimate_merge << ','
                << point.estimate_serial << ','
                << point.delta_merge_serial_abs << ','
                << point.delta_merge_serial_rel << '\n';
        }

        static void writeHeterogeneousMergeRecord(ofstream &out,
                                                  const HeterogeneousMergeCsvDescriptor &descriptor,
                                                  const size_t pairCount,
                                                  const HeterogeneousMergePoint &point) {
            out << csv::escapeCsvField(descriptor.algorithmName) << ','
                << "merge_heterogeneous,"
                << pairCount << ','
                << descriptor.metadata.sampleSize << ','
                << point.pair_index << ','
                << descriptor.metadata.seed << ','
                << csv::escapeCsvField(descriptor.left.hashName) << ','
                << csv::escapeCsvField(descriptor.right.hashName) << ','
                << descriptor.left.hashSeed << ','
                << descriptor.right.hashSeed << ','
                << csv::escapeCsvField(descriptor.left.params) << ','
                << csv::escapeCsvField(descriptor.right.params) << ','
                << toString(descriptor.strategy) << ','
                << toString(descriptor.validity) << ','
                << toString(descriptor.topology) << ','
                << point.exact_union << ','
                << point.estimate_merge << ','
                << point.estimate_serial << ','
                << point.error_merge_abs_exact << ','
                << point.error_merge_rel_exact << ','
                << point.error_serial_abs_exact << ','
                << point.error_serial_rel_exact << ','
                << point.baseline_homogeneous << ','
                << point.delta_vs_baseline << '\n';
        }
    };
} // namespace satp::evaluation
