#include <string>
#include <iostream>

// Mac OSX problems with a tolower macro
#include "lemon/lemon.hpp"
#include "chemfiles/File.hpp"

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace python = boost::python;

namespace lemon {

struct LemonPythonWrap : LemonPythonBase, python::wrapper<LemonPythonBase> {
    virtual std::string worker(chemfiles::Frame& frame,
                               const std::string& pdbid) override {
        std::string test(pdbid); 
        auto res = get_override("worker")(boost::ref(frame), test);

        // Silently handle errors
        try {
            return res.as<std::string>();
        } catch (python::error_already_set const &) {
            PyErr_Clear();
        }
        return "";
    }

    virtual void finalize() override {
        get_override("finalize")();
    }
};

template<typename T, typename ret, int maxval>
ret get_index(const T& v, int i) {
    if (i >= 0) {
        if (i >= maxval) {
            PyErr_SetString(PyExc_IndexError, "index is too large");
            python::throw_error_already_set();
        }
        return v[static_cast<size_t>(i)];
    }
    if (-i > maxval) {
        PyErr_SetString(PyExc_IndexError, "index is too small");
        python::throw_error_already_set();
    }
    return v[static_cast<size_t>(maxval + i)];
}

template<typename T, typename ret>
ret get_index(const T& v, int i) {
    if (i >= 0) {
        if (i >= static_cast<int>(v.size())) {
            PyErr_SetString(PyExc_IndexError, "index is too large");
            python::throw_error_already_set();
        }
        return v[static_cast<size_t>(i)];
    }
    if (-i > static_cast<int>(v.size())) {
        PyErr_SetString(PyExc_IndexError, "index is too small");
        python::throw_error_already_set();
    }
    return v[static_cast<size_t>(static_cast<int>(v.size()) + i)];
}

template<typename T>
bool check(const chemfiles::optional<T>& o) {
    return o != chemfiles::nullopt;
}

template<typename T>
T get(const chemfiles::optional<T>& o) {
    if (o) {
        return *o;
    } else {
        PyErr_SetString(PyExc_IndexError, "Cannot dereference nullopt");
        python::throw_error_already_set();
    }
    chemfiles::unreachable();
}

void translate(lemon::geometry::geometry_error const& e)
{
    // Use the Python 'C' API to set up an exception object
    auto msg = std::string("Geometry Error: ") + e.what();
    PyErr_SetString(PyExc_RuntimeError, msg.c_str());
}

using default_id_list = std::list<size_t>;
inline std::ostream& operator<<(std::ostream& os, const default_id_list& idlist) {
    os << '[';
    for (auto i : idlist) {
        os << i << ' ';
    }
    os << ']';
    return os;
}

chemfiles::Frame open_model_in_file(const std::string& filename, size_t index) {
    chemfiles::Trajectory traj(filename);
    return traj.read_step(index);
}

chemfiles::Frame* open_file(const std::string& filename) {
    chemfiles::Trajectory traj(filename);
    return new chemfiles::Frame(std::move(traj.read()));
}

void write_file(const chemfiles::Frame& frame, const std::string& filename) {
    chemfiles::Trajectory traj(filename, chemfiles::File::Mode::WRITE);
    traj.write(frame);
}

void append_file(const chemfiles::Frame& frame, const std::string& filename) {
    chemfiles::Trajectory traj(filename, chemfiles::File::Mode::APPEND);
    traj.write(frame);
}
}

// Pack the Base class wrapper into a module
BOOST_PYTHON_MODULE(lemon) {
    using namespace chemfiles;
    using namespace lemon;
    using boost::noncopyable;

    python::class_<LemonPythonWrap, noncopyable>("Workflow");

    /**************************************************************************
     * Optional
     **************************************************************************/
    python::class_<optional<uint64_t>> ("OptionalUInt64", python::no_init)
        .def("check", check<uint64_t>)
        .def("get", get<uint64_t>);

    python::class_<optional<std::string>> ("OptionalString", python::no_init)
        .def("check", check<std::string>)
        .def("get", get<std::string>);

    python::class_<optional<double>> ("OptionalDouble", python::no_init)
        .def("check", check<double>)
        .def("get", get<double>);

    python::class_<optional<const Property&>> ("OptionalProperty", python::no_init)
        .def("check", check<const Property&>)
        .def("get", get<const Property&>,
            python::return_internal_reference<>());

    python::class_<optional<const Residue&>> ("OptionalResidue", python::no_init)
        .def("check", check<const Residue&>)
        .def("get", get<const Residue&>,
            python::return_internal_reference<>());

    /**************************************************************************
     * Property
     **************************************************************************/
    python::class_<Property> ("Property", python::no_init)
        .def(python::init<bool>())
        .def(python::init<std::string>())
        .def(python::init<Vector3D>())
        .def(python::init<int>())
        .def(python::init<double>())
        .def("kind", &Property::kind)
        .def("as_bool", &Property::as_bool)
        .def("as_double", &Property::as_double)
        .def("as_vector3d", &Property::as_vector3d)
        .def("as_string", &Property::as_string,
            python::return_value_policy<python::copy_const_reference>());

    python::enum_<Property::Kind>("Kind")
        .value("BOOL", Property::BOOL)
        .value("DOUBLE", Property::DOUBLE)
        .value("STRING", Property::STRING)
        .value("VECTOR3D", Property::VECTOR3D);

    /**************************************************************************
     * Vector-likes
     **************************************************************************/
    python::class_<Bond, noncopyable>("Bond", python::no_init)
        .def("__getitem__", &get_index<Bond,size_t,2>);

    python::class_<std::vector<Bond>>("BondVec")
        .def(python::vector_indexing_suite<std::vector<Bond> >());

    python::class_<Angle, noncopyable>("Angle", python::no_init)
        .def("__getitem__", &get_index<Angle,size_t,3>);

    python::class_<std::vector<Angle>>("AngleVec")
        .def(python::vector_indexing_suite<std::vector<Angle> >());

    python::class_<Dihedral, noncopyable>("Dihedral", python::no_init)
        .def("__getitem__", &get_index<Dihedral,size_t,4>);

    python::class_<std::vector<Dihedral>>("DihedralVec")
        .def(python::vector_indexing_suite<std::vector<Dihedral> >());

    python::class_<Improper, noncopyable>("Improper", python::no_init)
        .def("__getitem__", &get_index<Improper,size_t,4>);

    python::class_<std::vector<Improper>>("ImproperVec")
        .def(python::vector_indexing_suite<std::vector<Improper> >());

    python::class_<Vector3D>("Vector3D")
        .def(python::init<double,double,double>())
        .def("__getitem__", &get_index<Vector3D,double,3>)
        .def("norm", &Vector3D::norm);

    python::class_<std::vector<Vector3D>>("PositionVec")
        .def(python::vector_indexing_suite<std::vector<Vector3D> >());

    /**************************************************************************
     * Residue
     **************************************************************************/
    chemfiles::optional<const chemfiles::Property&> (Residue::*residue_get)
        (const std::string&) const = &Residue::get;

    python::class_<Residue, noncopyable>("Residue", python::init<std::string>())
        .def(python::init<std::string, int>())
        .def("size", &Residue::size)
        .def("name", &Residue::name,
            python::return_value_policy<python::copy_const_reference>())
        .def("atoms", python::range(&Residue::cbegin,
                                    &Residue::cend))
        .def("contains", &Residue::contains)
        .def("id", &Residue::id)
        .def("get", residue_get);

    python::class_<std::vector<Residue>>("ResidueVec")
        .def(python::vector_indexing_suite<std::vector<Residue> >())
        .def("size", &std::vector<Residue>::size);

    /**************************************************************************
     * Topology
     **************************************************************************/
    python::class_<Topology, noncopyable>("Topology")
        .def("residue", &Topology::residue,
            python::return_internal_reference<>())
        .def("residues", &Topology::residues,
            python::return_internal_reference<>())
        .def("residue_for_atom", &Topology::residue_for_atom)
        .def("are_linked", &Topology::are_linked)
        .def("bonds", &Topology::bonds,
            python::return_internal_reference<>())
        .def("angles", &Topology::angles,
            python::return_internal_reference<>())
        .def("dihedrals", &Topology::dihedrals,
            python::return_internal_reference<>())
        .def("impropers", &Topology::impropers,
            python::return_internal_reference<>());

    /**************************************************************************
     * Frame
     **************************************************************************/
    chemfiles::optional<const chemfiles::Property&> (Frame::*frame_get)
        (const std::string&) const = &Frame::get;

    python::class_<Frame, noncopyable>("Frame")
        .def("size", &Frame::size)
        .def("atoms", python::range(&Frame::cbegin,
                                    &Frame::cend))
        .def("__get_item__", get_index<Frame,const Atom&>,
            python::return_internal_reference<>())
        .def("topology", &Frame::topology,
            python::return_internal_reference<>())
        .def("distance", &Frame::distance)
        .def("angle", &Frame::angle)
        .def("dihedral", &Frame::dihedral)
        .def("out_of_plane", &Frame::out_of_plane)
        .def("get", frame_get);

    /**************************************************************************
     * Atom
     **************************************************************************/
    chemfiles::optional<const chemfiles::Property&> (Atom::*atom_get)
        (const std::string&) const = &Atom::get;

    python::class_<Atom>("Atom", python::no_init)
        .def("name", &Atom::name,
            python::return_value_policy<python::copy_const_reference>())
        .def("type", &Atom::type,
            python::return_value_policy<python::copy_const_reference>())
        .def("mass", &Atom::mass)
        .def("charge", &Atom::charge)
        .def("full_name", &Atom::full_name)
        .def("vdw_radius", &Atom::vdw_radius)
        .def("covalent_radius", &Atom::covalent_radius)
        .def("atomic_number", &Atom::atomic_number)
        .def("get", atom_get);

    /**************************************************************************
     * Residue Name
     **************************************************************************/
    python::class_<ResidueName>("ResidueName", python::init<const std::string&>())
        .def(python::self_ns::str(python::self));

    typedef std::pair<ResidueNameSet::iterator, bool> rns_insert_ret;
    python::class_<rns_insert_ret>("ResidueNameRet", python::no_init);

    rns_insert_ret (ResidueNameSet::*rns_insert)
        (const ResidueNameSet::value_type&) = &ResidueNameSet::insert;
    python::class_<ResidueNameSet>("ResidueNameSet")
        .def(python::self_ns::str(python::self))
        .def("size", &ResidueNameSet::size)
        .def("__iter__", python::range(&ResidueNameSet::cbegin,
                                       &ResidueNameSet::cend))
        .def("append", rns_insert);

    ResidueNameCount::const_iterator (ResidueNameCount::*rnc_begin)(void) const =
        &ResidueNameCount::begin;
    ResidueNameCount::const_iterator (ResidueNameCount::*rnc_end)(void) const =
        &ResidueNameCount::end;

    python::class_<ResidueNameCount>("ResidueNameCount")
        .def(python::self_ns::str(python::self))
        .def(python::self += python::self)
        .def("size", &ResidueNameCount::size)
        .def("__iter__", python::range(rnc_begin,
                                       rnc_end));

    python::class_<std::pair<ResidueName const, size_t>>("ResidueCount",
                                                         python::no_init)
        .def_readonly("first", &std::pair<ResidueName const, size_t>::first)
        .def_readonly("second", &std::pair<ResidueName const, size_t>::second);

    void (default_id_list::*push_back)(const default_id_list::value_type&) =
        &default_id_list::push_back;

    python::class_<default_id_list>("ResidueIDs")
        //.def(python::self_ns::str(python::self))
        .def(python::init<const default_id_list&>())
        .def("__iter__", python::range(&default_id_list::cbegin,
                                       &default_id_list::cend))
        .def("size", &default_id_list::size)
        .def("append", push_back);

    /**************************************************************************
     * Constants
     **************************************************************************/
    python::class_<std::unordered_set<std::string>>("StringSet");
    python::scope().attr("small_molecule_types") = small_molecule_types;

    python::scope().attr("common_peptides") = common_peptides;
    python::scope().attr("common_cofactors") = common_cofactors;
    python::scope().attr("common_fatty_acids") = common_fatty_acids;
    python::scope().attr("proline_res") = proline_res;

    /**************************************************************************
     * Select
     **************************************************************************/

    // Returns a new object    
    default_id_list (*small_molecules)(const Frame&,
                                       const std::unordered_set<std::string>&,
                                       size_t) =
        &select::small_molecules;

    python::def("select_small_molecules", small_molecules);

    default_id_list (*metal_ions)(const Frame&) = &select::metal_ions;
    python::def("select_metal_ions", metal_ions);

    default_id_list (*nucleic_acids)(const Frame&) = &select::nucleic_acids;
    python::def("select_nucleic_acids", nucleic_acids);

    default_id_list (*peptides)(const Frame&) = &select::peptides;
    python::def("select_peptides", peptides);

    default_id_list (*specific_residues)(const Frame&, const ResidueNameSet&) =
        &select::specific_residues;
    python::def("select_specific_residues", specific_residues);

    // Inplace
    size_t (*small_molecules_i)(const Frame&, default_id_list&,
                                const std::unordered_set<std::string>&,
                                size_t) =
        &select::small_molecules;

    python::def("select_small_molecules", small_molecules_i);

    size_t (*metal_ions_i)(const Frame&, default_id_list&) =
         &select::metal_ions;
    python::def("select_metal_ions", metal_ions_i);

    size_t (*nucleic_acids_i)(const Frame&, default_id_list&) =
        &select::nucleic_acids;
    python::def("select_nucleic_acids", nucleic_acids_i);

    size_t (*peptides_i)(const Frame&, default_id_list&) =
        &select::peptides;
    python::def("select_peptides", peptides_i);

    size_t (*specific_residues_i)(const Frame&, default_id_list&,
                                  const ResidueNameSet&) =
        &select::specific_residues;
    python::def("select_specific_residues", specific_residues_i);

    /**************************************************************************
     * Count
     **************************************************************************/
    python::def("count_altloc", count::altloc);
    python::def("count_bioassemblies", count::bioassemblies);
    python::def("print_residue_name_counts",
        count::print_residue_name_counts<default_id_list>);

    void (*residues1)(const Frame&, ResidueNameCount&) = &count::residues;
    python::def("count_residues", residues1);

    void (*residues2)(const Frame&, const default_id_list&, ResidueNameCount&) =
        &count::residues;
    python::def("count_residues", residues2);

    /**************************************************************************
     * Prune
     **************************************************************************/
    python::def("prune_identical_residues",
        prune::identical_residues<default_id_list>);
    python::def("prune_cofactors", prune::cofactors<default_id_list>);
    python::def("keep_interactions", prune::keep_interactions<default_id_list>);
    python::def("remove_interactions", prune::remove_interactions<default_id_list>);

    /**************************************************************************
     * Separate
     **************************************************************************/
    python::def("separate_residues", separate::residues<default_id_list>);
    python::def("separate_protein_and_ligand", separate::protein_and_ligand);

    /**************************************************************************
     * geometry
     **************************************************************************/
    python::def("protein_bond_name", geometry::protein::bond_name);
    python::def("protein_angle_name", geometry::protein::angle_name);
    python::def("protein_dihedral_name", geometry::protein::dihedral_name);
    python::def("protein_improper_name", geometry::protein::improper_name);
    python::register_exception_translator<geometry::geometry_error>(&translate);

    /**************************************************************************
     * Vina Score
     **************************************************************************/
    python::class_<xscore::VinaScore>("VinaScore", python::no_init)
        .def_readonly("g1", &xscore::VinaScore::g1)
        .def_readonly("g2", &xscore::VinaScore::g2)
        .def_readonly("rep", &xscore::VinaScore::rep)
        .def_readonly("hydrophobic", &xscore::VinaScore::hydrophobic)
        .def_readonly("hydrogen", &xscore::VinaScore::hydrogen);

    python::def("vina_score", xscore::vina_score<default_id_list>);

    /**************************************************************************
     * TMAlign
     **************************************************************************/
    python::class_<tmalign::TMResult>("TMResult", python::no_init)
        .def_readonly("score", &tmalign::TMResult::score)
        .def_readonly("rmsd", &tmalign::TMResult::rmsd)
        .def_readonly("aligned", &tmalign::TMResult::aligned);

    python::def("TMscore", tmalign::TMscore);

    /**************************************************************************
    * File IO
    ***************************************************************************/
    python::def("open_model_in_file", open_model_in_file);
    python::def("open_file", open_file,
        python::return_value_policy<python::manage_new_object>());
    python::def("write_file", write_file);
    python::def("append_file", append_file);
}
