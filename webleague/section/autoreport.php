<?php //
function section_autoreportadmin () {
    section_autoreport_autoreport ();
}

function section_autoreport () {
    section_autoreport_autoreport ();
}

function section_autoreport_permissions()  {
    return array(
               'ipfilter' => 'Add/Edit autoreport ip filtering table'
           );
}


function section_autoreport_autoreport () {

    if (isFuncAllowed('autoreport::ipfilter')) {

        if ($_POST['action'] == 'add' && $_POST['f_ok_x']=='Submit' ) {
            sqlQuery ('insert into l_autoreport (servername, ip) VALUES("'.$_POST['servername'].'", "'. $_POST['serverip'].'")' );
        }
        if ($_POST['action'] == 'edit' && $_POST['f_ok_x']=='Submit' ) {
            sqlQuery ('UPDATE  l_autoreport  SET servername = "'.$_POST['servername'] .'", ip = "'.$_POST['serverip'].'" WHERE id="'. $_POST['id'].'"');
        }
        if ($_GET['action']=="remove") { 
              sqlQuery ('DELETE FROM  l_autoreport WHERE id='.$_GET['id']); 
              unset($_GET['action']);
        } 

        $res = sqlQuery('SELECT A.servername name, A.ip ip, A.id id FROM l_autoreport A ');

        if (!$_GET['action']) {
            $rownum=0;
            echo "<center><table><tr valign='top'><td><table border=0 cellspacing=0 cellpadding=1><tr class=menuHead><td> Server name &nbsp; &nbsp;</td><td>ip</td><td colspan=2>Actions</td></tr>";
            while ($obj = mysql_fetch_object($res)) {
                $c = $rownum++%2?'rowodd':'roweven';
                echo "<tr><td  class=$c>".  $obj->name."</td><td  class=$c>".$obj->ip."</td><td>".htmlURLbutton ('Remove', 'autoreport', 'action=remove&id='.$obj->id)."</td><td> ".htmlURLbutton ('Edit', 'autoreport', 'action=edit&id='.$obj->id)."</td></tr>";
            }
            echo "</table></td><td>".htmlURLbutton ('New entry', 'autoreport','action=add', ADMBUT)."</td></tr></table></center>";
        }

        if ($_GET['action']=="add") {
            echo '<FORM Method="POST" Action="index.php">';
            echo '<input type=hidden name=link value=autoreport>';
            echo '<input type=hidden name=action value=add>';
            echo '<center><table>';
            echo '<TR><TD align=right  > <b>Server name : &nbsp; </b></td> <TD><INPUT type=text name=servername size=40 maxlength=40  ></td></tr>';
            echo '<TR><TD align=right  > <b>Server IP : &nbsp; </b></td> <TD><INPUT type=text name=serverip size=40 maxlength=40  ></td></tr>';
            echo '</table><br>'. htmlFormButton ('Submit', 'f_ok_x') . '&nbsp;&nbsp;' . htmlFormButton ('Cancel', 'f_cancel_x', CLRBUT) .'</center>';
            echo '</FORM>';
        }

        if ($_GET['action']=="edit") { 
            $obj->id='-1';
            while ($obj->id != $_GET['id']) $obj = mysql_fetch_object($res);
            echo '<FORM Method="POST" Action="index.php">';
            echo '<input type=hidden name=link value=autoreport>';
            echo '<input type=hidden name=action value=edit>';
            echo '<input type=hidden name=id value="'.$obj->id.'">';
            echo '<center><table>';
            echo '<TR><TD align=right  > <b>Server name : &nbsp; </b></td> <TD><INPUT type=text name=servername size=40 maxlength=40 value="'.$obj->name.'" ></td></tr>';
            echo '<TR><TD align=right  > <b>Server IP : &nbsp; </b></td> <TD><INPUT type=text name=serverip size=40 maxlength=40  value="'.$obj->ip.'"></td></tr>';
            echo '</table><br>'. htmlFormButton ('Submit', 'f_ok_x') . '&nbsp;&nbsp;' . htmlFormButton ('Cancel', 'f_cancel_x', CLRBUT) .'</center>';
            echo '</FORM>';
        }


    }
}
?>


