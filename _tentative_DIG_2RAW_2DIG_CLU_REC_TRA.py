# Auto generated configuration file
# using: 
# Revision: 1.19 
# Source: /local/reps/CMSSW/CMSSW/Configuration/Applications/python/ConfigBuilder.py,v 
# with command line options: step2 --filein=file:GluGluTo2Jets_M_300_2000_13TeV_exhume_cff_py_GEN_SIM_HECTOR_CTPPS.root --conditions auto:run2_mc -s DIGI:pdigi_valid,DIGI2RAW --datatier GEN-SIM-DIGI-RAW -n 10 --era Run2_2016 --eventcontent FEVTDEBUG --no_exec
import FWCore.ParameterSet.Config as cms

from Configuration.StandardSequences.Eras import eras

process = cms.Process('CTPPS2',eras.Run2_2017)

# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.cmsCTPPSDigi_cff')
process.load('Configuration.StandardSequences.CTPPSDigiToRaw_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
############### using only CTPPS geometry 
process.load("Configuration.Geometry.geometry_CTPPS_2018_cfi")
#process.load("Geometry.VeryForwardGeometry.geometry_CTPPS_2018_cfi")
#process.load("CondFormats.CTPPSReadoutObjects.CTPPSPixelDAQMappingESSourceXML_cfi")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1)
)
process.MessageLogger = cms.Service("MessageLogger",
    destinations = cms.untracked.vstring('cclu_info'),
    cclu_info = cms.untracked.PSet( threshold = cms.untracked.string('INFO'))
)

process.load("IOMC.RandomEngine.IOMC_cff")
process.RandomNumberGeneratorService.generator.initialSeed = 456789
process.RandomNumberGeneratorService.g4SimHits.initialSeed = 9876
process.RandomNumberGeneratorService.VtxSmeared.initialSeed = 123456789
process.RandomNumberGeneratorService.RPixDetDigitizer = cms.PSet(initialSeed =cms.untracked.uint32(137137))

# Input source
process.source = cms.Source("PoolSource",
    dropDescendantsOfDroppedBranches = cms.untracked.bool(False),
    fileNames = cms.untracked.vstring('file:simevent_CTPPS_DIG_100.root'),
    inputCommands = cms.untracked.vstring('keep *', 
        'drop *_genParticles_*_*', 
        'drop *_genParticlesForJets_*_*', 
        'drop *_kt4GenJets_*_*', 
        'drop *_kt6GenJets_*_*', 
        'drop *_iterativeCone5GenJets_*_*', 
        'drop *_ak4GenJets_*_*', 
        'drop *_ak7GenJets_*_*', 
        'drop *_ak8GenJets_*_*', 
        'drop *_ak4GenJetsNoNu_*_*', 
        'drop *_ak8GenJetsNoNu_*_*', 
        'drop *_genCandidatesForMET_*_*', 
        'drop *_genParticlesForMETAllVisible_*_*', 
        'drop *_genMetCalo_*_*', 
        'drop *_genMetCaloAndNonPrompt_*_*', 
        'drop *_genMetTrue_*_*', 
        'drop *_genMetIC5GenJs_*_*'),
    secondaryFileNames = cms.untracked.vstring()
)

process.options = cms.untracked.PSet(
    SkipEvent = cms.untracked.vstring('ProductNotFound')
)

# Production Info
process.configurationMetadata = cms.untracked.PSet(
    annotation = cms.untracked.string('step2 nevts:10'),
    name = cms.untracked.string('Applications'),
    version = cms.untracked.string('$Revision: 1.19 $')
)



from Geometry.VeryForwardGeometry.geometryRP_cfi import *
from RecoCTPPS.PixelLocal.ctppsPixelLocalReconstruction_cff import *
process.load("RecoCTPPS.PixelLocal.ctppsPixelLocalReconstruction_cff")
process.load('Configuration.StandardSequences.CTPPSRawToDigi_cff')
# Output definition

process.o1 = cms.OutputModule("PoolOutputModule",
        outputCommands = cms.untracked.vstring('drop *',
                                               'keep CTPPS*_*_*_*'
    
),
        fileName = cms.untracked.string('simevent_CTPPS_CLU__REC_TRA_DB_mem_ALL.root')
        )

# Additional output definition
process.load('SimGeneral.MixingModule.MYmixNoPU_cfi')
process.load("SimCTPPS.CTPPSPixelDigiProducer.RPixDetConf_cfi")

# Other statements
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:run2_hlt_relval', '')
process.GlobalTag.toGet = cms.VPSet(
    cms.PSet(record = cms.string("CTPPSPixelDAQMappingRcd"),
             tag = cms.string("PixelDAQMapping"),
             label = cms.untracked.string("RPix"),
             connect = cms.string("sqlite_file:./myCTPPSPixel_DAQMapping_AnalysisMask.db")
             )
    )

#from CondCore.CondDB.CondDB_cfi import *
#process.PoolDBESSource = cms.ESSource("PoolDBESSource",
#                                      CondDB.clone(
#        connect = cms.string('sqlite_file:CTPPSPixel_DAQMapping_Jan23rd2018_3.db')),
#                                      toGet = cms.VPSet(
#        cms.PSet(
#            record = cms.string('CTPPSPixelDAQMappingRcd'),
#            tag = cms.string("PixelDAQMapping"),
##            label = cms.untracked.string("RPix")
#            ),
#        cms.PSet(
#            record = cms.string('CTPPSPixelAnalysisMaskRcd'),
#            tag = cms.string("PixelAnalysisMask"),
##            label = cms.untracked.string("RPix")
#            ),
#               cms.PSet(
#            record = cms.string('CTPPSPixelGainCalibrationsRcd'),
#            tag = cms.string("CTPPSPixelGainCalibNew_v4"),
#            )
#        )
#                                 )


# Path and EndPath definitions
process.mixedigi_step = cms.Path(process.mix*process.RPixDetDigitizer)
process.digi2raw_step = cms.Path(process.DigiToRaw)
process.raw2digi_step = cms.Path(process.RawToDigi)
process.reco_step = cms.Path(process.ctppsPixelLocalReconstruction)
process.endjob_step = cms.EndPath(process.endOfProcess)
process.outpath = cms.EndPath(process.o1)


# Schedule definition
process.schedule = cms.Schedule(process.mixedigi_step,process.digi2raw_step,process.raw2digi_step,process.reco_step,process.endjob_step,process.outpath)
from PhysicsTools.PatAlgos.tools.helpers import associatePatAlgosToolsTask
associatePatAlgosToolsTask(process)


# Customisation from command line

# Add early deletion of temporary data products to reduce peak memory need
from Configuration.StandardSequences.earlyDeleteSettings_cff import customiseEarlyDelete
process = customiseEarlyDelete(process)
# End adding early deletion
