#include "bzfsAPI.h"
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "TextUtils.h"
#include <string.h>
/* experimental plugin by Murielle Darc */

bool official = false;
bool isTeamcoloridentified=false;
bool isofficialrequested=false;
std::string URL;
std::string TeamA;
std::string TeamB;
int eTeamA;
int eTeamB;
int scoreA;
int scoreB;

BZ_GET_PLUGIN_VERSION

class MyURLHandler: public bz_URLHandler
{
public:
    std::string page;
    virtual void done ( const char* /*URL*/, void * data, unsigned int size, bool complete )
    {
        char *str = (char*)malloc(size+1);
        int k1=0;
        memcpy(str,data,size);
        str[size] = 0;

        page += str;
        free(str);
        if (!complete)
            return;
        std::vector<std::string> tokens = TextUtils::tokenize(page,std::string("\n"),0,false);
        page="";


        if (tokens[0]==std::string("firstdeclare")) {
            k1=2;
            if (tokens[1]!=std::string("NOTEAM"))  {
                k1=3;
                isofficialrequested=true;
                TeamA=tokens[2];
            }
        }



        if (tokens[0]==std::string("challenger")) {
            k1=2;
            if (tokens[1]!=std::string("NOTEAM"))  {
                k1=4;
                if ( tokens[2] == tokens[3] ) {
                    k1=8;
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s is not allowed to play an official match versus itself !",tokens[2].c_str());
                } else {
                    official=true;
                    TeamA=tokens[2];
                    TeamB=tokens[3];
                }
            }
        }

        if (tokens[0]==std::string("identTeam")) {
            k1=2;
            if (tokens[1]!=std::string("NOTEAM")) {
                k1=4;
                isTeamcoloridentified=true;
                if (tokens[3]==std::string("TEAMA")) {
                    std::istringstream iss( tokens[2] );
                    iss >>    eTeamA;
                    if (eTeamA == eRedTeam) eTeamB=eGreenTeam;
                    else eTeamB=eRedTeam;
                } else {
                    std::istringstream iss( tokens[2] );
                    iss >>  eTeamB;
                    if (eTeamB == eRedTeam) eTeamA=eGreenTeam;
                    else eTeamA=eRedTeam;
                }

            }
        }

        if (tokens[0]==std::string("spawn")) {
            k1=2;
            if (tokens[1]==std::string("NOTEAM")) {
                k1=3;
                int playerid;
                std::istringstream iss( tokens[2] );
                iss >>  playerid;
                bz_kickUser ( playerid, "An official match is currently running", true );
            }
        }

        if (tokens[0]==std::string("entermatch")) {
            k1=1;
        }

        if (k1!=0) {
            for (int k=k1;k<tokens.size();k++) bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s",tokens[k].c_str());
        }

    }
};

MyURLHandler myURL;

class AutoReport : public bz_EventHandler, public bz_CustomSlashCommandHandler
{

private: 

    float saveTimeLimit;
    double matchStartTime;
    double matchEndTime;
    double pauseStartTime;
    double pauseEndTime;
    bool pauseState;

public:

    AutoReport()
    {
         saveTimeLimit=0;
	 matchStartTime=0;
	 matchEndTime=0;
	 pauseStartTime=0;
	 pauseEndTime=0;
	 pauseState=false;
    }

    std::string encryptdata ( bzApiString data)
    {
        char hex[5];
        std::string data1;
        for (int i=0;  i < (int) data.size(); i++) {
            char c = data.c_str()[i];
            data1+='%';
            sprintf(hex, "%-2.2X", c);
            data1.append(hex);
        }
        return data1;

    }

    std::string encryptdata ( int idata)
    {
        char hex[5];
        std::ostringstream oss;
        oss << idata;
        std::string data = oss.str();
        std::string data1;
        for (int i=0;  i < (int) data.size(); i++) {
            char c = data.c_str()[i];
            data1+='%';
            sprintf(hex, "%-2.2X", c);
            data1.append(hex);
        }
        return data1;

    }

    virtual void process ( bz_EventData *eventData )
    {
        if (eventData->eventType == bz_ePlayerSpawnEvent)
        {
            if (!official) return;
            bz_PlayerSpawnEventData *PlayerSpawnData = (bz_PlayerSpawnEventData *) eventData;
            int playerID = PlayerSpawnData->playerID;
            bz_PlayerRecord *player = bz_getPlayerByIndex(playerID);
            bz_addURLJob(URL.c_str(), &myURL, ("&action=spawn&teama="+encryptdata(TeamA)
                                               +"&teamb=" + encryptdata(TeamB)
                                               +"&player="+ encryptdata(player->callsign)
                                               +"&playerid="+encryptdata(playerID)).c_str());
            bz_freePlayerRecord(player);
            return;
        }

        if (eventData->eventType == bz_eSlashCommandEvent )
	{
            if (!official) return;

            bz_SlashCommandEventData *SlashCommandData = (bz_SlashCommandEventData *) eventData;

	    bzAPIStringList msg;
	    msg.tokenize(SlashCommandData->message.c_str()," ",2);

	    if (msg.size() == 2 &&  msg.get(0) == "/countdown" &&  bz_hasPerm(SlashCommandData->from, "COUNTDOWN"))
	    {
		if ( msg.get(1)  == "pause" && !pauseState ) {
		  pauseStartTime = bz_getCurrentTime();
		  //bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"debug:: pause - %f", pauseStartTime);
		  pauseState=true;
		}
		else if ( msg.get(1)  == "resume" && pauseState ) {
		  pauseEndTime = bz_getCurrentTime();
		  if ( bz_getBZDBString("_countdownResumeDelay").size() )
		    pauseEndTime += atoi(bz_getBZDBString("_countdownResumeDelay").c_str());

		  //bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"debug:: resume - %f - %f", bz_getCurrentTime(), pauseEndTime);
		  pauseState=false;
		}
	    }
	}

        if (eventData->eventType == bz_eGameStartEvent)
	{
	  // save currently set timelimit to avoid collisions with other plugins that 
	  // manipulate the timelimit

	  saveTimeLimit = bz_getTimeLimit();
	  matchStartTime = bz_getCurrentTime();

	}

        if (eventData->eventType == bz_eGameEndEvent)
        {
            if (!official) return;

	    matchEndTime = bz_getCurrentTime();
	    double newTimeElapsed = (matchEndTime - matchStartTime) + (pauseEndTime - pauseStartTime);
            float timeLeft = saveTimeLimit - newTimeElapsed - 1;

            bz_debugMessagef(2, "DEBUG:: newTimeElapsed => %f timeLeft => %f",newTimeElapsed, timeLeft);

            if (timeLeft>0) {
                bz_sendTextMessage(BZ_SERVER,BZ_ALLUSERS,"/gameover before the end of the match !!! Official is canceled...");
                official = false;
                isTeamcoloridentified=false;
                isofficialrequested=false;
                TeamA="";
                TeamB="";
                return;
            }
            if (!isTeamcoloridentified) {
                scoreA = 0;
                scoreB = 0;
            }
            else {
                scoreA = bz_getTeamLosses(convertTeam (eTeamB));
                scoreB=  bz_getTeamLosses(convertTeam (eTeamA));
            }
            bz_addURLJob(URL.c_str(), &myURL, ("&action=entermatch&teama="+encryptdata(TeamA)
                                               +"&teamb=" + encryptdata(TeamB)
                                               +"&scorea="+ encryptdata(scoreA)
                                               +"&scoreb="+ encryptdata(scoreB)).c_str());
            official = false;
            isTeamcoloridentified=false;
            isofficialrequested=false;
            TeamA="";
            TeamB="";
            return;
        }
        if ((eventData->eventType == bz_eCaptureEvent) && (!isTeamcoloridentified))
        {
            bz_CTFCaptureEventData *CTFCaptureData = (bz_CTFCaptureEventData *) eventData;
            bz_PlayerRecord *playercapping = bz_getPlayerByIndex( CTFCaptureData->playerCapping);
            bz_eTeamType teamplayercapping = playercapping->team;

            bz_addURLJob(URL.c_str(), &myURL,
                         ("&action=identTeam&player="+encryptdata(playercapping->callsign)
                          +"&playerteam=" + encryptdata(teamplayercapping)
                          +"&teama="+encryptdata(TeamA)).c_str());
            bz_freePlayerRecord(playercapping);
            return;

        }
        return;
    }

    virtual bool handle ( int playerID, bzApiString command, bzApiString /*message*/, bzAPIStringList* /*params*/ ) {
        bz_PlayerRecord *player = bz_getPlayerByIndex(playerID);
        if (!player)
            return true;

        if ((!player->hasPerm(bz_perm_spawn)) || (player->team == eObservers)) {
            bz_sendTextMessage(BZ_SERVER,playerID,"Only players can use that command.");
            bz_freePlayerRecord(player);
            return true;
        }

        if (bz_isCountDownActive() || bz_isCountDownInProgress()) {
            bz_sendTextMessage(BZ_SERVER,playerID,"You can not use that command during a match.");
            bz_freePlayerRecord(player);
            return true;
        }

        if (command == "official") {
            if (official) {
                bz_sendTextMessage(BZ_SERVER,playerID,"An official match is already  planned :");
                bz_sendTextMessage(BZ_SERVER,playerID, (TeamA +" vs. " + TeamB).c_str());
                bz_sendTextMessage(BZ_SERVER,playerID,std::string("This match can still be canceled by using command /cancel").c_str());
                bz_freePlayerRecord(player);
                return true;
            }
            if (isofficialrequested ) {
                bz_addURLJob(URL.c_str(), &myURL, ("&action=challenger&teama="+encryptdata(TeamA)
                                                   +"&data=" + encryptdata(player->callsign)).c_str());
                bz_freePlayerRecord(player);
                return true;
            }
            bz_addURLJob(URL.c_str(), &myURL, ("&action=firstdeclare&data=" + encryptdata(player->callsign)).c_str());
            bz_freePlayerRecord(player);
            return true;
        }
        if (command == "cancel") {
            if (official) {
                bz_sendTextMessage(BZ_SERVER,BZ_ALLUSERS,("The match between " + TeamA +" and " + TeamB + " has been canceled by " +(std::string(player->callsign.c_str()))).c_str());
                bz_freePlayerRecord(player);
                official = false;
                isTeamcoloridentified=false;
                isofficialrequested=false;
                TeamA="";
                TeamB="";
                return true;
            }
            if (isofficialrequested ) {
                bz_sendTextMessage(BZ_SERVER,BZ_ALLUSERS,("The match declaration of "+ TeamA + " has been canceled by "+ std::string(player->callsign.c_str())).c_str());
                official = false;
                isTeamcoloridentified=false;
                isofficialrequested=false;
                TeamA="";
                TeamB="";
                return true;
            }
            bz_sendTextMessage(BZ_SERVER,playerID,(std::string("There is no nothing to be canceled")).c_str());
            return true;
        }


    }

};

AutoReport autoReport;

BZF_PLUGIN_CALL int bz_Load (const char* commandLine)
{
    bz_debugMessage(4, "autoReport Plugin Loaded");
    URL = commandLine;
    bz_registerEvent(bz_eCaptureEvent,&autoReport);
    bz_registerEvent(bz_ePlayerSpawnEvent,&autoReport);
    bz_registerEvent(bz_eGameStartEvent,&autoReport);
    bz_registerEvent(bz_eGameEndEvent,&autoReport);
    bz_registerEvent(bz_eSlashCommandEvent,&autoReport);
    bz_registerCustomSlashCommand ( "cancel", &autoReport  );
    bz_registerCustomSlashCommand ( "official" , &autoReport );
    bz_debugMessage(4, "autoReport Plugin loaded");
    return 0;
}

BZF_PLUGIN_CALL int bz_Unload (void)
{

    bz_removeCustomSlashCommand ( "official" );
    bz_removeCustomSlashCommand ( "cancel" );
    bz_removeEvent(bz_ePlayerSpawnEvent,&autoReport);// for controlling who is joining during game for protecting the match
    bz_removeEvent(bz_eGameStartEvent,&autoReport);// for counting time at end of game if a gameover occured
    bz_removeEvent(bz_eGameEndEvent,&autoReport);// for counting time at end of game if a gameover occured
    bz_removeEvent(bz_eCaptureEvent,&autoReport);// for counting time at end of game if a gameover occured
    bz_removeEvent(bz_eSlashCommandEvent,&autoReport);// for counting time at end of game if a gameover occured
    bz_debugMessage(4, "autoReport Plugin Unloaded");
    return 0;
}




