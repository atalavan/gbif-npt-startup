<?php

/**
 * 
 */
function silver_feeds_importer_default(){
  // Array of Feeds importers.
  $export = array();
  // Get a list of the vocabularies.
  $vocabularies = taxonomy_get_vocabularies();
  foreach($vocabularies as $vocabulary){
    $feeds_importer = new stdClass();
    $feeds_importer->disabled = FALSE;
    $feeds_importer->api_version = 1;
    $feeds_importer->id = 'taxonomy_importer_' . $vocabulary->machine_name;
    $feeds_importer->config = array(
      'name' => t('Import into: !vocabulary_name', array(
        '!vocabulary_name' => $vocabulary->name
      )),
      'description' => t('Import a classification into a specific vocabulary.'),
      'fetcher' => array(
        'plugin_key' => 'FeedsFileFetcher',
        'config' => array(
          'allowed_extensions' => 'xls xlsx',
          'direct' => 0
        )
      ),
      'parser' => array(
        'plugin_key' => 'FeedsExcelParser',
        'config' => array(
          'no_headers' => 0,
          'all_worksheets' => 1
        )
      ),
      'processor' => array(
        'plugin_key' => 'FeedsTermProcessor',
        'config' => array(
          'vocabulary' => $vocabulary->machine_name,
          'mappings' => array(
            array(
              'source' => 'Term name',
              'target' => 'name',
              'unique' => 0
            ),
            array(
              'source' => 'Parent Term Name',
              'target' => 'parent',
              'unique' => 0
            ),
            array(
              'source' => 'Term description',
              'target' => 'description',
              'unique' => FALSE
            ),
            array(
              'source' => 'GUID',
              'target' => 'guid',
              'unique' => 1
            ),
            array(
              'source' => 'Parent GUID',
              'target' => 'parentguid',
              'unique' => 0
            )
          ),
          'update_existing' => '2',
          'input_format' => 'plain_text'
        )
      ),
      'content_type' => '',
      'update' => 1,
      'import_period' => '-1',
      'expire_period' => 3600,
      'import_on_create' => 1,
      'process_in_background' => 0
    );
    $field_info = field_info_instances('taxonomy_term', $vocabulary->machine_name);
    _silver_feeds_importer_default_helper($field_info, $feeds_importer, 'taxonomy_term:' . $vocabulary->machine_name . ':');
    $export['taxonomy_importer_' . $vocabulary->machine_name] = $feeds_importer;
  }
  // Get a list of the content types.
  $entity_info = entity_get_info('node');
  foreach($entity_info['bundles'] as $node_type => $node_type_info){
    $feeds_importer = new stdClass();
    $feeds_importer->disabled = FALSE;
    $feeds_importer->api_version = 1;
    $feeds_importer->id = 'node_importer_' . $node_type;
    $feeds_importer->config = array(
      'name' => t('Import into: !node_type_name', array(
        '!node_type_name' => $node_type_info['label']
      )),
      'description' => t('Import a classification into a specific content type.'),
      'fetcher' => array(
        'plugin_key' => 'FeedsFileFetcher',
        'config' => array(
          'allowed_extensions' => 'xls xlsx',
          'direct' => 0
        )
      ),
      'parser' => array(
        'plugin_key' => 'FeedsExcelParser',
        'config' => array(
          'no_headers' => 0,
          'all_worksheets' => 1
        )
      ),
      'processor' => array(
        'plugin_key' => 'FeedsNodeProcessorValidate',
        'config' => array(
          'content_type' => $node_type,
          'expire' => '-1',
          'author' => 0,
          'mappings' => array(
            array(
              'source' => 'GUID',
              'target' => 'guid',
              'unique' => TRUE
            )
          ),
          'update_existing' => '2',
          'input_format' => 'filtered_html'
        )
      ),
      'content_type' => '',
      'update' => 0,
      'import_period' => -1,
      'expire_period' => 3600,
      'import_on_create' => TRUE,
      'process_in_background' => FALSE
    );
    if(auto_nodetitle_get_setting($node_type) != AUTO_NODETITLE_ENABLED){
      $feeds_importer->config['processor']['config']['mappings'][] = array(
        'source' => 'Title',
        'target' => 'title',
        'unique' => FALSE
      );
    }
    $field_info = field_info_instances('node', $node_type);
    _silver_feeds_importer_default_helper($field_info, $feeds_importer, "node:$node_type:");
    $export['node_importer_' . $node_type] = $feeds_importer;
  }
  // Get a list of the file types.
  $entity_info = entity_get_info('file');
  foreach($entity_info['bundles'] as $file_type => $file_type_info){
    $feeds_importer = new stdClass();
    $feeds_importer->disabled = FALSE;
    $feeds_importer->api_version = 1;
    $feeds_importer->id = 'file_importer_' . $file_type;
    $feeds_importer->config = array(
      'name' => t('Import into: @file_type_name', array(
        '@file_type_name' => $file_type_info['label']
      )),
      'description' => t('Update the metadata for a specific file type.'),
      'fetcher' => array(
        'plugin_key' => 'FeedsFileFetcher',
        'config' => array(
          'allowed_extensions' => 'xls xlsx',
          'direct' => 0
        )
      ),
      'parser' => array(
        'plugin_key' => 'FeedsExcelParser',
        'config' => array(
          'no_headers' => 0,
          'all_worksheets' => 1
        )
      ),
      'processor' => array(
        'plugin_key' => 'FeedsFileProcessor',
        'config' => array(
          'bundle' => $file_type,
          'expire' => '-1',
          'author' => 0,
          'mappings' => array(
            array(
              'source' => 'GUID',
              'target' => 'guid',
              'unique' => TRUE
            ),
            array(
              'source' => 'Filename',
              'target' => 'filename',
              'unique' => FALSE
            )
          )
        )
      ),
      'update' => 0,
      'import_period' => -1,
      'expire_period' => 3600,
      'import_on_create' => TRUE,
      'process_in_background' => FALSE
    );
    $field_info = field_info_instances('file', $file_type);
    _silver_feeds_importer_default_helper($field_info, $feeds_importer, "file:$file_type:");
    $export['file_importer_' . $file_type] = $feeds_importer;
  }
  // Users
  $feeds_importer = new stdClass();
  $feeds_importer->disabled = FALSE;
  $feeds_importer->api_version = 1;
  $feeds_importer->id = 'user_importer_user';
  $feeds_importer->config = array(
    'name' => t('Import into: Users'),
    'description' => t('Add and Update users.'),
    'fetcher' => array(
      'plugin_key' => 'FeedsFileFetcher',
      'config' => array(
        'allowed_extensions' => 'xls xlsx',
        'direct' => 0
      )
    ),
    'parser' => array(
      'plugin_key' => 'FeedsExcelParser',
      'config' => array(
        'no_headers' => 0,
        'all_worksheets' => 1
      )
    ),
    'processor' => array(
      'plugin_key' => 'FeedsUserProcessor',
      'config' => array(
        'status' => 1,
        'mappings' => array(),
        'update_existing' => '1'
      )
    ),
    'update' => 0,
    'import_period' => -1,
    'expire_period' => 3600,
    'import_on_create' => TRUE,
    'process_in_background' => FALSE
  );
  $field_info = field_info_instances('user', 'user');
  // reorder the array
  $order = array(
    "field_user_title",
    "field_user_given_names",
    "field_user_family_name",
    "field_user_country",
    "field_homepage",
    "field_user_institution",
    "field_user_taxonomic_interest"
  );
  $out = array();
  foreach($order as $element){
    $out[$element] = $field_info[$element];
  }
  $field_info = $out;
  _silver_feeds_importer_default_helper($field_info, $feeds_importer, 'user:user:');
  $feeds_importer->config['processor']['config']['mappings'][] = array(
    'source' => 'E-Mail',
    'target' => 'mail',
    'unique' => TRUE
  );
  $feeds_importer->config['processor']['config']['mappings'][] = array(
    'source' => 'Username',
    'target' => 'name',
    'unique' => TRUE
  );
  $feeds_importer->config['processor']['config']['mappings'][] = array(
    'source' => 'Password',
    'target' => 'pass',
    'unique' => 0
  );
  $feeds_importer->config['processor']['config']['mappings'][] = array(
    'source' => 'GUID',
    'target' => 'guid',
    'unique' => 1
  );
  $export['file_importer_' . $file_type] = $feeds_importer;
  return $export;
}

/**
 * Helper function which creates the correct mapping for certain field types.
 */
function _silver_feeds_importer_default_helper($field_info, &$feeds_importer, $additional = ''){
  foreach($field_info as $key => $field){
    $field_info = field_info_field($field['field_name']);
    switch($field_info['type']){
      case 'file':
        // Do nothing.  We can't import files in an Excel template.
        break;
      case 'country':
        // We use iso3 country codes
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (' . t('ISO alpha-2') . ')',
          'target' => $additional . $key . ':iso2',
          'unique' => 0
        );
        break;
      case 'date':
        // Add the from date, and also the to date if required.
        // We add two columns for the user reference field.
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (' . t('Start') . ')',
          'target' => $key . ':start',
          'unique' => 0
        );
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (' . t('End') . ')',
          'target' => $key . ':end',
          'unique' => 0
        );
        break;
      case 'link_field':
        // Add the from date, and also the to date if required.
        // We add two columns for the user reference field.
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (' . t('URL') . ')',
          'target' => $key . ':url',
          'unique' => 0
        );
        break;
      case 'user_reference':
        // We add two columns for the user reference field.
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (UID)',
          'target' => $key . ':uid:duplicates',
          'unique' => 0
        );
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (' . t('Name on site') . ')',
          'target' => $key . ':name:duplicates',
          'unique' => 0
        );
        break;
      case 'node_reference':
        // We add two columns for the node reference field.
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (NID)',
          'target' => $key . ':nid:duplicates',
          'unique' => 0
        );
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (' . t('Title') . ')',
          'target' => $key . ':title:duplicates',
          'unique' => 0
        );
        break;
      case 'taxonomy_term_reference':
        // We add three columns for the term reference field.
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (' . t('Name') . ')',
          'target' => $key,
          'unique' => 0
        );
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (TID)',
          'target' => $key . ':tid',
          'unique' => 0
        );
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => $field['label'] . ' (GUID)',
          'target' => $key . ':guid',
          'unique' => 0
        );
        break;
      default:
        $feeds_importer->config['processor']['config']['mappings'][] = array(
          'source' => trim($field['label']),
          'target' => $key,
          'unique' => 0
        );
    }
  }
}