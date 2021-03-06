<?php

class DrupalFieldProfileMigration extends DrupalMigration{

  /**
   * Field name mappings 
   */
  public static function getFieldNameMappings(){
    return array(
      'profile_real_name' => 'field_user_given_names',
      'profile_interest' => 'field_user_taxonomic_interest',
      'profile_webinterest' => 'field_user_taxonomic_interest',
      'profile_homepage' => 'field_homepage',
      'profile_affil' => 'field_user_institution',
      'profile_forename' => 'field_user_given_names',
      'profile_surname' => 'field_user_family_name'
    );
  }

  /**
   * @param array $arguments
   */
  public function __construct(array $arguments){
    // Add some extra source fields so they can be set in prepareRow()
    $this->sourceFields['widget_settings'] = t("Widget settings");
    $this->sourceFields['display_settings'] = t("Display settings");
    $this->sourceFields['global_settings'] = t("Global settings");
    $this->sourceFields['widget_type'] = t("Widget type");
    $this->sourceFields['module'] = t("Module");
    $this->sourceFields['widget_module'] = t("Widget module");
    parent::__construct($arguments);
    $this->source = new MigrateSourceSQL($this->sourceQuery(), $this->sourceFields, NULL, $this->sourceOptions);
    $this->map = new MigrateSQLMap($this->machineName, array(
      'name' => array(
        'type' => 'varchar',
        'length' => 255
      )
    ), MigrateDestinationField::getKeySchema());
    $this->destination = new MigrateDestinationField();
    // Field mappings
    $this->addFieldMapping('label', 'title');
    $this->addFieldMapping('field_name', 'name');
    $this->addFieldMapping('description', 'explanation');
    $this->addFieldMapping('bundle', NULL)->defaultValue('user');
    $this->addFieldMapping('entity_type', NULL)->defaultValue('user');
    $this->addFieldMapping('active', NULL)->defaultValue(1);
    $this->addFieldMapping('widget_settings', 'widget_settings')->defaultValue(array());
    $this->addFieldMapping('display_settings', 'display_settings')->defaultValue(array());
    $this->addFieldMapping('global_settings', 'global_settings')->defaultValue(array());
    $this->addSimpleMappings(array(
      'required',
      'weight',
      'widget_type', // Add these values in prepareRow()
      'module',
      'widget_module',
      'type'
    ));
  }

  /**
   * Query for the basic menu data.
   *
   * @return QueryConditionInterface
   */
  protected function sourceQuery(){
    $query = Database::getConnection('default', $this->sourceConnection)->select('profile_fields', 'pf');
    $query->fields('pf', array(
      'title',
      'required',
      'explanation',
      'weight',
      'type',
      'name'
    ));
    $query->condition('pf.name', array_keys($this->getFieldNameMappings()), 'NOT IN');
    return $query;
  }

  /**
   * Implementation of Migration::prepareRow().
   */
  public function prepareRow($row){
    if(parent::prepareRow($row) === FALSE){return FALSE;}
    switch($row->type){
      case 'textfield':
        $row->type = 'text';
        $row->widget_type = 'text_textfield';
        $row->module = $row->widget_module = 'text';
        break;
      case 'textarea':
      case 'list':
        $row->type = 'text';
        $row->widget_type = 'text_textarea';
        $row->module = $row->widget_module = 'text';
        break;
      case 'url':
        $row->widget_type = $row->type = 'link_field';
        $row->module = $row->widget_module = 'link';
        $row->widget_settings = array(
          'title' => 'none',
          'title_value' => ''
        );
        break;
      case 'selection':
        $allowed_values = array();
        if (!empty($row->options)) {
          $options = explode("\n", preg_replace('/(\r|\n)+/', "\n", $row->options));
          foreach ($options as $o) {
            $o = trim($o);
            if (!empty($o)) {
              $allowed_values[$o] = $o;
            }
          }
        }
        $row->module = 'list';
        $row->type = 'list_text';
        $row->widget_type = 'options_select';
        $row->widget_module = 'options';
        $row->widget_settings = array();
        $row->global_settings = array(
          'allowed_values' => $allowed_values,
          'allowed_values_function' => ''
        );
        break;
      default:
        watchdog('scratchpads_migrate', 'Could not create profile field %field_name, unknown type %type', array(
          '%field_name' => $row->name,
          '%type' => $row->type
        ), WATCHDOG_ERROR);
        return false;
        break;
    }
    $row->global_settings['text_processing'] = null;
    $row->multiple = 0;
    $row->name = DrupalFieldProfileMigration::destinationFieldName($row->name);
    if(field_read_instance('user', $row->name, 'user')){// Instance already exists, so skip it
      return false;}
  }

  /**
   * Generate destination field name from source field name
   */
  public static function destinationFieldName($field_name) {
    $map = array(
      'profile_url' => 'field_homepage'
    );
    $field_name = strtolower($field_name);
    if (isset($map[$field_name])) {
      return $map[$field_name];
    }
    return substr('field_' . $field_name, 0, 32);
  }
}