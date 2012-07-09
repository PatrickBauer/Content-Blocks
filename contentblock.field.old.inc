<?php

function _contentblock_element_process($element, &$form_state, $form) {
  $bid = isset($element['#value']['bid']) ? $element['#value']['bid'] : 0;

  $element['#tree'] = TRUE;

  $ajax_settings = array(
    'path' => 'file/ajax/' . implode('/', $element['#array_parents']) . '/' . $form['form_build_id']['#value'],
    'wrapper' => $element['#id'] . '-ajax-wrapper',
    'effect' => 'fade',
  );

  // Set up the buttons first since we need to check if they were clicked.
  $element['addblock_button'] = array(
    '#name' => implode('_', $element['#parents']) . '_addblock_button',
    '#type' => 'submit',
    '#value' => t('Add contentblock'),
    '#validate' => array(),
    '#submit' => array('contentblock_submit'),
    '#limit_validation_errors' => array(array_slice($element['#parents'], 0, -1)),
    //'#ajax' => $ajax_settings,
    '#weight' => -5,
  );

  $element['remove_button'] = array(
    '#name' => implode('_', $element['#parents']) . '_remove_button',
    '#type' => 'submit',
    '#value' => t('Remove'),
    '#validate' => array(),
    '#submit' => array('contentblock_submit'),
    '#limit_validation_errors' => array(array_slice($element['#parents'], 0, -1)),
    //'#ajax' => $ajax_settings,
    '#weight' => -5,
  );

  $element['bid'] = array(
    '#type' => 'hidden',
    '#value' => $bid,
  );

  $types = ContentBlockHelper::getInstance()->getBlockTypes();
  foreach($types as $type) {
    $options[$type->type] = $type->name;
  }
  
  //type select field
  $element['type'] = array(
    '#type' => 'select',
    '#title' => t('Choose a type'),
    '#options' => $options,
    '#weight' => -10,
  );

  // Prefix and suffix used for Ajax replacement.
  $element['#prefix'] = '<div id="' . $element['#id'] . '-ajax-wrapper">';
  $element['#suffix'] = '</div>';
  
  return $element;
}


/*function _contentblock_element_validate($element, &$form_state) {
  //dpm("element_validate");
  $types = ContentBlockHelper::getInstance()->getBlockTypes();
  $entity = $types[$element['#block']['type']]; 
  
  
  drupal_write_record('contentblock', $element['#block']);
  
  $entity->bid = $element['#block']['bid'];
  
  field_attach_submit('contentblock', $entity, $element, $form_state);
  field_attach_insert('contentblock', $entity);
  
  form_set_value($element, $element['#block']['bid'], $form_state);
}*/

function _contentblock_element_validate(contentblock_field_validate) {
}

function contentblock_submit($form, &$form_state) {
  $parents = $form_state['triggering_element']['#array_parents'];
  $button_key = array_pop($parents);
  $element = drupal_array_get_nested_value($form, $parents);

  if ($button_key == 'remove_button') {
    $values_element = $element['#extended'] ? $element['bid'] : $element;
    form_set_value($values_element, NULL, $form_state);
    drupal_array_set_nested_value($form_state['input'], $values_element['#parents'], NULL);
  } 
  
  $form_state['rebuild'] = TRUE;
  
  /////////////////////////////////////////////////////
  
  $parents = array_slice($form_state['triggering_element']['#parents'], 0, -2);
  drupal_array_set_nested_value($form_state['input'], $parents, NULL);

  $button = $form_state['triggering_element'];

  // Go one level up in the form, to the widgets container.
  $element = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -1));
  $field_name = $element['#field_name'];
  $langcode = $element['#language'];
  $parents = $element['#field_parents'];

  $submitted_values = drupal_array_get_nested_value($form_state['values'], array_slice($button['#array_parents'], 0, -2));
  foreach ($submitted_values as $delta => $submitted_value) {
    if (!$submitted_value['bid']) {
      unset($submitted_values[$delta]);
    }
  }

  // Re-index deltas after removing empty items.
  $submitted_values = array_values($submitted_values);

  // Update form_state values.
  drupal_array_set_nested_value($form_state['values'], array_slice($button['#array_parents'], 0, -2), $submitted_values);

  // Update items.
  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);
  $field_state['items'] = $submitted_values;
  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);
}

function _contentblock_field_widget_value($element, $input = FALSE, &$form_state) {
  
  $block = array(
    'bid' => 0,
    'type' => NULL,
  );

  if(isset($input['type']) && !empty($input['type'])) {
    $block = array(
      'bid' => 5,
      'type' => $input['type'],
    );
  }
  
  return $block;
}

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

function contenblock_field_widget_process($element, &$form_state, $form) {
  /* $item = $element['#value'];
  $item['bid'] = $element['bid']['#value'];

  $field = field_widget_field($element, $form_state);
  $instance = field_widget_instance($element, $form_state);
  $settings = $instance['widget']['settings'];

  $element['#theme'] = 'contentblock_widget';

  // Adjust the Ajax settings so that on upload and remove of any individual
  // file, the entire group of file fields is updated together.
  if ($field['cardinality'] != 1) {
    $parents = array_slice($element['#array_parents'], 0, -1);
    $new_path = 'file/ajax/' . implode('/', $parents) . '/' . $form['form_build_id']['#value'];
    $field_element = drupal_array_get_nested_value($form, $parents);
    $new_wrapper = $field_element['#id'] . '-ajax-wrapper';
    foreach (element_children($element) as $key) {
      if (isset($element[$key]['#ajax'])) {
        $element[$key]['#ajax']['path'] = $new_path;
        $element[$key]['#ajax']['wrapper'] = $new_wrapper;
      }
    }
    unset($element['#prefix'], $element['#suffix']);
  }

  // Add another submit handler to the upload and remove buttons, to implement
  // functionality needed by the field widget. This submit handler, along with
  // the rebuild logic in file_field_widget_form() requires the entire field,
  // not just the individual item, to be valid.
  foreach (array('addblock_button', 'remove_button') as $key) {
    //$element[$key]['#submit'][] = 'file_field_widget_submit';
    //$element[$key]['#limit_validation_errors'] = array(array_slice($element['#parents'], 0, -1));
  }
 */
  return $element;
}

function contentblock_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $types = ContentBlockHelper::getInstance()->getBlockTypes();
  
  $defaults = array(
    'bid' => 0,
    'type' => NULL,
  );
  
  //check for available blocktypes
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
  
  $field_parents = $element['#field_parents'];
  $field_name = $element['#field_name'];
  $language = $element['#language'];
  
  $element_info = element_info('contentblock');
  $element += array(
    '#type' => 'contentblock',
    '#value_callback' => '_contentblock_field_widget_value',
    '#extended' => TRUE,
    '#process' => array_merge($element_info['#process'], array('contenblock_field_widget_process')),
  );
  
  foreach ($items as $item) {
    $elements[$delta] = $element;
    $elements[$delta]['#default_value'] = $item;
    $elements[$delta]['#weight'] = $delta;
    $elements[$delta]['#parents'] = array_merge($field_parents, array($field_name, $language, $delta));
    
    field_attach_form('contentblock', $types[$item['type']], $elements[$delta], $form_state);

    $delta++;
  }
  
  // And then add one more empty row for new uploads except when this is a
  // programmed form as it is not necessary.
  if (($field['cardinality'] == FIELD_CARDINALITY_UNLIMITED || $delta < $field['cardinality'])) {
    $elements[$delta] = $element;
    $elements[$delta]['#default_value'] = $defaults;
    $elements[$delta]['#weight'] = $delta;
    $elements[$delta]['#required'] = ($element['#required'] && $delta == 0);
  }
  
  // The group of elements all-together need some extra functionality
  // after building up the full list (like draggable table rows).
  $elements['#file_upload_delta'] = $delta;
  $elements['#theme'] = 'contentblock_widget_multiple';
  $elements['#theme_wrappers'] = array('fieldset');
  $elements['#process'] = array('file_field_widget_process_multiple');
  $elements['#title'] = $element['#title'];
  $elements['#description'] = $element['#description'];
  $elements['#field_name'] = $element['#field_name'];
  $elements['#language'] = $element['#language'];
 
  return $elements;
  
  //$blockdata = isset($form_state[$instance['field_id']]['#blockdata']) ? $form_state[$instance['field_id']]['#blockdata'] : array();
  
  
  /*foreach($blockdata as $key => $block) {  
    $parents = array_merge($field_parents, array($field_name, $language, $key, 'bid'));
    
    $widget = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      //'#collapsed' => TRUE,
      '#title' => $block->name,
      '#block' => $block,
      '#default_value' => isset($block->bid) ? $block->bid : '',
      '#delta' => $key,
      '#tree' => true,
      '#parents' => $parents,
    );
    
    field_attach_form('contentblock', $block, $widget, $form_state);

    $element[$key]['bid'] = $widget;
  }
  
  $form_state[$instance['field_id']]['#blockdata'] = $blockdata;
  
  $element['add_more'] = array(
    '#type' => 'submit',
    '#value' => 'Add another block',
    '#name' => 'add_more_'.$instance['field_id'],
    '#limit_validation_errors' => array(),
    '#submit' => array('block_add_more'),
  );
  
  $element['switch'] = array(
    '#type' => 'submit',
    '#value' => 'Switch test',
    '#name' => 'switch_'.$instance['field_id'],
    '#limit_validation_errors' => array(),
    '#submit' => array('switch_test'),
  );
  
  return $element;*/
} 
  
  
/*function block_add_more($form, &$form_state) {
  $types = ContentBlockHelper::getInstance()->getBlockTypes();  
  $id = substr($form_state['triggering_element']['#name'], 9);
  
  $form_state[$id]['#blockdata'][] = $types['test'];
  $form_state['rebuild'] = TRUE;
}*/

/*function switch_test($form, &$form_state) {
  $id = substr($form_state['triggering_element']['#name'], 7);
  $blockdata = $form_state[$id]['#blockdata'];
  
  dpm($form_state);
  
  $form_state[$id]['#blockdata'] = array_reverse($blockdata);
  $form_state['rebuild'] = TRUE;
}*/

function contentblock_element_info() {
  return array(
    'contentblock' => array(
      '#input' => TRUE,
      '#process' => array('_contentblock_element_process'),
      '#element_validate' => array('_contentblock_element_validate'),
      '#theme_wrappers' => array('form_element'),
      '#pre_render' => array('contentblock_element_pre_render'),
  ));
}


function theme_contentblock_widget_multiple($variables) {
  $element = $variables['element'];

  // Special ID and classes for draggable tables.
  $weight_class = $element['#id'] . '-weight';
  $table_id = $element['#id'] . '-table';

  // Build up a table of applicable fields.
  $headers = array();
  $headers[] = t('Form');
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
    if ($widget['#value']['bid'] === 0) {
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
    
    $widget['_weight']['#attributes']['class'] = array($weight_class);
    $weight = render($widget['_weight']);

    // Arrange the row with all of the rendered columns.
    $row = array();
    $row[] = $information;
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


function contentblock_element_pre_render($element) {
  dpm($element);
  if (!empty($element['#value']['bid'])) {
    $element['type']['#access'] = FALSE;
    $element['addblock_button']['#access'] = FALSE;
  } else {
    $element['remove_button']['#access'] = FALSE;
  }
  return $element;
}