<?php

/**
 * File for all field functionality
 * Seperated into hooks and custom functions
 */


//------ HOOKS

/**
 * Implementation of hook_field_widget_info()
 * Defines all available contentblock widgets
 */
function contentblock_field_widget_info() {
  return array(
    'contentblock_default' => array(
      'label' => t('Content Blocks Default'), 
      'field types' => array('contentblock'), 
      'settings' => array(), 
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM,
        'default value' => FIELD_BEHAVIOR_NONE,
      ),
    ),
  );
}

/**
 * Implementation of hook_field_formatter_info()
 * Defines all available contentblock field formatter
 */
function contentblock_field_formatter_info() {
  return array(
    'contentblock_default' => array(
      'label' => t('Default'), 
      'field types' => array('contentblock'),
    ),
  );
}

/**
 * Implementation of hook_field_formatter_view()
 * Generates the output from a single contentblock field
 */
function contentblock_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  
  switch ($display['type']) {
    case 'contentblock_default':
      foreach ($items as $delta => $item) {
        $block = entity_load('contentblock', array($item['bid']), array(), TRUE);
        field_attach_prepare_view('contentblock', $block, 'full');
        $element[$delta] = field_attach_view('contentblock', $block[$item['bid']], 'full');
      }
      break;
  }
  
  return $element;
}

/**
 * Implementation of hook_field_info()
 * Defines the contentblock field and prevents recursion
 */
function contentblock_field_info() {
  $return = array(
    'contentblock' => array(
      'label' => t('Content Blocks'), 
      'description' => t('Field for using blocks to manage content'),
      'instance_settings' => array('allowed_blocks' => array()), 
      'default_widget' => 'contentblock_default', 
      'default_formatter' => 'contentblock_default',
    ),
  );
  
  //disable this field type for block entities to prevent recursion
  if(preg_match("~structure/blocktypes/manage/\w+/fields~", $_GET['q'])) {
    $return['contentblock']['no_ui'] = TRUE;
  }
  
  return $return;
}

 
/**
 * Implementation of hook_field_widget_form(
 * Function to generate the widget forms
 */
function contentblock_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $elements = array();
  $types = ContentBlockHelper::getInstance()->getBlockTypes();
  
  if(count($types) == 0) {
    return $element += array(
      '#type' => 'item',
      '#markup' => t('There are no blocktypes available. Please add at least one blocktype.'),
    );
  }
  
  $field_state = field_form_get_state($element['#field_parents'], $field['field_name'], $langcode, $form_state);
  if (isset($field_state['items'])) {
    $items = $field_state['items'];
  }
  
  switch ($instance['widget']['type']) {
    case 'contentblock_default':
      $element += array(
        '#extended' => TRUE,
      );

      //all existing items
      foreach($items as $item) {
        $elements[$delta] = $element;
        $elements[$delta]['#weight'] = $delta;
        $elements[$delta]['#delta'] = $delta;
        $elements[$delta]['#element_validate'] = array('contentblock_widget_default_validate');
             
        $elements[$delta]['bid'] = $element + array(
          '#type' => 'hidden',
          '#value' => $item['bid'],
        );
        
        $elements[$delta]['type'] = $element + array(
          '#type' => 'hidden',
          '#value' => $item['type'],
        );
                
        $elements[$delta]['fields'] = array(
          '#title' => $types[$item['type']]->name . ' Block',
          '#type' => 'fieldset',
          '#parents' => array_merge($element['#field_parents'], array($element['#field_name'], $element['#language'], $delta, 'fields')),
        );
        
        $elements[$delta]['fields']['remove_block'] = $element + array(
          '#type' => 'submit',
          '#name' => 'remove_block_' . $delta,
          '#submit' => array('contentblock_default_remove_block'),
          '#limit_validation_errors' => array(),
          '#value' => 'Remove block',
          '#weight' => 200,
          '#delta' => $delta,
          '#parents' => array_merge($element['#field_parents'], array($element['#field_name'], $element['#language'], $delta, 'fields', 'remove_block')),
        );
        
        $block = entity_load('contentblock', array($item['bid']), array(), TRUE);
        field_attach_form('contentblock', $block[$item['bid']], $elements[$delta]['fields'], $form_state);
        
        $delta++;
      }
      
      //only if more fields are allowed
      if (($field['cardinality'] == FIELD_CARDINALITY_UNLIMITED || $delta < $field['cardinality'])) {
        //generate options
        $types = ContentBlockHelper::getInstance()->getBlockTypes();
        foreach($types as $type) {
          $options[$type->type] = $type->name;
        }
        
        //and one extra to create a new row
        $elements[$delta] = $element;
        $elements[$delta]['#weight'] = $delta;
        $elements[$delta]['#delta'] = $delta;
        
        $elements[$delta]['bid'] = $element + array('#type' => 'hidden', '#default_value' => 0);
        $elements[$delta]['type'] = $element + array('#type' => 'hidden', '#default_value' => '');
        
        $elements[$delta]['typeselect'] = array(
          '#title' => t('Add a new contentblock'),
          '#type' => 'select',
          '#options' => $options,
        );
        
        $elements[$delta]['add_block'] = $element + array(
          '#type' => 'submit',
          '#submit' => array('contentblock_default_add_block'),
          '#limit_validation_errors' => array(),
          '#value' => 'Add new block',
        );
      }
       
      //general options for all elements
      $elements['#theme_wrappers'] = array('fieldset');
      $elements['#theme'] = 'contentblock_widget_multiple';
      $elements['#title'] = $element['#title'];
      
      break;
  }
  
  return $elements;
}


function theme_contentblock_widget_multiple($variables) {
  $element = $variables['element'];

  // Special ID and classes for draggable tables.
  $weight_class = $element['#id'] . '-weight';
  $table_id = $element['#id'] . '-table';

  // Build up a table of applicable fields.
  $headers = array();
  $headers[] = t('File information');
  if ($element['#display_field']) {
    $headers[] = array(
      'data' => t('Display'),
      'class' => array('checkbox'),
    );
  }
  $headers[] = t('Weight');
  $headers[] = t('Operations');

  // Get our list of widgets in order (needed when the form comes back after
  // preview or failed validation).
  $widgets = array();
  foreach (element_children($element) as $key) {
    $widgets[] = &$element[$key];
  }
  usort($widgets, '_field_sort_items_value_helper');

  $rows = array();
  foreach ($widgets as $key => &$widget) {
    // Save the uploading row for last.
    if ($widget['#file'] == FALSE) {
      $widget['#title'] = $element['#file_upload_title'];
      $widget['#description'] = $element['#file_upload_description'];
      continue;
    }

    // Delay rendering of the buttons, so that they can be rendered later in the
    // "operations" column.
    $operations_elements = array();
    foreach (element_children($widget) as $sub_key) {
      if (isset($widget[$sub_key]['#type']) && $widget[$sub_key]['#type'] == 'submit') {
        hide($widget[$sub_key]);
        $operations_elements[] = &$widget[$sub_key];
      }
    }

    // Delay rendering of the "Display" option and the weight selector, so that
    // each can be rendered later in its own column.
    if ($element['#display_field']) {
      hide($widget['display']);
    }
    hide($widget['_weight']);

    // Render everything else together in a column, without the normal wrappers.
    $widget['#theme_wrappers'] = array();
    $information = drupal_render($widget);

    // Render the previously hidden elements, using render() instead of
    // drupal_render(), to undo the earlier hide().
    $operations = '';
    foreach ($operations_elements as $operation_element) {
      $operations .= render($operation_element);
    }
    $display = '';
    if ($element['#display_field']) {
      unset($widget['display']['#title']);
      $display = array(
        'data' => render($widget['display']),
        'class' => array('checkbox'),
      );
    }
    $widget['_weight']['#attributes']['class'] = array($weight_class);
    $weight = render($widget['_weight']);

    // Arrange the row with all of the rendered columns.
    $row = array();
    $row[] = $information;
    if ($element['#display_field']) {
      $row[] = $display;
    }
    $row[] = $weight;
    $row[] = $operations;
    $rows[] = array(
      'data' => $row,
      'class' => isset($widget['#attributes']['class']) ? array_merge($widget['#attributes']['class'], array('draggable')) : array('draggable'),
    );
  }

  drupal_add_tabledrag($table_id, 'order', 'sibling', $weight_class);

  $output = '';
  $output = empty($rows) ? '' : theme('table', array('header' => $headers, 'rows' => $rows, 'attributes' => array('id' => $table_id)));
  $output .= drupal_render_children($element);
  return $output;
}




//------ CUSTOM FUNCTIONS

/**
 * Validates a single contentblock field
 */
function contentblock_widget_default_validate($element, &$form_state) {
  if(isset($element['bid']['#value']) && !empty($element['bid']['#value'])) {
    $block = (object) array('bid' => $element['bid']['#value'], 'type' => $element['type']['#value']);
    field_attach_submit('contentblock', $block, $element['fields'], $form_state);
    field_attach_update('contentblock', $block);
    module_invoke_all('entity_update', 'contentblock', $block);
  }
 }

/**
 * Submit function for the "remove block" button 
 */
function contentblock_default_remove_block(&$form, &$form_state) {
  $parents = $form_state['triggering_element']['#array_parents'];
  array_pop($parents);
  $element = drupal_array_get_nested_value($form, array_slice($parents, 0, -1));

  if(isset($element['bid']['#value']) && intval($element['bid']['#value']) > 0) {
    $button = $form_state['triggering_element'];
    $meta = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -2));
    $field_state = field_form_get_state($meta['#field_parents'], $meta['#field_name'], $meta['#language'], $form_state);

    $field_state['items'] = array();
    $submitted_values = drupal_array_get_nested_value($form_state['input'], array_slice($button['#array_parents'], 0, -3));

    foreach ($submitted_values as $delta => $submitted_value) {
        if (!$submitted_value['bid'] || $submitted_value['bid'] == $element['bid']['#value']) {
          unset($submitted_values[$delta]);
          continue;
        }
        
        $field_state['items'][] = $submitted_value;
    }
    
    field_form_set_state($meta['#field_parents'], $meta['#field_name'], $meta['#language'], $form_state, $field_state);
    drupal_array_set_nested_value($form_state['values'], array_slice($button['#array_parents'], 0, -3), $field_state['items']);
    drupal_array_set_nested_value($form_state['input'], array_slice($button['#array_parents'], 0, -3), $field_state['items']);
  }
    
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit function for the "add block" button 
 */
function contentblock_default_add_block(&$form, &$form_state) {
  $parents = $form_state['triggering_element']['#array_parents'];
  array_pop($parents);
  $element = drupal_array_get_nested_value($form, $parents);
  
  if(empty($element['bid']['#value']) && !empty($element['typeselect']['#value'])) {
    $block = array('bid', 'type' => $element['typeselect']['#value']);
    drupal_write_record('contentblock', $block);
    
    $button = $form_state['triggering_element'];
    $meta = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -1));
    $field_state = field_form_get_state($meta['#field_parents'], $meta['#field_name'], $meta['#language'], $form_state);
        
    $field_state['items'] = array();
    $submitted_values = drupal_array_get_nested_value($form_state['input'], array_slice($button['#array_parents'], 0, -2));
   
    foreach ($submitted_values as $delta => $submitted_value) {
      if (!$submitted_value['bid']) {
        unset($submitted_values[$delta]);
        continue;
      }
      $field_state['items'][] = $submitted_value;
    }
    
    $field_state['items'][] = $block;
    field_form_set_state($meta['#field_parents'], $meta['#field_name'], $meta['#language'], $form_state, $field_state);
    drupal_array_set_nested_value($form_state['values'], array_slice($button['#array_parents'], 0, -2), $field_state['items']);
  }
  
  $form_state['rebuild'] = TRUE;
}

/**
 * Returns if an item is empty
 */
function contentblock_field_is_empty($item, $field) {
  return empty($item['type']);
}
