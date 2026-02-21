#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

#include "satp/simulation/Stats.h"

namespace satp::evaluation {
    class CsvResultWriter {
    public:
        static void appendNormal(const std::filesystem::path &csvPath,
                                 const std::string &algorithmName,
                                 const std::string &algorithmParams,
                                 std::size_t runs,
                                 std::size_t sampleSize,
                                 std::size_t f0,
                                 std::uint32_t seed,
                                 double rseTheoretical,
                                 const Stats &stats) {
            std::ofstream out = openAppend(csvPath);
            writeRecord(out, algorithmName, algorithmParams, "normal", runs, sampleSize,
                        sampleSize, f0, seed, stats.truth_mean, stats.mean,
                        stats.variance, stats.stddev, rseTheoretical, stats.rse_observed,
                        stats.bias, stats.absolute_bias, stats.relative_bias,
                        stats.mean_relative_error, stats.rmse, stats.mae);
        }

        static void appendStreaming(const std::filesystem::path &csvPath,
                                    const std::string &algorithmName,
                                    const std::string &algorithmParams,
                                    std::size_t runs,
                                    std::size_t sampleSize,
                                    std::size_t f0,
                                    std::uint32_t seed,
                                    double rseTheoretical,
                                    const std::vector<StreamingPointStats> &series) {
            std::ofstream out = openAppend(csvPath);
            for (const auto &point : series) {
                writeRecord(out, algorithmName, algorithmParams, "streaming", runs, sampleSize,
                            point.number_of_elements_processed, f0, seed, point.truth_mean,
                            point.mean, point.variance, point.stddev, rseTheoretical,
                            point.rse_observed, point.bias, point.absolute_bias,
                            point.relative_bias, point.mean_relative_error,
                            point.rmse, point.mae);
            }
        }

        static void appendMergePairs(const std::filesystem::path &csvPath,
                                     const std::string &algorithmName,
                                     const std::string &algorithmParams,
                                     std::size_t pairs,
                                     std::size_t sampleSize,
                                     std::uint32_t seed,
                                     const std::vector<MergePairPoint> &points) {
            std::ofstream out = openAppendMerge(csvPath);
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
        static std::string escapeCsvField(const std::string &value) {
            const bool needsQuotes = value.find_first_of(",\"\n\r") != std::string::npos;
            if (!needsQuotes) return value;

            std::string out;
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

        static void writeHeaderIfNeeded(const std::filesystem::path &csvPath,
                                        std::ofstream &out) {
            const bool writeHeader =
                !std::filesystem::exists(csvPath) || std::filesystem::file_size(csvPath) == 0;
            if (!writeHeader) return;

            out << "algorithm,params,mode,runs,sample_size,number_of_elements_processed,f0,seed,"
                   "f0_mean_t,f0_heat_mean_t,"
                   "variance,stddev,rse_theoretical,rse_observed,bias,absolute_bias,relative_bias,"
                   "mean_relative_error,rmse,mae\n";
        }

        static std::ofstream openAppend(const std::filesystem::path &csvPath) {
            std::ofstream out(csvPath, std::ios::app);
            if (!out) throw std::runtime_error("Impossibile aprire il file CSV");
            out << std::setprecision(10);
            writeHeaderIfNeeded(csvPath, out);
            return out;
        }

        static void writeMergeHeaderIfNeeded(const std::filesystem::path &csvPath,
                                             std::ofstream &out) {
            const bool writeHeader =
                !std::filesystem::exists(csvPath) || std::filesystem::file_size(csvPath) == 0;
            if (!writeHeader) return;

            out << "algorithm,params,mode,pairs,sample_size,pair_index,seed,"
                   "estimate_merge,estimate_serial,delta_merge_serial_abs,delta_merge_serial_rel\n";
        }

        static std::ofstream openAppendMerge(const std::filesystem::path &csvPath) {
            std::ofstream out(csvPath, std::ios::app);
            if (!out) throw std::runtime_error("Impossibile aprire il file CSV merge");
            out << std::setprecision(10);
            writeMergeHeaderIfNeeded(csvPath, out);
            return out;
        }

        static void writeRecord(std::ofstream &out,
                                const std::string &algorithmName,
                                const std::string &algorithmParams,
                                const char *mode,
                                std::size_t runs,
                                std::size_t sampleSize,
                                std::size_t elementIndex,
                                std::size_t f0,
                                std::uint32_t seed,
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
