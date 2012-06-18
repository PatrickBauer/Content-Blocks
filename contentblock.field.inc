<?php

function _contentblock_element_process($element, &$form_state) {
  $types = ContentBlockHelper::getInstance()->getBlockTypes();
  $entity = $types[$element['#block']['type']]; 
    
  $element['bid'] = array(
    '#type' => 'value',
    '#default_value' => isset($element['#block']['bid']) ? $element['#block']['bid'] : '',
  );
  
  $element['type'] = array(
    '#type' => 'value',
    '#value' => $element['#block']['type'],
  );
  
  field_attach_form('contentblock', $entity, $element, $form_state);
  
  return $element;
}

function _contentblock_element_validate($element, &$form_state) {
  $types = ContentBlockHelper::getInstance()->getBlockTypes();
  $entity = $types[$element['#block']['type']]; 
  
  
  drupal_write_record('contentblock', $element['#block']);
  
  $entity->bid = $element['#block']['bid'];
  
  field_attach_submit('contentblock', $entity, $element, $form_state);
  field_attach_insert('contentblock', $entity);
  
  form_set_value($element, $element['#block']['bid'], $form_state);
}

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

function contentblock_field_formatter_info() {
  return array(
    'contentblock_default' => array(
      'label' => t('Default'), 
      'field types' => array('contentblock'),
    ),
  );
}

function contentblock_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $types = ContentBlockHelper::getInstance()->getBlockTypes();
  $blockdata = isset($form_state['#blockdata']) ? $form_state['#blockdata'] : array();
  
  //check for available blocktypes
  if(count($types) == 0) {
    return $element += array(
      '#type' => 'item',
      '#markup' => t('There are no blocktypes available. Please add at least one blocktype.'),
    );
  }
  
  $element += array(
    '#type' => 'fieldset',
    '#tree' => TRUE,
  );
  
  foreach($blockdata as $key => $block) {  
    $widget = array(
      '#type' => 'contentblock',
      '#block' => $block,
      '#default_value' => isset($block['bid']) ? $block['bid'] : '',
      '#delta' => $key,
    );

    $element[$key]['bid'] = $widget;
  }
  
  $form_state['#blockdata'] = $blockdata;
  
  $element['add_more'] = array(
    '#type' => 'submit',
    '#value' => 'Add another block',
    '#name' => 'add_more',
    '#limit_validation_errors' => array(),
    '#submit' => array('block_add_more'),
  );
  
  return $element;
}

function block_add_more($form, &$form_state) {
  $form_state['#blockdata'][] = array('bid' => '', 'type' => 'test');

  $form_state['rebuild'] = TRUE;
}


function contentblock_field_is_empty($item, $field) {
  return false;
}

function contentblock_element_info() {
  return array(
    'contentblock' => array(
      '#input' => TRUE,
      '#tree' => TRUE,
      '#process' => array('_contentblock_element_process'),
      //'#value_callback' => '_contentblock_element_value_callback',
      '#element_validate' => array('_contentblock_element_validate'),
      '#theme_wrappers' => array('form_element'),
  ));
}

function contentblock_node_presave($node) {
  dsm($node);
}