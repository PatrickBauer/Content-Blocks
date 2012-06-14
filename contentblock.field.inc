<?php

function contentblock_field_info() {
  return array(
    'contentblock' => array(
      'label' => t('Content Blocks'), 
      'description' => t('Field for using blocks to manage content'),
      'instance_settings' => array('allowed_blocks' => array()), 
      'default_widget' => 'contentblock_default', 
      'default_formatter' => 'contentblock_default',
    ),
  );
}