// -*- C++ -*-
//
// Package:    analysis/ZDilepton
// Class:      ZDilepton
// 
/**\class ZDilepton ZDilepton.cc ZDilepton/plugins/ZDilepton.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  charles harrington and bahareh roozbahani
//         Created:  Wed, 05 Oct 2016 17:09:43 GMT
//
//

// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "RecoEgamma/EgammaTools/interface/EffectiveAreas.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/Common/interface/Ptr.h"
#include "RecoEgamma/EgammaTools/interface/ConversionTools.h"
#include <DataFormats/HepMCCandidate/interface/GenParticle.h>
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/PatCandidates/interface/PackedTriggerPrescales.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include <SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h>
#include "parsePileUpJSON2.h"
#include <vector>
#include <algorithm>
#include <utility>

//root files
#include <TFile.h>
#include <TTree.h>

using namespace std;
using namespace edm;

const int MAXLEP = 10;
const int MAXGEN = 400;
const int MAXJET = 50;
const int MAXNPV = 100;
const int nFilters = 6;
const int METUNCERT = 4;

bool sortLepPt(reco::CandidatePtr lep1, reco::CandidatePtr lep2){ return lep1->pt() > lep2->pt(); }

class ZDilepton : public edm::EDAnalyzer {
  public:
    explicit ZDilepton(const edm::ParameterSet&);

  private:
    virtual void beginJob() override;
    virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
    virtual void endJob() override;

    TFile* root_file;
    TTree* tree;

    string filters[nFilters] = {
      "Flag_HBHENoiseFilter",
      "Flag_HBHENoiseIsoFilter", 
      "Flag_EcalDeadCellTriggerPrimitiveFilter",
      "Flag_goodVertices",
      "Flag_eeBadScFilter",
      "Flag_globalTightHalo2016Filter"
    };
    vector<bool> filter_failed;

    ULong64_t event;
    int run, lumi, bx;

    float rho, mu, genweight;
    int nPV;

    int nGen;
    int gen_status[MAXGEN], gen_PID[MAXGEN], gen_nMothers[MAXGEN], gen_nDaughters[MAXGEN], gen_mother0[MAXGEN], gen_mother1[MAXGEN];
    float gen_pt[MAXGEN], gen_mass[MAXGEN], gen_eta[MAXGEN], gen_phi[MAXGEN];
    int nGenJet;
    float genJet_pt[MAXJET], genJet_eta[MAXJET], genJet_phi[MAXJET], genJet_area[MAXJET], genJet_nDaught[MAXJET];

    int nMuon;
    bool muon_isGlob[MAXLEP], muon_IsLooseID[MAXLEP], muon_IsMediumID[MAXLEP], muon_IsTightID[MAXLEP];
    int muon_type[MAXLEP], muon_charge[MAXLEP];
    float muon_pt[MAXLEP], muon_eta[MAXLEP], muon_phi[MAXLEP], muon_D0[MAXLEP], muon_Dz[MAXLEP];
    float muon_chi2[MAXLEP], muon_tspm[MAXLEP], muon_kinkf[MAXLEP], muon_segcom[MAXLEP];
    
    int nEle;
    int ele_charge[MAXLEP];
    bool ele_VetoID[MAXLEP], ele_LooseID[MAXLEP], ele_MediumID[MAXLEP], ele_TightID[MAXLEP];
    float ele_dEtaIn[MAXLEP], ele_dPhiIn[MAXLEP];
    float ele_pt[MAXLEP], ele_eta[MAXLEP], ele_phi[MAXLEP], ele_D0[MAXLEP], ele_Dz[MAXLEP];
    float ele_sigmaIetaIeta[MAXLEP], ele_dEtaSeed[MAXLEP], ele_dPhiSeed[MAXLEP], ele_HE[MAXLEP], ele_rcpiwec[MAXLEP], ele_overEoverP[MAXLEP];
    float ele_missinghits[MAXLEP];

    int nJet;
    float jet_pt[MAXJET], jet_eta[MAXJET], jet_phi[MAXJET], jet_area[MAXJET], jet_jec[MAXJET], jet_btag[MAXJET];
    float jet_nhf[MAXJET], jet_nef[MAXJET], jet_chf[MAXJET], jet_muf[MAXJET];
    float jet_elef[MAXJET], jet_numconst[MAXJET], jet_numneutral[MAXJET], jet_chmult[MAXJET];
    char jet_clean[MAXJET];

    float genmet_pt, genmet_px, genmet_py, genmet_sumet, genmet_phi;

    float met_pt, met_px, met_py, met_sumet, met_phi;
    float pupmet_pt, pupmet_px, pupmet_py,  pupmet_sumet, pupmet_phi;
    int nMETUncert;
    float met_shiftedpx[METUNCERT], met_shiftedpy[METUNCERT], pupmet_shiftedpx[METUNCERT], pupmet_shiftedpy[METUNCERT];

    vector<float> trig_prescale; vector<bool> trig_failed; vector<string> trig_name;

    TString fileName_;
    string btag_;
    bool isMC_, metFilters_;
    double minLepPt_, minSubLepPt_;

    EffectiveAreas ele_areas_;
    edm::EDGetTokenT<edm::TriggerResults> patTrgLabel_;
    edm::EDGetTokenT<double> rhoTag_;
    edm::EDGetTokenT< edm::View<reco::Vertex> > pvTag_;
    edm::EDGetTokenT< edm::View<reco::GenParticle> > genParticleTag_;
    edm::EDGetTokenT< edm::View<reco::GenJet> > genJetTag_;
    edm::EDGetTokenT< edm::View<pat::Muon> > muonTag_;
    edm::EDGetTokenT< edm::View<pat::Electron> > electronTag_;
    edm::EDGetTokenT<edm::ValueMap<bool> > eleVetoIdMapToken_;
    edm::EDGetTokenT<edm::ValueMap<bool> > eleLooseIdMapToken_;
    edm::EDGetTokenT<edm::ValueMap<bool> > eleMediumIdMapToken_;
    edm::EDGetTokenT<edm::ValueMap<bool> > eleTightIdMapToken_;
    edm::EDGetTokenT< edm::View<pat::Jet> > jetTag_;
    edm::EDGetTokenT< edm::View<pat::MET> > metTag_;
    edm::EDGetTokenT< edm::View<pat::MET> > metPuppiTag_;
    edm::EDGetTokenT<bool> BadChCandFilterToken_;
    edm::EDGetTokenT<bool> BadPFMuonFilterToken_;
    edm::EDGetTokenT<edm::TriggerResults> triggerResultsTag_;
    edm::EDGetTokenT<pat::PackedTriggerPrescales> prescalesTag_;
    edm::EDGetTokenT<GenEventInfoProduct> genEventTag_;
    edm::EDGetTokenT< edm::View<PileupSummaryInfo> > muTag_;
};

ZDilepton::ZDilepton(const edm::ParameterSet& iConfig):
  ele_areas_( (iConfig.getParameter<edm::FileInPath>("effAreasConfigFile")).fullPath() )
{
  fileName_ = iConfig.getParameter<string>("fileName");
  btag_ = iConfig.getParameter<string>("btag");
  isMC_ = iConfig.getParameter<bool>("isMC");
  metFilters_ = iConfig.getParameter<bool>("metFilters");
  patTrgLabel_ = consumes<edm::TriggerResults>( iConfig.getParameter<edm::InputTag>("patTrgLabel") );
  rhoTag_ = consumes<double>( iConfig.getParameter<edm::InputTag>("rhoTag") );
  pvTag_ = consumes< edm::View<reco::Vertex> >( iConfig.getParameter<edm::InputTag>("pvTag") );
  genParticleTag_ =  consumes< edm::View<reco::GenParticle> >( iConfig.getParameter<edm::InputTag>("genParticleTag") );
  genJetTag_ =  consumes< edm::View<reco::GenJet> >( iConfig.getParameter<edm::InputTag>("genJetTag") );
  muonTag_ = consumes< edm::View<pat::Muon> >( iConfig.getParameter<edm::InputTag>("muonTag") );
  electronTag_ = consumes< edm::View<pat::Electron> >( iConfig.getParameter<edm::InputTag>("electronTag") );
  eleVetoIdMapToken_ = consumes<edm::ValueMap<bool> >( iConfig.getParameter<edm::InputTag>("eleVetoIdMapToken") );
  eleLooseIdMapToken_ = consumes<edm::ValueMap<bool> >( iConfig.getParameter<edm::InputTag>("eleLooseIdMapToken") );
  eleMediumIdMapToken_ = consumes<edm::ValueMap<bool> >( iConfig.getParameter<edm::InputTag>("eleMediumIdMapToken") );
  eleTightIdMapToken_ = consumes<edm::ValueMap<bool> >( iConfig.getParameter<edm::InputTag>("eleTightIdMapToken") );
  jetTag_ = consumes< edm::View<pat::Jet> >( iConfig.getParameter<edm::InputTag>("jetTag") );
  metTag_ = consumes< edm::View<pat::MET> >( iConfig.getParameter<edm::InputTag>("metTag") );
  metPuppiTag_ = consumes< edm::View<pat::MET> >( iConfig.getParameter<edm::InputTag>("metPuppiTag") );
  BadChCandFilterToken_ = consumes<bool>(iConfig.getParameter<edm::InputTag>("BadChargedCandidateFilter"));
  BadPFMuonFilterToken_ = consumes<bool>(iConfig.getParameter<edm::InputTag>("BadPFMuonFilter"));
  minLepPt_ = iConfig.getParameter<double>("minLepPt");
  minSubLepPt_ = iConfig.getParameter<double>("minSubLepPt");
  triggerResultsTag_ = consumes<edm::TriggerResults>( iConfig.getParameter<edm::InputTag>("triggerResultsTag") );
  prescalesTag_ = consumes<pat::PackedTriggerPrescales>( iConfig.getParameter<edm::InputTag>("prescalesTag") );
  genEventTag_ = consumes<GenEventInfoProduct>( iConfig.getParameter<edm::InputTag>("genEventTag") );
  muTag_ = consumes< edm::View<PileupSummaryInfo> >( iConfig.getParameter<edm::InputTag>("muTag") );
}

// ------------ method called once each job just before starting event loop  ------------
void  ZDilepton::beginJob() {

  root_file = new TFile(fileName_, "RECREATE");
  tree = new TTree("T", "Analysis Tree");

  tree->Branch("filter_failed", "std::vector<bool>", &filter_failed);
  tree->Branch("trig_prescale", "std::vector<float>", &trig_prescale);
  tree->Branch("trig_failed", "std::vector<bool>", &trig_failed);
  tree->Branch("trig_name", "std::vector<string>", &trig_name);

  if(isMC_){
    tree->Branch("nGen", &nGen, "nGen/I");
    tree->Branch("gen_status",  gen_status, "gen_status[nGen]/I");
    tree->Branch("gen_PID",  gen_PID, "gen_PID[nGen]/I");
    tree->Branch("gen_pt", gen_pt, "gen_pt[nGen]/F");
    tree->Branch("gen_mass", gen_mass, "gen_mass[nGen]/F");
    tree->Branch("gen_eta", gen_eta, "gen_eta[nGen]/F");
    tree->Branch("gen_phi", gen_phi, "gen_phi[nGen]/F");
    tree->Branch("gen_nMothers", gen_nMothers, "gen_nMothers[nGen]/I");
    tree->Branch("gen_nDaughters", gen_nDaughters, "gen_nDaughters[nGen]/I");
    tree->Branch("gen_mother0", gen_mother0, "gen_mother0[nGen]/I");
    tree->Branch("gen_mother1", gen_mother1, "gen_mother1[nGen]/I");

    tree->Branch("nGenJet", &nGenJet, "nGenJet/I");
    tree->Branch("genJet_pt", genJet_pt, "genJet_pt[nGenJet]/F");
    tree->Branch("genJet_eta", genJet_eta, "genJet_eta[nGenJet]/F");
    tree->Branch("genJet_phi", genJet_phi, "genJet_phi[nGenJet]/F");
    tree->Branch("genJet_area",  genJet_area, "genJet_area[nGenJet]/F");
    tree->Branch("genJet_nDaught",  genJet_nDaught, "genJet_nDaught[nGenJet]/F");

    tree->Branch("genmet_pt", &genmet_pt, "genmet_pt/F");
    tree->Branch("genmet_px", &genmet_px, "genmet_px/F");
    tree->Branch("genmet_py", &genmet_py, "genmet_py/F");
    tree->Branch("genmet_sumet", &genmet_sumet, "genmet_sumet/F");
    tree->Branch("genmet_phi", &genmet_phi, "genmet_phi/F");

    tree->Branch("genweight", &genweight, "genweight/F");
  }
  else{
    tree->Branch("run", &run, "run/I");
    tree->Branch("lumi", &lumi, "lumi/I");
    tree->Branch("bx", &bx, "bx/I");
    tree->Branch("event", &event, "event/l");
  }

  tree->Branch("rho", &rho, "rho/F");
  tree->Branch("mu", &mu, "mu/F");
  tree->Branch("nPV", &nPV, "nPV/I");

  tree->Branch("nMuon", &nMuon, "nMuon/I");
  tree->Branch("muon_charge", muon_charge, "muon_charge[nMuon]/I");
  tree->Branch("muon_type", muon_type, "muon_type[nMuon]/I");
  tree->Branch("muon_isGlob", muon_isGlob, "muon_isGlob[nMuon]/O");
  tree->Branch("muon_pt", muon_pt, "muon_pt[nMuon]/F");
  tree->Branch("muon_eta", muon_eta, "muon_eta[nMuon]/F");
  tree->Branch("muon_phi", muon_phi, "muon_phi[nMuon]/F");
  tree->Branch("muon_D0", muon_D0, "muon_D0[nMuon]/F");
  tree->Branch("muon_Dz", muon_Dz, "muon_Dz[nMuon]/F");
  tree->Branch("muon_chi2", muon_chi2, "muon_chi2[nMuon]/F");
  tree->Branch("muon_tspm", muon_tspm, "muon_tspm[nMuon]/F");
  tree->Branch("muon_kinkf", muon_kinkf, "muon_kinkf[nMuon]/F");
  if (!isMC_) tree->Branch("muon_segcom", muon_segcom, "muon_segcom[nMuon]/F");
  tree->Branch("muon_IsLooseID", muon_IsLooseID, "muon_IsLooseID[nMuon]/O");
  tree->Branch("muon_IsMediumID", muon_IsMediumID, "muon_IsMediumID[nMuon]/O");
  tree->Branch("muon_IsTightID", muon_IsTightID, "muon_IsTightID[nMuon]/O");

  tree->Branch("nEle", &nEle, "nEle/I");
  tree->Branch("ele_charge", ele_charge, "ele_charge[nEle]/I");
  tree->Branch("ele_pt", ele_pt, "ele_pt[nEle]/F");
  tree->Branch("ele_eta", ele_eta, "ele_eta[nEle]/F");
  tree->Branch("ele_phi", ele_phi, "ele_phi[nEle]/F");
  tree->Branch("ele_VetoID", ele_VetoID , "ele_VetoID/O");
  tree->Branch("ele_LooseID", ele_LooseID , "ele_LooseID/O");
  tree->Branch("ele_MediumID", ele_MediumID , "ele_MediumID/O");
  tree->Branch("ele_TightID", ele_TightID , "ele_TightID/O");
  tree->Branch("ele_D0", ele_D0, "ele_D0[nEle]/F");
  tree->Branch("ele_Dz", ele_Dz, "ele_Dz[nEle]/F");
  tree->Branch("ele_sigmaIetaIeta", ele_sigmaIetaIeta, "ele_sigmaIetaIeta[nEle]/F");
  tree->Branch("ele_dEtaSeed", ele_dEtaSeed, "ele_dEtaSeed[nEle]/F");
  tree->Branch("ele_dPhiSeed", ele_dPhiSeed, "ele_dPhiSeed[nEle]/F");
  tree->Branch("ele_dEtaIn", ele_dEtaIn, "ele_dEtaIn[nEle]/F");
  tree->Branch("ele_dPhiIn", ele_dPhiIn, "ele_dPhiIn[nEle]/F");
  tree->Branch("ele_overEoverP", ele_overEoverP, "ele_overEoverP[nEle]/F");
  if (!isMC_){
    tree->Branch("ele_HE", ele_HE, "ele_HE[nEle]/F");
    tree->Branch("ele_rcpiwec", ele_rcpiwec, "ele_rcpiwec[nEle]/F");
    tree->Branch("ele_missinghits", ele_missinghits, "ele_missinghits[nEle]/F");
  }

  tree->Branch("nJet", &nJet, "nJet/I");
  tree->Branch("jet_pt", jet_pt, "jet_pt[nJet]/F");
  tree->Branch("jet_eta", jet_eta, "jet_eta[nJet]/F");
  tree->Branch("jet_phi", jet_phi, "jet_phi[nJet]/F");
  tree->Branch("jet_area", jet_area, "jet_area[nJet]/F");
  tree->Branch("jet_jec", jet_jec, "jet_jec[nJet]/F");
  tree->Branch("jet_btag", jet_btag, "jet_btag[nJet]/F");
  tree->Branch("jet_clean", jet_clean, "jet_clean[nJet]/B");

  tree->Branch("jet_nhf", jet_nhf, "jet_nhf[nJet]/F");
  tree->Branch("jet_nef", jet_nef, "jet_nef[nJet]/F");
  tree->Branch("jet_chf", jet_chf, "jet_chf[nJet]/F");
  tree->Branch("jet_muf", jet_muf, "jet_muf[nJet]/F");
  tree->Branch("jet_elef", jet_elef, "jet_elef[nJet]/F");
  tree->Branch("jet_numconst", jet_numconst, "jet_numconst[nJet]/F");
  tree->Branch("jet_numneutral", jet_numneutral, "jet_numneutral[nJet]/F");
  tree->Branch("jet_chmult", jet_chmult, "jet_chmult[nJet]/F");

  tree->Branch("met_pt", &met_pt, "met_pt/F");
  tree->Branch("met_px", &met_px, "met_px/F");
  tree->Branch("met_py", &met_py, "met_py/F");
  tree->Branch("met_sumet", &met_sumet, "met_sumet/F");
  tree->Branch("met_phi", &met_phi, "met_phi/F");

  tree->Branch("pupmet_pt", &pupmet_pt, "pupmet_pt/F");
  tree->Branch("pupmet_px", &pupmet_px, "pupmet_px/F");
  tree->Branch("pupmet_py", &pupmet_py, "pupmet_py/F");
  tree->Branch("pupmet_sumet", &pupmet_sumet, "pupmet_sumet/F");
  tree->Branch("pupmet_phi", &pupmet_phi, "pupmet_phi/F");

  tree->Branch("nMETUncert", &nMETUncert, "nMETUncert/I");
  tree->Branch("met_shiftedpx", met_shiftedpx, "met_shiftedpx[nMETUncert]/F");
  tree->Branch("met_shiftedpy", met_shiftedpy, "met_shiftedpy[nMETUncert]/F");
  tree->Branch("pupmet_shiftedpx", pupmet_shiftedpx, "pupmet_shiftedpx[nMETUncert]/F");
  tree->Branch("pupmet_shiftedpy", pupmet_shiftedpy, "pupmet_shiftedpy[nMETUncert]/F");
}

// ------------ method called for each event  ------------
void ZDilepton::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {

  //------------ Lepton Pt Filter ------------//

  edm::Handle< edm::View<pat::Muon> > muons;
  iEvent.getByToken(muonTag_, muons);

  nMuon = muons->size();

  edm::Handle< edm::View<pat::Electron> > electrons;
  iEvent.getByToken(electronTag_, electrons);

  nEle = electrons->size();

  vector<reco::CandidatePtr> leps;

  for (int i=0; i<nMuon && i<2; i++) leps.push_back( muons->ptrAt(i) );
  for (int i=0; i<nEle && i<2; i++) leps.push_back( electrons->ptrAt(i) );

  if (leps.size() < 2) return;

  sort(leps.begin(), leps.end(), sortLepPt);

  if (leps[0]->pt() < minLepPt_ || leps[1]->pt() < minSubLepPt_) return;

  vector<reco::CandidatePtr> lep0Sources, lep1Sources;
  for (unsigned int i=0, n=leps[0]->numberOfSourceCandidatePtrs(); i<n; i++) lep0Sources.push_back(leps[0]->sourceCandidatePtr(i));
  for (unsigned int i=0, n=leps[1]->numberOfSourceCandidatePtrs(); i<n; i++) lep1Sources.push_back(leps[1]->sourceCandidatePtr(i));

  //------------ MET Filters ------------//

  if (metFilters_){

    filter_failed.assign(nFilters+2, false);

    Handle<edm::TriggerResults> patFilterHandle;
    iEvent.getByToken(patTrgLabel_, patFilterHandle);

    if (patFilterHandle.isValid()){
      const edm::TriggerNames& trigNames = iEvent.triggerNames(*patFilterHandle);

      //for (unsigned int i=0; i<trigNames.size(); i++) cout << trigNames.triggerName(i) << endl;

      for (int i=0; i<nFilters; i++){

        if( filters[i] == "Flag_eeBadScFilter" && isMC_ ) continue;

        const unsigned int trig = trigNames.triggerIndex(filters[i]);
        if (trig != trigNames.size()){
          if (!patFilterHandle->accept(trig)) filter_failed[i] = true;
        }
        else cout << filters[i] << " not found." << endl;
      }
    }

    edm::Handle<bool> ifilterbadChCand;
    iEvent.getByToken(BadChCandFilterToken_, ifilterbadChCand);
    bool filterbadChCandidate = *ifilterbadChCand;

    if (!filterbadChCandidate) filter_failed[nFilters] = true;

    edm::Handle<bool> ifilterbadPFMuon;
    iEvent.getByToken(BadPFMuonFilterToken_, ifilterbadPFMuon);
    bool filterbadPFMuon = *ifilterbadPFMuon;

    if (!filterbadPFMuon) filter_failed[nFilters+1] = true;
  }

  //------------ Event Info ------------//

  if (isMC_){
    edm::Handle<GenEventInfoProduct> genEventHandle;
    iEvent.getByToken(genEventTag_, genEventHandle);
    genweight = genEventHandle->weight();

    edm::Handle< edm::View<PileupSummaryInfo> > pileups;
    iEvent.getByToken(muTag_, pileups);

    mu = pileups->at(1).getTrueNumInteractions();
  }
  else{
    run = int(iEvent.id().run());
    lumi = int(iEvent.getLuminosityBlock().luminosityBlock());
    bx = iEvent.bunchCrossing();
    event = iEvent.id().event();

    mu = getAvgPU( run, lumi );
  }

  //------------ Rho ------------//

  edm::Handle<double> rhoHandle;
  iEvent.getByToken(rhoTag_, rhoHandle);
  rho = *rhoHandle;

  //------------ Primary Vertices ------------//

  edm::Handle< edm::View<reco::Vertex> > primaryVertices;
  iEvent.getByToken(pvTag_, primaryVertices);

  nPV = primaryVertices->size();
  reco::Vertex pvtx = primaryVertices->at(0);

  //--------------Generated Particles-------------//

  if(isMC_){
    edm::Handle< edm::View<reco::GenParticle> > genParticles;
    iEvent.getByToken(genParticleTag_, genParticles);

    nGen = genParticles->size();

    for (int i=0; i<nGen; i++){
      reco::GenParticle p = genParticles->at(i);

      gen_status[i] = p.status();
      gen_PID[i] = p.pdgId();
      gen_pt[i]  = p.pt();
      gen_mass[i] = p.mass();
      gen_eta[i] = p.eta();
      gen_phi[i] = p.phi();
      gen_nMothers[i] = p.numberOfMothers();
      gen_nDaughters[i] = p.numberOfDaughters();

      if (gen_nMothers[i] > 0) gen_mother0[i] = p.motherRef(0).key();
      else gen_mother0[i] = -1;
      if (gen_nMothers[i] > 1) gen_mother1[i] = p.motherRef(1).key();
      else gen_mother1[i] = -1;
    }

    edm::Handle< edm::View<reco::GenJet> > genJets;
    iEvent.getByToken(genJetTag_, genJets);

    nGenJet = genJets->size();

    for (int i=0; i<nGenJet; i++){
      reco::GenJet jet = genJets->at(i);

      genJet_pt[i]  = jet.pt();
      genJet_eta[i] = jet.eta();
      genJet_phi[i] = jet.phi();
      genJet_area[i] = jet.jetArea();
      genJet_nDaught[i] = jet.numberOfDaughters();
    }
  }

  //--------------Muons-------------//

  nMuon = muons->size();

  for (int i=0; i<nMuon; i++){
    pat::Muon muon = muons->at(i);

    muon_isGlob[i] = muon.isGlobalMuon();
    muon_charge[i] = muon.charge();
    muon_type[i] = muon.type();
    muon_pt[i] = muon.pt();
    muon_eta[i] = muon.eta();
    muon_phi[i] = muon.phi();

    muon_D0[i] = muon.muonBestTrack()->dxy(pvtx.position());
    muon_Dz[i] = muon.muonBestTrack()->dz(pvtx.position());
    muon_tspm[i] = muon.combinedQuality().chi2LocalPosition;
    muon_kinkf[i] = muon.combinedQuality().trkKink;

    if (!isMC_) muon_segcom[i] = muon::segmentCompatibility(muon);

    if (muon_isGlob[i]) muon_chi2[i] = muon.globalTrack()->normalizedChi2();
    else                muon_chi2[i] = -1;

    muon_IsLooseID[i] = muon.isLooseMuon();
    muon_IsMediumID[i] = muon.isMediumMuon();
    muon_IsTightID[i] = muon.isTightMuon(pvtx);   
  }

  //------------ Electrons ------------//

  nEle = electrons->size();

  edm::Handle<edm::ValueMap<bool> > Veto_id_decisions;
  iEvent.getByToken(eleVetoIdMapToken_, Veto_id_decisions);

  edm::Handle<edm::ValueMap<bool> > loose_id_decisions;
  iEvent.getByToken(eleLooseIdMapToken_, loose_id_decisions);

  edm::Handle<edm::ValueMap<bool> > Medium_id_decisions;
  iEvent.getByToken(eleMediumIdMapToken_, Medium_id_decisions);

  edm::Handle<edm::ValueMap<bool> > Tight_id_decisions;
  iEvent.getByToken(eleTightIdMapToken_, Tight_id_decisions);

  for (int i=0; i<nEle; i++){
    pat::Electron ele = electrons->at(i);
    const Ptr<pat::Electron> elPtr(electrons, i);

    ele_charge[i] = ele.charge();
    ele_pt[i] = ele.pt();
    ele_eta[i] = ele.eta();
    ele_phi[i] = ele.phi();

    ele_D0[i] = ele.gsfTrack()->dxy(pvtx.position());
    ele_Dz[i] = ele.gsfTrack()->dz(pvtx.position());
    ele_sigmaIetaIeta[i] = ele.full5x5_sigmaIetaIeta();

    if ( ele.superCluster().isNonnull() && ele.superCluster()->seed().isNonnull() )
      ele_dEtaSeed[i] = ele.deltaEtaSuperClusterTrackAtVtx() - ele.superCluster()->eta() + ele.superCluster()->seed()->eta();
    else ele_dEtaSeed[i] = -99;

    ele_dPhiSeed[i] = abs(ele.deltaPhiSuperClusterTrackAtVtx());

    ele_dEtaIn[i] = ele.deltaEtaSuperClusterTrackAtVtx();
    ele_dPhiIn[i] = ele.deltaPhiSuperClusterTrackAtVtx();
    ele_overEoverP[i] = abs(1.0 - ele.eSuperClusterOverP()) / ele.ecalEnergy();

    if (!isMC_){
      ele_HE[i] = ele.hadronicOverEm();
      ele_missinghits[i] = ele.gsfTrack()->hitPattern().numberOfHits(reco::HitPattern::MISSING_INNER_HITS);

      reco::GsfElectron::PflowIsolationVariables pfIso = ele.pfIsolationVariables();
      float abseta =  abs(ele.superCluster()->eta());
      float eA = ele_areas_.getEffectiveArea(abseta);
      ele_rcpiwec[i] = ( pfIso.sumChargedHadronPt + max( 0.0f, pfIso.sumNeutralHadronEt + pfIso.sumPhotonEt - eA*rho) ) / ele_pt[i];
    }

    ele_VetoID[i]   = (*Veto_id_decisions)[ elPtr ];
    ele_LooseID[i]  = (*loose_id_decisions)[ elPtr ];
    ele_MediumID[i]  = (*Medium_id_decisions)[ elPtr ];
    ele_TightID[i]  = (*Tight_id_decisions)[ elPtr ];
  }

  //------------ Jets ------------//

  edm::Handle< edm::View<pat::Jet> > jets;
  iEvent.getByToken(jetTag_, jets);

  nJet = jets->size();

  for (int i=0; i<nJet; i++){

    pat::Jet jet = jets->at(i).correctedJet(0);
    reco::Candidate::LorentzVector jet_p4 = jet.p4();

    if ( reco::deltaR(jet, *leps[0]) < 0.4 || reco::deltaR(jet, *leps[1]) < 0.4 ){

      const vector<reco::CandidatePtr> & dvec = jet.daughterPtrVector();
      for (vector<reco::CandidatePtr>::const_iterator i_d = dvec.begin(); i_d != dvec.end(); ++i_d){

        if ( find(lep0Sources.begin(), lep0Sources.end(), *i_d ) != lep0Sources.end() ) {jet_p4 -= (*i_d)->p4(); jet_clean[i] = 'l';}
        else if ( find(lep1Sources.begin(), lep1Sources.end(), *i_d ) != lep1Sources.end() ) {jet_p4 -= (*i_d)->p4(); jet_clean[i] = 's';}
      }
    }
    if (jet_clean[i] != 'l' && jet_clean[i] != 's') jet_clean[i] = 'n';

    jet_pt[i] = jet_p4.Pt();
    jet_eta[i] = jet_p4.Eta();
    jet_phi[i] = jet_p4.Phi();
    jet_area[i] = jet.jetArea();
    jet_jec[i] = jets->at(i).jecFactor(0);

    jet_nhf[i] = jet.neutralHadronEnergyFraction();
    jet_nef[i] = jet.neutralEmEnergyFraction();
    jet_chf[i] = jet.chargedHadronEnergyFraction();
    jet_muf[i] = jet.muonEnergyFraction();
    jet_elef[i] = jet.chargedEmEnergyFraction();
    jet_numconst[i] = jet.chargedMultiplicity()+jet.neutralMultiplicity();
    jet_numneutral[i] =jet.neutralMultiplicity();
    jet_chmult[i] = jet.chargedMultiplicity();
    jet_btag[i] = jet.bDiscriminator(btag_);
  }

  //------------ MET ------------//

  edm::Handle< edm::View<pat::MET> > mets;
  iEvent.getByToken(metTag_, mets);

  pat::MET met = mets->at(0);

  if (isMC_){
    const reco::GenMET *genmet = met.genMET();

    genmet_pt = genmet->pt();
    genmet_px = genmet->px();
    genmet_py = genmet->py();
    genmet_sumet = genmet->sumEt();
    genmet_phi = genmet->phi();
  }

  met_pt = met.uncorPt();
  met_px = met.uncorPx();
  met_py = met.uncorPy();
  met_sumet = met.uncorSumEt();
  met_phi = met.uncorPhi();

  edm::Handle< edm::View<pat::MET> > pupmets;
  iEvent.getByToken(metPuppiTag_, pupmets);

  pat::MET pupmet = pupmets->at(0);
  pupmet_pt = pupmet.uncorPt();
  pupmet_px = pupmet.uncorPx();
  pupmet_py = pupmet.uncorPy();
  pupmet_sumet = pupmet.uncorSumEt();
  pupmet_phi = pupmet.uncorPhi();

  nMETUncert = METUNCERT;

  for (int i=0; i<nMETUncert; i++){

    pat::MET::METUncertainty uncert = static_cast<pat::MET::METUncertainty>(i);
    pat::MET::METCorrectionLevel level = pat::MET::Type1;

    met_shiftedpx[i] = met.shiftedPx(uncert, level);
    met_shiftedpy[i] = met.shiftedPy(uncert, level);
    pupmet_shiftedpx[i] = pupmet.shiftedPx(uncert, level);
    pupmet_shiftedpy[i] = pupmet.shiftedPy(uncert, level);
  }

  //------------ Triggers ------------//

  edm::Handle<edm::TriggerResults> triggerResults;
  iEvent.getByToken(triggerResultsTag_, triggerResults);
  
  edm::Handle<pat::PackedTriggerPrescales> triggerPrescales;
  iEvent.getByToken(prescalesTag_, triggerPrescales);
    
  if (triggerResults.isValid()){
    const edm::TriggerNames& triggerNames = iEvent.triggerNames(*triggerResults); 
    
    for (size_t i=0, n=triggerResults->size(); i<n; i++){
      string name = triggerNames.triggerName(i);
      size_t end = string::npos;

      if (name.find("HLT")==end || name.find("PF")!=end || name.find("Photon")!=end || name.find("MET")!=end || name.find("eta")!=end
           || name.find("Multiplicity")!=end || name.find("WP")!=end || name.find("Iso")!=end || name.find("Jpsi")!=end || name.find("DZ")!=end
             || name.find("Upsilon")!=end || name.find("Eta")!=end || name.find("Vtx")!=end || name.find("Jet")!=end || name.find("Displaced")!=end
               || (name.find("_Mu")==end && name.find("_Ele")==end && name.find("_DoubleMu")==end && name.find("_DoubleEle")==end) )
                 continue;
      
      unsigned int index = triggerNames.triggerIndex(name);
      bool failed = !triggerResults->accept(index);
      float prescale = triggerPrescales->getPrescaleForIndex(index);
      
      trig_prescale.push_back(prescale);  
      trig_failed.push_back(failed);
      trig_name.push_back(name);
    }
  }

  //------------ Fill Tree ------------//

  tree->Fill();
}


// ------------ method called once each job just after ending the event loop  ------------
void ZDilepton::endJob() {

  if (root_file !=0) {

    root_file->Write();
    delete root_file;
    root_file = 0;
  }

}

//define this as a plug-in
DEFINE_FWK_MODULE(ZDilepton);
