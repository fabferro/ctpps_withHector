
import FWCore.ParameterSet.Config as cms

from Configuration.StandardSequences.Eras import eras

process = cms.Process('CTPPSAN',eras.Run2_25ns)

# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

process.maxEvents = cms.untracked.PSet(
        input = cms.untracked.int32(20000)
        )

process.MessageLogger = cms.Service("MessageLogger",
    destinations = cms.untracked.vstring('an_info'),
    an_info = cms.untracked.PSet( threshold = cms.untracked.string('ERROR'))
)
process.source = cms.Source("EmptyIOVSource",
    timetype = cms.string('runnumber'),
    firstValue = cms.uint64(1),
    lastValue = cms.uint64(1),
    interval = cms.uint64(1)
)

# Track memory leaks
#process.SimpleMemoryCheck = cms.Service("SimpleMemoryCheck",ignoreTotal = cms.untracked.int32(1) )


process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
'file:./simevent_CTPPS_CLU__REC_TRA_DB_mem.root',

),
duplicateCheckMode = cms.untracked.string("checkEachFile")
)

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:run2_hlt_relval', '')


process.load("RecoCTPPS.PixelLocal.ctppsPixelTrackAnalyzer_cfi")
process.ctppsPixelTrackAnalyzer.Verbosity = cms.untracked.uint32(0)

process.ALL = cms.Path(process.ctppsPixelTrackAnalyzer)


#process.outpath = cms.EndPath(process.o1)

process.schedule = cms.Schedule(process.ALL)

# filter all path with the production filter sequence
for path in process.paths:
  #  getattr(process,path)._seq = process.ProductionFilterSequence * getattr(process,path)._seq
    getattr(process,path)._seq =  getattr(process,path)._seq


