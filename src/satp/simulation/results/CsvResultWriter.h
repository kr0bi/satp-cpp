#pragma once

#include <filesystem>
#include <vector>

#include "satp/simulation/metrics/Stats.h"
#include "satp/simulation/results/csv/CsvField.h"
#include "satp/simulation/results/csv/CsvFile.h"
#include "satp/simulation/results/csv/CsvRunDescriptor.h"

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
    };
} // namespace satp::evaluation
