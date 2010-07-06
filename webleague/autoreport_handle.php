<?php


require_once(".config/config.php");
require_once("lib/session.php");
require_once("lib/helpers.php");
require_once("lib/common.php");
require_once("lib/sql.php");
require_once("section/entermatch.php");

sqlConnect(SQL_CONFIG);
define (FIELD_SCORE,          '(ts.won * s.points_win + ts.draw *  s.points_draw + ts.lost * s.points_lost)');
define (QUERY_RANKINGORDER,   'ORDER BY tscore desc, ts.zelo desc, ts.won desc, ts.draw desc, ts.matches asc');


if (isset($_POST['action'])) {
    $action=$_POST['action'];
}


if ($action == "challenger") {
    echo "challenger\n";
    $teama=$_POST['teama'];
    $data=$_POST['data'];
    if ($teama==""||$data=="") {
        echo "NOTEAM\nWe failed to declare a match with this player";
        return;
    }
    $res = mysql_query("SELECT T.id team_id, T.name teamname, T.score score
                       FROM l_player P
                       JOIN l_team T ON P.team = T.id
                       WHERE P.callsign = \"".mysql_real_escape_string($_POST['data'])."\"");
    $obj = mysql_fetch_object($res);
    if ($obj->team_id == 0 )  {
        echo "NOTEAM\n$data does not belong to any team and tried to declare a match ?!";
        return;
    }
    else  
    {
         echo "TEAMFOUND\n";
         $res_Team= mysql_query('SELECT l_player.callsign FROM (l_player, l_team) WHERE l_player.team=l_team.id AND l_team.name="'.mysql_real_escape_string($teama).'"');
         while ($obj_Team = mysql_fetch_object($res_Team)) echo "$obj_Team->callsign\t";
         echo "\n";
         $res_Team= mysql_query("SELECT l_player.callsign FROM (l_player, l_team) WHERE l_player.team=l_team.id AND l_team.name=\"$obj->teamname\"");
         while ($obj_Team = mysql_fetch_object($res_Team)) echo "$obj_Team->callsign\t";
         echo "\n$teama\n".$obj->teamname."\n$data has  declared that the team ".$obj->teamname." will play an official match too.
        \nThe match will be $teama versus ".$obj->teamname."\nThis match can still be canceled by using command /cancel";
    }
    return;
}

if ($action == "firstdeclare") {
    echo "firstdeclare\n";
    $data=$_POST['data'];
    if ($data=="") {
        echo "NOTEAM\nWe failed to declare a match with this player";
        return;
    }
    $res = mysql_query("SELECT T.id team_id, T.name teamname, T.score score
                       FROM l_player P
                       JOIN l_team T ON P.team = T.id
                       WHERE P.callsign = \"".mysql_real_escape_string($_POST['data'])."\"");
    $obj = mysql_fetch_object($res);
    if ($obj->team_id == 0 )  {
        echo "NOTEAM\n$data does not belong to any team and tried to declare a match ?!";
        return;
    }
    else  echo "TEAMFOUND\n".$obj->teamname."\n$data has  declared that the team ".$obj->teamname." will play an official match.\n".$obj->teamname." is waiting for a challenger !!";
    return;
}


if ($action == "entermatch") {
    echo "entermatch\n";
    $teamb=$_POST['teamb'];
    $teama=$_POST['teama'];
    $scoreb=$_POST['scoreb'];
    $scorea=$_POST['scorea'];
    $hash=$_POST['hash'];
    $mlen=$_POST['mlen'];
    $remoteip=gethostbyname(gethostbyaddr($_SERVER["REMOTE_ADDR"]));
    $res = mysql_query("SELECT A.hostname hostname
                       FROM  l_autoreport A WHERE A.hash = \"".mysql_real_escape_string($_POST['hash'])."\"");
    $obj = mysql_fetch_object($res);
    $ISOK=false;
    if ($obj->hostname != "") {
        foreach (gethostbynamel($obj->hostname) as $hostip)
        if ($hostip==$remoteip) $ISOK=true;
    }
    if ($ISOK) {
        $res = mysql_query("SELECT T.id team_id, T.name teamname, T.score score
                           FROM  l_team T WHERE T.name = \"".mysql_real_escape_string($_POST['teama'])."\"");
        $obj = mysql_fetch_object($res);
        $idteama = $obj->team_id;
        $zeloa = $obj->score;
        $res = mysql_query("SELECT T.id team_id, T.name teamname, T.score score
                           FROM  l_team T WHERE T.name = \"".mysql_real_escape_string($_POST['teamb'])."\"");
        $obj = mysql_fetch_object($res);
        $idteamb = $obj->team_id;
        $zelob = $obj->score;
        $res = mysql_query("SELECT l_player.id id  FROM  l_player  WHERE l_player.callsign = \"autoreport\"");
        $obj = mysql_fetch_object($res);
        $_SESSION['playerid'] =  $obj->id;
        section_entermatch_calculateRating ($scorea, $scoreb, $zeloa , $zelob, &$newa, &$newb);
        $vara=$newa-$zeloa;
        $varb=$newb-$zelob;
        ob_start();
        section_entermatch_postIt ($idteama, $idteamb,$scorea,$scoreb, time(),$mlen);
        ob_end_clean();
        echo  "$teama  $scorea - $scoreb  $teamb\n$teama => $newa ". (($vara>=0) ? "(+$vara)" : "($vara)")."\n$teamb => $newb ". (($varb>=0) ? "(+$varb)" : "($varb)") ;

    }
    else echo  "The match has not been reported\nIt seems this server has not received the authorizations\nfor such an operation on the league ...";
}

if ($action == "ladder") {
    echo "ladder\n";
    echo $_POST['player']."\n";
    $season = null;
    $now = nowDateTime();
    $season = sqlQuerySingle("select * from l_season where startdate <= '$now' and fdate >= '$now'");
    if ($season == null) $season = sqlQuerySingle("select * from l_season where active = 'yes' and id > 1");
    if ($season == null) $season = sqlQuerySingle("select * from l_season where enddate <= '$now' and id > 1 order by enddate desc limit 1");
    if ($season)  $season_id = $season->id;
    else $season_id = 0;

    $res = sqlQuery("SELECT ts.team, " . FIELD_SCORE . " tscore, ts.won,ts.lost,ts.draw, ts.zelo szelo,ts.matches,
                    team.name, team.leader,team.score gzelo,team.status,
                    p.callsign
                    FROM (l_teamscore ts, l_season s)
                    left join l_team team on team.id = ts.team
                    left join l_player p on p.id = team.leader
                    WHERE ts.season = $season_id
                    AND   s.id = $season_id
                    GROUP BY ts.team
                    " . QUERY_RANKINGORDER);

    printf("rank    Team name                                 Score\n");
    $irank=0;
    while ($obj = mysql_fetch_object($res)) {
        printf("%2d.     %-40s  %d\n",++$irank,$obj->name,$obj->tscore);
    }

    return;
}

if ($action == "online") {
    echo "online\n";
    echo $_POST['player']."\n";
    if (($content=file_get_contents('http://bzstats.strayer.de/stuff/ShowDown2.php'))!==false) {
        $lines=explode("\n",$content);
        if (count($lines)<2) {
            printf("Currently, no data has been received from strayer\n");
        }
        else {
            $lines=explode("\n",$content,-1);
            $ind=0;
            foreach($lines as $record)
            {
                $val=explode("\t", $record);
                $tab->name[$ind]=$val[0];
                $tab->color[$ind]=$val[1];
                $tab->server[$ind++]=$val[2].":".$val[3];
            }
            $res_team = mysql_query("SELECT l_team.id,l_team.name FROM `l_team`");
            $numonline=0;
            printf(" \n");
            while ($obj_team = mysql_fetch_object($res_team))
            {
                $res = mysql_query("SELECT l_player.callsign FROM `l_player` WHERE l_player.team=$obj_team->id");
                while ($obj = mysql_fetch_object($res))
                {
                    $index = array_keys($tab->name, $obj->callsign);
                    foreach ($index as $key)
                    {
                        printf("%-20s %-20s %-9s %s\n",$obj_team->name, $obj->callsign, $tab->color[$key], $tab->server[$key]);
                        $numonline++;
                    }
                }
            }
            if ($numonline == 0) printf("No online player :(\n");
            printf(" \n");
            
        }
    }
    return;
}


?>
