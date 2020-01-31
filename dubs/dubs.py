from candiy_lemon import lemon
import sys

# List of dictionaries to keep track of parts of the file

# Key: reference pdbid, Value: path to .mmtf file
pathDict = {}
# Key: reference pdbID, Value: list of associated ligands (both SM and non SM)
referenceDict = {}
# Key: reference pdbID, Value: list of proteins to aling to reference (like in pinc)
alignProtDict = {}
# Key: pdbID, Value: chemical id for SM ligand
pdbIDSMDict = {}
# Key: pdbID, Value: tuple(resCode, chainID, residue ID)
pdbIDNonSMDict = {}
# Key: pdbID, Value: chemical id for SM ligand
noAlignSMDict = {}
# Key: pdbID, Value: tuple(resCode, chainID, residue ID)
noAlignNonSMDict = {}

entries = lemon.Entries()

# Method for parsing a formated input file
def parse_input_file(fname):
    # Open file and initialize flags to 0
    f = open(fname,"r")
    curRefPdbID = ""
    refFlag = 0
    protFlag = 0
    SMLigFlag = 0
    nonSMligFlag = 0
    noAlignSMFlag = 0
    noAlignNonSMFlag = 0

    for line in f:
        # Check to see if the line contains any of the tags
        # Set appropriate flags if it does
        if line.startswith("@<reference>"):
            refFlag = 1
            protFlag = 0
            SMLigFlag = 0
            nonSMligFlag = 0
            noAlignSMFlag = 0
            noAlignNonSMFlag = 0
        elif line.startswith("@<align_prot>"):
            refFlag = 0
            protFlag = 1
            SMLigFlag = 0
            nonSMligFlag = 0
            noAlignSMFlag = 0
            noAlignNonSMFlag = 0
        elif line.startswith("@<align_sm_ligands>"):
            refFlag = 0
            protFlag = 0
            SMLigFlag = 1
            nonSMligFlag = 0
            noAlignSMFlag = 0
            noAlignNonSMFlag = 0
        elif line.startswith("@<align_non_sm_ligands>"):
            refFlag = 0
            protFlag = 0
            SMLigFlag = 0 
            nonSMligFlag = 1
            noAlignSMFlag = 0
            noAlignNonSMFlag = 0
        elif line.startswith("@<no_align_sm_ligands>"):
            refFlag = 0
            protFlag = 0
            SMLigFlag = 0 
            nonSMligFlag = 0
            noAlignSMFlag = 1
            noAlignNonSMFlag = 0
        elif line.startswith("@<no_align_non_sm_ligands>"):
            refFlag = 0
            protFlag = 0
            SMLigFlag = 0 
            nonSMligFlag = 0
            noAlignSMFlag = 0
            noAlignNonSMFlag = 1
        elif line.startswith("@<end>"):
            refFlag = 0
            protFlag = 0
            SMLigFlag = 0
            nonSMligFlag = 0
            noAlignSMFlag = 0
            noAlignNonSMFlag = 0
        else:
            # If the line does not contain a flag
            # Add info to appropriate dictionary based of set flags
            if refFlag == 1:
                pdbID = line.split(" ")[0].strip()
                path = line.split(" ")[1].strip()
                curRefPdbID = pdbID
                pathDict[pdbID] = path
            
            elif protFlag == 1:
                pdbID = line.strip()
                if alignProtDict.get(curRefPdbID,0) == 0:
                    alignProtDict[curRefPdbID] = [pdbID]
                else:
                    alignProtDict[curRefPdbID].append(pdbID)

            elif SMLigFlag == 1:
                pdbID = line.split(" ")[0].strip()
                chemID = line.split(" ")[1].strip()

                if referenceDict.get(curRefPdbID,0) == 0:
                    referenceDict[curRefPdbID] = [pdbID]
                else:
                    referenceDict[curRefPdbID].append(pdbID)
                
                if pdbIDSMDict.get(pdbID,0) == 0:
                    pdbIDSMDict[pdbID] = [chemID]
                else:
                    pdbIDSMDict[pdbID].append(chemID)

                entries.add(pdbID)

            elif nonSMligFlag == 1:
                pdbID = line.split(" ")[0].strip()
                residueCode = line.split(" ")[1].split("-")[0].strip()
                chainID = line.split(" ")[1].split("-")[1].strip()
                residueID = line.split(" ")[1].split("-")[2].strip()

                if referenceDict.get(curRefPdbID,0) == 0:
                    referenceDict[curRefPdbID] = [pdbID]
                else:
                    referenceDict[curRefPdbID].append(pdbID)
                
                if pdbIDNonSMDict.get(pdbID,0) == 0:
                    pdbIDNonSMDict[pdbID] = [tuple([residueCode,chainID,residueID])]
                else:
                    pdbIDNonSMDict[pdbID].append(tuple([residueCode,chainID,residueID]))

                entries.add(pdbID)
            
            elif noAlignSMFlag == 1:
                pdbID = line.split(" ")[0].strip()
                chemID = line.split(" ")[1].strip()

                if noAlignSMDict.get(pdbID,0) == 0:
                    noAlignSMDict[pdbID] = [chemID]
                else:
                    noAlignSMDict[pdbID].append(chemID)

                entries.add(pdbID)

            elif noAlignNonSMFlag == 1:
                pdbID = line.split(" ")[0].strip()
                residueCode = line.split(" ")[1].split("-")[0].strip()
                chainID = line.split(" ")[1].split("-")[1].strip()
                residueID = line.split(" ")[1].split("-")[2].strip()

                if noAlignNonSMDict.get(pdbID,0) == 0:
                    noAlignNonSMDict[pdbID] = [tuple([residueCode,chainID,residueID])]
                else:
                    noAlignNonSMDict[pdbID].append(tuple([residueCode,chainID,residueID]))

                entries.add(pdbID)

# Define Lemon workflow class
class MyWorkflow(lemon.Workflow):
    def __init__(self):
        lemon.Workflow.__init__(self)
        
        self.reference_structures = {}
        for key, value in pathDict:
            self.reference_structures[key] = lemon.open_file(value)

    def worker(self, entry, pdbid):
        # Define and assign the reference pdbid
        refpdbid = ""
        # mode is 0 unassigned, 1 for alignment for protein, 2 for alignment for ligand
        mode = 0

        # Check for pdbID as a protein to be aligned (like in PINC)
        for key, value in alignProtDict.items():
            if pdbid in value:
                refpdbid = key
                mode = 1

        # Check for protein-ligand pair for alignment 
        for key, value in referenceDict.items():
            if pdbid in value:
                refpdbid = key
                mode = 2

        if mode == 0:

            for ligand_code in pdbIDSMDict.get(pdbid, []):

                ligand_ids = lemon.select_specific_residues(entry, lemon.ResidueName(ligand_code))
                lemon.prune_identical_residues(entry, ligand_ids)

                for ligand_id in ligand_ids:
                    protein = lemon.Frame()
                    ligand = lemon.Frame()

                    lemon.separate_protein_and_ligand(entry, ligand_id, 25.0, protein, ligand)
                    lemon.write_file(protein, pdbid + "_" + ligand_code + ".pdb")
                    lemon.write_file(ligand, pdbid + "_" + ligand_code + ".sdf")

            return pdbid + " no alignment"

        elif mode == 1:
            # If we need to align to a protein (like in PINC)
            alignment = lemon.TMscore(entry, self.reference_structures[refpdbid])
            positions = entry.positions()
            lemon.align(positions, alignment.affine)
            return "Align Protein: " + pdbid + " to " + refpdbid + " with score of " + alignment.score

        elif mode == 2:
            # If we are doing ligand alignment, that can be done here
            # Get a list of the ligands associated with the protein we are trying to align
            SM_ligandList = pdbIDSMDict.get(pdbid, []) 
            Non_SM_ligandList = pdbIDNonSMDict.get(pdbid, [])

            alignment = lemon.TMscore(entry, self.reference_structures[refpdbid])
            positions = entry.positions()
            lemon.align(positions, alignment.affine)

            if len(SM_ligandList) > 0:

                for ligand_code in SM_ligandList:

                    ligand_ids = lemon.select_specific_residues(entry, lemon.ResidueName(ligand_code))
                    lemon.prune_identical_residues(entry, ligand_ids)

                    for ligand_id in ligand_ids:
                        protein = lemon.Frame()
                        ligand = lemon.Frame()

                        lemon.separate_protein_and_ligand(entry, ligand_id, 25.0, protein, ligand)
                        lemon.write_file(protein, pdbid + "_" + ligand_code + ".pdb")
                        lemon.write_file(ligand, pdbid + "_" + ligand_code + ".sdf")

            if len(Non_SM_ligandList > 0):
                # TODO stuff here for non-small ligands
                pass

            return pdbid

    def finalize(self):
        pass

# Get from the command line
# for testing we also can hard set the path
if len(sys.argv) > 1:
    input_file_path = sys.argv[1]
    hadoop_path = sys.argv[2]
    cores = int(sys.argv[3])
else:
    #TODO change this if needed for testing
    input_file_path = "format.txt"
    hadoop_path = "../../full"
    cores = 8

# Parse the input file
parse_input_file(input_file_path)

# Initilize the workflow
wf = MyWorkflow()

# TODO Get these from the command-line or ask the user
lemon.launch(wf, hadoop_path, cores, entries)