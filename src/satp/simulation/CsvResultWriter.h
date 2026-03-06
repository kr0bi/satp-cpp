#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

#include "satp/simulation/Stats.h"

using namespace std;

namespace satp::evaluation {
    class CsvResultWriter {
    public:
        static void appendNormal(const filesystem::path &csvPath,
                                 const string &algorithmName,
                                 const string &algorithmParams,
                                 size_t runs,
                                 size_t sampleSize,
                                 size_t f0,
                                 uint32_t seed,
                                 double rseTheoretical,
                                 const Stats &stats) {
            ofstream out = openAppend(csvPath);
            writeRecord(out, algorithmName, algorithmParams, "normal", runs, sampleSize,
                        sampleSize, f0, seed, stats.truth_mean, stats.mean,
                        stats.variance, stats.stddev, rseTheoretical, stats.rse_observed,
                        stats.bias, stats.absolute_bias, stats.relative_bias,
                        stats.mean_relative_error, stats.rmse, stats.mae);
        }

        static void appendStreaming(const filesystem::path &csvPath,
                                    const string &algorithmName,
                                    const string &algorithmParams,
                                    size_t runs,
                                    size_t sampleSize,
                                    size_t f0,
                                    uint32_t seed,
                                    double rseTheoretical,
                                    const vector<StreamingPointStats> &series) {
            ofstream out = openAppend(csvPath);
            for (const auto &point : series) {
                writeRecord(out, algorithmName, algorithmParams, "streaming", runs, sampleSize,
                            point.number_of_elements_processed, f0, seed, point.truth_mean,
                            point.mean, point.variance, point.stddev, rseTheoretical,
                            point.rse_observed, point.bias, point.absolute_bias,
                            point.relative_bias, point.mean_relative_error,
                            point.rmse, point.mae);
            }
        }

        static void appendMergePairs(const filesystem::path &csvPath,
                                     const string &algorithmName,
                                     const string &algorithmParams,
                                     size_t pairs,
                                     size_t sampleSize,
                                     uint32_t seed,
                                     const vector<MergePairPoint> &points) {
            ofstream out = openAppendMerge(csvPath);
            for (const auto &point : points) {
                out << escapeCsvField(algorithmName) << ','
                    << escapeCsvField(algorithmParams) << ','
                    << "merge,"
                    << pairs << ','
                    << sampleSize << ','
                    << point.pair_index << ','
                    << seed << ','
                    << point.estimate_merge << ','
                    << point.estimate_serial << ','
                    << point.delta_merge_serial_abs << ','
                    << point.delta_merge_serial_rel << '\n';
            }
        }

    private:
        static string escapeCsvField(const string &value) {
            const bool needsQuotes = value.find_first_of(",\"\n\r") != string::npos;
            if (!needsQuotes) return value;

            string out;
            out.reserve(value.size() + 2);
            out.push_back('"');
            for (const char c : value) {
                if (c == '"') {
                    out.push_back('"');
                    out.push_back('"');
                } else {
                    out.push_back(c);
                }
            }
            out.push_back('"');
            return out;
        }

        static void writeHeaderIfNeeded(const filesystem::path &csvPath,
                                        ofstream &out) {
            const bool writeHeader =
                !filesystem::exists(csvPath) || filesystem::file_size(csvPath) == 0;
            if (!writeHeader) return;

            out << "algorithm,params,mode,runs,sample_size,number_of_elements_processed,f0,seed,"
                   "f0_mean_t,f0_hat_mean_t,"
                   "variance,stddev,rse_theoretical,rse_observed,bias,absolute_bias,relative_bias,"
                   "mean_relative_error,rmse,mae\n";
        }

        static ofstream openAppend(const filesystem::path &csvPath) {
            ofstream out(csvPath, ios::app);
            if (!out) throw runtime_error("Impossibile aprire il file CSV");
            out << setprecision(10);
            writeHeaderIfNeeded(csvPath, out);
            return out;
        }

        static void writeMergeHeaderIfNeeded(const filesystem::path &csvPath,
                                             ofstream &out) {
            const bool writeHeader =
                !filesystem::exists(csvPath) || filesystem::file_size(csvPath) == 0;
            if (!writeHeader) return;

            out << "algorithm,params,mode,pairs,sample_size,pair_index,seed,"
                   "estimate_merge,estimate_serial,delta_merge_serial_abs,delta_merge_serial_rel\n";
        }

        static ofstream openAppendMerge(const filesystem::path &csvPath) {
            ofstream out(csvPath, ios::app);
            if (!out) throw runtime_error("Impossibile aprire il file CSV merge");
            out << setprecision(10);
            writeMergeHeaderIfNeeded(csvPath, out);
            return out;
        }

        static void writeRecord(ofstream &out,
                                const string &algorithmName,
                                const string &algorithmParams,
                                const char *mode,
                                size_t runs,
                                size_t sampleSize,
                                size_t elementIndex,
                                size_t f0,
                                uint32_t seed,
                                double truthMean,
                                double estimateMean,
                                double variance,
                                double stddev,
                                double rseTheoretical,
                                double rseObserved,
                                double bias,
                                double absoluteBias,
                                double relativeBias,
                                double meanRelativeError,
                                double rmse,
                                double mae) {
            out << escapeCsvField(algorithmName) << ','
                << escapeCsvField(algorithmParams) << ','
                << mode << ','
                << runs << ','
                << sampleSize << ','
                << elementIndex << ','
                << f0 << ','
                << seed << ','
                << truthMean << ','
                << estimateMean << ','
                << variance << ','
                << stddev << ','
                << rseTheoretical << ','
                << rseObserved << ','
                << bias << ','
                << absoluteBias << ','
                << relativeBias << ','
                << meanRelativeError << ','
                << rmse << ','
                << mae << '\n';
        }
    };
} // namespace satp::evaluation
