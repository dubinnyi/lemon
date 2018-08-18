#include <iostream>
#include <sstream>

#include <boost/filesystem.hpp>

#include <chemfiles.hpp>

#include "lemon/entries.hpp"
#include "lemon/count.hpp"
#include "lemon/archive_run.hpp"
#include "lemon/select.hpp"
#include "lemon/options.hpp"
#include "lemon/hadoop.hpp"

int main(int argc, char* argv[]) {
    lemon::Options o(argc, argv);

    auto worker = [](chemfiles::Frame complex,
                     const std::string& pdbid) {

        // Selection phase
        auto result = lemon::select_metal_ions(complex);

        // No pruning, straight to out output phase
        lemon::print_residue_name_counts(std::cout, pdbid, complex, result);
    };

    auto p = o.work_dir();
    auto ncpu = o.npu();
    auto entries = o.entries();

    if (!boost::filesystem::is_directory(p)) {
        std::cerr << "You must supply a valid directory" << std::endl;
        return 2;
    }

    if (!entries.empty()) {

        if (!boost::filesystem::is_regular_file(entries)) {
            std::cerr << "You must supply a valid entries file" << std::endl;
            return 1;
        }

        lemon::PDBIDVec vec;
        lemon::read_entry_file(entries, vec);
        boost::filesystem::current_path(p);
        lemon::run_archive(worker, vec, ncpu, 1);
    } else {
        lemon::run_hadoop(worker, p, ncpu);
    }
}
