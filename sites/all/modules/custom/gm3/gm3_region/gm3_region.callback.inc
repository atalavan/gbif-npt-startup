<?php

/**
 * Get the region_id from the lat,lng and zoom
 */
function gm3_region_get_region_id_from_latlng($latlng, $iso_codes, $level){
  drupal_add_http_header('Content-Type', 'application/json; charset=utf-8');
  // Because the TDWG data and Google data don't quite match, here we convert
  // some of the codes sent to us by Google.
  $convert_codes = array(
    'GB' => 'UK',
    'RS' => 'YU',
    'ME' => 'MK',
    'SS' => 'SD',
    'UNKNOWN' => array(
      'YU',
      'IN'
    )
  );
  if(isset($convert_codes[$iso_codes])){
    $iso_codes = $convert_codes[$iso_codes];
  }
  if(!is_array($iso_codes)){
    $iso_codes = array(
      $iso_codes
    );
  }
  // First we try to get a single hit using the MySQL geo extension.
  $latlng = preg_replace("/[)(]/", "", $latlng);
  $latlng = explode(", ", $latlng);
  $point_wkt = "POINT({$latlng[1]} {$latlng[0]})";
  $results = db_query("SELECT CONCAT(level_1_code, ':', level_2_code, ':', level_3_code, ':', level_3_code, '-', level_4_code) as code 
                       FROM {gm3_region_data} WHERE WITHIN(POINTFROMTEXT('$point_wkt'), mysql_polygons) AND level_4_code != ''", array(
    ':iso_code' => $iso_codes,
    ':point_wkt' => $point_wkt
  ));
  // FIXME - The following code caused issues when trying to select Sardinia
  // (which is actually marked as Corsica in the TDWG data set).  Until the bug
  // with the dataset, or with MySQL is fixed, this code can not be used.
  /*if($results->rowCount() > 1){
    $results = db_query("SELECT CONCAT(level_1_code, ':', level_2_code, ':', level_3_code, ':', level_3_code, '-', level_4_code) as code
        FROM {gm3_region_data} WHERE WITHIN(POINTFROMTEXT(:point_wkt), mysql_polygons) AND level_4_code != '' AND iso_code IN (:iso_code)", array(
      ':iso_code' => $iso_codes,
      ':point_wkt' => $point_wkt
    ));
  }*/
  if($results->rowCount() == 1){
    // We've had success with our trimmed down method. WOOT!
    $results = $results->fetch();
    if(arg(4) < 4){
      $code_parts = explode(':', $results->code);
      for($i = arg(4); $i < 4; $i++){
        array_pop($code_parts);
      }
      $results->code = implode(":", $code_parts);
    }
    print json_encode($results->code);
    // Perform end-of-request tasks.
    ajax_footer();
    exit();
  }else if($results->rowCount() > 1){
    $results = db_query("SELECT * FROM {gm3_region_data} WHERE WITHIN(POINTFROMTEXT(:point_wkt), mysql_polygons) AND level_4_code != '' AND iso_code IN (:iso_code)", array(
      ':iso_code' => $iso_codes,
      ':point_wkt' => $point_wkt
    ));
    // Load the Library.
    gm3_load_geophp();
    $wkt_reader = new WKT();
    // Create a point object from the sent data.
    $point = geoPHP::load($point_wkt, 'wkt');
    foreach($results as $result){
      $polygon = $wkt_reader->read($result->polygons, TRUE);
      if($point->within($polygon)){
        switch(arg(4)){
          case 4:
            $region_id = "{$result->level_1_code}:{$result->level_2_code}:{$result->level_3_code}:{$result->level_3_code}-{$result->level_4_code}";
            break;
          case 3:
            $region_id = "{$result->level_1_code}:{$result->level_2_code}:{$result->level_3_code}";
            break;
          case 2:
            $region_id = "{$result->level_1_code}:{$result->level_2_code}";
            break;
          case 1:
            $region_id = "{$result->level_1_code}";
            break;
        }
        print json_encode($region_id);
        // Perform end-of-request tasks.
        ajax_footer();
        exit();
      }
    }
  }
  // Load the Library.
  gm3_load_geophp();
  $wkt_reader = new WKT();
  // Create a point object from the sent data.
  $point = geoPHP::load($point_wkt, 'wkt');
  foreach($iso_codes as $iso_code){
    // Loop through each Level 1 region.
    $region_id = '';
    $level4s = db_select('gm3_region_data', 'g')->fields('g')->condition('level_4_code', '', '!=')->condition('iso_code', $iso_code, 'LIKE')->execute();
    $level_2_codes = array();
    foreach($level4s as $level4){
      $level_2_codes[$level4->level_2_code] = $level4->level_2_code;
      $polygon = $wkt_reader->read($level4->polygons, TRUE);
      if($point->within($polygon)){
        $region_id = "{$level4->level_1_code}:{$level4->level_2_code}:{$level4->level_3_code}:{$level4->level_3_code}-{$level4->level_4_code}";
        // Stop on the first match.
        break;
      }
    }
    if(!$region_id){
      // Due to a fuck up with the TDWG data, we can't restrict by the
      // iso_code
      // being null, so instead we search by the level_2_code.
      foreach($level_2_codes as $level_2_code){
        $level4s = db_select('gm3_region_data', 'g')->fields('g')->condition('level_4_code', '', '!=')->condition('level_2_code', $level_2_code)->execute();
        foreach($level4s as $level4){
          $polygon = $wkt_reader->read($level4->polygons, TRUE);
          if($point->within($polygon)){
            $region_id = "{$level4->level_1_code}:{$level4->level_2_code}:{$level4->level_3_code}:{$level4->level_3_code}-{$level4->level_4_code}";
            // stop on the first match
            break 2;
          }
        }
      }
    }
    if($region_id){
      break;
    }
  }
  if($region_id && $level < 4){
    $region_id = explode(":", $region_id);
    for($i = 4; $i > $level; $i--){
      array_pop($region_id);
    }
    $region_id = implode(":", $region_id);
  }
  print json_encode($region_id);
  // Perform end-of-request tasks.
  ajax_footer();
  exit();
}

/**
 * gm3_region_get_points
 */
function gm3_region_get_points($region_ids){
  drupal_add_http_header('Content-Type', 'application/json; charset=utf-8');
  $region_ids = explode(",", $region_ids);
  print "[";
  $first = TRUE;
  foreach($region_ids as $region_id){
    if($first){
      $first = FALSE;
    }else{
      print ",";
    }
    $cid = $region_id;
    $polygons = cache_get($cid, 'cache_gm3_polygon');
    if($polygons === false){
      module_load_include('functions.inc', 'gm3');
      $region_id_parts = explode(":", $region_id);
      // Load the geoPHP library.
      gm3_load_geophp();
      $wkt_reader = new WKT();
      switch(count($region_id_parts)){
        case 4:
          $region_id_4_parts = explode("-", $region_id_parts[3]);
          $row = db_select('gm3_region_data', 'g')->condition('level_4_code', $region_id_4_parts[1])->condition('level_3_code', $region_id_4_parts[0])->fields('g')->execute()->fetch();
          break;
        case 3:
          $row = db_select('gm3_region_data', 'g')->condition(db_or()->condition('level_3_code', $region_id_parts[2])->condition('name', $region_id_parts[2]))->condition('level_4_code', '')->fields('g')->execute()->fetch();
          break;
        case 2:
          if(is_numeric($region_id_parts[1])){
            $row = db_select('gm3_region_data', 'g')->condition('level_2_code', $region_id_parts[1])->condition('level_3_code', '')->fields('g')->execute()->fetch();
          }else{
            $row = db_select('gm3_region_data', 'g')->condition('name', $region_id_parts[1])->condition('level_3_code', '')->fields('g')->execute()->fetch();
          }
          break;
        case 1:
          if(is_numeric($region_id_parts[0])){
            $row = db_select('gm3_region_data', 'g')->condition('level_1_code', $region_id_parts[0])->condition('level_2_code', 0)->fields('g')->execute()->fetch();
          }else{
            $row = db_select('gm3_region_data', 'g')->condition('name', $region_id_parts[0])->condition('level_2_code', 0)->fields('g')->execute()->fetch();
          }
          break;
      }
      if($row){
        $matches = array();
        preg_match_all('/\.[0-9]{1,20}/', $row->polygons, $matches);
        $matches = $matches[0];
        usort($matches, 'gm3_region_sort_by_length');
        $replaces = array();
        foreach($matches as $match){
          $replaces[] = substr($match, 0, 5); // Accurate to 4 decimal places, or 11 metres.
        }
        $row->polygons = str_replace($matches, $replaces, $row->polygons);
        $polygons = $wkt_reader->read($row->polygons, TRUE);
        $polygons = $polygons->out('json');
        $polygons = '{"' . $region_id . '":{"name":"' . $row->name . '","shape":' . $polygons . '}}';
      }
      cache_set($cid, $polygons, 'cache_gm3_polygon', CACHE_PERMANENT);
    }else{
      $polygons = $polygons->data;
    }
    print $polygons;
  }
  print "]";
  // Perform end-of-request tasks.
  ajax_footer();
}

/**
 * Helper function to sort the array of matches by their length.
 */
function gm3_region_sort_by_length($a, $b){
  return strlen($b) - strlen($a);
}