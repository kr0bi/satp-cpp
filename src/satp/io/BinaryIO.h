#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace satp::io {
    inline void saveDataset(const std::filesystem::path &path,
                            const std::vector<std::uint32_t> &data,
                            const std::vector<std::vector<std::uint32_t> > &subsets) {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot open file for writing");

        const char magic[4] = {'S', 'A', 'T', 'P'};
        const std::uint32_t ver = 1;
        const std::uint64_t N = data.size();
        const std::uint32_t R = subsets.size();
        const std::uint32_t S = subsets.empty() ? 0 : subsets[0].size();

        out.write(magic, 4);
        out.write(reinterpret_cast<const char *>(&ver), sizeof(ver));
        out.write(reinterpret_cast<const char *>(&N), sizeof(N));
        out.write(reinterpret_cast<const char *>(data.data()), N * sizeof(std::uint32_t));
        out.write(reinterpret_cast<const char *>(&R), sizeof(R));
        out.write(reinterpret_cast<const char *>(&S), sizeof(S));

        for (const auto &subset: subsets)
            out.write(reinterpret_cast<const char *>(subset.data()), S * sizeof(std::uint32_t));
    }

    inline void loadDataset(const std::filesystem::path &path,
                            std::vector<std::uint32_t> &data,
                            std::vector<std::vector<std::uint32_t> > &subsets) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open file for reading");

        char magic[4];
        in.read(magic, 4);
        if (std::memcmp(magic, "SATP", 4) != 0) throw std::runtime_error("Bad file");

        std::uint32_t ver;
        in.read(reinterpret_cast<char *>(&ver), sizeof(ver));
        if (ver != 1) throw std::runtime_error("Version mismatch");

        std::uint64_t N;
        in.read(reinterpret_cast<char *>(&N), sizeof(N));
        data.resize(N);
        in.read(reinterpret_cast<char *>(data.data()), N * sizeof(std::uint32_t));

        std::uint32_t R, S;
        in.read(reinterpret_cast<char *>(&R), sizeof(R));
        in.read(reinterpret_cast<char *>(&S), sizeof(S));

        subsets.assign(R, std::vector<std::uint32_t>(S));
        for (auto &subset: subsets)
            in.read(reinterpret_cast<char *>(subset.data()), S * sizeof(std::uint32_t));
    }
} // namespace satp::io
