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
                       WHERE P.callsign = \"".mysql_real_escape_string($_POST['data'])."\"");
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
                       WHERE P.callsign = \"".mysql_real_escape_string($_POST['data'])."\"");
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
                       WHERE P.callsign = \"".mysql_real_escape_string($_POST['player'])."\"");
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
                       WHERE P.callsign = \"".mysql_real_escape_string($_POST['player'])."\"");
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
        $s_playerid = $obj->id;
        section_entermatch_postIt ($idteama, $idteamb,$scorea,$scoreb, time(),$mlen,$s_playerid);
        $vara=$newa-$zeloa;
        $varb=$newb-$zelob;
        echo  "$teama  $scorea - $scoreb  $teamb\n$teama => $newa ". (($vara>=0) ? "(+$vara)" : "($vara)")."\n$teamb => $newb ". (($varb>=0) ? "(+$varb)" : "($varb)") ;

    }
    else echo  "The match has not been reported\nIt seems this server has not received the authorizations\nfor such an operation on the league ...";
    }

?>
