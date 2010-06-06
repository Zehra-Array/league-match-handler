<?php


require_once(".config/config.php");
require_once("lib/session.php");
require_once("lib/helpers.php");
require_once("lib/common.php");
require_once("lib/sql.php");
require_once("section/entermatch.php");

sqlConnect(SQL_CONFIG);


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
                       WHERE P.callsign = \"$data\"");
    $obj = mysql_fetch_object($res);
    if ($obj->team_id == 0 )  {
        echo "NOTEAM\n$data does not belong to any team and tried to declare a match ?!";
        return;
    }
    else  echo "TEAMFOUND\n$teama\n".$obj->teamname."\n$data has  declared that the team ".$obj->teamname." will play an official match too.
        \nThe match will be $teama versus ".$obj->teamname."\nThis match can still be canceled by using command /cancel";

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
                       WHERE P.callsign = \"$data\"");
    $obj = mysql_fetch_object($res);
    if ($obj->team_id == 0 )  {
        echo "NOTEAM\n$data does not belong to any team and tried to declare a match ?!";
        return;
    }
    else  echo "TEAMFOUND\n".$obj->teamname."\n$data has  declared that the team ".$obj->teamname." will play an official match.\n".$obj->teamname." is waiting for a challenger !!";
    return;
}

if ($action == "identTeam") {
    echo "identTeam\n";
    $player=$_POST['player'];
    $playerteam=$_POST['playerteam'];
    $teama=$_POST['teama'];

    if ($player=="" || $playerteam=="" ||$teama=="") {
        echo "NOTEAM";
        return;
    }
    $res = mysql_query("SELECT T.id team_id, T.name teamname, T.score score
                       FROM l_player P
                       JOIN l_team T ON P.team = T.id
                       WHERE P.callsign = \"$player\"");
    $obj = mysql_fetch_object($res);
    if ($obj->team_id == 0 )  {
        echo "NOTEAM";
        return;
    }
    else  {
        if ($teama == $obj->teamname) $teamnum="TEAMA";
        else $teamnum="TEAMB";
        echo "TEAMFOUND\n$playerteam\n$teamnum";
    }
    return;
}

if ($action == "spawn") {
    echo "spawn\n";
    $player=$_POST['player'];
    $teamb=$_POST['teamb'];
    $teama=$_POST['teama'];
    $playerid=$_POST['playerid'];
    if ($player=="" || $teamb=="" ||$teama=="" || $playerid=="" ) {
        echo "ERROR";
        return;
    }
    $res = mysql_query("SELECT T.id team_id, T.name teamname, T.score score
                       FROM l_player P
                       JOIN l_team T ON P.team = T.id
                       WHERE P.callsign = \"$player\"");
    $obj = mysql_fetch_object($res);
    if ($obj->team_id == 0 )  {
        echo "NOTEAM\n$playerid";
        return;
    }
    if ($teama != $obj->teamname  && $teamb != $obj->teamname) {
        echo "NOTEAM\n$playerid";
        return;
    }
    echo "TEAMOK";
    return;
}

if ($action == "entermatch") {
     echo "entermatch\n";
    $teamb=$_POST['teamb'];
    $teama=$_POST['teama'];
    $scoreb=$_POST['scoreb'];
    $scorea=$_POST['scorea'];
    $res = mysql_query("SELECT T.id team_id, T.name teamname, T.score score
                       FROM  l_team T WHERE T.name = \"$teama\"");
    $obj = mysql_fetch_object($res);
    $idteama = $obj->team_id;
    $zeloa = $obj->score;
    $res = mysql_query("SELECT T.id team_id, T.name teamname, T.score score
                       FROM  l_team T WHERE T.name = \"$teamb\"");
    $obj = mysql_fetch_object($res);
    $idteamb = $obj->team_id;
    $zelob = $obj->score;
    section_entermatch_calculateRating ($scorea, $scoreb, $zeloa , $zelob, &$newa, &$newb);
    entermatch_postIt ($idteama, $idteamb,$scorea,$scoreb, time());
    $vara=$newa-$zeloa;
    $varb=$newb-$zelob;
    echo  "$teama  $scorea - $scoreb  $teamb\n$teama => $newa ". (($vara>=0) ? "(+$vara)" : "($vara)")."\n$teamb => $newb ". (($varb>=0) ? "(+$varb)" : "($varb)") ;
}

function entermatch_postIt ($teamA,$teamB,$scoreA,$scoreB, $tsActUnix) {

    section_entermatch_orderResults (&$teamA, &$teamB, &$scoreA, &$scoreB);
    $rowA = queryGetTeam ($teamA);
    $rowB = queryGetTeam ($teamB);
    section_entermatch_calculateRating ($scoreA, $scoreB, $rowA->score, $rowB->score, &$newA, &$newB);

    // Insert data into MATCH table
    $tsActStr = date ("Y-m-d H:i:s", $tsActUnix);
    $now = gmdate("Y-m-d H:i:s");
    $s_playerid = 46;
    sqlQuery("insert into ". TBL_MATCH ."
             (team1, score1, team2, score2, tsactual, identer, tsenter,
             oldrankt1, oldrankt2, newrankt1, newrankt2)
             values($teamA, $scoreA, $teamB, $scoreB, '$tsActStr', $s_playerid, '$now',
             $rowA->score, $rowB->score, $newA, $newB)");

    if ($tsActUnix < section_entermatch_queryLastMatchTime ()) {
        section_entermatch_recalcAllRatings();
    } else {
        sqlQuery("update l_team SET score='$newA', active='yes' where id = '$teamA'");
        sqlQuery("update l_team SET score='$newB', active='yes' where id = '$teamB'");
    }

}


?>
