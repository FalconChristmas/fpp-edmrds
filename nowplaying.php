<?php
  echo 'Now Playing:';
  echo exec("python /home/fpp/media/plugins/fpp-edmrds/rds-song.py -n 2>&1 ");
  echo '<br>';
  echo 'Station ID:';
  echo exec("python /home/fpp/media/plugins/fpp-edmrds/rds-song.py -l 2>&1 ");
?>
