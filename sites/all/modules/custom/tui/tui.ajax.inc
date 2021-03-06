<?php

/**
 * Callback function to return HTML
 *
 * FIXME - We need to reload part of the tree too.
 */
function tui_ajax_form_callback(&$form, &$form_state){
  // Unset the parent message, as it doesn't make sense once the term has been
  // created.
  if(isset($form['parent_message'])){
    unset($form['parent_message']);
  }
  $ajax = array(
    '#type' => 'ajax',
    '#commands' => array()
  );
  if(isset($form_state['values']['tid'])){
    $ajax['#commands'][] = ajax_command_replace('[data-tui-this-term="' . $form_state['values']['tid'] . '"] div', theme('tui_term_name', array(
      'term' => taxonomy_term_load($form_state['values']['tid'])
    )));
    // Get the parent TIDs
    $tids = array();
    foreach(taxonomy_get_parents_all($form_state['values']['tid']) as $parent_term){
      $tids[] = $parent_term->tid;
    }
    $ajax['#commands'][] = ajax_command_html('#tui-tree-subcontainer', theme('tui_branch', array(
      'vocabulary' => taxonomy_vocabulary_load($form_state['values']['vid']),
      'tids' => $tids,
      'highlight_tids' => $tids
    )));
  }
  foreach($form_state['values']['parent'] as $parent_tid){
    $ajax['#commands'][] = ajax_command_invoke('[data-tui-this-term="' . $parent_tid . '"]', 'removeClass', array(
      'tui-no-children tui-open'
    ));
    $ajax['#commands'][] = ajax_command_invoke('[data-tui-this-term="' . $parent_tid . '"]', 'addClass', array(
      'tui-has-children tui-closed tui-never-opened'
    ));
    $ajax['#commands'][] = ajax_command_invoke('[data-tui-this-term="' . $parent_tid . '"] div span', 'click');
  }
  if(!form_get_errors()){
    // Unfortunately, we unset the actions as the form must be reloaded before
    // it can be used for editing again!  We only do this if the form doesn't
    // have errors (error forms may be submitted again).
    unset($form['actions']);
  }
  $ajax['#commands'][] = ajax_command_insert(NULL, drupal_render($form));
  $ajax['#commands'][] = ajax_command_prepend(NULL, theme('status_messages'));
  return $ajax;
}

/**
 * Callback to simply hide the form.
 */
function tui_ajax_blank(&$form, &$form_state){
  if(isset($form_state['tui-ajax'])){
    $ajax = $form_state['tui-ajax'];
  }else{
    $ajax = array();
  }
  $ajax[] = ajax_command_invoke('#tui-tree-form', 'slideUp', array(
    200
  ));
  return array(
    '#type' => 'ajax',
    '#commands' => $ajax
  );
}

/**
 * AJAX Callback to get a term form
 */
function tui_form_callback($vocabulary, $action, $tids = ''){
  switch($action){
    case 'edit-form':
      // We can only show one form
      $tids = explode(',', $tids);
      $term = taxonomy_term_load($tids[0]);
      if($term){
        module_load_include('admin.inc', 'taxonomy');
        $form = drupal_get_form('taxonomy_form_term', $term);
        // Remove the delete button, as the user should use the "delete" icon.
        unset($form['actions']['delete']);
        print ajax_render(array(
          ajax_command_html('#tui-form-container', drupal_render($form)),
          ajax_command_html('#tui-name-live h2', $term->name)
        ));
      }else{
        print ajax_render(array(
          ajax_command_html('#tui-tree-form', '<div class="messages error"><h2 class="element-invisible">Error message</h2>' . t('You are trying to edit a term that has been deleted.  Please reload this page.') . '</div>'),
          ajax_command_invoke('#tui-tree-form', 'slideDown')
        ));
      }
      exit();
    case 'delete-form':
      $form = drupal_get_form('tui_taxonomy_form_term_delete', $tids);
      print ajax_render(array(
        ajax_command_html('#tui-tree-form', drupal_render($form)),
        ajax_command_invoke('#tui-tree-form', 'show')
      ));
      exit();
    case 'search-form':
      $form = drupal_get_form('tui_search_form', $tids);
      print ajax_render(array(
        ajax_command_html('#tui-tree-form', drupal_render($form)),
        ajax_command_invoke('#tui-tree-form', 'show')
      ));
      exit();
    case 'add-form':
      module_load_include('admin.inc', 'taxonomy');
      $form = drupal_get_form('taxonomy_form_term', array(), $vocabulary);
      print ajax_render(array(
        ajax_command_html('#tui-form-container', drupal_render($form))
      ));
      exit();
  }
}

/**
 * TUI autocomplete search form
 */
function tui_search_form($form, &$form_state, $vid){
  $ajax = array(
    'callback' => 'tui_ajax_blank',
    'wrapper' => 'tui-tree-form',
    'effect' => 'slide'
  );
  return array(
    'tui-search-input' => array(
      '#autocomplete_path' => 'tui/autocomplete/' . $vid,
      '#size' => FALSE,
      '#type' => 'textfield',
      '#title' => ''
    ),
    'tui-vid' => array(
      '#value' => $vid,
      '#type' => 'value'
    ),
    'tui-search-input-button' => array(
      '#type' => 'submit',
      '#value' => 'Search',
      '#ajax' => $ajax
    ),
    'tui-cancel-input-button' => array(
      '#type' => 'button',
      '#value' => 'Cancel',
      '#ajax' => $ajax
    )
  );
}

/**
 * search submit
 */
function tui_search_form_submit(&$form, &$form_state){
  $results = db_select('taxonomy_term_data', 't')->fields('t', array(
    'tid'
  ))->condition('t.name', db_like($form_state['values']['tui-search-input']), 'LIKE')->execute();
  $highlight_tids = array();
  $open_tids = array();
  foreach($results as $row){
    $parents = taxonomy_get_parents_all($row->tid);
    array_shift($parents);
    foreach($parents as $parent){
      $open_tids[] = $parent->tid;
    }
    $highlight_tids[] = $row->tid;
  }
  $html = theme('tui_branch', array(
    'vocabulary' => taxonomy_vocabulary_load($form_state['values']['tui-vid']),
    'tids' => $open_tids,
    'highlight_tids' => $highlight_tids
  ));
  // $('#tui-tree-subcontainer').animate({scrollTop:$(element).position().top+$('#tui-tree-subcontainer').scrollTop()-50},
  // 500);
  $form_state['tui-ajax'] = array(
    ajax_command_html('#tui-tree-subcontainer', $html),
    ajax_command_invoke('#tui-tree', 'resize')
  );
}

/**
 * Sort stuff
 */
function tui_sort_callback(){
  $results = array();
  if(isset($_POST['parent_change'])){
    $term = taxonomy_term_load($_POST['parent_change']['tid']);
    if($term){
      $term->parent = isset($_POST['parent_change']['parent']) ? $_POST['parent_change']['parent'] : 0;
      taxonomy_term_save($term);
    }else{
      $results[] = ajax_command_html('#tui-tree-form', '<div class="messages error"><h2 class="element-invisible">Error message</h2>' . t('There has been an error.  Please reload this page.') . '</div>');
      watchdog('femail', 'Attempted to move a nonexistent term');
    }
  }
  if(isset($_POST['sort_change'])){
    $tids = explode(",", $_POST['sort_change']['tids']);
    $weight = 0;
    foreach($tids as $tid){
      $term = taxonomy_term_load($tid);
      if($term){
        db_update('taxonomy_term_data')->fields(array(
          'weight' => $weight
        ))->condition('tid', $term->tid)->execute();
        $term->weight = $weight;
        $weight++;
      }else{
        $results[] = ajax_command_html('#tui-tree-form', '<div class="messages error"><h2 class="element-invisible">Error message</h2>' . t('One of the terms being sorted appears to have been deleted.  Please reload the page.') . '</div>');
        watchdog('femail', 'Attempted to move a nonexistent term');
      }
    }
    // Save the final term if the leftandright module is installed, just for
    // neatness.
    if(module_exists('leftandright')){
      taxonomy_term_save($term);
    }
  }
  print ajax_render($results);
  exit();
}

/**
 * tui_taxonomy_form_term_delete
 */
function tui_taxonomy_form_term_delete($form, $form_state, $tids = ''){
  $tids = explode(",", $tids);
  $terms = taxonomy_term_load_multiple($tids);
  $term_names = array();
  $tids = array();
  foreach($terms as $term){
    $term_names[] = $term->name;
    $tids[] = $term->tid;
  }
  $ajax = array(
    'callback' => 'tui_ajax_blank',
    'wrapper' => 'tui-tree-form',
    'effect' => 'slide'
  );
  return array(
    '#ajax' => $ajax,
    'you_sure' => array(
      '#markup' => t('Are you sure you would like to delete the highlighted terms and their children?') . theme('item_list', array(
        'items' => $term_names
      ))
    ),
    'tids' => array(
      '#type' => 'value',
      '#value' => $tids
    ),
    array(
      '#prefix' => '<div id="edit-actions" class="form-actions form-wrapper">',
      '#suffix' => '</div>',
      'cancel' => array(
        '#type' => 'button',
        '#title' => t('Cancel'),
        '#value' => t('Cancel'),
        '#ajax' => $ajax
      ),
      'submit' => array(
        '#type' => 'submit',
        '#title' => t('Delete'),
        '#value' => t('Delete'),
        '#ajax' => $ajax
      )
    )
  );
}

/**
 * tui_taxonomy_form_term_delete_submit()
 */
function tui_taxonomy_form_term_delete_submit(&$form, &$form_state){
  // We have passed validation, lets delete the terms.
  foreach($form_state['values']['tids'] as $tid){
    taxonomy_term_delete($tid);
  }
  $form_state['tui-ajax'] = array(
    ajax_command_invoke('.tui-highlight', 'hide')
  );
}