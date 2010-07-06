/* Copyright 2010 Murielle Darc
 * Distributed under the terms of the GNU General Public License v2
 * Based in part upon a autreport plugin from Brad Wark.
 */

#include "bzfsAPI.h"
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "TextUtils.h"
#include <string.h>
#include <map>



bool official = false;
bool isTeamcoloridentified=false;
bool isofficialrequested=false;
bool isloaded=false;
std::string URL;
std::string HASH;
std::string TeamA;
std::string TeamB;
int eTeamA;
int eTeamB;
int scoreA;
int scoreB;
bool isGameRunning=false;
double lasttick;
double Totaltime_1vs1;
double Totaltime_mixed_team;
double Totaltime_even;
BZ_GET_PLUGIN_VERSION

typedef class trackplayer
{
public:
    bool isTeamA;
    bz_eTeamType eteam;
    double lastupdatetime;
    bool ispaused;
    trackplayer(bool isteama)
    {
        isTeamA=isteama;
        eteam=eNoTeam;
        lastupdatetime=-1.0f;
        ispaused=false;
    }

    void part()
    {
        eteam=eNoTeam;
        lastupdatetime=-1.0f;
        ispaused=false;
    }

} trackplayer;

std::map <std::string, trackplayer> MapPlayersData;

std::string tolower(const std::string& s)
{
	std::string trans = s;

	for (std::string::iterator i=trans.begin(), end=trans.end(); i!=end; ++i)
		*i = ::tolower(*i);
	return trans;
}


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
                k1=6;
                if ( tokens[4] == tokens[5] ) {
                    k1=10;
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s is not allowed to play an official match versus itself !",tokens[4].c_str());
                } else {
                    official=true;
                    TeamA=tokens[4];
                    TeamB=tokens[5];
                    std::vector<std::string> teamAplayers = TextUtils::tokenize(tokens[2],std::string("\t"),0,false);
                    std::vector<std::string> teamBplayers = TextUtils::tokenize(tokens[3],std::string("\t"),0,false);
                    MapPlayersData.clear();
                    for (int k=0;k<teamAplayers.size();k++) MapPlayersData.insert ( std::pair<std::string,trackplayer>(tolower(teamAplayers[k]),true) );
                    for (int k=0;k<teamBplayers.size();k++) MapPlayersData.insert ( std::pair<std::string,trackplayer>(tolower(teamBplayers[k]),false) );

                }
            }
        }


        if (tokens[0]==std::string("entermatch")) {
            k1=1;
        }

        if (tokens[0]==std::string("ladder")) {
            k1=2;
            int playerid;
            std::istringstream iss( tokens[1] );
            iss >>  playerid;
            for (int k=k1;k<tokens.size();k++) bz_sendTextMessagef(BZ_SERVER,playerid,"%s",tokens[k].c_str());
            k1=0;
        }

        if (tokens[0]==std::string("online")) {
            k1=2;
            int playerid;
            std::istringstream iss( tokens[1] );
            iss >>  playerid;
            for (int k=k1;k<tokens.size();k++) bz_sendTextMessagef(BZ_SERVER,playerid,"%s",tokens[k].c_str());
            k1=0;
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
    bool pauseState;

public:

    AutoReport()
    {
        saveTimeLimit=0;
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
    std::string encryptdata ( std::string data)
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
        if (eventData->eventType == bz_eTickEvent)
        {
            if (!official || !isGameRunning) return;
            bz_TickEventData *PlayerTickData = (bz_TickEventData *) eventData;
            double newtick=PlayerTickData->time;

            bzAPIIntList *PlayerIndexList= bz_newIntList();
            bz_getPlayerIndexList(PlayerIndexList);
            bool isteamAred=false;
            bool isteamBred=false;
            bool isteamAgreen=false;
            bool isteamBgreen=false;
            int nb_red_pause = 0;
            int nb_green_pause = 0;
            int nb_red_NR = 0;
            int nb_green_NR = 0;

            for (int k=0;k<PlayerIndexList->size();k++)
            {
                bz_PlayerRecord *player = bz_getPlayerByIndex (PlayerIndexList->get(k));
                std::map<std::string,trackplayer >::const_iterator it = MapPlayersData.find(tolower(player->callsign.c_str()));
                if ( it != MapPlayersData.end( ))
                {
                    /* check if team are mixed */
                    if (!isTeamcoloridentified)
                        if ((it->second.isTeamA) && (player->team == eRedTeam)) isteamAred=true;
                        else if ((it->second.isTeamA) && (player->team == eGreenTeam)) isteamAgreen=true;
                        else if ((!it->second.isTeamA) && (player->team == eRedTeam)) isteamBred=true;
                        else isteamBgreen=true;

                    /* check paused players */
                    if (  it->second.ispaused && it->second.eteam==eRedTeam)   nb_red_pause++;
                    if (  it->second.ispaused && it->second.eteam==eGreenTeam) nb_green_pause++;

                    /* check NR players */
                    if ( (newtick - it->second.lastupdatetime > 5.0f) && it->second.eteam==eRedTeam) nb_red_NR++;
                    if ( (newtick - it->second.lastupdatetime > 5.0f) && it->second.eteam==eGreenTeam) nb_green_NR++;

                }
                bz_freePlayerRecord(player);
            }
           
            double tempTimeElapsed = (bz_getCurrentTime() - matchStartTime);
            /* amount of time teams are mixed!!! */
            if ((isteamBred && isteamAred) || (isteamBgreen && isteamAgreen))
                Totaltime_mixed_team +=   newtick-lasttick;
            else Totaltime_mixed_team =0.0f;


            /* amount of time 1vs1 is forbiden!!! */
            int nb_Green = bz_getTeamCount(eGreenTeam)-nb_green_NR-nb_green_pause ;
            int nb_Red = bz_getTeamCount(eRedTeam)-nb_red_NR-nb_red_pause;


            if ( nb_Green <= 0 && nb_Red  <= 0 )   Totaltime_1vs1 += newtick-lasttick;
            else Totaltime_1vs1 =0.0f;


            /* amount of time of uneven teams */
            if ( nb_Green  != nb_Red )
                Totaltime_even += newtick-lasttick;
            else Totaltime_even =0.0f;
         //    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"even time %f => nbred %d  => nbgreen  %d", Totaltime_even, nb_Red ,nb_Green   );

            /* identify teams */
            if (!isTeamcoloridentified && Totaltime_mixed_team==0.0f && Totaltime_1vs1==0.0f && Totaltime_even==0.0f  && tempTimeElapsed>5.0f)
            {
                isTeamcoloridentified = true;
                if (isteamAred)
                {
                    eTeamA=eRedTeam;
                    eTeamB=eGreenTeam;
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"*************");
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"Green team is %s",TeamB.c_str());
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"Red team is %s",TeamA.c_str());
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"*************");
                }
                else
                {
                    eTeamA=eGreenTeam;
                    eTeamB=eRedTeam;
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"*************");
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"Green team is %s",TeamA.c_str());
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"Red team is %s",TeamB.c_str());
                    bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"*************");
                }
            }
            lasttick = newtick;

            /* check mixed team */
            if (Totaltime_mixed_team > 5.0f  &&  !pauseState)
            {
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"**************************");
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"Teams should not be mixed !!");
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"**************************");
                bz_pauseCountdown ( "SERVER");
                pauseState=true;
            }
            /* check uneven team */
            else if (Totaltime_even > 10.0f  &&  !pauseState)
            {
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"**********************");
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"Teams should be even !!");
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"*********************");
                bz_pauseCountdown ( "SERVER");
                pauseState=true;
            }
            /* check 1vs1 */
            else if (Totaltime_1vs1 > 10.0f  &&  !pauseState)
            {
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"******************************************");
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"Teams should not be in 1vs1 configuration !!");
                bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"*******************************************");
                bz_pauseCountdown ( "SERVER");
                pauseState=true;
            }
            return;
        }

        if (eventData->eventType == bz_eGetAutoTeamEvent)
        {
            if (!official || !(bz_isCountDownActive() || bz_isCountDownInProgress())) return;
            bz_GetAutoTeamEventData *GetAutoTeamEventData = (bz_GetAutoTeamEventData *) eventData;
            int playerID = GetAutoTeamEventData->playeID;           
            std::map<std::string,trackplayer >::iterator it = MapPlayersData.find(tolower(GetAutoTeamEventData->callsign.c_str()));
            if (GetAutoTeamEventData->team != eObservers)
            {
                if  (it == MapPlayersData.end( ))   GetAutoTeamEventData->team = eObservers;
                else 
                {
                     if (!isTeamcoloridentified)  it->second.eteam  = GetAutoTeamEventData->team;
                     else if (eTeamA==eRedTeam)
                     {
                        if ( it->second.isTeamA && GetAutoTeamEventData->team == eGreenTeam)  GetAutoTeamEventData->team = eRedTeam;
                        else if (!it->second.isTeamA && GetAutoTeamEventData->team == eRedTeam) GetAutoTeamEventData->team = eGreenTeam;
                     }
                     else if (eTeamA==eGreenTeam)
                     {
                        if( !it->second.isTeamA && GetAutoTeamEventData->team == eGreenTeam) GetAutoTeamEventData->team = eRedTeam; 
                        else if  (it->second.isTeamA && GetAutoTeamEventData->team == eRedTeam) GetAutoTeamEventData->team = eGreenTeam;
                     }
                     it->second.eteam  = GetAutoTeamEventData->team;
                 }
             }

        }

        if (eventData->eventType == bz_ePlayerPartEvent)
        {
            if (!official || !(bz_isCountDownActive() || bz_isCountDownInProgress())) return;
            bz_PlayerJoinPartEventData *PlayerPartData = (bz_PlayerJoinPartEventData *) eventData;
            std::map<std::string,trackplayer >::iterator it = MapPlayersData.find(tolower(PlayerPartData->callsign.c_str()));
            if ( it != MapPlayersData.end( ))   it->second.part();
        }

        if (eventData->eventType == bz_eSlashCommandEvent )
        {
            if (!official) return;

            bz_SlashCommandEventData *SlashCommandData = (bz_SlashCommandEventData *) eventData;

            bzAPIStringList msg;
            msg.tokenize(SlashCommandData->message.c_str()," ",2);
            if (msg.size() == 1 &&  msg.get(0) == "/gameover" &&  bz_hasPerm(SlashCommandData->from, "ENDGAME"))
            {
                bz_sendTextMessage(BZ_SERVER,BZ_ALLUSERS,"/gameover before the end of the match !!! Official is canceled...");
                official = false;
                isGameRunning=false;
                isTeamcoloridentified=false;
                isofficialrequested=false;
                TeamA="";
                TeamB="";
                return;

            }
            if (msg.size() == 2 && msg.get(0) == "/countdown" && bz_hasPerm(SlashCommandData->from, "COUNTDOWN"))
            {
                if ( msg.get(1) == "pause" && !pauseState ) pauseState=true;
                else if ( msg.get(1) == "resume" && pauseState )  pauseState=false;
            }
        }

        if (eventData->eventType == bz_eGameStartEvent)
        {
            if (!official) return;

            // save currently set timelimit to avoid collisions with other plugins that
            // manipulate the timelimit
            isGameRunning=true;
            matchStartTime = bz_getCurrentTime();
            saveTimeLimit = bz_getTimeLimit();
            lasttick=matchStartTime;
            Totaltime_1vs1=-1.0f;
            Totaltime_mixed_team=-1.0f;
            Totaltime_even=-1.0f;

            bzAPIIntList *PlayerIndexList= bz_newIntList();
            bz_getPlayerIndexList(PlayerIndexList);
            for (int k=0;k<PlayerIndexList->size();k++) {
                bz_PlayerRecord *player = bz_getPlayerByIndex (PlayerIndexList->get(k));
                // kick all the players that doesnt belong in one of the two teams
                std::map<std::string,trackplayer >::iterator it = MapPlayersData.find(tolower(player->callsign.c_str()));
                if (player->team != eObservers)
                {
                    if  (it == MapPlayersData.end( ))  bz_kickUser ( player->playerID, " An official match is currently running. Please, join as observer.", true );
                    else  it->second.eteam  = player->team;
                }
                bz_freePlayerRecord(player);
            }
        }

        if (eventData->eventType == bz_eGameEndEvent)
        {
            if (!official||!isGameRunning) return;
            isGameRunning=false;

            if (isTeamcoloridentified)
            {
                scoreA = bz_getTeamLosses(convertTeam (eTeamB));
                scoreB=  bz_getTeamLosses(convertTeam (eTeamA));

                bz_addURLJob(URL.c_str(), &myURL, ("&action=entermatch&teama="+encryptdata(TeamA)
                                                   +"&teamb=" + encryptdata(TeamB)
                                                   +"&scorea="+ encryptdata(scoreA)
                                                   +"&scoreb="+ encryptdata(scoreB)
                                                   +"&hash=" + encryptdata(HASH)
                                                   +"&mlen="+encryptdata((int)saveTimeLimit/60)).c_str());
            }
            else bz_sendTextMessage(BZ_SERVER,BZ_ALLUSERS,"Teams were not identified so match has not been entered :(");

            official = false;
            isTeamcoloridentified=false;
            isofficialrequested=false;
            TeamA="";
            TeamB="";
            return;
        }



        if (eventData->eventType == bz_ePlayerUpdateEvent)
        {
            if (!official||!isGameRunning) return;

            bz_PlayerUpdateEventData *PlayerUpdateEventData = (bz_PlayerUpdateEventData *) eventData;
            int playerID = PlayerUpdateEventData->playerID;
            bz_PlayerRecord *player = bz_getPlayerByIndex(playerID);

            std::map<std::string,trackplayer >::iterator it = MapPlayersData.find(tolower(player->callsign.c_str()));
            if ( it != MapPlayersData.end( ) && player->team != eObservers)   it->second.lastupdatetime = PlayerUpdateEventData->time;
            bz_freePlayerRecord(player);
            return;
        }


        if (eventData->eventType == bz_ePlayerPausedEvent)
        {
            if (!official||!isGameRunning) return;

            bz_PlayerPausedEventData *PlayerPausedEventData = (bz_PlayerPausedEventData *) eventData;
            int playerID = PlayerPausedEventData->player;
            bz_PlayerRecord *player = bz_getPlayerByIndex(playerID);

            std::map<std::string,trackplayer >::iterator it = MapPlayersData.find(tolower(player->callsign.c_str()));
            if ( it != MapPlayersData.end( ))
            {
                if (PlayerPausedEventData->pause) it->second.ispaused = true;
                else it->second.ispaused = false;
            }
            bz_freePlayerRecord(player);
            return;
        }

        return;
    }

    virtual bool handle ( int playerID, bzApiString command, bzApiString /*message*/, bzAPIStringList* /*params*/ ) {
        bz_PlayerRecord *player = bz_getPlayerByIndex(playerID);
        if (!player)
            return true;

        if ((!player->hasPerm("AUTOREPORT"))) {
            bz_sendTextMessage(BZ_SERVER,playerID,"Only allowed players can use that command.");
            bz_freePlayerRecord(player);
            return true;
        }


        if (command == "official") {
            if (bz_isCountDownActive() || bz_isCountDownInProgress()) {
                bz_sendTextMessage(BZ_SERVER,playerID,"You can not use that command during a match.");
                bz_freePlayerRecord(player);
                return true;
            }

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
            if (bz_isCountDownActive() || bz_isCountDownInProgress()) {
                bz_sendTextMessage(BZ_SERVER,playerID,"You can not use that command during a match.");
                bz_freePlayerRecord(player);
                return true;
            }

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
                bz_freePlayerRecord(player);
                return true;
            }
            bz_sendTextMessage(BZ_SERVER,playerID,(std::string("There is no nothing to be canceled")).c_str());
            bz_freePlayerRecord(player);
            return true;
        }


        if (command == "ladder") {
            bz_addURLJob(URL.c_str(), &myURL, ("&action=ladder&player="+ encryptdata(playerID)).c_str());
            bz_freePlayerRecord(player);
            return true;
        }


        if (command == "online") {
            bz_addURLJob(URL.c_str(), &myURL, ("&action=online&player="+ encryptdata(playerID)).c_str());
            bz_freePlayerRecord(player);
            return true;
        }


    }

};

AutoReport autoReport;

BZF_PLUGIN_CALL int bz_Load (const char* commandLine)
{
    std::vector<std::string> tokens = TextUtils::tokenize(commandLine,std::string(","),0,false);


    isloaded=false;
    if (tokens.size() == 2) {
        URL = tokens[0];
        HASH = tokens[1];
//        bz_registerEvent(bz_ePlayerJoinEvent,&autoReport);
        bz_registerEvent(bz_ePlayerPartEvent,&autoReport);
        bz_registerEvent(bz_eGetAutoTeamEvent,&autoReport);
        bz_registerEvent(bz_ePlayerPausedEvent,&autoReport);
        bz_registerEvent(bz_ePlayerUpdateEvent,&autoReport);
        bz_registerEvent(bz_eGameStartEvent,&autoReport);
        bz_registerEvent(bz_eSlashCommandEvent,&autoReport);
        bz_registerEvent(bz_eGameEndEvent,&autoReport);
        bz_registerEvent(bz_eTickEvent,&autoReport);
        bz_registerCustomSlashCommand ( "cancel", &autoReport  );
        bz_registerCustomSlashCommand ( "official" , &autoReport );
        bz_registerCustomSlashCommand ( "ladder", &autoReport  );
        bz_registerCustomSlashCommand ( "online" , &autoReport );
        bz_debugMessage(4, "autoReport Plugin loaded");
        isloaded=true;
    }
    return 0;
}

BZF_PLUGIN_CALL int bz_Unload (void)
{
    if (isloaded) {
        bz_removeCustomSlashCommand ( "official" );
        bz_removeCustomSlashCommand ( "cancel" );
        bz_removeCustomSlashCommand ( "ladder" );
        bz_removeCustomSlashCommand ( "online" );
        bz_removeEvent(bz_eTickEvent,&autoReport);
        bz_removeEvent(bz_eGameStartEvent,&autoReport);
//        bz_removeEvent(bz_ePlayerJoinEvent,&autoReport);
        bz_removeEvent(bz_eGetAutoTeamEvent,&autoReport);
        bz_removeEvent(bz_ePlayerPartEvent,&autoReport);
        bz_removeEvent(bz_ePlayerPausedEvent,&autoReport);
        bz_removeEvent(bz_ePlayerUpdateEvent,&autoReport);
        bz_removeEvent(bz_eGameEndEvent,&autoReport);
        bz_removeEvent(bz_eSlashCommandEvent,&autoReport);
        bz_debugMessage(4, "autoReport Plugin Unloaded");
    }
    return 0;
}

