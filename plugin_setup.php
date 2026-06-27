<div id="edmrds" class="settings">
<fieldset>
<legend>EDM RDS Support</legend>

<p>Drives the MRDS192 RDS encoder inside an EDM FM transmitter over a bit-banged
I2C bus. Connect the RDS chip's SCL and SDA pins to two GPIOs (the Pi default is
GPIO 23 = SCL and GPIO 24 = SDA). On a BeagleBone, set the pins below to FPP pin
names (e.g. <code>P9-12</code>).</p>

<p>Hardware/RDS register details:
<a href="http://www.edmdesign.com/docs/EDM-TX-RDS.pdf" target="_blank">EDM-TX-RDS.pdf</a>,
<a href="http://pira.cz/rds/mrds192.pdf" target="_blank">mrds192.pdf</a>.
Tag your MP3/OGG files with Artist and Title and the radiotext updates
automatically as media plays.</p>

<?
PrintSettingGroup("edmrds", "", "", 1, "fpp-edmrds");
?>

<p>The <b>Station Name (PS)</b> is written when the plugin starts. Two FPP
commands are also registered for use in playlists/scheduled events:
<b>RDS Set Station Name</b> and <b>RDS Install</b> (the latter disables dynamic
PS and saves to the MRDS192 EEPROM &mdash; run once after wiring).</p>

<p>Restart FPPD after changing any setting for it to take effect.</p>

<p>To report a bug, file it against the
<a href="https://github.com/FalconChristmas/fpp-edmrds/issues/new" target="_blank">fpp-edmrds GitHub Issues</a>.</p>
</fieldset>
</div>
<br />
