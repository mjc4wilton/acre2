#include "Engine.h"
#include "NamedPipeServer.h"
#include "AcreSettings.h"

#include "getClientID.h"
#include "setVoiceCurveModel.h"
#include "updateSpeakingData.h"
#include "ext_remoteStartSpeaking.h"
#include "ext_remoteStopSpeaking.h"
#include "ext_handleGetClientID.h"
#include "getPluginVersion.h"
#include "localMute.h"
#include "setMuted.h"
#include "startRadioSpeaking.h"
#include "stopRadioSpeaking.h"
#include "setPTTKeys.h"
#include "loadSound.h"
#include "playSound.h"
#include "setSoundSystemMasterOverride.h"
#include "ext_reset.h"
#include "ping.h"
#include "updateSelf.h"
#include "setSelectableVoiceCurve.h"
#include "setSetting.h"
#include "setTs3ChannelDetails.h"


ACRE_RESULT CEngine::initialize(IClient *const ac_client, IServer *const ac_externalServer, const std::string ac_fromPipeName, const std::string ac_toPipeName) {

    if (!g_Log) {
        g_Log = (Log *)new Log("acre2.log");
        LOG("* Logging engine initialized.");
    }
    LOG("Configuration Path: {%s\\acre2.ini}", ac_client->getConfigFilePath().c_str());
    CAcreSettings::getInstance()->load(ac_client->getConfigFilePath() + "\\acre2.ini");

    this->setClient(ac_client);
    this->setExternalServer(ac_externalServer);

    this->m_gameServer = new CNamedPipeServer(ac_fromPipeName, ac_toPipeName);
    this->m_soundEngine = new CSoundEngine();
    this->m_rpcEngine = new CRpcEngine();
    //this->m_KeyHandlerEngine = new CKeyHandlerEngine();
    this->m_soundPlayback = new CSoundPlayback();
    this->setSoundSystemOverride(FALSE);

    this->setSelf(new CSelf());

    this->getRpcEngine()->addProcedure(new getClientID());
    this->getRpcEngine()->addProcedure(new setVoiceCurveModel());
    this->getRpcEngine()->addProcedure(new updateSpeakingData());
    this->getRpcEngine()->addProcedure(new ext_remoteStartSpeaking());
    this->getRpcEngine()->addProcedure(new ext_remoteStopSpeaking());
    this->getRpcEngine()->addProcedure(new ext_handleGetClientID());
    this->getRpcEngine()->addProcedure(new getPluginVersion());
    this->getRpcEngine()->addProcedure(new localMute());
    this->getRpcEngine()->addProcedure(new setMuted());
    this->getRpcEngine()->addProcedure(new startRadioSpeaking());
    this->getRpcEngine()->addProcedure(new stopRadioSpeaking());
    this->getRpcEngine()->addProcedure(new setPTTKeys());
    this->getRpcEngine()->addProcedure(new loadSound());
    this->getRpcEngine()->addProcedure(new playLoadedSound());
    this->getRpcEngine()->addProcedure(new setSoundSystemMasterOverride());
    this->getRpcEngine()->addProcedure(new ext_reset());
    this->getRpcEngine()->addProcedure(new ping());
    this->getRpcEngine()->addProcedure(new updateSelf());
    this->getRpcEngine()->addProcedure(new setSelectableVoiceCurve());
    this->getRpcEngine()->addProcedure(new setSetting());
    this->getRpcEngine()->addProcedure(new setTs3ChannelDetails());

    // Initialize the client, because it never was derp
    this->getClient()->initialize();

    return ACRE_OK;
}

ACRE_RESULT CEngine::initialize(IClient *const ac_client, IServer *const ac_externalServer, const std::string ac_fromPipeName, const std::string ac_toPipeName, const std::string ac_loggingPath) {

    g_Log = (Log *) new Log(const_cast<char *>(ac_loggingPath.c_str()));
    LOG("* Logging engine initialized.");

    return initialize(ac_client, ac_externalServer, ac_fromPipeName, ac_toPipeName);
}

ACRE_RESULT CEngine::start(const ACRE_ID ac_id) {
    if (this->getExternalServer()) {
        this->getExternalServer()->initialize();
    } else {
        return ACRE_ERROR;
    }

    if (this->getGameServer()) {
        this->getGameServer()->initialize();
    } else {
        return ACRE_ERROR;
    }
    /*
    if (this->getKeyHandlerEngine()) {
        this->getKeyHandlerEngine()->initialize();
    } else {
        return ACRE_ERROR;
    }
    */
    this->getClient()->unMuteAll();
    this->setState(ACRE_STATE_RUNNING);
    return ACRE_OK;
}

ACRE_RESULT CEngine::stop() {
    LOG("Engine Shutting Down");
    this->setState(ACRE_STATE_STOPPING);

    if (this->getClient()) {
        this->getClient()->enableMicrophone(true);        // unmute local microphone on stopping
        this->getClient()->unMuteAll();
    }

    if (this->getRpcEngine()) {
        this->getRpcEngine()->stopWorker();
    }

    if (this->getGameServer()) {
        this->getGameServer()->shutdown();
    }

    if (this->getExternalServer()) {
        this->getExternalServer()->shutdown();
    }

    CAcreSettings::getInstance()->save();

    this->setState(ACRE_STATE_STOPPED);
    LOG("Engine Shutdown Complete");
    return ACRE_OK;
}

ACRE_RESULT CEngine::localStartSpeaking(const ACRE_SPEAKING_TYPE speakingType) {
    this->localStartSpeaking(speakingType, "");
    return ACRE_OK;
}

ACRE_RESULT CEngine::localStartSpeaking(const ACRE_SPEAKING_TYPE speakingType, const std::string &ac_radioId) {
    // send a start speaking event to everyone
    TRACE("Local START speaking: %d, %s", speakingType, ac_radioId.c_str());
    this->getSelf()->lock();
    this->getSelf()->setCurrentRadioId(ac_radioId);
    this->getSelf()->setSpeakingType(speakingType);
    this->getSelf()->clearSoundChannels();

    CEngine::getInstance()->getExternalServer()->sendMessage(
        CTextMessage::formatNewMessage("ext_remoteStartSpeaking",
            "%d,%d,%s,%d,%s,%f,",
            this->getSelf()->getId(),
            this->getSelf()->getCurrentLanguageId(),
            this->getSelf()->getNetId().c_str(),
            this->getSelf()->getSpeakingType(),
            this->getSelf()->getCurrentRadioId().c_str(),
            this->getSelf()->getSelectableCurveScale()
        )
    );

    // Send the local start speaking event to ourselves, so we can play squawk, etc in game.
    CEngine::getInstance()->getGameServer()->sendMessage( CTextMessage::formatNewMessage("localStartSpeaking",
        "%d,%d,%d,%s,",
        this->getSelf()->getId(),
        this->getSelf()->getCurrentLanguageId(),
        this->getSelf()->getSpeakingType(),
        this->getSelf()->getCurrentRadioId().c_str()
        )
    );

    this->getSelf()->setSpeaking(TRUE);
    this->getSelf()->unlock();
    return ACRE_OK;
}

ACRE_RESULT CEngine::localStopSpeaking( void ) {
    this->getSelf()->setSpeaking(FALSE);
    CEngine::getInstance()->getExternalServer()->sendMessage(
        CTextMessage::formatNewMessage("ext_remoteStopSpeaking",
            "%d,%s,",
            this->getSelf()->getId(),
            this->getSelf()->getNetId().c_str()
        )
    );

    CEngine::getInstance()->getGameServer()->sendMessage( CTextMessage::formatNewMessage("localStopSpeaking",
        "%d,%d,%s,",
        this->getSelf()->getId(),
        this->getSelf()->getSpeakingType(),
        this->getSelf()->getCurrentRadioId().c_str()
        )
    );


    return ACRE_OK;
}

ACRE_RESULT CEngine::remoteStartSpeaking(const ACRE_ID ac_remoteId, const int32_t ac_languageId, const std::string &ac_netId, const ACRE_SPEAKING_TYPE ac_speakingType, const std::string &ac_radioId, const ACRE_VOLUME ac_curveScale) {
    TRACE("Remote Start Speaking Enter: %d, %d", ac_remoteId, ac_speakingType);
    auto it = this->speakingList.find(ac_remoteId);
    if (it != this->speakingList.end()) {
        //ghetto rig the remote players curveScale updates
        it->second->setSelectableCurveScale(ac_curveScale);
        return ACRE_OK;
    }
    CPlayer *remotePlayer = new CPlayer(ac_remoteId);
    this->speakingList.insert(std::pair<ACRE_ID, CPlayer *>(ac_remoteId, remotePlayer));
    remotePlayer->setSpeakingType(ac_speakingType);
    remotePlayer->setSelectableCurveScale(ac_curveScale);
    remotePlayer->setCurrentRadioId(ac_radioId);
    remotePlayer->setNetId(ac_netId);

    CEngine::getInstance()->getGameServer()->sendMessage( CTextMessage::formatNewMessage("remoteStartSpeaking",
        "%d,%d,%s,%d,%s,",
        ac_remoteId,
        ac_languageId,
        ac_netId.c_str(),
        ac_speakingType,
        ac_radioId.c_str()
        )
    );

    return ACRE_OK;
}

ACRE_RESULT CEngine::remoteStopSpeaking(const ACRE_ID ac_remoteId) {
    TRACE("Remote STOP Speaking Enter: %d", ac_remoteId);
    auto it = this->speakingList.find(ac_remoteId);
    if (it != this->speakingList.end()) {
        CPlayer *remotePlayer = (CPlayer *)it->second;

        CEngine::getInstance()->getGameServer()->sendMessage( CTextMessage::formatNewMessage("remoteStopSpeaking",
            "%d,%s,%d,%s,",
            remotePlayer->getId(),
            remotePlayer->getNetId().c_str(),
            remotePlayer->getSpeakingType(),
            remotePlayer->getCurrentRadioId().c_str()
            )
        );

        this->speakingList.erase(it);
        if (remotePlayer) {
            delete remotePlayer;
        }
    }
    return ACRE_OK;
}
