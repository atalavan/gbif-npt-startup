<?php

/**
 * Autocomplete callback
 * @return multitype:unknown
 */
function scratchpads_statistics_autocomplete($type, $string){
  $return = array();
  if(in_array($type, array(
    'site',
    'user',
    'term'
  ))){
    switch($type){
      case 'site':
        $condition = db_or()->condition('title', db_like($string) . '%', 'LIKE')->condition('url', db_like($string) . '%', 'LIKE');
        $table = 'scratchpads_statistics_site';
        $field_expression = "CONCAT(title, ' (', url, ')')";
        $order = 'title';
        break;
      case 'user':
        $condition = db_or()->condition('name', db_like($string) . '%', 'LIKE')->condition('email', db_like($string) . '%', 'LIKE');
        $table = 'scratchpads_statistics_user';
        $field_expression = 'name';
        $order = 'name';
        break;
      case 'term':
        $condition = db_and()->condition('term', db_like($string) . '%', 'LIKE');
        $table = 'scratchpads_statistics_term';
        $field_expression = 'term';
        $order = 'term';
        break;
    }
    $query = db_select($table, 't')->condition($condition)->fields('t')->orderBy($order)->range(0, 20);
    $query->addExpression($field_expression, 'matches');
    $results = $query->execute();
    foreach($results as $row){
      $return[$row->matches] = $row->matches;
    }
  }
  drupal_json_output($return);
}

/**
 * 
 * @return multitype:unknown
 */
function scratchpads_statistics_stats_page_json(){
  // Get the content, and build it.
  return _scratchpads_statistics_build_from_time(isset($_GET['since']) ? $_GET['since'] : 0);
}

function scratchpads_statistics_stats_page_login_json(){
  $stats = array();
  $query = db_select('scratchpads_statistics_login', 'l');
  $query->innerJoin('scratchpads_statistics_site', 's', 'l.site = s.id');
  $query->innerJoin('scratchpads_statistics_user', 'u', 'l.user = u.id');
  $query->innerJoin('scratchpads_statistics_user', 'o', 's.owner = o.id');
  $query->fields('l', array(
    'created',
    'access'
  ));
  $query->fields('s', array(
    'title',
    'url'
  ));
  $query->addField('s', 'created', 'site_created');
  $query->addField('o', 'name', 'owner_name');
  $query->addField('o', 'email', 'owner_email');
  $query->addField('o', 'country', 'owner_country');
  $query->addField('o', 'latitude', 'owner_latitude');
  $query->addField('o', 'longitude', 'owner_longitude');
  $query->fields('u', array(
    'name',
    'email',
    'country',
    'latitude',
    'longitude'
  ));
  $results = $query->execute();
  foreach($results as $row){
    $stats[] = $row;
  }
  return $stats;
}

function scratchpads_statistics_stats_page(){
  $content = array();
  // Users --------------------------------------------------------------------
  // First we get the entity_id for the user/user entity/bundle combo.
  $id = db_select('scratchpads_statistics_entity_bundle', 'eb')->fields('eb', array(
    'id'
  ))->condition('entity', 'user')->condition('bundle', 'user')->execute()->fetchCol();
  $id = array_pop($id);
  $query = db_select('scratchpads_statistics_data', 'd')->fields('d', array(
    'captured',
    'site'
  ))->condition('entity_bundle', $id);
  $query->groupBy('site');
  $query->groupBy('captured');
  $query->orderBy('captured', 'ASC');
  $query->addExpression('SUM(d.number_created)', 'sum_created');
  $site = FALSE;
  if(isset($_GET['site'])){
    $site = db_or();
    foreach($_GET['site'] as $site_id){
      $site->condition('d.site', $site_id);
    }
    $query->condition($site);
  }
  $results = $query->execute();
  $site_month_user_counts = array();
  foreach($results as $row){
    $site_month_user_counts[$row->site][date('Y', $row->captured) . "/" . date('n', $row->captured)] = $row->sum_created;
  }
  $month_counts = array();
  foreach($site_month_user_counts as $data){
    foreach($data as $key => $value){
      if(!isset($month_counts[$key])){
        $month_counts[$key] = array(
          'users' => 0,
          'objects' => 0,
          'edits' => 0,
          'views' => 0
        );
      }
      $month_counts[$key]['users'] += $value;
    }
  }
  // Objects ------------------------------------------------------------------
  $query = db_select('scratchpads_statistics_data', 'd')->fields('d', array(
    'captured',
    'entity_bundle'
  ));
  $query->groupBy('entity_bundle');
  $query->groupBy('captured');
  $query->orderBy('captured', 'ASC');
  $query->addExpression('SUM(d.number_created)', 'sum_created');
  if($site){
    $query->condition($site);
  }
  $results = $query->execute();
  $site_month_user_counts = array();
  foreach($results as $row){
    $site_month_object_counts[$row->entity_bundle][date('Y', $row->captured) . "/" . date('n', $row->captured)] = $row->sum_created;
  }
  foreach($site_month_object_counts as $data){
    foreach($data as $key => $value){
      if(!isset($month_counts[$key])){
        $month_counts[$key] = array(
          'users' => 0,
          'objects' => 0,
          'edits' => 0,
          'views' => 0
        );
      }
      $month_counts[$key]['objects'] += $value;
    }
  }
  // Edits --------------------------------------------------------------------
  $query = db_select('scratchpads_statistics_data', 'd')->fields('d', array(
    'captured',
    'entity_bundle'
  ));
  $query->groupBy('entity_bundle');
  $query->groupBy('captured');
  $query->orderBy('captured', 'ASC');
  $query->addExpression('SUM(d.number_edited)', 'sum_created');
  if($site){
    $query->condition($site);
  }
  $results = $query->execute();
  $site_month_user_counts = array();
  foreach($results as $row){
    $site_month_object_counts[$row->entity_bundle][date('Y', $row->captured) . "/" . date('n', $row->captured)] = $row->sum_created;
  }
  foreach($site_month_object_counts as $data){
    foreach($data as $key => $value){
      if(!isset($month_counts[$key])){
        $month_counts[$key] = array(
          'users' => 0,
          'objects' => 0,
          'edits' => 0,
          'views' => 0
        );
      }
      $month_counts[$key]['edits'] += $value;
    }
  }
  // Views --------------------------------------------------------------------
  $query = db_select('scratchpads_statistics_data', 'd')->fields('d', array(
    'captured',
    'entity_bundle'
  ));
  $query->groupBy('entity_bundle');
  $query->groupBy('captured');
  $query->orderBy('captured', 'ASC');
  $query->addExpression('SUM(d.number_views)', 'sum_created');
  if($site){
    $query->condition($site);
  }
  $results = $query->execute();
  $site_month_user_counts = array();
  foreach($results as $row){
    $site_month_object_counts[$row->entity_bundle][date('Y', $row->captured) . "/" . date('n', $row->captured)] = $row->sum_created;
  }
  foreach($site_month_object_counts as $data){
    foreach($data as $key => $value){
      if(!isset($month_counts[$key])){
        $month_counts[$key] = array(
          'users' => 0,
          'objects' => 0,
          'edits' => 0,
          'views' => 0
        );
      }
      $month_counts[$key]['views'] += $value;
    }
  }
  $data = array(
    array(
      t('Year/Month'),
      t('Objects'),
      t('Edits'),
      t('Users'),
      t('Views')
    )
  );
  foreach($month_counts as $key => $value){
    $data[] = array(
      $key,
      $value['objects'],
      $value['edits'],
      $value['users'],
      $value['views']
    );
  }
  $content[] = array(
    '#theme' => 'simple_google_chart',
    '#type' => 'LineChart',
    '#data' => $data,
    '#title' => t('Number of users')
  );
  // Bar chart of entities ----------------------------------------------------
  // The following query will give us a total for each entity/bundle across all
  // of the sites.
  // Create a temporary table first which contains the sites and the max 
  // captured value for each.
  $temp_table_name = db_query_temporary('SELECT MAX(captured) AS captured, site FROM {scratchpads_statistics_data} GROUP BY site');
  // Now join to that table.
  $query = db_select('scratchpads_statistics_data', 'd');
  if($site){
    $query->condition($site);
  }
  $query->innerJoin($temp_table_name, 't', 't.site = d.site AND t.captured = d.captured');
  $query->innerJoin('scratchpads_statistics_entity_bundle', 'e', 'e.id = d.entity_bundle');
  $query->groupBy('entity_bundle');
  $query->fields('e', array(
    'name'
  ));
  $query->addExpression('SUM(number_created)', 'sum_number_created');
  $query->addExpression('SUM(number_edited)', 'sum_number_edited');
  $query->addExpression('SUM(number_views)', 'sum_number_views');
  $query->orderBy('sum_number_created', 'DESC');
  $results = $query->execute();
  $counts = array();
  $i = 0;
  $total = 0;
  $eight_count = PHP_INT_MAX;
  $data = array(
    array(
      t('Content types'),
      t('Created'),
      t('Edited'),
      t('Views')
    )
  );
  $max_value = FALSE;
  foreach($results as $row){
    // PieChart stuff
    if($i >= 8 && $row->sum_number_created < $eight_count){
      if($i == 8){
        $eight_count = $row->sum_number_created;
        $counts[$row->name] = $row->sum_number_created;
      }else{
        if(!isset($counts['Other'])){
          $counts['Other'] = 0;
        }
        $counts['Other'] += $row->sum_number_created;
      }
    }else{
      $counts[$row->name] = $row->sum_number_created;
    }
    $total += $row->sum_number_created;
    $i++;
    // BarChart stuff.
    if(!$max_value){
      $max_value = (integer)$row->sum_number_created + (integer)$row->sum_number_edited;
    }
    if(((integer)$row->sum_number_created + (integer)$row->sum_number_edited) / $max_value > 0.03){
      $data[] = array(
        $row->name,
        (integer)$row->sum_number_created,
        (integer)$row->sum_number_edited,
        (integer)$row->sum_number_views
      );
    }
  }
  $content[] = array(
    '#theme' => 'simple_google_chart',
    '#type' => 'ColumnChart',
    '#data' => $data,
    '#title' => t('Distribution of views and edits'),
    '#options' => array(
      'isStacked' => TRUE,
      'bar' => array(
        'groupWidth' => '80%'
      ),
      'height' => '400'
    )
  );
  $data = array(
    array(
      t('Content types'),
      t('Percent')
    )
  );
  foreach($counts as $title => $value){
    $data[] = array(
      $title,
      (integer)$value
    );
  }
  $content[] = array(
    '#theme' => 'simple_google_chart',
    '#type' => 'PieChart',
    '#data' => $data,
    '#title' => t('Content distribution (percent of !total)', array(
      '!total' => $total
    )),
    '#options' => array(
      'is3D' => TRUE,
      'height' => 400
    )
  );
  // Next we look for the number of users that have logged in in the past week,
  // and then the number of users that have logged in in the past month.
  //
  // Users
  //
  // Total
  $query = db_select('scratchpads_statistics_login', 'd');
  if($site){
    $query->condition($site);
  }
  $query = $query->countQuery();
  $result = $query->execute()->fetchAssoc();
  $total_users = (integer)array_pop($result);
  // Last year 31536000 seconds
  $query = db_select('scratchpads_statistics_login', 'd')->condition('access', time() - 31536000, '>');
  if($site){
    $query->condition($site);
  }
  $query = $query->countQuery();
  $result = $query->execute()->fetchAssoc();
  $last_year = (integer)array_pop($result);
  // Last month 2629740 seconds
  $query = db_select('scratchpads_statistics_login', 'd')->condition('access', time() - 2629740, '>');
  if($site){
    $query->condition($site);
  }
  $query = $query->countQuery();
  $result = $query->execute()->fetchAssoc();
  $last_month = (integer)array_pop($result);
  // Last week 604800 seconds
  $query = db_select('scratchpads_statistics_login', 'd')->condition('access', time() - 604800, '>');
  if($site){
    $query->condition($site);
  }
  $query = $query->countQuery();
  $result = $query->execute()->fetchAssoc();
  $last_week = (integer)array_pop($result);
  if($site){
    $content[] = array(
      '#theme' => 'simple_google_chart',
      '#type' => 'BarChart',
      '#title' => t('User activity'),
      '#data' => array(
        array(
          'Time period',
          'Active users'
        ),
        array(
          'Last week',
          $last_week
        ),
        array(
          'Last month',
          $last_month
        ),
        array(
          'Last year',
          $last_year
        ),
        array(
          'Ever',
          $total_users
        )
      )
    );
  }else{
    //
    // Sites
    //
    // Total
    $query = db_select('scratchpads_statistics_login', 'l');
    $query->addExpression('COUNT(DISTINCT site)');
    $result = $query->execute()->fetchAssoc();
    $total_users_site = (integer)array_pop($result);
    // Last year 31536000 seconds
    $query = db_select('scratchpads_statistics_login', 'l')->condition('access', time() - 31536000, '>');
    $query->addExpression('COUNT(DISTINCT site)');
    $result = $query->execute()->fetchAssoc();
    $last_year_site = (integer)array_pop($result);
    // Last month 2629740 seconds
    $query = db_select('scratchpads_statistics_login', 'l')->condition('access', time() - 2629740, '>');
    $query->addExpression('COUNT(DISTINCT site)');
    $result = $query->execute()->fetchAssoc();
    $last_month_site = (integer)array_pop($result);
    // Last week 604800 seconds
    $query = db_select('scratchpads_statistics_login', 'l')->condition('access', time() - 604800, '>');
    $query->addExpression('COUNT(DISTINCT site)');
    $result = $query->execute()->fetchAssoc();
    $last_week_site = (integer)array_pop($result);
    $content[] = array(
      '#theme' => 'simple_google_chart',
      '#type' => 'BarChart',
      '#title' => t('User activity'),
      '#data' => array(
        array(
          'Time period',
          'Active users',
          'Active sites'
        ),
        array(
          'Last week',
          $last_week,
          $last_week_site
        ),
        array(
          'Last month',
          $last_month,
          $last_month_site
        ),
        array(
          'Last year',
          $last_year,
          $last_year_site
        ),
        array(
          'Ever',
          $total_users,
          $total_users_site
        )
      )
    );
  }
  return $content;
}

/**
 * Build the stats for the required time period
 */
function _scratchpads_statistics_build_from_time($time){
  $stats = array();
  $query = db_select('scratchpads_statistics_data', 'd');
  $query->innerJoin('scratchpads_statistics_site', 's', 'd.site = s.id');
  $query->innerJoin('scratchpads_statistics_user', 'u', 'd.user = u.id');
  $query->leftJoin('scratchpads_statistics_term', 't', 'd.term = t.id');
  $query->innerJoin('scratchpads_statistics_entity_bundle', 'e', 'd.entity_bundle = e.id');
  $query->innerJoin('scratchpads_statistics_user', 'o', 's.owner = o.id');
  $query->fields('d', array(
    'number_created',
    'number_edited',
    'number_views',
    'captured'
  ));
  $query->fields('s', array(
    'title',
    'url',
    'created'
  ));
  $query->addField('o', 'name', 'owner_name');
  $query->addField('o', 'email', 'owner_email');
  $query->addField('o', 'country', 'owner_country');
  $query->addField('o', 'latitude', 'owner_latitude');
  $query->addField('o', 'longitude', 'owner_longitude');
  $query->fields('u', array(
    'name',
    'email',
    'country',
    'latitude',
    'longitude'
  ));
  $query->fields('t', array(
    'term'
  ));
  $query->fields('e', array(
    'entity',
    'bundle'
  ));
  $query->addField('e', 'name', 'bundle_label');
  $query->condition('s.id', 1)->condition('captured', $time, '>');
  $results = $query->execute();
  foreach($results as $row){
    $stats[] = $row;
  }
  return $stats;
}

/**
 * Simple configuration form.
 */
function scratchpads_statistics_config_page(){
  return system_settings_form(array(
    'scratchpads_statistics_hash' => array(
      '#title' => t('Hash'),
      '#type' => 'textfield',
      '#default_value' => variable_get('scratchpads_statistics_hash', ''),
      '#description' => t('Enter the hash required to access this site.')
    )
  ));
}