<?php

class DrupalFieldMigration extends DrupalMigration{

  /**
   * @param array $arguments
   */
  public function __construct(array $arguments){
    parent::__construct($arguments);
    $this->sourceFields['entity_type'] = t('Entity type');
    $this->source = new MigrateSourceSQL($this->sourceQuery(), $this->sourceFields, NULL, $this->sourceOptions);
    $this->map = new MigrateSQLMap($this->machineName, array(
      'field_name' => array(
        'type' => 'varchar',
        'length' => 255
      ),
      'type_name' => array(
        'type' => 'varchar',
        'length' => 255
      )
    ), MigrateDestinationField::getKeySchema());
    $this->destination = new MigrateDestinationField();
    // Field mappings
    $this->addSimpleMappings(array(
      'label',
      'description',
      'required',
      'field_name'
    ));
    $this->addFieldMapping('bundle', 'type_name');
    // These are all node entity types
    $this->addFieldMapping('entity_type', 'entity_type')->defaultValue('node');
    $this->addFieldMapping('active', 'active'); // Used to create field
    $this->addFieldMapping('widget_settings', 'widget_settings'); // Merge into field_config_instance.data
    $this->addFieldMapping('global_settings', 'global_settings'); // Merge into field_config.data
    $this->addFieldMapping('display_settings', 'display_settings'); // Merge into field_config_instance.data
    $this->addFieldMapping('weight', 'weight'); // Now part of field_config_instance.data
    $this->addFieldMapping('widget_type', 'widget_type'); // Now part of field_config_instance.data
    $this->addFieldMapping('module', 'module'); // Now part of field_config_instance.data
    $this->addFieldMapping('type', 'type'); // Used to create field
    $this->addFieldMapping('widget_module', 'widget_module'); // Used to create field instance
    // Unmapped source fields
    $this->addUnmigratedSources(array(
      'db_columns',
      'db_storage',
      'locked',
      'multiple',
      'widget_active'
    ));
  }

  /**
   * Query for the basic source query.
   *
   * @return QueryConditionInterface
   */
  protected function sourceQuery(){
    $query = Database::getConnection('default', $this->sourceConnection)->select('content_node_field_instance', 'i');
    $query->fields('i');
    $query->innerJoin('content_node_field', 'f', 'f.field_name = i.field_name');
    $query->fields('f');
    $query->condition('i.widget_active', 1);
    $query->condition('i.type_name', $this->excludedNodeTypes(), 'NOT IN');
    $query->condition('f.type', array(
      'noderelationships_backref', // We use a special migration class for these
      'nodereferrer'
    ), 'NOT IN');
    return $query;
  }

  /**
   * Implementation of Migration::prepareRow().
   */
  public function prepareRow($row){
    if(parent::prepareRow($row) === FALSE){return FALSE;}
    // Unserialize field & widget serttings
    if(!is_array($row->widget_settings)){
      $row->widget_settings = unserialize($row->widget_settings);
    }
    if(!is_array($row->display_settings)){
      $row->display_settings = unserialize($row->display_settings);
    }
    if(!is_array($row->global_settings)){
      $row->global_settings = unserialize($row->global_settings);
    }
    // Check if this field has a different destination entity type / bundle
    $mapping = DrupalContentTypeMigration::getContentTypeMappings($row->type_name);
    if($mapping && is_array($mapping)){
      $row->entity_type = $mapping['entity_type'];
      $row->type_name = $mapping['bundle'];
      if(isset($mapping['fields']) && isset($mapping['fields'][$row->field_name])){
        $row->field_name = $mapping['fields'][$row->field_name];
      }
    }elseif(!isset($row->entity_type)){
      $row->entity_type = 'node';
    }
    // Check the node type exists
    if($row->entity_type == 'node' && !scratchpads_migrate_destination_node_type_exists($row->type_name)){
      // We want to log this, but we don't want to throw an exception - this is a possible error (content types
      // being disabled while fields are still present), and we want further processing to continue.
      watchdog('scratchpads_migrate', 'Node type %type does not exist. Cannot create field %field_name.', array(
        '%type' => $row->type_name,
        '%field_name' => $row->field_name
      ), WATCHDOG_ERROR);
      return false;
    }
    // We want all image fields to use the media field
    if($row->type == 'image' || $row->type == 'filefield'){
      if ($row->field_name != 'field_mediafiles') {
        $row->field_name = 'field_media';
      }
    } else {
      $row->field_name = DrupalFieldMigration::getCustomMapping($row->field_name, $row->type);
    }
    // Special cases for node references
    if($row->type == 'nodereference'){
      if(!isset($row->global_settings['referenceable_types']) || !is_array($row->global_settings['referenceable_types'])){
        // If referenceable_types is not set, prevent error
        $referenceable_types = array();
      }else{
        $referenceable_types = array_filter($row->global_settings['referenceable_types']);
      }
      // Node references to itis terms are changed to taxonomy reference fields
      if(in_array('itis_term', $referenceable_types)){
        $row->widget_type = 'content_taxonomy_autocomplete';
        $itis_vocabs = scratchpads_migrate_get_term_vocabularies();
        if(count($itis_vocabs) > 1){
          drupal_set_message(t('Multiple ITIS fields for !field_name (field.inc:109).', array(
            '!field_name' => $row->field_name
          )), 'error');
          $row->widget_settings = array();
        }else{
          $row->widget_settings = array(
            'vid' => key($itis_vocabs)
          );
        }
        $row->widget_module = 'content_taxonomy_autocomplete';
        $row->type = 'content_taxonomy';
        $row->module = 'content_taxonomy';
        $row->global_settings = array();
      }else if(in_array('image', $referenceable_types)){
        // Node references to Image nodes become image field
        $row->type = 'image';
        $row->module = 'image';
        $row->widget_type = 'image';
        $row->widget_settings = array(
          'file_extensions' => 'png gif jpg jpeg',
          'max_filesize' => '',
          'file_directory' => '',
          'description_field' => '',
          'max_resolution' => '',
          'min_resolution' => '',
          'alt_field' => '',
          'title_field' => ''
        );
        $row->display_settings = array_merge_recursive($row->display_settings, array(
          'full' => array(
            'module' => 'file_entity',
            'type' => 'file_rendered',
            'settings' => array(
              'file_view_mode' => 'file_styles_square_thumbnail'
            )
          ),
          'teaser' => array(
            'module' => 'file_entity',
            'type' => 'file_rendered',
            'settings' => array(
              'file_view_mode' => 'file_styles_square_thumbnail'
            )
          )
        ));
        $row->global_settings = array();
      }else{
        $referenceable_types_count = count($referenceable_types);
        // Do we not have any referencable types and a view is defined
        if($referenceable_types_count == 0 && isset($row->global_settings['advanced_view']) && $row->global_settings['advanced_view'] != '--'){
          switch($row->global_settings['advanced_view']){
            case 'biblio_nodes':
              $row->global_settings['referenceable_types']['biblio'] = 'biblio';
              break;
            default:
              drupal_set_message(t('ERROR: Node reference field !type:!field_name using views - requires manual intervention', array(
                '!type' => $row->type_name,
                '!field_name' => $row->field_name
              )), 'error');
              break;
          }
        }elseif($referenceable_types_count > 1){
          drupal_set_message(t('ERROR: Node reference field !type:!field_name has multiple node types - requires manual intervention', array(
            '!type' => $row->type_name,
            '!field_name' => $row->field_name
          )), 'error');
        }
        // Always unset advanced view here - if we have one with a ref typ we don't want it
        unset($row->global_settings['advanced_view']);
      }
    }
    // Does an instance of this field already exists?
    if(field_read_instance($row->entity_type, $row->field_name, $row->type_name)){return false;    // Instance already exists, so skip it
}
  }

  /**
   * Get a list of node types not to migrate fields for
   *
   */
  static public function excludedNodeTypes(){
    $node_types = array();
    foreach(DrupalContentTypeMigration::getContentTypeMappings() as $node_type => $mapping){
      if($mapping === MIGRATE_NONE || $mapping === MIGRATE_CONTENT){
        $node_types[] = $node_type;
      }
    }
    return $node_types;
  }

  /**
   * Provide additional custom field name mapping
   */
  static public function getCustomMapping($field_name, $field_type = NULL) {
    // Ensure that text-based country fields do not get mapped to the field_country field.
    if ($field_name == 'field_country' && $field_type == 'text') {
      return 'field_country_text';
    }
    // Custom field field_keywords that is not a taxonmy term reference need to be renamed
    if ($field_name == 'field_keywords' && $field_type == 'text') {
      return 'field_keywords_t';
    }
    // Field name field_name is reserved.
    if ($field_name == 'field_name') {
      return 'field_name_r';
    }
    return $field_name;
  }
}
