from WMCore.Configuration import Configuration
config = Configuration()

config.section_("General")
config.General.requestName = 'offset_treemaker'
config.General.workArea = 'crab_projects/analysis'
config.General.transferLogs = True

config.section_("JobType")
config.JobType.pluginName = 'Analysis'
config.JobType.psetName = 'run_offset.py'
config.JobType.allowUndistributedCMSSW = True
config.JobType.outputFiles = ["analysis.root"]

config.section_("Data")
config.Data.inputDataset = '/SingleMuon/Run2016B-PromptReco-v2/MINIAOD'
config.Data.splitting = 'LumiBased'
config.Data.lumiMask = '/afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions15/13TeV/Cert_246908-260627_13TeV_PromptReco_Collisions15_25ns_JSON_v2.txt'
config.Data.unitsPerJob = 10
config.Data.outLFNDirBase = '/store/user/charring/analysis'
config.Data.publication = False
#config.Data.ignoreLocality = True
#config.Data.publishDataName = 'offset_analysis'

config.section_("Site")
#config.Site.blacklist = ['T1_US_FNAL']
#config.Site.whitelist = ['T2_CH_CERN']
config.Site.storageSite = "T3_US_FNALLPC"

# source /cvmfs/cms.cern.ch/crab3/crab.csh
