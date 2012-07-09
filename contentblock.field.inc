<?php

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
 * Function to process the widget form
 */
function contentblock_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $elements = array();
  $types = ContentBlockHelper::getInstance()->getBlockTypes();
  
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
          '#type' => 'value',
          '#value' => $item['bid'],
        );
        
        $elements[$delta]['type'] = $element + array(
          '#type' => 'value',
          '#value' => $item['type'],
        );
                
        $elements[$delta]['fields'] = array(
          '#title' => $types[$item['type']]->name . ' - '. $item['bid'],
          '#type' => 'fieldset',
          '#parents' => array_merge($element['#field_parents'], array($element['#field_name'], $element['#language'], $delta, 'fields')),
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
          '#limit_validation_errors' => array(array_merge($element['#field_parents'], array($element['#field_name'], $element['#language']))),
          '#value' => 'Add new block',
        );
      }
       
      //general options for all elements
      $elements['#theme_wrappers'] = array('fieldset');
      $elements['#title'] = $element['#title'];
      
      break;
  }
  
  return $elements;
}

/**
 * Submit function for the add block button 
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
        
    $submitted_values = drupal_array_get_nested_value($form_state['values'], array_slice($button['#array_parents'], 0, -2));
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
 * Returns if a field is empty
 */
function contentblock_field_is_empty($item, $field) {
  return empty($item['type']);
}
