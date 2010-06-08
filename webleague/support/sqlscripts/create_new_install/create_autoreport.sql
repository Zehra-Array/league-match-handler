drop table if exists l_autoreport;
CREATE TABLE  l_autoreport(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `description` varchar(80) NOT NULL,
  `hostname` varchar(80) NOT NULL,
  `hash` varchar(80) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

