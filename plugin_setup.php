

<br />




<br />

<div id="EDMRDSsettings" class="settings">
<fieldset>
<legend>EDM RDS Settings</legend>

<p>RDS Station - Sent 8 characters at a time.  Max of 64 characters.<br />
Station Text: <?php PrintSettingTextSaved("StationText", 2, 0, 64, 32, "fpp-edmrds", "Merry   Christ- mas"); ?>

<br />

<p>RDS Text: <?php PrintSettingTextSaved("RDSTextText", 2, 0, 64, 32, "fpp-edmrds", "[{Artist} - {Title}]"); ?>
<p>
Place {Artist} or {Title} where the media artist/title should be placed. Area's wrapped in brackets ( [] ) will not be output unless media is present.


<p>Program Type (PTY North America / Europe): <?php PrintSettingSelect("Pty", "Pty", 2, 0, 2,
Array(
"0 - None / None"=>0, 
"1 - News / News"=>1, 
"2 - Information / Current Affairs"=>2, 
"3 - Sport / Information"=>3, 
"4 - Talk / Sport"=>4, 
"5 - Rock / Education"=>5, 
"6 - Classic Rock / Drama"=>6, 
"7 - Adult Hits / Culture"=>7, 
"8 - Soft Rock / Science"=>8, 
"9 - Top 40 / Varied"=>9, 
"10 - Country / Pop"=>10, 
"11 - Oldies / Rock"=>11, 
"12 - Soft Music / Easy Listening"=>12, 
"13 - Nostalgia / Light Classical"=>13, 
"14 - Jazz / Serious Classical"=>14, 
"15 - Classical / Other Music"=>15, 
"16 - R&B / Weather"=>16, 
"17 - Soft R&B / Finance"=>17, 
"18 - Language / Childrens"=>18, 
"19 - Religious Music / Social Affairs"=>19, 
"20 - Religious Talk / Religion"=>20, 
"21 - Personality / Phone-In"=>21, 
"22 - Public / Travel"=>22, 
"23 - College / Leisure"=>23, 
"24 - --- / Jazz"=>24, 
"25 - --- / Country"=>25, 
"26 - --- / National Music"=>26, 
"27 - --- / Oldies"=>27, 
"28 - --- / Folk"=>28, 
"29 - Weather / Documentary"=>29), 
"fpp-edmrds", ""); ?> - <a href="https://www.electronics-notes.com/articles/audio-video/broadcast-audio/rds-radio-data-system-pty-codes.php">Additional PTY information</a></p>
</fieldset>
</div>

<br />
